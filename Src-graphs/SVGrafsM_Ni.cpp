
#include "ui_ChanListDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "GraphsWindow.h"
#include "AOCtl.h"
#include "ChanMapCtl.h"
#include "ColorTTLCtl.h"
#include "SVGrafsM_Ni.h"
#include "SVShankCtl_Ni.h"
#include "Biquad.h"

#include <QAction>
#include <QComboBox>
#include <QSettings>
#include <QMessageBox>


#define MAX16BIT    32768


/* ---------------------------------------------------------------- */
/* class SVGrafsM_Ni ---------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM_Ni::SVGrafsM_Ni(
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 jpanel )
    :   SVGrafsM(gw, p, jsNI, 0, jpanel)
{
    if( neurChanCount() ) {
        shankCtl = new SVShankCtl_Ni( p, jpanel, this );
        shankCtl->init();
        ConnectUI( shankCtl, SIGNAL(selChanged(int,bool)), this, SLOT(externSelectChan(int)) );
        ConnectUI( shankCtl, SIGNAL(closed(QWidget*)), mainApp(), SLOT(modelessClosed(QWidget*)) );
    }

    audioLAction = new QAction( "Listen Left Ear", this );
    ConnectUI( audioLAction, SIGNAL(triggered()), this, SLOT(setAudioL()) );

    audioBAction = new QAction( "Listen Both Ears", this );
    ConnectUI( audioBAction, SIGNAL(triggered()), this, SLOT(setAudioB()) );

    audioRAction = new QAction( "Listen Right Ear", this );
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

    car.setChans( chanCount(), neurChanCount() );
    car.setSU( &p.ni.sns.shankMap );
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

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

#define V_S_AVE( d_ic )                         \
    (sAveLocal ? car.lcl_1( d_ic, ic ) : *d_ic)


void SVGrafsM_Ni::putSamps( vec_i16 &data, quint64 headCt )
{
#if 0
    double  tProf = getTime();
#endif
    float       ysc     = 1.0F / MAX16BIT;
    const int   nC      = chanCount(),
                nNu     = neurChanCount(),
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

// -------------------------
// Push data to shank viewer
// -------------------------

    if( shankCtl && shankCtl->isVisible() )
        shankCtl->putSamps( data, headCt );

// ------------
// TTL coloring
// ------------

    gw->getTTLColorCtl()->scanBlock( theX, data, headCt, nC, jsNI, 0 );

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

    // ---------------------------------
    // -<Tn>; not applied if AP filtered
    // ---------------------------------

    if( set.tnChkOn ) {

        Tn.updateLvl( &data[0], ntpts, dwnSmp );

        if( set.bandSel != 1 )
            Tn.apply( &data[0], ntpts, (drawBinMax ? 1 : dwnSmp) );
    }

    // ----
    // -<S>
    // ----

    car.setChans( nC, nNu, (drawBinMax ? 1 : dwnSmp) );

    switch( set.sAveSel ) {
        case 1:
        case 2:
            sAveLocal = true;
            break;
        case 3:
            car.gbl_ave_auto( &data[0], ntpts );
            break;
        case 4:
            car.gbl_dmx_stride_auto( &data[0], ntpts, p.ni.muxFactor );
            break;
        default:
            ;
    }

    // -----
    // -<Tx>
    // -----

    if( set.txChkOn ) {
        Tx.updateLvl( &data[0], ntpts, dwnSmp );
        Tx.apply( &data[0], ntpts, dwnSmp );
    }

// ---------------------
// Append data to graphs
// ---------------------

    std::vector<float>  ybuf( ntpts ),  // append en masse
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

            ic2Y[ic].drawBinMax = false;

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

                        // By NOT statting every point in the bin:
                        // (1) Stats agree for all binMax settings.
                        // (2) BinMax ~30% fatster.
                        //
                        // stat.add( val );

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

                for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                    int val = car.lcl_1( d, ic );

                    stat.add( val );
                    ybuf[ny++] = val * ysc;
                }
            }
            else
                goto draw_analog;
        }
        else if( ic < nAn ) {

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

    std::vector<int>    vAI;

    if( mainApp()->getAOCtl()->uniqueAIs( vAI, p.jsip2stream( jsNI, 0 ) ) ) {

        for( int ic : vAI ) {

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
    return p.stream_nChans( jsNI, 0 );
}


int SVGrafsM_Ni::neurChanCount() const
{
    return p.ni.niCumTypCnt[CniCfg::niSumNeural];
}


int SVGrafsM_Ni::analogChanCount() const
{
    return p.ni.niCumTypCnt[CniCfg::niSumAnalog];
}


bool SVGrafsM_Ni::isSelAnalog() const
{
    return selected < analogChanCount();
}


void SVGrafsM_Ni::setRecordingEnabled( bool checked )
{
    sortAction->setEnabled( !checked );
    saveAction->setEnabled( !checked );
}


void SVGrafsM_Ni::nameLocalFilters( QComboBox *CB ) const
{
    CB->addItem( "Loc 1,2" );
    CB->addItem( "Loc 2,8" );
}


void SVGrafsM_Ni::setLocalFilters( int &rin, int &rout, int iflt )
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Selections: {0=PassAll, 1=300:inf, 2=0.1:300}
//
void SVGrafsM_Ni::bandSelChanged( int sel )
{
    double  srate = p.ni.srate;

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
        hipass = new Biquad( bq_type_highpass, 300/srate );
    else {
        hipass = new Biquad( bq_type_highpass, 0.1/srate );
        lopass = new Biquad( bq_type_lowpass,  300/srate );
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

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM_Ni::myMouseOverGraph( double x, double y, int iy )
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


void SVGrafsM_Ni::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );

    if( shankCtl && lastMouseOverChan < neurChanCount() ) {

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

        if( shankCtl )
            shankCtl->selChan( ic, myChanName( ic ) );
    }
}


void SVGrafsM_Ni::setAudioL()
{
    setAudio( -1 );
}


void SVGrafsM_Ni::setAudioB()
{
    setAudio( 0 );
}


void SVGrafsM_Ni::setAudioR()
{
    setAudio( 1 );
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

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void SVGrafsM_Ni::myInit()
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
    theM->addAction( audioBAction );
    theM->addAction( sep0 );
    theM->addAction( sortAction );
    theM->addAction( saveAction );
    theM->addAction( sep1 );
    theM->addAction( refreshAction );
    theM->addAction( cTTLAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
}


double SVGrafsM_Ni::mySampRate() const
{
    return p.stream_rate( jsNI, 0 );
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
    return p.ni.sns.chanMap.name( ic, p.trig_isChan( jsNI, 0, ic ) );
}


const QBitArray& SVGrafsM_Ni::mySaveBits() const
{
    return p.ni.sns.saveBits;
}


// Set the stream type codes {0=NU, 1=AN, 2=DG}.
// Return digital type code which is rendered differently in MGraphs.
//
int SVGrafsM_Ni::mySetUsrTypes()
{
    int c0, cLim;

    c0      = 0;
    cLim    = neurChanCount();

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 0;

    c0      = cLim;
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
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.bandSel     = settings.value( "bandSel", 0 ).toInt();
    set.sAveSel     = settings.value( "sAveSel", 0 ).toInt();
    set.tnChkOn     = settings.value( "tnChkOn", false ).toBool();
    set.txChkOn     = settings.value( "txChkOn", false ).toBool();
    set.binMaxOn    = settings.value( "binMaxOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", true ).toBool();
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
    settings.setValue( "navNChan", set.navNChan );
    settings.setValue( "bandSel", set.bandSel );
    settings.setValue( "sAveSel", set.sAveSel );
    settings.setValue( "tnChkOn", set.tnChkOn );
    settings.setValue( "txChkOn", set.txChkOn );
    settings.setValue( "binMaxOn", set.binMaxOn );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM_Ni::setAudio( int LBR )
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, LBR, p.jsip2stream( jsNI, 0 ) );
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsM_Ni::scalePlotValue( double v, double gain ) const
{
    return p.ni.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for analog channels!
//
void SVGrafsM_Ni::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit ) const
{
    const GraphStats    &stat = ic2stat[ic];

    double  gain = p.ni.chanGain( ic ),
            vmax;

    y       = scalePlotValue( y, gain );

    drawMtx.lock();
    mean    = scalePlotValue( stat.mean(), gain );
    stdev   = scalePlotValue( stat.stdDev(), gain );
    rms     = scalePlotValue( stat.rms(), gain );
    drawMtx.unlock();

    vmax = p.ni.range.rmax / (ic2Y[ic].yscl * gain);
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

    const int   *cum    = p.ni.niCumTypCnt;
    int         nMux    = p.ni.muxFactor;

    ChanMapNI defMap(
        cum[CniCfg::niTypeMN] / nMux,
        (cum[CniCfg::niTypeMA] - cum[CniCfg::niTypeMN]) / nMux,
        nMux,
        cum[CniCfg::niTypeXA] - cum[CniCfg::niTypeMA],
        cum[CniCfg::niTypeXD] - cum[CniCfg::niTypeXA] );

// Launch editor

    ChanMapCtl  CM( gw, defMap );

    cmFile = CM.edit( p.ni.sns.chanMapFile, -1 );

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
        & ~(Qt::WindowContextHelpButtonHint
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
                        err, p.jsip2stream( jsNI, 0 ),
                        p.stream_nChans( jsNI, 0 ) ) ) {

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


