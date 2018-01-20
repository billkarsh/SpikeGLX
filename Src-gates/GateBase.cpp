
#include "IMReader.h"
#include "NIReader.h"
#include "GateImmed.h"
#include "GateTCP.h"
#include "TrigBase.h"
#include "Util.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* GateBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

GateBase::GateBase(
    const DAQ::Params   &p,
    IMReader            *im,
    NIReader            *ni,
    TrigBase            *trg  )
    :   QObject(0), p(p), im(im), ni(ni),
        _canSleep(true), pleaseStop(false),
        trg(trg)
{
}


bool GateBase::baseStartReaders()
{
// ------------------------------------------------
// Start streams...they should configure, then wait
// ------------------------------------------------

    double  tStart = getTime();

    if( im )
        im->configure();

    if( ni )
        ni->configure();

// ------------------------
// Watch for all configured
// ------------------------

    while( !isStopped() ) {

// BK: Gain correction takes ~5 minutes

        if( getTime() - tStart > 6*60.0 ) {
            QString err = "Gate config synchronizer timed out.";
            Error() << err;
            emit daqError( err );
            goto wait_external_kill;
        }

        // Test thread state so we don't access
        // a worker that exited due to an error.

        if( im && !im->thread->isRunning() )
            goto wait_external_kill;

        if( ni && !ni->thread->isRunning() )
            goto wait_external_kill;

        if( im && !im->worker->isReady() )
            continue;

        if( ni && !ni->worker->isReady() )
            continue;

        break;
    }

// -----------------
// Start acquisition
// -----------------

    if( im )
        im->worker->start();

    if( ni )
        ni->worker->start();

// -----------------
// Watch for samples
// -----------------

    tStart = getTime();

    while( !isStopped() ) {

// BK: Wait up to ten seconds for samples

        if( getTime() - tStart > 10.0 ) {
            QString err = "Gate sample detector timed out.";
            Error() << err;
            emit daqError( err );
            goto wait_external_kill;
        }

        // Test thread state so we don't access
        // a worker that exited due to an error.

        if( im && !im->thread->isRunning() )
            goto wait_external_kill;

        if( ni && !ni->thread->isRunning() )
            goto wait_external_kill;

        if( im && !im->worker->getAIQ()->endCount() )
            continue;

        if( ni && !ni->worker->getAIQ()->endCount() )
            continue;

        break;
    }

// ---------
// Adjust t0
// ---------

    if( im && ni && p.sync.sourceIdx != DAQ::eSyncSourceNone ) {

        double      tSync0 = getTime();
        SyncStream  imS, niS;

        imS.init( im->worker->getAIQ(),  0, p );
        niS.init( ni->worker->getAIQ(), -1, p );

        while( !isStopped() ) {

            msleep( 1000*p.sync.sourcePeriod/8 );

            syncDstTAbs( niS.Q->endCount(), &niS, &imS, p );

            if( imS.bySync ) {

                if( imS.tAbs != niS.tAbs ) {

                    AIQ *Q = im->worker->getAIQ();
                    Q->setTZero( Q->tZero() - imS.tAbs + niS.tAbs );
                }

                break;
            }

            if( getTime() - tSync0 > 10 * p.sync.sourcePeriod ) {
                QString err = "Sync pulser signal not detected.";
                Error() << err;
                emit daqError( err );
                goto wait_external_kill;
            }
        }
    }

// -------------
// Start clients
// -------------

    trg->setStartT();
    emit runStarted();
    return true;

// ------------------------------------
// Run termination initialed externally
// ------------------------------------

wait_external_kill:
    baseSleep();
    emit finished();
    return false;
}


void GateBase::baseSleep()
{
    if( canSleep() ) {
        runMtx.lock();
        condWake.wait( &runMtx );
        runMtx.unlock();
    }
}

/* ---------------------------------------------------------------- */
/* Gate ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Gate::Gate(
    const DAQ::Params   &p,
    IMReader            *im,
    NIReader            *ni,
    TrigBase            *trg )
{
    thread  = new QThread;

    if( p.mode.mGate == DAQ::eGateImmed )
        worker = new GateImmed( p, im, ni, trg );
    else if( p.mode.mGate == DAQ::eGateTCP )
        worker = new GateTCP( p, im, ni, trg );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Thread manually started by run, via startRun().
//    thread->start();
}


Gate::~Gate()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stayAwake();
        worker->wake();
        worker->stop();
        thread->wait();
    }

    delete thread;
}


void Gate::startRun()
{
    thread->start();
}


