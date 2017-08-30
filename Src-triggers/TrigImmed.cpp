
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
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = imQ[ip]->getAllScansFromCt( vB, shr.imNextCt[ip] );

    if( !nb )
        return true;

    shr.imNextCt[ip] = imQ[ip]->nextCt( vB );

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
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

        if( !allWriteSome( shr, niNextCt ) )
            break;

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 0.25 ) {

            QString sOn, sWr;
            int     ig, it;

            getGT( ig, it );
            statusOnSince( sOn, loopT, ig, it );
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

    endRun();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


// Next file @ X12 boundary
//
// One-time concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. It's not likely too old since
// immediate triggering starts as soon as the gate goes
// high. Rather, the target time might be newer than
// any sample tag, which is fixed by retrying on another
// loop iteration.
//
bool TrigImmed::alignFiles(
    QVector<quint64>    &imNextCt,
    quint64             &niNextCt )
{
// MS: Assuming sample counts and mapping for imQ[0] serve for all
    if( (nImQ && !imNextCt.size()) || (niQ && !niNextCt) ) {

        double  gateT = getGateHiT();
        quint64 imNext, niNext;

        if( niQ && !niQ->mapTime2Ct( niNext, gateT ) )
            return false;

        if( nImQ ) {

            if( !imQ[0]->mapTime2Ct( imNext, gateT ) )
                return false;

            alignX12( imNext, niNext );
            imNextCt.fill( imNext, nImQ );
        }

        niNextCt = niNext;
    }

    return true;
}


// Return true if no errors.
//
bool TrigImmed::writeSomeNI( quint64 &nextCt )
{
    if( !niQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getAllScansFromCt( vB, nextCt );

    if( !nb )
        return true;

    nextCt = niQ->nextCt( vB );

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Return true if no errors.
//
bool TrigImmed::xferAll( TrImmShared &shr, quint64 &niNextCt )
{
    int niOK;

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


