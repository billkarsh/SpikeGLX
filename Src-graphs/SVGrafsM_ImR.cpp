
#include "ui_ChanListDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "AOCtl.h"
#include "ChanMapCtl.h"
#include "ColorTTLCtl.h"
#include "SaveChansCtl.h"
#include "SVGrafsM_ImR.h"
#include "SVShankCtl_Im.h"
#include "Biquad.h"

#include <QAction>
#include <QComboBox>
#include <QSettings>
#include <QMessageBox>


/* ---------------------------------------------------------------- */
/* class SVGrafsM_ImR --------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM_ImR::SVGrafsM_ImR(
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel )
    :   SVGrafsM(gw, p, jsIM, ip, jpanel)
{
    shankCtl = new SVShankCtl_Im( p, ip, jpanel, this );
    shankCtl->init();
    ConnectUI( shankCtl, SIGNAL(runSaveChansDlg(QString)), this, SLOT(editSaved(QString)) );
    ConnectUI( shankCtl, SIGNAL(selChanged(int,bool)), this, SLOT(externSelectChan(int,bool)) );
    ConnectUI( shankCtl, SIGNAL(gimmeTraces()), this, SLOT(colorTraces()) );
    ConnectUI( shankCtl, SIGNAL(closed(QWidget*)), mainApp(), SLOT(modelessClosed(QWidget*)) );

    tracesAction = new QAction( "Traces", this );
    ConnectUI( tracesAction, SIGNAL(triggered()), this, SLOT(setTraces()) );

    rasterAction = new QAction( "Raster", this );
    ConnectUI( rasterAction, SIGNAL(triggered()), this, SLOT(setRaster()) );

    audioLAction = new QAction( "Listen Left Ear", this );
    ConnectUI( audioLAction, SIGNAL(triggered()), this, SLOT(setAudioL()) );

    audioBAction = new QAction( "Listen Both Ears", this );
    ConnectUI( audioBAction, SIGNAL(triggered()), this, SLOT(setAudioB()) );

    audioRAction = new QAction( "Listen Right Ear", this );
    ConnectUI( audioRAction, SIGNAL(triggered()), this, SLOT(setAudioR()) );

    spike1Action = new QAction( "Spike Viewer 1", this );
    ConnectUI( spike1Action, SIGNAL(triggered()), this, SLOT(setSpike1()) );

    spike2Action = new QAction( "Spike Viewer 2", this );
    ConnectUI( spike2Action, SIGNAL(triggered()), this, SLOT(setSpike2()) );

    spike3Action = new QAction( "Spike Viewer 3", this );
    ConnectUI( spike3Action, SIGNAL(triggered()), this, SLOT(setSpike3()) );

    spike4Action = new QAction( "Spike Viewer 4", this );
    ConnectUI( spike4Action, SIGNAL(triggered()), this, SLOT(setSpike4()) );

    stdbyAction = new QAction( "Edit Channel On/Off...", this );
    stdbyAction->setEnabled( p.mode.manOvInitOff );
    ConnectUI( stdbyAction, SIGNAL(triggered()), this, SLOT(editStdby()) );

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

    car.setAuto( p.im.prbj[ip].roTbl );
    car.setChans( p.stream_nChans( jsIM, ip ), p.im.prbj[ip].imCumTypCnt[CimCfg::imSumNeural] );
    car.setSU( &p.im.prbj[ip].sns.shankMap );
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

void SVGrafsM_ImR::putSamps( vec_i16 &data, quint64 headCt )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

#if 0
    double  tProf = getTime();
#endif
    const int   nC      = chanCount(),
                nNu     = neurChanCount(),
                nAP     = E.imCumTypCnt[CimCfg::imSumAP],
                maxInt  = E.roTbl->maxInt(),
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

    if( shankCtl->isVisible() )
        shankCtl->putSamps( data, headCt );

// ------------
// TTL coloring
// ------------

    gw->getTTLColorCtl()->scanBlock( theX, data, headCt, nC, jsIM, ip );

// -------
// Filters
// -------

    // -----------
    // AP bandpass
    // -----------

    fltMtx.lock();
    if( hipass )
        hipass->applyBlockwiseMem( &data[0], maxInt, ntpts, nC, 0, nAP );
    fltMtx.unlock();

    // ------------------------------------------
    // LOCK MUTEX before accessing settings (set)
    // ------------------------------------------

    drawMtx.lock();

    bool    sAveLocal = false;

    // ----
    // -<S>
    // ----

    car.setChans( nC, nAP, 1 );

    switch( set.sAveSel ) {
        case 1:
        case 2:
            sAveLocal = true;
            break;
        case 3:
            car.gbl_ave_auto( &data[0], ntpts );
            break;
        case 4:
            car.gbl_dmx_tbl_auto( &data[0], ntpts );
            break;
        default:
            ;
    }

// ---------------------
// Append data to graphs
// ---------------------

    std::vector<float>  ybuf( ntpts ),  // append en masse
                        ybuf2( tsp.pDo ? ntpts : 0 );
    std::vector<qint16> wrk( ntpts );

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

        ic2Y[ic].rasFlags = 0;

        if( ic < nAP ) {

            // -----------
            // Default put
            // -----------

            ny = (ntpts + dwnSmp - 1) / dwnSmp;
            memset( &ybuf[0], 0, ntpts * sizeof(float) );

            if( !(tsp.nDo || tsp.pDo) || !E.sns.shankMap.e[ic].u )
                goto putData;

            // -------------------------
            // Precondition and fill wrk
            // -------------------------

            if( sAveLocal ) {
                for( int it = 0; it < ntpts; ++it, d += nC ) {
                    int val = car.lcl_1( d, ic );
                    wrk[it] = val;
                    stat.add( val );
                }
            }
            else {
                for( int it = 0; it < ntpts; ++it, d += nC ) {
                    wrk[it] = *d;
                    stat.add( *d );
                }
            }

            // ----
            // Scan
            // ----

#define INAROW  3

            float   i2uV  = 1E6 * E.intToV( 1, ic );

            // State {0=seekSUBT, 1=seekT, 2=seekPEAK}

            // ---
            // Neg
            // ---

            if( tsp.nDo ) {

                ic2Y[ic].rasFlags = 1;

                float   Vscl = i2uV / -tsp.nMax;

                int T       = -tsp.nFixT / i2uV,
                    state   = 0,
                    inarow  = 0,
                    ithr    = 0,
                    max     = 0,
                    val;

                for( int it = 0; it < ntpts; ++it ) {

                    val = wrk[it];

                    if( state == 0 ) {

                        if( val <= T )
                            inarow  = 0;
                        else if( ++inarow >= INAROW ) {
                            state   = 1;
                            inarow  = 0;
                        }
                    }
                    else if( state == 1 ) {

                        if( val > T )
                            inarow  = 0;
                        else if( ++inarow >= INAROW ) {
                            ithr    = it - inarow + 1;
                            state   = 2;
                            inarow  = 0;
                            max     = wrk[ithr];
                        }
                    }
                    else {

                        if( val >= T ) {
                            ybuf[ithr] = qMin( max * Vscl, 1.0f );
                            state   = 0;
                            inarow  = 0;
                        }
                        else if( val < max )
                            max = val;
                    }
                }

                if( state == 2 )
                    ybuf[ithr] = qMin( max * Vscl, 1.0f );

                if( dwnSmp > 1 ) {
                    ny = 0;
                    for( int it = 0; it < ntpts; it += dwnSmp ) {
                        float   fmax = ybuf[it];
                        for( int j = 1; j < dwnSmp; ++j ) {
                            if( ybuf[it+j] > fmax )
                                fmax = ybuf[it+j];
                        }
                        ybuf[ny++] = fmax;
                    }
                }
            }

            // ---
            // Pos
            // ---

            if( tsp.pDo ) {

                ic2Y[ic].rasFlags |= 2;
                memset( &ybuf2[0], 0, ntpts * sizeof(float) );

                float   Vscl = i2uV / tsp.pMax;

                int T       = tsp.pFixT / i2uV,
                    state   = 0,
                    inarow  = 0,
                    ithr    = 0,
                    max     = 0,
                    val;

                for( int it = 0; it < ntpts; ++it ) {

                    val = wrk[it];

                    if( state == 0 ) {

                        if( val >= T )
                            inarow  = 0;
                        else if( ++inarow >= INAROW ) {
                            state   = 1;
                            inarow  = 0;
                        }
                    }
                    else if( state == 1 ) {

                        if( val < T )
                            inarow  = 0;
                        else if( ++inarow >= INAROW ) {
                            ithr    = it - inarow + 1;
                            state   = 2;
                            inarow  = 0;
                            max     = wrk[ithr];
                        }
                    }
                    else {

                        if( val <= T ) {
                            ybuf2[ithr] = qMin( max * Vscl, 1.0f );
                            state   = 0;
                            inarow  = 0;
                        }
                        else if( val > max )
                            max = val;
                    }
                }

                if( state == 2 )
                    ybuf2[ithr] = qMin( max * Vscl, 1.0f );

                if( dwnSmp > 1 ) {
                    ny = 0;
                    for( int it = 0; it < ntpts; it += dwnSmp ) {
                        float   fmax = ybuf2[it];
                        for( int j = 1; j < dwnSmp; ++j ) {
                            if( ybuf2[it+j] > fmax )
                                fmax = ybuf2[it+j];
                        }
                        ybuf2[ny++] = fmax;
                    }
                }
            }
        }
        else if( ic < nNu ) {

            // ---
            // LFP
            // ---

            ny = (ntpts + dwnSmp - 1) / dwnSmp;
            memset( &ybuf[0], 0, ny * sizeof(float) );
            goto putData;
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

        if( ic2Y[ic].rasFlags & 2 )
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


void SVGrafsM_ImR::updateRHSFlags()
{
    QMutexLocker    ml( &drawMtx );
    QMutexLocker    ml2( &theX->dataMtx );

// Init and do save flags

    const CimCfg::PrbEach   &E = p.im.prbj[ip];
    const QBitArray         *B;
    QBitArray               bits;

    if( E.imCumTypCnt[CimCfg::imSumNeural] == E.imCumTypCnt[CimCfg::imSumAP] ) {
        bits    = E.saveBits( p.sns.exclBadShks );
        B       = &bits;
    }
    else
        B = &E.sns.saveBits;

    for( int ic = 0, nC = (int)ic2Y.size(); ic < nC; ++ic )
        ic2Y[ic].rhsLabel = (B->testBit( ic ) ? rhsSave : 0);

// Audio

    std::vector<int>    vChan;

    if( mainApp()->getAOCtl()->uniqueChans( vChan, p.jsip2stream( jsIM, ip ) ) ) {

        for( int ic : vChan )
            ic2Y[ic].rhsLabel |= rhsAudio;
    }

// Opto

    if( E.roTbl->optoGetNeib( vChan, 0 ) >= 0 ) {
        for( int ic : vChan ) {
            if( ic >= 100000 )
                ic2Y[ic - 100000].rhsLabel |= rhsBlue | rhsLocase;
            else
                ic2Y[ic].rhsLabel |= rhsBlue;
        }
    }

    if( E.roTbl->optoGetNeib( vChan, 1 ) >= 0 ) {
        for( int ic : vChan ) {
            if( ic >= 100000 )
                ic2Y[ic - 100000].rhsLabel |= rhsRed | rhsLocase;
            else
                ic2Y[ic].rhsLabel |= rhsRed;
        }
    }

    shankCtl->setEmitters(
        E.roTbl->optoGetCur( 0 ),
        E.roTbl->optoGetCur( 1 ) );
}


void SVGrafsM_ImR::updateProbe( bool shankMap, bool chanMap )
{
    if( shankMap ) {
        car.setSU( &p.im.prbj[ip].sns.shankMap );
        sAveSelChanged( set.sAveSel );
        shankCtl->mapChanged();
    }

    if( chanMap )
        setSorting( true );
}


void SVGrafsM_ImR::setAnatomyPP( const QString &elems, int sk )
{
    shankCtl->setAnatomyPP( elems, sk );
}


int SVGrafsM_ImR::chanCount() const
{
    return p.stream_nChans( jsIM, ip );
}


int SVGrafsM_ImR::neurChanCount() const
{
    return p.im.prbj[ip].imCumTypCnt[CimCfg::imSumNeural];
}


bool SVGrafsM_ImR::isSelAnalog() const
{
    return selected < analogChanCount();
}


void SVGrafsM_ImR::setRecordingEnabled( bool checked )
{
    stdbyAction->setEnabled( !checked );
    sortAction->setEnabled( !checked );
    saveAction->setEnabled( !checked );
}


void SVGrafsM_ImR::nameLocalFilters( QComboBox *CB ) const
{
    IMROTbl *R = p.im.prbj[ip].roTbl;
    int     rin, rout;

    R->locFltRadii( rin, rout, 1 );
    CB->addItem( QString("Loc %1,%2").arg( qMax( rin, 1 ) ).arg( rout ) );

    R->locFltRadii( rin, rout, 2 );
    CB->addItem( QString("Loc %1,%2").arg( rin ).arg( rout ) );
}


void SVGrafsM_ImR::setLocalFilters( int &rin, int &rout, int iflt )
{
    p.im.prbj[ip].roTbl->locFltRadii( rin, rout, iflt );
}


void SVGrafsM_ImR::setTspike( const TSpikeSettings& T )
{
    QMutexLocker    ml( &drawMtx );
    tsp = T;
}


void SVGrafsM_ImR::getTspike( TSpikeSettings& T ) const
{
    T = tsp;
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM_ImR::sAveSelChanged( int sel )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];
    QMutexLocker            ml( &drawMtx );

    set.sAveSel = sel;
    sAveTable( E.sns.shankMap, E.imCumTypCnt[CimCfg::imSumAP], sel );
    saveSettings();
}


void SVGrafsM_ImR::rasNMaxChanged( int val )
{
    tsp.nMax = val;
}


void SVGrafsM_ImR::rasPMaxChanged( int val )
{
    tsp.pMax = val;
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM_ImR::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );

    if( lastMouseOverChan < neurChanCount() ) {

        shankCtl->selChan(
            lastMouseOverChan % p.im.prbj[ip].imCumTypCnt[CimCfg::imSumAP],
            myChanName( lastMouseOverChan ) );
    }

    if( mainApp()->getAOCtl()->isClickPlay() )
        setAudioB();
}


void SVGrafsM_ImR::myRClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
}


void SVGrafsM_ImR::externSelectChan( int ic, bool shift )
{
    if( ic >= 0 ) {

        int icUnshift = ic;

        if( shift )
            ic += p.im.prbj[ip].imCumTypCnt[CimCfg::imSumAP];

        if( maximized >= 0 )
            toggleMaximized();

        selected = ic;
        ensureVisible();

        selected = -1;  // force tb update
        selectChan( lastMouseOverChan = ic );

        shankCtl->selChan( icUnshift, myChanName( ic ) );

        if( mainApp()->getAOCtl()->isClickPlay() )
            setAudioB();
    }
}


void SVGrafsM_ImR::colorTraces()
{
    QMutexLocker    ml( &drawMtx );
    QMutexLocker    ml2( &theX->dataMtx );
    shankCtl->colorTraces( theX, ic2Y );
}


void SVGrafsM_ImR::setTraces()
{
    mainApp()->getRun()->grfShowRaster( jpanel, false );
}


void SVGrafsM_ImR::setRaster()
{
    return;
}


void SVGrafsM_ImR::setAudioL()
{
    setAudio( -1 );
}


void SVGrafsM_ImR::setAudioB()
{
    setAudio( 0 );
}


void SVGrafsM_ImR::setAudioR()
{
    setAudio( 1 );
}


void SVGrafsM_ImR::setSpike1()
{
    setSpike( 1 );
}


void SVGrafsM_ImR::setSpike2()
{
    setSpike( 2 );
}


void SVGrafsM_ImR::setSpike3()
{
    setSpike( 3 );
}


void SVGrafsM_ImR::setSpike4()
{
    setSpike( 4 );
}


void SVGrafsM_ImR::editStdby()
{
#ifdef PAUSEWHOLESLOT
    if( !okToPause() )
        return;
#endif

// Launch editor

    QString stdbyStr;
    bool    changed = stdbyDialog( stdbyStr );

// Update world

    if( changed ) {

        Run *run = mainApp()->getRun();

        run->grfHardPause( true );
        run->grfWaitPaused();

        mainApp()->cfgCtl()->graphSetsStdbyStr( stdbyStr, ip );
        updateProbe( true, false );
        run->imecUpdate( ip );

        run->grfHardPause( false );
    }
}


void SVGrafsM_ImR::editChanMap()
{
// Launch editor

    QString cmFile;
    bool    changed = chanMapDialog( cmFile );

    if( changed ) {
        mainApp()->cfgCtl()->graphSetsImChanMap( cmFile, ip );
        updateProbe( false, true );
    }
}


void SVGrafsM_ImR::editSaved( QString sInit )
{
// Launch editor

    SaveChansCtl    SV( this, p.im.prbj[ip], ip );
    QString         saveStr = sInit;
    bool            lfPair  = p.sns.lfPairChk,
                    exclBad = p.sns.exclBadShks;

    if( SV.edit( saveStr, lfPair, exclBad ) ) {
        mainApp()->cfgCtl()->graphSetsImSaveStr( saveStr, ip, lfPair, exclBad );
        updateRHSFlags();
    }
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void SVGrafsM_ImR::myInit()
{
    int maxInt = p.im.prbj[ip].roTbl->maxInt();

    for( int ic = 0, nNu = neurChanCount(); ic < nNu; ++ic )
        ic2stat[ic].setMaxInt( maxInt );

    hipass = new Biquad( bq_type_highpass, 300/p.stream_rate( jsIM, ip ) );

    QAction *sep0 = new QAction( this ),
            *sep1 = new QAction( this ),
            *sep2 = new QAction( this ),
            *sep3 = new QAction( this ),
            *sep4 = new QAction( this );
    sep0->setSeparator( true );
    sep1->setSeparator( true );
    sep2->setSeparator( true );
    sep3->setSeparator( true );
    sep4->setSeparator( true );

    theM->addAction( tracesAction );
    theM->addAction( rasterAction );
    theM->addAction( sep0 );
    theM->addAction( audioLAction );
    theM->addAction( audioRAction );
    theM->addAction( audioBAction );
    theM->addAction( sep1 );
    theM->addAction( spike1Action );
    theM->addAction( spike2Action );
    theM->addAction( spike3Action );
    theM->addAction( spike4Action );
    theM->addAction( sep2 );
    theM->addAction( stdbyAction );
    theM->addAction( sep3 );
    theM->addAction( sortAction );
    theM->addAction( saveAction );
    theM->addAction( sep4 );
    theM->addAction( refreshAction );
    theM->addAction( cTTLAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
}


double SVGrafsM_ImR::mySampRate() const
{
    return p.stream_rate( jsIM, ip );
}


void SVGrafsM_ImR::mySort_ig2ic()
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( set.usrOrder )
        E.sns.chanMap.userOrder( ig2ic );
    else
        E.sns.chanMap.defaultOrder( ig2ic );
}


QString SVGrafsM_ImR::myChanName( int ic ) const
{
    return p.im.prbj[ip].sns.chanMap.name( ic, p.trig_isChan( jsIM, ip, ic ) );
}


const QBitArray SVGrafsM_ImR::mySaveBits() const
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( E.imCumTypCnt[CimCfg::imSumNeural] == E.imCumTypCnt[CimCfg::imSumAP] )
        return E.saveBits( p.sns.exclBadShks );
    else
        return E.sns.saveBits;
}


// Set the stream type codes {0=AP, 1=LF, 2=SY}.
// Return SY type code which is rendered differently in MGraphs.
//
int SVGrafsM_ImR::mySetUsrTypes()
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];
    int                     c0, cLim;

    c0      = 0;
    cLim    = E.imCumTypCnt[CimCfg::imTypeAP];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 0;

    c0      = cLim;
    cLim    = E.imCumTypCnt[CimCfg::imTypeLF];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 1;

    c0      = cLim;
    cLim    = E.imCumTypCnt[CimCfg::imTypeSY];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 2;

    return 2;
}


// Call this only for neural channels!
//
void SVGrafsM_ImR::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &vmax,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit ) const
{
    const CimCfg::PrbEach   &E      = p.im.prbj[ip];
    const GraphStats        &stat   = ic2stat[ic];

    double  gain = E.chanGain( ic );

    y       = scalePlotValue( y, gain );

    drawMtx.lock();
    mean    = scalePlotValue( stat.mean(), gain );
    stdev   = scalePlotValue( stat.stdDev(), gain );
    rms     = scalePlotValue( stat.rms(), gain );
    drawMtx.unlock();

    vmax = E.roTbl->maxVolts() / (ic2Y[ic].yscl * gain);
    unit = "V";

    if( vmax < 0.001 ) {
        y       *= 1e6;
        vmax    *= 1e6;
        mean    *= 1e6;
        stdev   *= 1e6;
        rms     *= 1e6;
        unit     = "uV";
    }
    else if( vmax < 1.0 ) {
        y       *= 1e3;
        vmax    *= 1e3;
        mean    *= 1e3;
        stdev   *= 1e3;
        rms     *= 1e3;
        unit     = "mV";
    }
}


// Called only from init().
//
void SVGrafsM_ImR::loadSettings()
{
    STDSETTINGS( settings, "graphs_raster" );

    settings.beginGroup( QString("Graphs_Gen_Panel%1").arg( jpanel ) );
    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.yscl0       = settings.value( "yscl0", 1.0 ).toDouble();
    set.yscl1       = settings.value( "yscl1", 1.0 ).toDouble();
    set.yscl2       = settings.value( "yscl2", 1.0 ).toDouble();
    set.clr0        = clrFromString( settings.value( "clr0", "ffeedd82" ).toString() );
    set.clr1        = clrFromString( settings.value( "clr1", "ffff5500" ).toString() );
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.bandSel     = 1;
    set.sAveSel     = settings.value( "sAveSel", 4 ).toInt();
    set.blChkOn     = settings.value( "blChkOn", true ).toBool();
    set.tnChkOn     = false;
    set.txChkOn     = false;
    set.binMaxOn    = false;
    set.usrOrder    = settings.value( "usrOrder", true ).toBool();
    settings.endGroup();

    settings.beginGroup( QString("Graphs_Tspk_Panel%1").arg( jpanel ) );
    tsp.nFixT   = settings.value( "nFixT", 75 ).toInt();
    tsp.pFixT   = settings.value( "pFixT", 75 ).toInt();
    tsp.nMax    = settings.value( "nMax", 500 ).toInt();
    tsp.pMax    = settings.value( "pMax", 500 ).toInt();
    tsp.nDo     = settings.value( "nDo", true ).toBool();
    tsp.pDo     = settings.value( "pDo", false ).toBool();
    tsp.nFix    = settings.value( "nFix", true ).toBool();
    tsp.pFix    = settings.value( "pFix", true ).toBool();
    settings.endGroup();
}


void SVGrafsM_ImR::saveSettings() const
{
    if( inConstructor )
        return;

    STDSETTINGS( settings, "graphs_raster" );

    settings.beginGroup( QString("Graphs_Gen_Panel%1").arg( jpanel ) );
    settings.setValue( "secs", set.secs );
    settings.setValue( "yscl0", set.yscl0 );
    settings.setValue( "yscl1", set.yscl1 );
    settings.setValue( "yscl2", set.yscl2 );
    settings.setValue( "clr0", clrToString( set.clr0 ) );
    settings.setValue( "clr1", clrToString( set.clr1 ) );
    settings.setValue( "navNChan", set.navNChan );
    settings.setValue( "sAveSel", set.sAveSel );
    settings.setValue( "blChkOn", set.blChkOn );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();

    settings.beginGroup( QString("Graphs_Tspk_Panel%1").arg( jpanel ) );
    settings.setValue( "nFixT", tsp.nFixT );
    settings.setValue( "pFixT", tsp.pFixT );
    settings.setValue( "nMax", tsp.nMax );
    settings.setValue( "pMax", tsp.pMax );
    settings.setValue( "nDo", tsp.nDo );
    settings.setValue( "pDo", tsp.pDo );
    settings.setValue( "nFix", tsp.nFix );
    settings.setValue( "pFix", tsp.pFix );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM_ImR::setAudio( int LBR )
{
    mainApp()->getAOCtl()->
        graphSetsChannel( lastMouseOverChan, LBR, p.jsip2stream( jsIM, ip ) );
}


void SVGrafsM_ImR::setSpike( int gp )
{
    mainApp()->getRun()->grfShowSpikes( gp, ip,
        lastMouseOverChan % p.im.prbj[ip].roTbl->nAP() );
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsM_ImR::scalePlotValue( double v, double gain ) const
{
    return p.im.prbj[ip].roTbl->unityToVolts( (v+1)/2 ) / gain;
}


#ifdef PAUSEWHOLESLOT
bool SVGrafsM_ImR::okToPause()
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
#endif


bool SVGrafsM_ImR::stdbyDialog( QString &stdbyStr )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];
    QDialog                 dlg;
    Ui::ChanListDialog      ui;
    bool                    changed = false;

    dlg.setWindowFlags( dlg.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    dlg.setWindowTitle( "Turn Channels Off" );

    ui.curLbl->setText( E.stdbyStr.isEmpty() ? "all on" : E.stdbyStr );
    ui.chansLE->setText( E.stdbyStr );

// Run dialog until ok or cancel

    for(;;) {

        if( QDialog::Accepted == dlg.exec() ) {

            CimCfg::PrbEach     E2;
            QString             err;

            E2.stdbyStr = ui.chansLE->text().trimmed();

            if( E2.deriveUserStdbyBits(
                err, E.imCumTypCnt[CimCfg::imSumAP], ip ) ) {

                changed = E2.stdbyBits( true ) != E.stdbyBits( true );

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


bool SVGrafsM_ImR::chanMapDialog( QString &cmFile )
{
// Create default map

    const CimCfg::PrbEach   &E      = p.im.prbj[ip];
    const int               *cum    = E.imCumTypCnt;

    ChanMapIM defMap(
        cum[CimCfg::imTypeAP],
        cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP],
        cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

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


