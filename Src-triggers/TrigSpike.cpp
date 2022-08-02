
#include "TrigSpike.h"
#include "Util.h"
#include "Biquad.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"

#include <QTimer>
#include <QThread>


#define LOOP_MS     100


static TrigSpike    *ME;


/* ---------------------------------------------------------------- */
/* TrSpkWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrSpkWorker::run()
{
    const int   niq = viq.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iiq = 0; iiq < niq; ++iiq ) {

            if( !(ok = writeSome( viq[iiq] )) )
                break;
        }
    }

    emit finished();
}


bool TrSpkWorker::writeSome( int iq )
{
    const SyncStream    &S = vS[iq];
    TrigSpike::Counts   &C = ME->cnt;

    vec_i16 data;
    quint64 headCt  = C.nextCt[iq];

    if( !ME->nScansFromCt( data, headCt, C.remCt[iq], S.js, S.ip ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// ---------------
// Update tracking
// ---------------

    C.nextCt[iq]    += size / S.Q->nChans();
    C.remCt[iq]     -= C.nextCt[iq] - headCt;

// -----
// Write
// -----

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}

/* ---------------------------------------------------------------- */
/* TrSpkThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrSpkThread::TrSpkThread(
    TrSpkShared             &shr,
    std::vector<SyncStream> &vS,
    std::vector<int>        &viq )
{
    thread  = new QThread;
    worker  = new TrSpkWorker( shr, vS, viq );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


TrSpkThread::~TrSpkThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* struct HiPassFnctr --------------------------------------------- */
/* ---------------------------------------------------------------- */

// IMPORTANT!!
// -----------
// The Biquad filter functions have internal memory, so if there's
// a discontinuity in a filter's input stream, transients ensue.
// Here's the strategy to combat filter transients...
// We look for edges using findFltFallingEdge(), starting from the
// position 'edgeCt'. Every time we modify edgeCt we will tell the
// filter to reset its 'zero' counter to BIQUAD_TRANS_WIDE. We'll
// have the filter zero that many leading data points.

TrigSpike::HiPassFnctr::HiPassFnctr( const DAQ::Params &p )
{
    int ip, js = p.stream2jsip( ip, p.trgSpike.stream );

    fltbuf.resize( nmax = 256 );

    chan    = p.trgSpike.aiChan;
    flt     = 0;

    switch( js ) {
        case jsNI:
            {
                if( chan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {
                    flt     = new Biquad( bq_type_highpass, 300/p.ni.srate );
                    maxInt  = 32768;
                }
            }
            break;
        case jsOB:
            // no neural channels
            break;
        case jsIM:
            {
                const CimCfg::PrbEach   &E = p.im.prbj[ip];
                if( chan < E.imCumTypCnt[CimCfg::imSumAP] ) {
                    flt     = new Biquad( bq_type_highpass, 300/E.srate );
                    maxInt  = E.roTbl->maxInt();
                }
            }
            break;
    }

    reset();
}


TrigSpike::HiPassFnctr::~HiPassFnctr()
{
    if( flt )
        delete flt;
}


void TrigSpike::HiPassFnctr::reset()
{
    nzero = BIQUAD_TRANS_WIDE;
}


void TrigSpike::HiPassFnctr::operator()( int nflt )
{
    if( flt ) {

        flt->apply1BlockwiseMem1( &fltbuf[0], maxInt, nflt, 1, 0 );

        if( nzero > 0 ) {

            // overwrite with zeros

            if( nflt > nzero )
                nflt = nzero;

            memset( &fltbuf[0], 0, nflt*sizeof(qint16) );
            nzero -= nflt;
        }
    }
}

/* ---------------------------------------------------------------- */
/* Counts --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigSpike::Counts::Counts( const DAQ::Params &p ) : nq(p.stream_nq())
{
    nextCt.resize( nq );
    remCt.resize( nq );

    periEvtCt.resize( nq );
    refracCt.resize( nq );
    latencyCt.resize( nq );

    for( int iq = 0; iq < nq; ++iq ) {

        double  srate;
        int     ip, js = p.iq2jsip( ip, iq );

        switch( js ) {
            case jsNI: srate = p.ni.srate; break;
            case jsOB: srate = p.im.obxj[ip].srate; break;
            case jsIM: srate = p.im.prbj[ip].srate; break;
        }

        periEvtCt[iq]   = p.trgSpike.periEvtSecs * srate;
        refracCt[iq]    = qMax( p.trgSpike.refractSecs * srate, 5.0 );
        latencyCt[iq]   = 0.25 * srate;
    }
}


void TrigSpike::Counts::setupWrite( const std::vector<quint64> &vEdge )
{
    for( int iq = 0; iq < nq; ++iq ) {
        nextCt[iq]  = vEdge[iq] - periEvtCt[iq];
        remCt[iq]   = 2 * periEvtCt[iq] + 1;
    }
}


quint64 TrigSpike::Counts::minCt( int iq )
{
    return periEvtCt[iq] + latencyCt[iq];
}


bool TrigSpike::Counts::remCtDone()
{
    for( int iq = 0; iq < nq; ++iq ) {

        if( remCt[iq] > 0 )
            return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* TrigSpike ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

TrigSpike::TrigSpike(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const QVector<AIQ*> &imQ,
    const QVector<AIQ*> &obQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, obQ, niQ ),
        usrFlt(new HiPassFnctr( p )),
        cnt( p ),
        spikesMax(p.trgSpike.isNInf ? UNSET64 : p.trgSpike.nS),
        aEdgeCtNext(0),
        thresh(p.trigThreshAsInt())
{
}


#define ISSTATE_GetEdge     (state == 0)
#define ISSTATE_Write       (state == 1)
#define ISSTATE_Done        (state == 2)


// Spike logic is driven by TrgSpikeParams:
// {periEvtSecs, refractSecs, inarow, nS, T}.
// Corresponding states defined above.
//
void TrigSpike::run()
{
    Debug() << "Trigger thread started.";

// ---------
// Configure
// ---------

    ME = this;

// Create workers and threads

    std::vector<TrSpkThread*>   trT;
    TrSpkShared                 shr( p );
    const int                   nPrbPerThd  = 2;

    iqMax   = cnt.nq - 1;
    nThd    = 0;

    for( int iq0 = 0; iq0 < iqMax; iq0 += nPrbPerThd ) {

        std::vector<int>    viq;

        for( int k = 0; k < nPrbPerThd; ++k ) {

            if( iq0 + k < iqMax )
                viq.push_back( iq0 + k );
            else
                break;
        }

        trT.push_back( new TrSpkThread( shr, vS, viq ) );
        ++nThd;
    }

    {   // local worker (last stream)
        std::vector<int>    iq1( 1, iqMax );
        locWorker = new TrSpkWorker( shr, vS, iq1 );
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                QThread::usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    setYieldPeriod_ms( LOOP_MS );

    initState();

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive ) {

            initState();
            goto next_loop;
        }

        // --------------
        // Seek next edge
        // --------------

        if( ISSTATE_GetEdge ) {

            if( !getEdge() )
                goto next_loop;

            QMetaObject::invokeMethod(
                gw, "blinkTrigger",
                Qt::QueuedConnection );

            // ---------------
            // Start new files
            // ---------------

            {
                int ig, it;

                if( !newTrig( ig, it, false ) ) {
                    err = "open file failed";
                    break;
                 }

                setSyncWriteMode();
            }

            SETSTATE_Write();
        }

        // ----------------
        // Handle this edge
        // ----------------

        if( ISSTATE_Write ) {

            if( !xferAll( shr, err ) )
                break;

            // -----
            // Done?
            // -----

            if( cnt.remCtDone() ) {

                endTrig();

                if( ++nSpikes >= spikesMax )
                    SETSTATE_Done();
                else {

                    usrFlt->reset();

                    for( int iq = 0; iq <= iqMax; ++iq )
                        vEdge[iq] += cnt.refracCt[iq];

                    SETSTATE_GetEdge();
                }
            }
        }

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 1.0 ) {

            QString sOn, sWr;

            statusOnSince( sOn );
            statusWrPerf( sWr );

            Status() << sOn << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd ) {
        trT[iThd]->thread->wait( 10000/nThd );
        delete trT[iThd];
    }

    delete locWorker;

// Done

    endRun( err );
}


void TrigSpike::SETSTATE_GetEdge()
{
    aEdgeCtNext = 0;
    state       = 0;
}


void TrigSpike::SETSTATE_Write()
{
    cnt.setupWrite( vEdge );
    state = 1;
}


void TrigSpike::SETSTATE_Done()
{
    state = 2;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigSpike::initState()
{
    usrFlt->reset();
    vEdge.clear();
    nSpikes = 0;
    SETSTATE_GetEdge();
}


// Find edge in iSrc stream but translate to all others.
//
// Return true if found.
//
bool TrigSpike::getEdge()
{
    int iSrc = p.stream2iq( p.trgSpike.stream );

// Start getEdge() search at gate edge, subject to
// periEvent criteria. Precision not needed here;
// sync only applied to getEdge() results.

    if( !vEdge.size() ) {

        const SyncStream    &S = vS[iSrc];

        usrFlt->reset();
        vEdge.resize( iqMax + 1 );

        vEdge[iSrc] = qMax(
            S.TAbs2Ct( getGateHiT() ),
            S.Q->qHeadCt() + cnt.minCt( iSrc ) );
    }

// It may take several tries to achieve pulser sync for multi streams.
// aEdgeCtNext saves us from costly refinding of edge-A while hunting.

    bool    found;

    if( aEdgeCtNext )
        found = true;
    else {
        found = vS[iSrc].Q->findFltFallingEdge(
                    aEdgeCtNext,
                    vEdge[iSrc],
                    thresh,
                    p.trgSpike.inarow,
                    *usrFlt );

        if( !found ) {
            vEdge[iSrc] = aEdgeCtNext;  // pick up search here
            aEdgeCtNext = 0;
        }
    }

    if( found && iqMax > 0 ) {

        syncDstTAbsMult( aEdgeCtNext, iSrc, vS, p );

        for( int iq = 0; iq <= iqMax; ++iq ) {

            if( iq != iSrc ) {

                const SyncStream    &S = vS[iq];

                if( p.sync.sourceIdx != DAQ::eSyncSourceNone && !S.bySync )
                    return false;

                vEdge[iq] = S.TAbs2Ct( S.tAbs );
            }
        }
    }

    if( found )
        vEdge[iSrc] = aEdgeCtNext;

    return found;
}


// Return true if no errors.
//
bool TrigSpike::xferAll( TrSpkShared &shr, QString &err )
{
    bool    maxOK;

    shr.awake   = 0;
    shr.asleep  = 0;
    shr.errors  = 0;

// Wake all threads

    shr.condWake.wakeAll();

// Do last stream locally

    maxOK = locWorker->writeSome( iqMax );

// Wait all threads started, and all done

    shr.runMtx.lock();
        while( shr.awake  < nThd
            || shr.asleep < nThd ) {

            shr.runMtx.unlock();
                QThread::msleep( LOOP_MS/8 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

    if( maxOK && !shr.errors )
        return true;

    err = "write failed";
    return false;
}


