
#include "ui_ChanListDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "GraphsWindow.h"
#include "AOCtl.h"
#include "ChanMapCtl.h"
#include "ColorTTLCtl.h"
#include "SVGrafsM_Ob.h"

#include <QAction>
#include <QSettings>
#include <QMessageBox>


#define MAX16BIT    32768


/* ---------------------------------------------------------------- */
/* class SVGrafsM_Ob ---------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM_Ob::SVGrafsM_Ob(
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel )
    :   SVGrafsM( gw, p ), ip(ip), jpanel(jpanel)
{
    audioLAction = new QAction( "Select As Left Audio Channel", this );
    ConnectUI( audioLAction, SIGNAL(triggered()), this, SLOT(setAudioL()) );

    audioRAction = new QAction( "Select As Right Audio Channel", this );
    ConnectUI( audioRAction, SIGNAL(triggered()), this, SLOT(setAudioR()) );

    sortAction = new QAction( "Edit Channel Order...", this );
    sortAction->setEnabled( p.mode.manOvInitOff );
    ConnectUI( sortAction, SIGNAL(triggered()), this, SLOT(editChanMap()) );

    saveAction = new QAction( "Edit Saved Channels...", this );
    saveAction->setEnabled( p.mode.manOvInitOff );
    ConnectUI( saveAction, SIGNAL(triggered()), this, SLOT(editSaved()) );

    refreshAction = new QAction( "Refresh Graphs", this );
    ConnectUI( refreshAction, SIGNAL(triggered()), this, SLOT(refresh()) );

    cTTLAction = new QAction( "Color TTL Events...", this );
    ConnectUI( cTTLAction, SIGNAL(triggered()), this, SLOT(colorTTL()) );
}


/*  Time Scaling
    ------------
    Each graph has its own wrapping data buffer (yval) but shares
    the time axis span. As fresh data arrive they wrap around such
    that the latest data are present as well as one span's worth of
    past data. We will draw the data using a wipe effect. Older data
    remain visible while they are progressively overwritten by the
    new from left to right. In this mode selection ranges do not
    make sense, nor do precise cursor readouts of time-coordinates.
    Rather, min_x and max_x suggest only the span of depicted data.
*/

void SVGrafsM_Ob::putScans( vec_i16 &data, quint64 headCt )
{
    const CimCfg::ObxEach   &E = p.im.obxj[ip];

#if 0
    double  tProf = getTime();
#endif
    float       ysc     = 1.0F / MAX16BIT;
    const int   nC      = chanCount(),
                nAn     = analogChanCount(),
                dwnSmp  = theX->nDwnSmp(),
                dstep   = dwnSmp * nC;

// ---------------
// Trim data block
// ---------------

    int dSize   = int(data.size()),
        ntpts   = (dSize / (dwnSmp * nC)) * dwnSmp,
        newSize = ntpts * nC;

    if( dSize != newSize )
        data.resize( newSize );

    if( !newSize )
        return;

// ------------
// TTL coloring
// ------------

    gw->getTTLColorCtl()->scanBlock( theX, data, headCt, nC, jsOB, ip );

// ------------------------------------------
// LOCK MUTEX before accessing settings (set)
// ------------------------------------------

    drawMtx.lock();

// ---------------------
// Append data to graphs
// ---------------------

    std::vector<float>  ybuf( ntpts );  // append en masse

    theX->dataMtx.lock();

    for( int ic = 0; ic < nC; ++ic ) {

        // -----------------
        // For active graphs
        // -----------------

        if( ic2iy[ic] < 0 )
            continue;

        // ----------
        // Init stats
        // ----------

        // Collect points, update mean, stddev

        GraphStats  &stat = ic2stat[ic];

        stat.clear();

        // ------------------
        // By channel type...
        // ------------------

        qint16  *d  = &data[ic];
        int     ny  = 0;

        if( ic < nAn ) {

            // ----------
            // Aux analog
            // ----------

            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                stat.add( *d );
                ybuf[ny++] = *d * ysc;
            }
        }
        else {

            // -------
            // Digital
            // -------

            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                ybuf[ny++] = *d;
        }

        // Append points en masse
        // Renormalize x-coords -> consecutive indices.

        ic2Y[ic].yval.putData( &ybuf[0], ny );
    }

// -----------------------
// Update pseudo time axis
// -----------------------

    theX->spanMtx.lock();

    double  span        = theX->spanSecs(),
            TabsCursor  = (headCt + ntpts) / E.srate,
            TwinCursor  = span * theX->Y[0]->yval.cursor()
                            / theX->Y[0]->yval.capacity();

    theX->min_x = qMax( TabsCursor - TwinCursor, 0.0 );
    theX->max_x = theX->min_x + span;

    theX->spanMtx.unlock();

// ----
// Draw
// ----

    theX->dataMtx.unlock();

    drawMtx.unlock();

    QMetaObject::invokeMethod( theM, "update", Qt::QueuedConnection );

// ---------
// Profiling
// ---------

#if 0
    tProf = getTime() - tProf;
    Log() << "Graph millis " << 1000*tProf;
#endif
}


void SVGrafsM_Ob::updateRHSFlags()
{
    drawMtx.lock();
    theX->dataMtx.lock();

// First consider only save flags for all channels

    const QBitArray &saveBits = p.im.obxj[ip].sns.saveBits;

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        if( saveBits.testBit( ic ) )
            Y.rhsLabel = "S";
        else
            Y.rhsLabel.clear();
    }

// Next rewrite the few audio channels

    std::vector<int>    vAI;

    if( mainApp()->getAOCtl()->uniqueAIs( vAI, p.jsip2stream( jsOB, ip ) ) ) {

        foreach( int ic, vAI ) {

            MGraphY &Y = ic2Y[ic];

            if( saveBits.testBit( ic ) )
                Y.rhsLabel = "A S";
            else
                Y.rhsLabel = "A  ";
        }
    }

    theX->dataMtx.unlock();
    drawMtx.unlock();
}


int SVGrafsM_Ob::chanCount() const
{
    return p.stream_nChans( jsOB, ip );
}


int SVGrafsM_Ob::analogChanCount() const
{
    return p.im.obxj[ip].obCumTypCnt[CimCfg::obSumAnalog];
}


bool SVGrafsM_Ob::isSelAnalog() const
{
    return selected < analogChanCount();
}


void SVGrafsM_Ob::setRecordingEnabled( bool checked )
{
    sortAction->setEnabled( !checked );
    saveAction->setEnabled( !checked );
}


void SVGrafsM_Ob::setLocalFilters( int &rin, int &rout, int iflt )
{
    Q_UNUSED( iflt )

    rin  = 0;
    rout = 2;
}


void SVGrafsM_Ob::myMouseOverGraph( double x, double y, int iy )
{
    if( iy < 0 || iy >= int(theX->Y.size()) ) {
        timStatBar.latestString( "" );
        return;
    }

    int     ic          = lastMouseOverChan = theX->Y[iy]->usrChan;
    bool    isNowOver   = true;

    if( ic < 0 || ic >= chanCount() ) {
        timStatBar.latestString( "" );
        return;
    }

    QWidget *w = QApplication::widgetAt( QCursor::pos() );

    if( !w || !dynamic_cast<MGraph*>(w) )
        isNowOver = false;

    double      mean, rms, stdev;
    QString     msg;
    const char  *unit,
                *swhere = (isNowOver ? "Mouse over" : "Last mouse-over");
    int         h,
                m;

    h = int(x / 3600);
    x = x - h * 3600;
    m = x / 60;
    x = x - m * 60;

    if( ic < analogChanCount() ) {

        // analog readout

        computeGraphMouseOverVars( ic, y, mean, stdev, rms, unit );

        msg = QString(
            "%1 %2 @ pos (%3h%4m%5s, %6 %7)"
            " -- {mean, rms, stdv} %7: {%8, %9, %10}")
            .arg( swhere )
            .arg( myChanName( ic ) )
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( x, 0, 'f', 3 )
            .arg( y, 0, 'f', 4 )
            .arg( unit )
            .arg( mean, 0, 'f', 4 )
            .arg( rms, 0, 'f', 4 )
            .arg( stdev, 0, 'f', 4 );
    }
    else {

        // digital readout

        msg = QString(
            "%1 %2 @ pos %3h%4m%5s")
            .arg( swhere )
            .arg( myChanName( ic ) )
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( x, 0, 'f', 3 );
    }

    timStatBar.latestString( msg );
}


void SVGrafsM_Ob::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );
}


void SVGrafsM_Ob::myRClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
}


void SVGrafsM_Ob::setAudioL()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, true, p.jsip2stream( jsOB, ip ) );
}


void SVGrafsM_Ob::setAudioR()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, false, p.jsip2stream( jsOB, ip ) );
}


void SVGrafsM_Ob::editChanMap()
{
// Launch editor

    QString cmFile;
    bool    changed = chanMapDialog( cmFile );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsObChanMap( cmFile, ip );
        setSorting( true );
    }
}


void SVGrafsM_Ob::editSaved()
{
// Launch editor

    QString saveStr;
    bool    changed = saveDialog( saveStr );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsObSaveStr( saveStr, ip );
        updateRHSFlags();
    }
}


void SVGrafsM_Ob::myInit()
{
    int nAnalog = analogChanCount();

    for( int ic = 0; ic < nAnalog; ++ic )
        ic2stat[ic].setMaxInt( MAX16BIT );

    QAction *sep0 = new QAction( this ),
            *sep1 = new QAction( this );
    sep0->setSeparator( true );
    sep1->setSeparator( true );

    theM->addAction( audioLAction );
    theM->addAction( audioRAction );
    theM->addAction( sep0 );
    theM->addAction( sortAction );
    theM->addAction( saveAction );
    theM->addAction( sep1 );
    theM->addAction( refreshAction );
    theM->addAction( cTTLAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
}


double SVGrafsM_Ob::mySampRate() const
{
    return p.im.obxj[ip].srate;
}


void SVGrafsM_Ob::mySort_ig2ic()
{
    const CimCfg::ObxEach   &E = p.im.obxj[ip];

    if( set.usrOrder )
        E.sns.chanMap.userOrder( ig2ic );
    else
        E.sns.chanMap.defaultOrder( ig2ic );
}


QString SVGrafsM_Ob::myChanName( int ic ) const
{
    return p.im.obxj[ip].sns.chanMap.name( ic, p.trig_isChan( jsOB, ip, ic ) );
}


const QBitArray& SVGrafsM_Ob::mySaveBits() const
{
    return p.im.obxj[ip].sns.saveBits;
}


// Set the stream type codes {1=AN, 2=DG}.
// Return digital type code which is rendered differently in MGraphs.
//
int SVGrafsM_Ob::mySetUsrTypes()
{
    int c0, cLim;

    c0      = 0;
    cLim    = analogChanCount();

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 1;

    c0      = cLim;
    cLim    = chanCount();

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 2;

    return 2;
}


// Called only from init().
//
void SVGrafsM_Ob::loadSettings()
{
    STDSETTINGS( settings, "graphs_obx" );

    settings.beginGroup( QString("Graphs_Obx_Panel%1").arg( jpanel ) );
    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.yscl0       = settings.value( "yscl0", 1.0 ).toDouble();
    set.yscl1       = settings.value( "yscl1", 1.0 ).toDouble();
    set.yscl2       = settings.value( "yscl2", 1.0 ).toDouble();
    set.clr0        = clrFromString( settings.value( "clr0", "ff44eeff" ).toString() );
    set.clr1        = clrFromString( settings.value( "clr1", "ff44eeff" ).toString() );
    set.clr2        = clrFromString( settings.value( "clr2", "ff44eeff" ).toString() );
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.bandSel     = settings.value( "bandSel", 0 ).toInt();
    set.sAveSel     = settings.value( "sAveSel", 0 ).toInt();
    set.dcChkOn     = settings.value( "dcChkOn", false ).toBool();
    set.binMaxOn    = settings.value( "binMaxOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", true ).toBool();
    settings.endGroup();
}


void SVGrafsM_Ob::saveSettings() const
{
    if( inConstructor )
        return;

    STDSETTINGS( settings, "graphs_obx" );

    settings.beginGroup( QString("Graphs_Obx_Panel%1").arg( jpanel ) );
    settings.setValue( "secs", set.secs );
    settings.setValue( "yscl0", set.yscl0 );
    settings.setValue( "clr0", clrToString( set.clr0 ) );
    settings.setValue( "navNChan", set.navNChan );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsM_Ob::scalePlotValue( double v ) const
{
    return p.im.obxj[ip].range.unityToVolts( (v+1)/2 );
}


// Call this only for analog channels!
//
void SVGrafsM_Ob::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit ) const
{
    const GraphStats    &stat = ic2stat[ic];

    double  vmax;

    y       = scalePlotValue( y );

    drawMtx.lock();
    mean    = scalePlotValue( stat.mean() );
    stdev   = scalePlotValue( stat.stdDev() );
    rms     = scalePlotValue( stat.rms() );
    drawMtx.unlock();

    vmax = p.im.obxj[ip].range.rmax / ic2Y[ic].yscl;
    unit = "V";

    if( vmax < 0.001 ) {
        y       *= 1e6;
        mean    *= 1e6;
        stdev   *= 1e6;
        rms     *= 1e6;
        unit     = "uV";
    }
    else if( vmax < 1.0 ) {
        y       *= 1e3;
        mean    *= 1e3;
        stdev   *= 1e3;
        rms     *= 1e3;
        unit     = "mV";
    }
}


bool SVGrafsM_Ob::chanMapDialog( QString &cmFile )
{
// Create default map

    const CimCfg::ObxEach   &E      = p.im.obxj[ip];
    const int               *type   = E.obCumTypCnt;

    ChanMapOB defMap(
        type[CimCfg::obTypeXA],
        type[CimCfg::obTypeXD] - type[CimCfg::obTypeXA],
        type[CimCfg::obTypeSY] - type[CimCfg::obTypeXD] );

// Launch editor

    ChanMapCtl  CM( gw, defMap );

    cmFile = CM.edit( E.sns.chanMapFile, ip );

    if( cmFile != E.sns.chanMapFile )
        return true;
    else if( !cmFile.isEmpty() ) {

        QString msg;

        if( defMap.loadFile( msg, cmFile ) )
            return defMap != E.sns.chanMap;
    }

    return false;
}


bool SVGrafsM_Ob::saveDialog( QString &saveStr )
{
    const CimCfg::ObxEach   &E = p.im.obxj[ip];

    QDialog             dlg;
    Ui::ChanListDialog  ui;
    bool                changed = false;

    dlg.setWindowFlags( dlg.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    dlg.setWindowTitle( "Save These Channels" );

    ui.curLbl->setText( E.sns.uiSaveChanStr );
    ui.chansLE->setText( E.sns.uiSaveChanStr );

// Run dialog until ok or cancel

    for(;;) {

        if( QDialog::Accepted == dlg.exec() ) {

            SnsChansImec    sns;
            QString         err;

            sns.uiSaveChanStr = ui.chansLE->text().trimmed();

            if( sns.deriveSaveBits(
                        err, p.jsip2stream( jsOB, ip ),
                        p.stream_nChans( jsOB, ip ) ) ) {

                changed = E.sns.saveBits != sns.saveBits;

                if( changed )
                    saveStr = sns.uiSaveChanStr;

                break;
            }
            else
                QMessageBox::critical( this, "Channels Error", err );
        }
        else
            break;
    }

    return changed;
}


