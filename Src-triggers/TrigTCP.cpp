
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
    const int   nID = vID.size();
    bool        ok  = true;

    for(;;) {

        if( !shr.wake( ok ) )
            break;

        for( int iID = 0; iID < nID; ++iID ) {

            if( shr.tRem > 0 )
                ok = writeRemIM( vID[iID], shr.tRem );
            else
                ok = writeSomeIM( vID[iID] );

            if( !ok )
                break;
        }
    }

    emit finished();
}


// Return true if no errors.
//
bool TrTCPWorker::writeSomeIM( int ip )
{
    std::vector<AIQ::AIQBlock>  vB;

    if( !imQ[ip]->getAllScansFromCt( vB, shr.imNextCt[ip] ) )
        return false;

    if( !vB.size() )
        return true;

    shr.imNextCt[ip] = imQ[ip]->nextCt( vB );

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}


// Return true if no errors.
//
bool TrTCPWorker::writeRemIM( int ip, double tlo )
{
    quint64     spnCt = tlo * imQ[ip]->sRate(),
                curCt = ME->scanCount( ME->DstImec );

    if( curCt >= spnCt )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !imQ[ip]->getNScansFromCt( vB, shr.imNextCt[ip], spnCt - curCt ) )
        return false;

    if( !vB.size() )
        return true;

    return ME->writeAndInvalVB( ME->DstImec, ip, vB );
}

/* ---------------------------------------------------------------- */
/* TrTCPThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrTCPThread::TrTCPThread(
    TrTCPShared         &shr,
    const QVector<AIQ*> &imQ,
    QVector<int>        &vID )
{
    thread  = new QThread;
    worker  = new TrTCPWorker( shr, imQ, vID );

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

        if( trigHi )
            Error() << "SetTrig(HI) twice in a row...ignoring second.";
        else
            trigHiT = getTime();
    }
    else {
        trigLoT = getTime();

        if( !trigHi )
            Warning() << "SetTrig(LO) twice in a row.";
    }

    trigHi = hi;

    runMtx.unlock();
}


void TrigTCP::setGate( bool hi )
{
    runMtx.lock();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigTCP::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
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

    quint64 niNextCt = 0;

// Create worker threads

    const int               nPrbPerThd = 2;

    QVector<TrTCPThread*>   trT;
    TrTCPShared             shr( p );

    nThd = 0;

    for( int ip0 = 0; ip0 < nImQ; ip0 += nPrbPerThd ) {

        QVector<int>    vID;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < nImQ )
                vID.push_back( ip0 + id );
            else
                break;
        }

        trT.push_back( new TrTCPThread( shr, imQ, vID ) );
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

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();

        // ---------------
        // If finishing up
        // ---------------

        if( !isGateHi() || !isTrigHi() ) {

            if( allFilesClosed() )
                goto next_loop;

            if( !allFinalWrite( shr, niNextCt ) ) {
                err = "Generic error";
                break;
            }

            endTrig();
            goto next_loop;
        }

        // -------------
        // If trigger ON
        // -------------

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

    endRun( err );
}


// Per-trigger concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. It's not likely too old since
// trigger high command was just received. Rather, the
// target time might be too new, which is fixed by
// retrying on another loop iteration.
//
bool TrigTCP::alignFiles(
    QVector<quint64>    &imNextCt,
    quint64             &niNextCt )
{
    if( (nImQ && !imNextCt.size()) || (niQ && !niNextCt) ) {

        double              trigT   = getTrigHiT();
        int                 ns      = vS.size(),
                            offset  = 0;
        QVector<quint64>    nextCt( ns );

        for( int is = 0; is < ns; ++is ) {
            if( 0 != vS[is].Q->mapTime2Ct( nextCt[is], trigT ) )
                return false;
        }

        if( ns > 1 )
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
bool TrigTCP::writeSomeNI( quint64 &nextCt )
{
    if( !niQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !niQ->getAllScansFromCt( vB, nextCt ) )
        return false;

    if( !vB.size() )
        return true;

    nextCt = niQ->nextCt( vB );

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Return true if no errors.
//
bool TrigTCP::writeRemNI( quint64 &nextCt, double tlo )
{
    if( !niQ )
        return true;

    quint64 spnCt = tlo * niQ->sRate(),
            curCt = scanCount( DstNidq );

    if( curCt >= spnCt )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !niQ->getNScansFromCt( vB, nextCt, spnCt - curCt ) )
        return false;

    if( !vB.size() )
        return true;

    return writeAndInvalVB( DstNidq, 0, vB );
}


// Return true if no errors.
//
bool TrigTCP::xferAll( TrTCPShared &shr, quint64 &niNextCt, double tRem )
{
    bool    niOK;

    shr.tRem    = tRem;
    shr.awake   = 0;
    shr.asleep  = 0;
    shr.errors  = 0;

// Wake all imec threads

    shr.condWake.wakeAll();

// Do nidq locally

    if( tRem > 0 )
        niOK = writeRemNI( niNextCt, tRem );
    else
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
bool TrigTCP::allWriteSome( TrTCPShared &shr, quint64 &niNextCt )
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

    return xferAll( shr, niNextCt, -1 );
}


// Return true if no errors.
//
bool TrigTCP::allFinalWrite( TrTCPShared &shr, quint64 &niNextCt )
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

    return xferAll( shr, niNextCt, tlo );
}


