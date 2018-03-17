
#include "TrigImmed.h"
#include "Util.h"
#include "DataFile.h"

#include <QThread>


#define LOOP_MS     100


static TrigImmed    *ME;


/* ---------------------------------------------------------------- */
/* TrImmWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrImmWorker::run()
{
    const int   nID = vID.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iID = 0; iID < nID; ++iID ) {

            if( !(ok = writeSomeIM( vID[iID] )) )
                break;
        }
    }

    emit finished();
}


bool TrImmWorker::writeSomeIM( int ip )
{
    vec_i16 data;
    quint64 headCt = shr.imNextCt[ip];

    try {
        data.reserve( 1.05 * 0.10 * imQ[ip]->chanRate() );
    }
    catch( const std::exception& ) {
        Error() << "Trigger low mem";
        return false;
    }

    if( !imQ[ip]->getAllScansFromCt( data, headCt ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

    shr.imNextCt[ip] += size / imQ[ip]->nChans();

    return ME->writeAndInvalData( ME->DstImec, ip, data, headCt );
}

/* ---------------------------------------------------------------- */
/* TrImmThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrImmThread::TrImmThread(
    TrImmShared         &shr,
    const QVector<AIQ*> &imQ,
    QVector<int>        &vID )
{
    thread  = new QThread;
    worker  = new TrImmWorker( shr, imQ, vID );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


TrImmThread::~TrImmThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* TrigImmed ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void TrigImmed::setGate( bool hi )
{
    runMtx.lock();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigImmed::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    runMtx.unlock();
}


// Immediate mode triggering simply follows the gate. When the
// gate is high we are saving; when the gate is low we aren't.
//
void TrigImmed::run()
{
    Debug() << "Trigger thread started.";

// ---------
// Configure
// ---------

    ME = this;

    QString err;
    quint64 niNextCt = 0;

// Create worker threads

    const int               nPrbPerThd = 2;

    QVector<TrImmThread*>   trT;
    TrImmShared             shr( p );

    nThd = 0;

    for( int ip0 = 0; ip0 < nImQ; ip0 += nPrbPerThd ) {

        QVector<int>    vID;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < nImQ )
                vID.push_back( ip0 + id );
            else
                break;
        }

        trT.push_back( new TrImmThread( shr, imQ, vID ) );
        ++nThd;
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    setYieldPeriod_ms( LOOP_MS );

    while( !isStopped() ) {

        double  loopT = getTime();

        // -------
        // Active?
        // -------

        if( !isGateHi() ) {

            endTrig();
            goto next_loop;
        }

        if( !allWriteSome( shr, niNextCt ) ) {
            err = "Generic error";
            break;
        }

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 0.25 ) {

            QString sOn, sWr;
            int     ig, it;

            getGT( ig, it );
            statusOnSince( sOn, ig, it );
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

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete trT[iThd];

// Done

    endRun( err );
}


// One-time concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. It's not likely too old since
// immediate triggering starts as soon as the gate goes
// high. Rather, the target time might be too new, which
// is fixed by retrying on another loop iteration.
//
bool TrigImmed::alignFiles(
    QVector<quint64>    &imNextCt,
    quint64             &niNextCt )
{
    if( (nImQ && !imNextCt.size()) || (niQ && !niNextCt) ) {

        double              gateT   = getGateHiT();
        int                 ns      = vS.size(),
                            offset  = 0;
        QVector<quint64>    nextCt( ns );

        for( int is = 0; is < ns; ++is ) {
            if( 0 != vS[is].Q->mapTime2Ct( nextCt[is], gateT ) )
                return false;
        }

        // set everybody's tAbs
        syncDstTAbsMult( nextCt[0], 0, vS, p );

        if( niQ ) {
           niNextCt = nextCt[0];
           offset   = 1;
        }

        if( nImQ ) {

            imNextCt.resize( nImQ );

            for( int ip = 0; ip < nImQ; ++ip ) {
                const SyncStream    &S = vS[offset+ip];
                imNextCt[ip] = S.TAbs2Ct( S.tAbs );
            }
        }
    }

    return true;
}


// Return true if no errors.
//
bool TrigImmed::writeSomeNI( quint64 &nextCt )
{
    if( !niQ )
        return true;

    vec_i16 data;
    quint64 headCt = nextCt;

    try {
        data.reserve( 1.05 * 0.10 * niQ->chanRate() );
    }
    catch( const std::exception& ) {
        Error() << "Trigger low mem";
        return false;
    }

    if( !niQ->getAllScansFromCt( data, nextCt ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

    nextCt += size / niQ->nChans();

    return writeAndInvalData( DstNidq, 0, data, headCt );
}


// Return true if no errors.
//
bool TrigImmed::xferAll( TrImmShared &shr, quint64 &niNextCt )
{
    bool    niOK;

    shr.awake   = 0;
    shr.asleep  = 0;
    shr.errors  = 0;

// Wake all imec threads

    shr.condWake.wakeAll();

// Do nidq locally

    niOK = writeSomeNI( niNextCt );

// Wait all threads started, and all done

    shr.runMtx.lock();
        while( shr.awake  < nThd
            || shr.asleep < nThd ) {

            shr.runMtx.unlock();
                msleep( LOOP_MS/8 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

    return niOK && !shr.errors;
}


// Return true if no errors.
//
bool TrigImmed::allWriteSome( TrImmShared &shr, quint64 &niNextCt )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        shr.imNextCt.clear();
        niNextCt = 0;

        if( !newTrig( ig, it ) )
            return false;
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFiles( shr.imNextCt, niNextCt ) )
        return true;    // too early

// ----------------------
// Fetch from all streams
// ----------------------

    return xferAll( shr, niNextCt );
}


