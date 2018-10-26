
#include "ui_ChanListDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "GraphsWindow.h"
#include "AOCtl.h"
#include "ChanMapCtl.h"
#include "ColorTTLCtl.h"
#include "SVGrafsM_Ni.h"
#include "ShankCtl_Ni.h"
#include "Biquad.h"

#include <QSettings>
#include <QMessageBox>

#include <math.h>


#define MAX16BIT    32768


/* ---------------------------------------------------------------- */
/* class SVGrafsM_Ni ---------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM_Ni::SVGrafsM_Ni( GraphsWindow *gw, const DAQ::Params &p )
    :   SVGrafsM( gw, p )
{
    shankCtl = new ShankCtl_Ni( p );
    shankCtl->init( -1 );
    ConnectUI( shankCtl, SIGNAL(selChanged(int,bool)), this, SLOT(externSelectChan(int)) );
    ConnectUI( shankCtl, SIGNAL(closed(QWidget*)), mainApp(), SLOT(modelessClosed(QWidget*)) );

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

    cTTLAction = new QAction( "Color TTL Events...", this );
    ConnectUI( cTTLAction, SIGNAL(triggered()), this, SLOT(colorTTL()) );
}


SVGrafsM_Ni::~SVGrafsM_Ni()
{
    saveSettings();
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

#define V_S_AVE( d_ic )                                         \
    (sAveLocal ? sAveApplyLocal( d_ic, ic ) : *d_ic)


void SVGrafsM_Ni::putScans( vec_i16 &data, quint64 headCt )
{
#if 0
    double  tProf = getTime();
#endif
    float       ysc     = 1.0F / MAX16BIT;
    const int   nC      = chanCount(),
                nNu     = neurChanCount(),
                dwnSmp  = theX->nDwnSmp(),
                dstep   = dwnSmp * nC;

// ---------------
// Trim data block
// ---------------

    int dSize   = (int)data.size(),
        ntpts   = (dSize / (dwnSmp * nC)) * dwnSmp,
        newSize = ntpts * nC;

    if( dSize != newSize )
        data.resize( newSize );

    if( !newSize )
        return;

// -------------------------
// Push data to shank viewer
// -------------------------

    if( shankCtl->isVisible() )
        shankCtl->putScans( data );

// ------------
// TTL coloring
// ------------

    gw->getTTLColorCtl()->scanBlock( theX, data, headCt, nC, -1 );

// -------
// Filters
// -------

    // --------
    // Bandpass
    // --------

    fltMtx.lock();
    if( hipass )
        hipass->applyBlockwiseMem( &data[0], MAX16BIT, ntpts, nC, 0, nNu );
    if( lopass )
        lopass->applyBlockwiseMem( &data[0], MAX16BIT, ntpts, nC, 0, nNu );
    fltMtx.unlock();

    // ------------------------------------------
    // LOCK MUTEX before accessing settings (set)
    // ------------------------------------------

    drawMtx.lock();

    bool    drawBinMax  = set.binMaxOn && dwnSmp > 1 && set.bandSel != 2,
            sAveLocal   = false;

    // ------------------------------------------
    // -<T>; not applied to AP if hipass filtered
    // ------------------------------------------

    if( set.dcChkOn ) {

        dc.updateLvl( &data[0], ntpts, dwnSmp );

        if( set.bandSel != 1 ) {
            dc.apply(
                &data[0], ntpts, 0,
                (drawBinMax ? 1 : dwnSmp) );
        }
    }

    // ----
    // -<S>
    // ----

    switch( set.sAveSel ) {

        case 1:
        case 2:
            sAveLocal = true;
            break;
        case 3:
            sAveApplyGlobal(
                p.ni.sns.shankMap,
                &data[0], ntpts, nC, nNu,
                (drawBinMax ? 1 : dwnSmp) );
            break;
        case 4:
            sAveApplyGlobalStride(
                p.ni.sns.shankMap,
                &data[0], ntpts, nC, nNu, p.ni.muxFactor,
                (drawBinMax ? 1 : dwnSmp) );
            break;
        default:
            ;
    }

// ---------------------
// Append data to graphs
// ---------------------

    QVector<float>  ybuf( ntpts ),  // append en masse
                    ybuf2( drawBinMax ? ntpts : 0 );

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

        if( ic < nNu ) {

            if( !p.ni.sns.shankMap.e[ic].u ) {

                ny = (ntpts + dwnSmp - 1) / dwnSmp;
                memset( &ybuf[0], 0, ny * sizeof(float) );
                goto putData;
            }

            // -------------------
            // Neural downsampling
            // -------------------

            // Within each bin, report both max and min
            // values. This ensures spikes aren't missed.
            // Max in ybuf, min in ybuf2.

            if( drawBinMax ) {

                int ndRem = ntpts;

                ic2Y[ic].drawBinMax = true;

                for( int it = 0; it < ntpts; it += dwnSmp ) {

                    int val     = V_S_AVE( d ),
                        vmax    = val,
                        vmin    = val,
                        binWid  = dwnSmp;

                    stat.add( val );

                    d += nC;

                    if( ndRem < binWid )
                        binWid = ndRem;

                    for( int ib = 1; ib < binWid; ++ib, d += nC ) {

                        val = V_S_AVE( d );

                        stat.add( val );

                        if( val > vmax )
                            vmax = val;
                        else if( val < vmin )
                            vmin = val;
                    }

                    ndRem -= binWid;

                    ybuf[ny]  = vmax * ysc;
                    ybuf2[ny] = vmin * ysc;
                    ++ny;
                }
            }
            else if( sAveLocal ) {

                ic2Y[ic].drawBinMax = false;

                for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                    int val = sAveApplyLocal( d, ic );

                    stat.add( val );
                    ybuf[ny++] = val * ysc;
                }
            }
            else {
                ic2Y[ic].drawBinMax = false;
                goto draw_analog;
            }
        }
        else if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {

            // ----------
            // Aux analog
            // ----------

draw_analog:
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

putData:
        ic2Y[ic].yval.putData( &ybuf[0], ny );

        if( ic2Y[ic].drawBinMax )
            ic2Y[ic].yval2.putData( &ybuf2[0], ny );
    }

// -----------------------
// Update pseudo time axis
// -----------------------

    theX->spanMtx.lock();

    double  span        = theX->spanSecs(),
            TabsCursor  = (headCt + ntpts) / p.ni.srate,
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


void SVGrafsM_Ni::updateRHSFlags()
{
    drawMtx.lock();
    theX->dataMtx.lock();

// First consider only save flags for all channels

    const QBitArray &saveBits = p.ni.sns.saveBits;

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        if( saveBits.testBit( ic ) )
            Y.rhsLabel = "S";
        else
            Y.rhsLabel.clear();
    }

// Next rewrite the few audio channels

    QVector<int>    vAI;

    if( mainApp()->getAOCtl()->uniqueAIs( vAI, -1 ) ) {

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


int SVGrafsM_Ni::chanCount() const
{
    return p.ni.niCumTypCnt[CniCfg::niSumAll];
}


int SVGrafsM_Ni::neurChanCount() const
{
    return p.ni.niCumTypCnt[CniCfg::niSumNeural];
}


bool SVGrafsM_Ni::isSelAnalog() const
{
    return selected < p.ni.niCumTypCnt[CniCfg::niSumAnalog];
}


void SVGrafsM_Ni::setRecordingEnabled( bool checked )
{
    sortAction->setEnabled( !checked );
    saveAction->setEnabled( !checked );
}


// Selections: {0=PassAll, 1=300:inf, 2=0.1:300}
//
void SVGrafsM_Ni::bandSelChanged( int sel )
{
    fltMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }

    if( !sel )
        ;
    else if( sel == 1 )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );
    else {
        hipass = new Biquad( bq_type_highpass, 0.1/p.ni.srate );
        lopass = new Biquad( bq_type_lowpass,  300/p.ni.srate );
    }

    fltMtx.unlock();

    drawMtx.lock();
    set.bandSel = sel;
    saveSettings();
    drawMtx.unlock();

    if( set.binMaxOn )
        eraseGraphs();
}


void SVGrafsM_Ni::sAveSelChanged( int sel )
{
    drawMtx.lock();
    set.sAveSel = sel;
    sAveTable( p.ni.sns.shankMap, neurChanCount(), sel );
    saveSettings();
    drawMtx.unlock();
}


void SVGrafsM_Ni::mySaveGraphClicked( bool checked )
{
    Q_UNUSED( checked )
}


void SVGrafsM_Ni::myMouseOverGraph( double x, double y, int iy )
{
    if( iy < 0 || iy >= theX->Y.size() ) {
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

    if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {

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


void SVGrafsM_Ni::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );

    if( lastMouseOverChan < neurChanCount() ) {

        shankCtl->selChan(
            lastMouseOverChan,
            myChanName( lastMouseOverChan ) );
    }
}


void SVGrafsM_Ni::myRClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
}


void SVGrafsM_Ni::externSelectChan( int ic )
{
    if( ic >= 0 ) {

        if( maximized >= 0 )
            toggleMaximized();

        selected = ic;
        ensureVisible();

        selected = -1;  // force tb update
        selectChan( ic );

        shankCtl->selChan( ic, myChanName( ic ) );
    }
}


void SVGrafsM_Ni::setAudioL()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, true, -1 );
}


void SVGrafsM_Ni::setAudioR()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, false, -1 );
}


void SVGrafsM_Ni::editChanMap()
{
// Launch editor

    QString cmFile;
    bool    changed = chanMapDialog( cmFile );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsNiChanMap( cmFile );
        setSorting( true );
    }
}


void SVGrafsM_Ni::editSaved()
{
// Launch editor

    QString saveStr;
    bool    changed = saveDialog( saveStr );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsNiSaveStr( saveStr );
        updateRHSFlags();
    }
}


void SVGrafsM_Ni::colorTTL()
{
    gw->getTTLColorCtl()->showDialog();
}


void SVGrafsM_Ni::myInit()
{
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
    theM->addAction( cTTLAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
}


double SVGrafsM_Ni::mySampRate() const
{
    return p.ni.srate;
}


void SVGrafsM_Ni::mySort_ig2ic()
{
    if( set.usrOrder )
        p.ni.sns.chanMap.userOrder( ig2ic );
    else
        p.ni.sns.chanMap.defaultOrder( ig2ic );
}


QString SVGrafsM_Ni::myChanName( int ic ) const
{
    return p.ni.sns.chanMap.name( ic, p.isTrigChan( "nidq", ic ) );
}


const QBitArray& SVGrafsM_Ni::mySaveBits() const
{
    return p.ni.sns.saveBits;
}


// Set the stream type codes {0=Neu, 1=Aux, 2=Dig}.
// Return Dig type code which is rendered differently in MGraphs.
//
int SVGrafsM_Ni::mySetUsrTypes()
{
    int c0, cLim;

    c0      = 0;
    cLim    = p.ni.niCumTypCnt[CniCfg::niSumNeural];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 0;

    c0      = p.ni.niCumTypCnt[CniCfg::niSumNeural];
    cLim    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 1;

    c0      = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
    cLim    = p.ni.niCumTypCnt[CniCfg::niSumAll];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[c0].usrType = 2;

    return 2;
}


// Called only from init().
//
void SVGrafsM_Ni::loadSettings()
{
    STDSETTINGS( settings, "graphs_nidq" );

    settings.beginGroup( "Graphs_Nidq" );
    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.yscl0       = settings.value( "yscl0", 1.0 ).toDouble();
    set.yscl1       = settings.value( "yscl1", 1.0 ).toDouble();
    set.yscl2       = settings.value( "yscl2", 1.0 ).toDouble();
    set.clr0        = clrFromString( settings.value( "clr0", "ffeedd82" ).toString() );
    set.clr1        = clrFromString( settings.value( "clr1", "ff44eeff" ).toString() );
    set.clr2        = clrFromString( settings.value( "clr2", "ff44eeff" ).toString() );
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.bandSel     = settings.value( "bandSel", 0 ).toInt();
    set.sAveSel     = settings.value( "sAveSel", 0 ).toInt();
    set.dcChkOn     = settings.value( "dcChkOn", false ).toBool();
    set.binMaxOn    = settings.value( "binMaxOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", false ).toBool();
    settings.endGroup();
}


void SVGrafsM_Ni::saveSettings() const
{
    if( inConstructor )
        return;

    STDSETTINGS( settings, "graphs_nidq" );

    settings.beginGroup( "Graphs_Nidq" );
    settings.setValue( "secs", set.secs );
    settings.setValue( "yscl0", set.yscl0 );
    settings.setValue( "yscl1", set.yscl1 );
    settings.setValue( "yscl2", set.yscl2 );
    settings.setValue( "clr0", clrToString( set.clr0 ) );
    settings.setValue( "clr1", clrToString( set.clr1 ) );
    settings.setValue( "clr2", clrToString( set.clr2 ) );
    settings.setValue( "navNChan", set.navNChan );
    settings.setValue( "bandSel", set.bandSel );
    settings.setValue( "sAveSel", set.sAveSel );
    settings.setValue( "dcChkOn", set.dcChkOn );
    settings.setValue( "binMaxOn", set.binMaxOn );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsM_Ni::scalePlotValue( double v, double gain ) const
{
    return p.ni.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for neural channels!
//
void SVGrafsM_Ni::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit ) const
{
    double  gain = p.ni.chanGain( ic );

    y       = scalePlotValue( y, gain );

    drawMtx.lock();

    mean    = scalePlotValue( ic2stat[ic].mean() / MAX16BIT, gain );
    stdev   = scalePlotValue( ic2stat[ic].stdDev() / MAX16BIT, gain );
    rms     = scalePlotValue( ic2stat[ic].rms() / MAX16BIT, gain );

    drawMtx.unlock();

    double  vmax = p.ni.range.rmax / (ic2Y[ic].yscl * gain);

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


bool SVGrafsM_Ni::chanMapDialog( QString &cmFile )
{
// Create default map

    const int   *type   = p.ni.niCumTypCnt;
    int         nMux    = p.ni.muxFactor;

    ChanMapNI defMap(
        type[CniCfg::niTypeMN] / nMux,
        (type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN]) / nMux,
        nMux,
        type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA],
        type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

// Launch editor

    ChanMapCtl  CM( gw, defMap );

    cmFile = CM.Edit( p.ni.sns.chanMapFile, -1 );

    if( cmFile != p.ni.sns.chanMapFile )
        return true;
    else if( !cmFile.isEmpty() ) {

        QString msg;

        if( defMap.loadFile( msg, cmFile ) )
            return defMap != p.ni.sns.chanMap;
    }

    return false;
}


bool SVGrafsM_Ni::saveDialog( QString &saveStr )
{
    QDialog             dlg;
    Ui::ChanListDialog  ui;
    bool                changed = false;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    dlg.setWindowTitle( "Save These Channels" );

    ui.curLbl->setText( p.ni.sns.uiSaveChanStr );
    ui.chansLE->setText( p.ni.sns.uiSaveChanStr );

// Run dialog until ok or cancel

    for(;;) {

        if( QDialog::Accepted == dlg.exec() ) {

            SnsChansNidq    sns;
            QString         err;

            sns.uiSaveChanStr = ui.chansLE->text().trimmed();

            if( sns.deriveSaveBits(
                        err, "nidq",
                        p.ni.niCumTypCnt[CniCfg::niSumAll] ) ) {

                changed = p.ni.sns.saveBits != sns.saveBits;

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


