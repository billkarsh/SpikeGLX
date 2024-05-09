
#include "TrigTimed.h"
#include "Util.h"
#include "MainApp.h"
#include "Run.h"

#include <QThread>


#define LOOP_MS     100


static TrigTimed    *ME;


/* ---------------------------------------------------------------- */
/* TrTimWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrTimWorker::run()
{
    const int   niq = viq.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iiq = 0; iiq < niq; ++iiq ) {

            if( !(ok = doSomeH( viq[iiq] )) )
                break;
        }
    }

    emit finished();
}


// Return true if no errors.
//
bool TrTimWorker::doSomeH( int iq )
{
    const SyncStream    &S = vS[iq];
    TrigTimed::Counts   &C = ME->cnt;

    vec_i16 data;
    quint64 headCt  = C.nextCt[iq],
            remCt   = C.hiCtMax[iq] - C.hiCtCur[iq];
    uint    nMax    = (remCt <= C.maxFetch[iq] ? remCt : C.maxFetch[iq]);

    if( !ME->nSampsFromCt( data, headCt, nMax, S.js, S.ip ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// ---------------
// Update tracking
// ---------------

    C.nextCt[iq]    += size / S.Q->nChans();
    C.hiCtCur[iq]   += C.nextCt[iq] - headCt;

// -----
// Write
// -----

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}

/* ---------------------------------------------------------------- */
/* TrTimThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrTimThread::TrTimThread(
    TrTimShared             &shr,
    std::vector<SyncStream> &vS,
    std::vector<int>        &viq )
{
    thread  = new QThread;
    worker  = new TrTimWorker( shr, vS, viq );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


TrTimThread::~TrTimThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* Counts --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigTimed::Counts::Counts( const DAQ::Params &p ) : nq(p.stream_nq())
{
    nextCt.assign( nq, 0 );
    hiCtCur.assign( nq, 0 );

    hiCtMax.resize( nq );
    loCt.resize( nq );
    maxFetch.resize( nq );

    double  hitim = p.trgTim.tH - (p.trgTim.tL < 0.0 ? p.trgTim.tL : 0.0);

    for( int iq = 0; iq < nq; ++iq ) {

        double  srate = p.stream_rate( iq );

        hiCtMax[iq]     = (p.trgTim.isHInf ? UNSET64 : qint64(hitim * srate));
        loCt[iq]        = p.trgTim.tL * srate;
        maxFetch[iq]    = 0.400 * srate;
    }
}


bool TrigTimed::Counts::isReset()
{
    for( int iq = 0; iq < nq; ++iq ) {

        if( hiCtCur[iq] )
            return false;
    }

    return true;
}


bool TrigTimed::Counts::hDone()
{
    for( int iq = 0; iq < nq; ++iq ) {

        if( hiCtCur[iq] < hiCtMax[iq] )
            return false;
    }

    return true;
}


void TrigTimed::Counts::hNext()
{
    for( int iq = 0; iq < nq; ++iq )
        nextCt[iq] += loCt[iq];
}

/* ---------------------------------------------------------------- */
/* TrigTimed ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

TrigTimed::TrigTimed(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const QVector<AIQ*> &imQ,
    const QVector<AIQ*> &obQ,
    const AIQ           *niQ )
    :   TrigBase(p, gw, imQ, obQ, niQ),
        cnt(p),
        nCycMax(p.trgTim.isNInf ? UNSET64 : p.trgTim.nH)
{
}


#define SETSTATE_L0     (state = 0)
#define SETSTATE_H      (state = 1)
#define SETSTATE_L      (state = 2)

#define ISSTATE_L0      (state == 0)
#define ISSTATE_H       (state == 1)
#define ISSTATE_L       (state == 2)
#define ISSTATE_Done    (state == 3)


// Timed logic is driven by TrgTimParams: {tL0, tH, tL, nH}.
// There are four corresponding states defined above.
//
void TrigTimed::run()
{
    Debug() << "Trigger thread started.";

// ---------
// Configure
// ---------

    ME = this;

// Create workers and threads

    std::vector<TrTimThread*>   trT;
    TrTimShared                 shr( p );
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

        trT.push_back( new TrTimThread( shr, vS, viq ) );
        ++nThd;
    }

    {   // local worker (last stream)
        std::vector<int>    iq1( 1, iqMax );
        locWorker = new TrTimWorker( shr, vS, iq1 );
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

        double  loopT   = getTime(),
                gHiT,
                delT    = 0;
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive ) {

            endTrig();
            initState();
            goto next_loop;
        }

        // ------------------
        // Current gate start
        // ------------------

        gHiT = getGateHiT();

        // ----------------------------
        // L0 phase (waiting for start)
        // ----------------------------

        if( ISSTATE_L0 )
            delT = remainingL0( loopT, gHiT );

        // -----------------
        // H phase (writing)
        // -----------------

        if( ISSTATE_H ) {

            if( !allDoSomeH( shr, gHiT, err ) )
                break;

            // Done?

            if( cnt.hDone() ) {

                if( ++nH >= nCycMax ) {
                    SETSTATE_Done();
                    inactive = true;
                }
                else {
                    cnt.hNext();
                    SETSTATE_L;
                }

                endTrig();
            }
        }

        // ----------------------------
        // L phase (waiting for next H)
        // ----------------------------

        if( ISSTATE_L )
            delT = remainingL();

        // ------
        // Status
        // ------

next_loop:
        if( loopT - statusT > 1.0 ) {

            QString sOn, sT, sWr;

            statusOnSince( sOn );
            statusWrPerf( sWr );

            if( inactive )
                sT = " TX";
            else if( ISSTATE_L0 || ISSTATE_L )
                sT = QString(" T-%1s").arg( delT, 0, 'f', 1 );
            else {
                // any representative stream OK
                double  hisec = cnt.hiCtCur[0] / vS[0].Q->sRate();
                sT = QString(" T+%1s").arg( hisec, 0, 'f', 1 );
            }

            Status() << sOn << sT << sWr;

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


void TrigTimed::SETSTATE_Done()
{
    state = 3;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigTimed::initState()
{
    nH = 0;

    if( p.trgTim.tL0 > 0 )
        SETSTATE_L0;
    else
        SETSTATE_H;
}


// Return time remaining in L0 phase.
//
double TrigTimed::remainingL0( double loopT, double gHiT )
{
    double  elapsedT = loopT - gHiT;

    if( elapsedT < p.trgTim.tL0 )
        return p.trgTim.tL0 - elapsedT;  // remainder

    SETSTATE_H;
    return 0;
}


// Return time remaining in L phase.
//
double TrigTimed::remainingL()
{
    const AIQ   *aiQ    = vS[0].Q;
    quint64     nextCt  = cnt.nextCt[0],
                endCt   = aiQ->endCount();

    if( endCt < nextCt )
        return (nextCt - endCt) / aiQ->sRate();

    SETSTATE_H;
    return 0;
}


// Per-trigger concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream.
//
bool TrigTimed::alignFiles( double gHiT, QString &err )
{
    if( cnt.isReset() ) {

        double  startT;

        if( !nH ) {

            startT = gHiT + p.trgTim.tL0;

            for( int iq = 0; iq <= iqMax; ++iq ) {
                int where = vS[iq].Q->mapTime2Ct( cnt.nextCt[iq], startT );
                if( where < 0 ) {
                    err = QString("stream %1 started writing late; samples lost"
                          " (disk busy or large files overwritten)"
                          " <T2Ct %2 T0 %3 endCt %4 srate %5>")
                          .arg( iq )
                          .arg( startT )
                          .arg( vS[iq].Q->tZero() )
                          .arg( vS[iq].Q->endCount() )
                          .arg( vS[iq].Q->sRate() );
                }
                if( where != 0 )
                    return false;
            }
        }
        else {

            for( int iq = 0; iq <= iqMax; ++iq ) {
                int where = vS[iq].Q->mapCt2Time( startT, cnt.nextCt[iq] );
                if( where < 0 ) {
                    err = QString("stream %1 started writing late; samples lost"
                          " (disk busy or large files overwritten)"
                          " <Ct2T %2 T0 %3 endCt %4 srate %5>")
                          .arg( iq )
                          .arg( cnt.nextCt[iq] )
                          .arg( vS[iq].Q->tZero() )
                          .arg( vS[iq].Q->endCount() )
                          .arg( vS[iq].Q->sRate() );
                }
                if( where != 0 )
                    return false;
            }
        }

        // set everybody's tAbs
        syncDstTAbsMult( cnt.nextCt[0], 0, vS, p );

        for( int iq = 1; iq <= iqMax; ++iq ) {
            const SyncStream    &S = vS[iq];
            cnt.nextCt[iq] = S.TAbs2Ct( S.tAbs );
        }
    }

    return true;
}


// Return true if no errors.
//
bool TrigTimed::xferAll( TrTimShared &shr, QString &err )
{
    bool    maxOK;

    shr.awake   = 0;
    shr.asleep  = 0;
    shr.errors  = 0;

// Wake all threads

    shr.condWake.wakeAll();

// Do last stream locally

    maxOK = locWorker->doSomeH( iqMax );

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


// Return true if no errors.
//
bool TrigTimed::allDoSomeH( TrTimShared &shr, double gHiT, QString &err )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        cnt.hiCtCur.assign( iqMax + 1, 0 );

        if( !newTrig( ig, it ) ) {
            err = "open file failed";
            return false;
        }
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFiles( gHiT, err ) )
        return err.isEmpty();

// ----------------------
// Fetch from all streams
// ----------------------

    return xferAll( shr, err );
}


