
#include "ui_ChanListDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "AOCtl.h"
#include "ChanMapCtl.h"
#include "ColorTTLCtl.h"
#include "IMROEditor.h"
#include "SVGrafsM_Im.h"
#include "ShankCtl_Im.h"
#include "Biquad.h"

#include <QAction>
#include <QSettings>
#include <QMessageBox>

#include <math.h>


#define MAX10BIT    512


/* ---------------------------------------------------------------- */
/* class SVGrafsM_Im ---------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM_Im::SVGrafsM_Im(
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel )
    :   SVGrafsM( gw, p ), ip(ip), jpanel(jpanel)
{
    shankCtl = new ShankCtl_Im( p, ip, jpanel );
    shankCtl->init();
    ConnectUI( shankCtl, SIGNAL(selChanged(int,bool)), this, SLOT(externSelectChan(int,bool)) );
    ConnectUI( shankCtl, SIGNAL(closed(QWidget*)), mainApp(), SLOT(modelessClosed(QWidget*)) );

    imroAction = new QAction( "Edit Banks, Refs, Gains...", this );
    imroAction->setEnabled( p.mode.manOvInitOff );
    ConnectUI( imroAction, SIGNAL(triggered()), this, SLOT(editImro()) );

    stdbyAction = new QAction( "Edit Channel On/Off...", this );
    stdbyAction->setEnabled( p.mode.manOvInitOff );
    ConnectUI( stdbyAction, SIGNAL(triggered()), this, SLOT(editStdby()) );

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


static void addLF2AP(
    const CimCfg::AttrEach  &E,
    qint16                  *d,
    int                     ntpts,
    int                     nC,
    int                     nAP,
    int                     dwnSmp )
{
    QVector<float>  _fgain( nAP );
    float           *fgain  = &_fgain[0];
    int             dStep   = nC * dwnSmp;

    for( int ic = 0; ic < nAP; ++ic )
        fgain[ic] = E.chanGain( ic ) / E.chanGain( ic+nAP );

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic = 0; ic < nAP; ++ic )
            d[ic] += fgain[ic]*d[ic+nAP];
    }
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


void SVGrafsM_Im::putScans( vec_i16 &data, quint64 headCt )
{
    const CimCfg::AttrEach  &E = p.im.each[ip];

#if 0
    double  tProf = getTime();
#endif
    float       ysc     = 1.0F / MAX10BIT;
    const int   nC      = chanCount(),
                nNu     = neurChanCount(),
                nAP     = E.imCumTypCnt[CimCfg::imSumAP],
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

    gw->getTTLColorCtl()->scanBlock( theX, data, headCt, nC, ip );

// -------
// Filters
// -------

    // ---------
    // AP hipass
    // ---------

    fltMtx.lock();
    if( hipass )
        hipass->applyBlockwiseMem( &data[0], MAX10BIT, ntpts, nC, 0, nAP );
    fltMtx.unlock();

    // ------------------------------------------
    // LOCK MUTEX before accessing settings (set)
    // ------------------------------------------

    drawMtx.lock();

    bool    drawBinMax  = set.binMaxOn && dwnSmp > 1,
            sAveLocal   = false;

    // ------------
    // AP = AP + LF
    // ------------

    // BK: We should superpose traces to see AP & LF, not add.

    if( set.bandSel == 2 )
        addLF2AP( E, &data[0], ntpts, nC, nAP, (drawBinMax ? 1 : dwnSmp) );

    // ------------------------------------------
    // -<T>; not applied to AP if hipass filtered
    // ------------------------------------------

    if( set.dcChkOn ) {

        dc.updateLvl( &data[0], ntpts, dwnSmp );

        dc.apply(
            &data[0], ntpts,
            (set.bandSel == 1 ? nAP : 0),
            (drawBinMax ? 1 : dwnSmp) );
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
                E.sns.shankMap,
                &data[0], ntpts, nC, nAP,
                (drawBinMax ? 1 : dwnSmp) );
            break;
        case 4:
            sAveApplyGlobalStride(
                E.sns.shankMap,
                &data[0], ntpts, nC, nAP,
                24, (drawBinMax ? 1 : dwnSmp) );
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

        if( ic < nAP ) {

            if( !E.sns.shankMap.e[ic].u ) {

                ny = (ntpts + dwnSmp - 1) / dwnSmp;
                memset( &ybuf[0], 0, ny * sizeof(float) );
                goto putData;
            }

            // ---------------
            // AP downsampling
            // ---------------

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
        else if( ic < nNu ) {

            // ---
            // LFP
            // ---

            if( !E.sns.shankMap.e[ic - nAP].u ) {

                ny = (ntpts + dwnSmp - 1) / dwnSmp;
                memset( &ybuf[0], 0, ny * sizeof(float) );
                goto putData;
            }

draw_analog:
            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                stat.add( *d );
                ybuf[ny++] = *d * ysc;
            }
        }
        else {

            // ----
            // Sync
            // ----

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


void SVGrafsM_Im::updateRHSFlags()
{
    drawMtx.lock();
    theX->dataMtx.lock();

// First consider only save flags for all channels

    const QBitArray &saveBits = p.im.each[ip].sns.saveBits;

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        if( saveBits.testBit( ic ) )
            Y.rhsLabel = "S";
        else
            Y.rhsLabel.clear();
    }

// Next rewrite the few audio channels

    QVector<int>    vAI;

    if( mainApp()->getAOCtl()->uniqueAIs( vAI, ip ) ) {

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


int SVGrafsM_Im::chanCount() const
{
    return p.im.each[ip].imCumTypCnt[CimCfg::imSumAll];
}


int SVGrafsM_Im::neurChanCount() const
{
    return p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural];
}


bool SVGrafsM_Im::isSelAnalog() const
{
// MS: Analog and digital aux may be redefined in phase 3B2
    return selected < p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural];
}


void SVGrafsM_Im::setRecordingEnabled( bool checked )
{
    imroAction->setEnabled( !checked );
    stdbyAction->setEnabled( !checked );
    sortAction->setEnabled( !checked );
    saveAction->setEnabled( !checked );
}


// Selections: {0=Native, 1=300:inf, 2=AP+LF}
//
void SVGrafsM_Im::bandSelChanged( int sel )
{
    fltMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( sel == 1 )
        hipass = new Biquad( bq_type_highpass, 300/p.im.each[ip].srate );

    fltMtx.unlock();

    drawMtx.lock();
    set.bandSel = sel;
    saveSettings();
    drawMtx.unlock();

    if( set.binMaxOn )
        eraseGraphs();
}


void SVGrafsM_Im::sAveSelChanged( int sel )
{
    const CimCfg::AttrEach  &E = p.im.each[ip];

    drawMtx.lock();
    set.sAveSel = sel;
    sAveTable( E.sns.shankMap, E.imCumTypCnt[CimCfg::imSumAP], sel );
    saveSettings();
    drawMtx.unlock();
}


void SVGrafsM_Im::mySaveGraphClicked( bool checked )
{
    Q_UNUSED( checked )
}


void SVGrafsM_Im::myMouseOverGraph( double x, double y, int iy )
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

    if( ic < neurChanCount() ) {

        // neural readout

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

        // sync readout

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


void SVGrafsM_Im::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );

    if( lastMouseOverChan < neurChanCount() ) {

        shankCtl->selChan(
            lastMouseOverChan % p.im.each[ip].imCumTypCnt[CimCfg::imSumAP],
            myChanName( lastMouseOverChan ) );
    }
}


void SVGrafsM_Im::myRClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
}


void SVGrafsM_Im::externSelectChan( int ic, bool shift )
{
    if( ic >= 0 ) {

        int icUnshift = ic;

        if( shift )
            ic += p.im.each[ip].imCumTypCnt[CimCfg::imSumAP];

        if( maximized >= 0 )
            toggleMaximized();

        selected = ic;
        ensureVisible();

        selected = -1;  // force tb update
        selectChan( ic );

        shankCtl->selChan( icUnshift, myChanName( ic ) );
    }
}


void SVGrafsM_Im::setAudioL()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, true, ip );
}


void SVGrafsM_Im::setAudioR()
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, false, ip );
}


void SVGrafsM_Im::editImro()
{
    int chan = lastMouseOverChan;

    if( chan >= neurChanCount() )
        return;

    if( !okToPause() )
        return;

// Launch editor

    const CimCfg::AttrEach  &E = p.im.each[ip];

    IMROEditor  ED( this, E.roTbl.type );
    QString     imroFile;
    bool        changed = ED.Edit( imroFile, E.imroFile, chan );

// Update world

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsImroFile( imroFile, ip );
        sAveSelChanged( set.sAveSel );
        shankCtl->layoutChanged();
        mainApp()->getRun()->imecUpdate( ip );
    }
}


void SVGrafsM_Im::editStdby()
{
    if( !okToPause() )
        return;

// Launch editor

    QString stdbyStr;
    bool    changed = stdbyDialog( stdbyStr );

// Update world

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsStdbyStr( stdbyStr, ip );
        sAveSelChanged( set.sAveSel );
        shankCtl->layoutChanged();
        mainApp()->getRun()->imecUpdate( ip );
    }
}


void SVGrafsM_Im::editChanMap()
{
// Launch editor

    QString cmFile;
    bool    changed = chanMapDialog( cmFile );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsImChanMap( cmFile, ip );
        setSorting( true );
    }
}


void SVGrafsM_Im::editSaved()
{
// Launch editor

    QString saveStr;
    bool    changed = saveDialog( saveStr );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsImSaveStr( saveStr, ip );
        updateRHSFlags();
    }
}


void SVGrafsM_Im::colorTTL()
{
    gw->getTTLColorCtl()->showDialog();
}


void SVGrafsM_Im::myInit()
{
    QAction *sep0 = new QAction( this ),
            *sep1 = new QAction( this ),
            *sep2 = new QAction( this );
    sep0->setSeparator( true );
    sep1->setSeparator( true );
    sep2->setSeparator( true );

    theM->addAction( audioLAction );
    theM->addAction( audioRAction );
    theM->addAction( sep0 );
    theM->addAction( imroAction );
    theM->addAction( stdbyAction );
    theM->addAction( sep1 );
    theM->addAction( sortAction );
    theM->addAction( saveAction );
    theM->addAction( sep2 );
    theM->addAction( cTTLAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
}


double SVGrafsM_Im::mySampRate() const
{
    return p.im.each[ip].srate;
}


void SVGrafsM_Im::mySort_ig2ic()
{
    const CimCfg::AttrEach  &E = p.im.each[ip];

    if( set.usrOrder )
        E.sns.chanMap.userOrder( ig2ic );
    else
        E.sns.chanMap.defaultOrder( ig2ic );
}


QString SVGrafsM_Im::myChanName( int ic ) const
{
    return p.im.each[ip].sns.chanMap.name(
            ic, p.isTrigChan( QString("imec%1").arg( ip ), ic ) );
}


const QBitArray& SVGrafsM_Im::mySaveBits() const
{
    return p.im.each[ip].sns.saveBits;
}


// Set the stream type codes {0=AP, 1=LF, 2=SY}.
// Return SY type code which is rendered differently in MGraphs.
//
int SVGrafsM_Im::mySetUsrTypes()
{
// MS: Analog and digital aux may be redefined in phase 3B2
    const CimCfg::AttrEach  &E = p.im.each[ip];
    int                     c0, cLim;

    c0      = 0;
    cLim    = E.imCumTypCnt[CimCfg::imTypeAP];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 0;

    c0      = E.imCumTypCnt[CimCfg::imTypeAP];
    cLim    = E.imCumTypCnt[CimCfg::imTypeLF];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 1;

    c0      = E.imCumTypCnt[CimCfg::imTypeLF];
    cLim    = E.imCumTypCnt[CimCfg::imTypeSY];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 2;

    return 2;
}


// Called only from init().
//
void SVGrafsM_Im::loadSettings()
{
    STDSETTINGS( settings, "graphs_imec" );

    settings.beginGroup( QString("Graphs_Imec_Panel%1").arg( jpanel ) );
    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.yscl0       = settings.value( "yscl0", 1.0 ).toDouble();
    set.yscl1       = settings.value( "yscl1", 1.0 ).toDouble();
    set.yscl2       = settings.value( "yscl2", 1.0 ).toDouble();
    set.clr0        = clrFromString( settings.value( "clr0", "ffeedd82" ).toString() );
    set.clr1        = clrFromString( settings.value( "clr1", "ffff5500" ).toString() );
    set.clr2        = clrFromString( settings.value( "clr2", "ff44eeff" ).toString() );
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.bandSel     = settings.value( "bandSel", 0 ).toInt();
    set.sAveSel     = settings.value( "sAveSel", 0 ).toInt();
    set.dcChkOn     = settings.value( "dcChkOn", false ).toBool();
    set.binMaxOn    = settings.value( "binMaxOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", false ).toBool();
    settings.endGroup();
}


void SVGrafsM_Im::saveSettings() const
{
    if( inConstructor )
        return;

    STDSETTINGS( settings, "graphs_imec" );

    settings.beginGroup( QString("Graphs_Imec_Panel%1").arg( jpanel ) );
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
double SVGrafsM_Im::scalePlotValue( double v, double gain ) const
{
    return p.im.all.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for neural channels!
//
void SVGrafsM_Im::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit ) const
{
    double  gain = p.im.each[ip].chanGain( ic );

    y       = scalePlotValue( y, gain );

    drawMtx.lock();

    mean    = scalePlotValue( ic2stat[ic].mean() / MAX10BIT, gain );
    stdev   = scalePlotValue( ic2stat[ic].stdDev() / MAX10BIT, gain );
    rms     = scalePlotValue( ic2stat[ic].rms() / MAX10BIT, gain );

    drawMtx.unlock();

    double  vmax = p.im.all.range.rmax / (ic2Y[ic].yscl * gain);

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


bool SVGrafsM_Im::okToPause()
{
    int yesNo = QMessageBox::warning(
        0,
        "Editing Requires Hardware Pause",
        "Right-click editing makes it easy to tune/try different imec settings...\n"
        "but we have to interrupt the hardware, and that affects stream timing.\n\n"
        "    If you proceed, please restart the run before\n"
        "    recording any time-critical data files.\n\n"
        "Edit imec settings now?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    return yesNo == QMessageBox::Yes;
}


bool SVGrafsM_Im::stdbyDialog( QString &stdbyStr )
{
    const CimCfg::AttrEach  &E = p.im.each[ip];
    QDialog                 dlg;
    Ui::ChanListDialog      ui;
    bool                    changed = false;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    dlg.setWindowTitle( "Turn Channels Off" );

    ui.curLbl->setText( E.stdbyStr.isEmpty() ? "all on" : E.stdbyStr );
    ui.chansLE->setText( E.stdbyStr );

// Run dialog until ok or cancel

    for(;;) {

        if( QDialog::Accepted == dlg.exec() ) {

            CimCfg::AttrEach    E2;
            QString             err;

            E2.stdbyStr = ui.chansLE->text().trimmed();

            if( E2.deriveStdbyBits(
                err, E.imCumTypCnt[CimCfg::imSumAP] ) ) {

                changed = E2.stdbyBits != E.stdbyBits;

                if( changed )
                    stdbyStr = E2.stdbyStr;

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


bool SVGrafsM_Im::chanMapDialog( QString &cmFile )
{
// Create default map

    const CimCfg::AttrEach  &E      = p.im.each[ip];
    const int               *type   = E.imCumTypCnt;

    ChanMapIM defMap(
        type[CimCfg::imTypeAP],
        type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP],
        type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF] );

// Launch editor

    ChanMapCtl  CM( gw, defMap );

    cmFile = CM.Edit( E.sns.chanMapFile, ip );

    if( cmFile != E.sns.chanMapFile )
        return true;
    else if( !cmFile.isEmpty() ) {

        QString msg;

        if( defMap.loadFile( msg, cmFile ) )
            return defMap != E.sns.chanMap;
    }

    return false;
}


bool SVGrafsM_Im::saveDialog( QString &saveStr )
{
    const CimCfg::AttrEach  &E = p.im.each[ip];

    QDialog             dlg;
    Ui::ChanListDialog  ui;
    bool                changed = false;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
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
                        err, QString("imec%1").arg( ip ),
                        E.imCumTypCnt[CimCfg::imSumAll] ) ) {

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


