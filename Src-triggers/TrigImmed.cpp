
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

    if( !ME->nScansFromCt( data, headCt, -LOOP_MS, ip ) )
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
    std::vector<int>    &vID )
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

    const int                   nPrbPerThd = 2;

    std::vector<TrImmThread*>   trT;
    TrImmShared                 shr( p );

    nThd = 0;

    for( int ip0 = 0; ip0 < nImQ; ip0 += nPrbPerThd ) {

        std::vector<int>    vID;

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
                QThread::usleep( 10 );
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

        if( !allWriteSome( shr, niNextCt, err ) )
            break;

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

// Done

    endRun( err );
}


// One-time concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream.
//
bool TrigImmed::alignFiles(
    std::vector<quint64>    &imNextCt,
    quint64                 &niNextCt,
    QString                 &err )
{
    if( (nImQ && !imNextCt.size()) || (niQ && !niNextCt) ) {

        double                  gateT   = getGateHiT();
        int                     ns      = vS.size(),
                                offset  = 0;
        std::vector<quint64>    nextCt( ns );

        for( int is = 0; is < ns; ++is ) {
            int where = vS[is].Q->mapTime2Ct( nextCt[is], gateT );
            if( where < 0 ) {
                err = "writing started late; samples lost"
                      " (disk busy or large files overwritten)";
            }
            if( where != 0 )
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

    if( !nScansFromCt( data, headCt, -LOOP_MS, -1 ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

    nextCt += size / niQ->nChans();

    return writeAndInvalData( DstNidq, 0, data, headCt );
}


// Return true if no errors.
//
bool TrigImmed::xferAll(
    TrImmShared &shr,
    quint64     &niNextCt,
    QString     &err )
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
                QThread::msleep( LOOP_MS/8 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

    if( niOK && !shr.errors )
        return true;

    err = "write failed";
    return false;
}


// Return true if no errors.
//
bool TrigImmed::allWriteSome(
    TrImmShared &shr,
    quint64     &niNextCt,
    QString     &err )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        shr.imNextCt.clear();
        niNextCt = 0;

        if( !newTrig( ig, it ) ) {
            err = "open file failed";
            return false;
        }
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFiles( shr.imNextCt, niNextCt, err ) )
        return err.isEmpty();

// ----------------------
// Fetch from all streams
// ----------------------

    return xferAll( shr, niNextCt, err );
}


