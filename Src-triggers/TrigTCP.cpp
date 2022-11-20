
#include "TrigTCP.h"
#include "Util.h"

#include <QThread>


#define LOOP_MS     100


static TrigTCP      *ME;


/* ---------------------------------------------------------------- */
/* TrTCPWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrTCPWorker::run()
{
    const int   niq = viq.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iiq = 0; iiq < niq; ++iiq ) {

            if( shr.tRem > 0 )
                ok = writeRem( viq[iiq], shr.tRem );
            else
                ok = writeSome( viq[iiq] );

            if( !ok )
                break;
        }
    }

    emit finished();
}


// Return true if no errors.
//
bool TrTCPWorker::writeSome( int iq )
{
    const SyncStream    &S = vS[iq];

    vec_i16 data;
    quint64 headCt = shr.iqNextCt[iq];

    if( !ME->nSampsFromCt( data, headCt, -LOOP_MS, S.js, S.ip ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

    shr.iqNextCt[iq] += size / S.Q->nChans();

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}


// Return true if no errors.
//
bool TrTCPWorker::writeRem( int iq, double tlo )
{
    const SyncStream    &S = vS[iq];

    quint64 spnCt = tlo * S.Q->sRate(),
            curCt = ME->sampCount( S.js );

    if( curCt >= spnCt )
        return true;

    vec_i16 data;
    quint64 headCt = shr.iqNextCt[iq];

    if( !ME->nSampsFromCt( data, headCt, spnCt - curCt, S.js, S.ip ) )
        return false;

    if( !data.size() )
        return true;

    return ME->writeAndInvalData( S.js, S.ip, data, headCt );
}

/* ---------------------------------------------------------------- */
/* TrTCPThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrTCPThread::TrTCPThread(
    TrTCPShared             &shr,
    std::vector<SyncStream> &vS,
    std::vector<int>        &viq )
{
    thread  = new QThread;
    worker  = new TrTCPWorker( shr, vS, viq );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


TrTCPThread::~TrTCPThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* TrigTCP -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void TrigTCP::rgtSetTrig( bool hi )
{
    runMtx.lock();

    if( hi ) {

        if( _trigHi )
            Error() << "SetTrig(HI) twice in a row...ignoring second.";
        else
            _trigHiT = nowCalibrated();
    }
    else {
        _trigLoT = nowCalibrated();

        if( !_trigHi )
            Error() << "SetTrig(LO) twice in a row.";
    }

    _trigHi = hi;

    runMtx.unlock();
}


// Remote mode triggering is turned on/off by remote app.
//
void TrigTCP::run()
{
    Debug() << "Trigger thread started.";

// ---------
// Configure
// ---------

    ME = this;

// Create workers and threads

    std::vector<TrTCPThread*>   trT;
    TrTCPShared                 shr( p );
    const int                   nPrbPerThd  = 2;

    iqMax   = vS.size() - 1;
    nThd    = 0;

    for( int iq0 = 0; iq0 < iqMax; iq0 += nPrbPerThd ) {

        std::vector<int>    viq;

        for( int k = 0; k < nPrbPerThd; ++k ) {

            if( iq0 + k < iqMax )
                viq.push_back( iq0 + k );
            else
                break;
        }

        trT.push_back( new TrTCPThread( shr, vS, viq ) );
        ++nThd;
    }

    {   // local worker (last stream)
        std::vector<int>    iq1( 1, iqMax );
        locWorker = new TrTCPWorker( shr, vS, iq1 );
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

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();

        // ---------------
        // If finishing up
        // ---------------

        if( !isGateHi() || !isTrigHi() ) {

            if( allFilesClosed() )
                goto next_loop;

            if( !allFinalWrite( shr, err ) )
                break;

            endTrig();
            goto next_loop;
        }

        // -------------
        // If trigger ON
        // -------------

        if( !allWriteSome( shr, err ) )
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

    delete locWorker;

// Done

    endRun( err );
}


// Per-trigger concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream.
//
bool TrigTCP::alignFiles( std::vector<quint64> &iqNextCt, QString &err )
{
    if( !iqNextCt.size() ) {

        double  trigT = getTrigHiT();

        iqNextCt.resize( iqMax + 1 );

        for( int iq = 0; iq <= iqMax; ++iq ) {
            int where = vS[iq].Q->mapTime2Ct( iqNextCt[iq], trigT );
            if( where < 0 ) {
                err = QString("stream %1 started writing late; samples lost"
                      " (disk busy or large files overwritten)"
                      " <T2Ct %2 T0 %3 endCt %4 srate %5>")
                      .arg( iq )
                      .arg( trigT )
                      .arg( vS[iq].Q->tZero() )
                      .arg( vS[iq].Q->endCount() )
                      .arg( vS[iq].Q->sRate() );
            }
            if( where != 0 ) {
                iqNextCt.clear();
                return false;
            }
        }

        // set everybody's tAbs
        syncDstTAbsMult( iqNextCt[0], 0, vS, p );

        for( int iq = 1; iq <= iqMax; ++iq ) {
            const SyncStream    &S = vS[iq];
            iqNextCt[iq] = S.TAbs2Ct( S.tAbs );
        }
    }

    return true;
}


// Return true if no errors.
//
bool TrigTCP::xferAll( TrTCPShared &shr, double tRem, QString &err )
{
    bool    maxOK;

    shr.tRem    = tRem;
    shr.awake   = 0;
    shr.asleep  = 0;
    shr.errors  = 0;

// Wake all threads

    shr.condWake.wakeAll();

// Do last stream locally

    if( tRem > 0 )
        maxOK = locWorker->writeRem( iqMax, tRem );
    else
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


// Return true if no errors.
//
bool TrigTCP::allWriteSome( TrTCPShared &shr, QString &err )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        shr.iqNextCt.clear();

        if( !newTrig( ig, it ) ) {
            err = "open file failed";
            return false;
        }
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFiles( shr.iqNextCt, err ) )
        return err.isEmpty();

// ----------------------
// Fetch from all streams
// ----------------------

    return xferAll( shr, -1, err );
}


// Return true if no errors.
//
bool TrigTCP::allFinalWrite( TrTCPShared &shr, QString &err )
{
// Stopping due to gate or trigger going low.
// Set tlo to the shorter time span from thi.

    double  glo = getGateLoT(),
            tlo = getTrigLoT(),
            thi = getTrigHiT();

    if( glo > thi )
        glo -= thi;
    else
        glo = 48*3600;  // arb time (48 hrs) > AIQ capacity

    if( tlo > thi )
        tlo -= thi;
    else
        tlo = 48*3600;  // arb time (48 hrs) > AIQ capacity

    if( tlo > glo )
        tlo = glo;

// If our current count is short, fetch remainder.

    return xferAll( shr, tlo, err );
}


