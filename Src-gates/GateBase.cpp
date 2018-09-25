
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

// @@@ FIX Need real test of software triggering here.

    double  tStart      = getTime();
    int     np          = (im ? im->worker->nAIQ() : 0);
    bool    softTrig    = (p.im.all.trgSource == 0);

    if( im )
        im->configure();

    if( ni )
        ni->configure();

// ------------------------
// Watch for all configured
// ------------------------

// Config allowance: np probes X 20 sec each + 10 sec NI.

    while( !isStopped() ) {

        if( getTime() - tStart > np*20.0 + 10.0 ) {
            QString err = "Gate hardware configuration timed out.";
            Warning() << err;
            emit daqError( err );
            goto wait_external_kill;
        }

        // Test thread state so we don't access
        // a worker that exited due to an error
        // or due to user abort.

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

// If triggering by software command, allow modest delay (10 sec)
// for trigger signal. If hardware triggering, wait indefinitely.

    tStart = getTime();

    while( !isStopped() ) {

        if( softTrig && (getTime() - tStart > 10.0) ) {
            QString err = "Gate sample detector timed out.";
            Warning() << err;
            emit daqError( err );
            goto wait_external_kill;
        }

        // Test thread state so we don't access
        // a worker that exited due to an error
        // or due to user abort.

        if( im && !im->thread->isRunning() )
            goto wait_external_kill;

        if( ni && !ni->thread->isRunning() )
            goto wait_external_kill;

        if( im ) {

            for( int ip = 0; ip < np; ++ip ) {

                if( !im->worker->getAIQ( ip )->endCount() )
                    goto samples_loop_again;
            }
        }

        if( ni && !ni->worker->getAIQ()->endCount() )
            goto samples_loop_again;

        break;

samples_loop_again:;
        // MS: We might consider sleeping here to allow acquisition
        // threads more cycles to get going. That needs to be tested
        // on several machines because sleeps are not well controlled,
        // and we might introduce long startup latency.
    }

// ---------
// Adjust t0
// ---------

    if( im && (ni || np > 1)
        && p.sync.sourceIdx != DAQ::eSyncSourceNone ) {

        double  tSync0 = getTime();

        // Load vS up with streams, NI in front if used.
        // That is, assume changing only imec tZeros.

        QVector<SyncStream>  vS( np );

        for( int ip = 0; ip < np; ++ip )
            vS[ip].init( im->worker->getAIQ( ip ), ip, p );

        if( ni ) {
            vS.push_front( SyncStream() );
            vS[0].init( ni->worker->getAIQ(), -1, p );
        }

        // Use entry zero as ref, delete others as completed

        while( !isStopped() && vS.size() > 1 ) {

            QThread::msleep( 1000*p.sync.sourcePeriod/8 );

            syncDstTAbsMult( vS[0].Q->endCount(), 0, vS, p );

            double  srcTAbs = vS[0].tAbs;

            for( int is = vS.size() - 1; is > 0; --is ) {

                const SyncStream    &dst = vS[is];

                if( dst.bySync ) {

                    if( dst.tAbs != srcTAbs ) {

                        AIQ *Q = im->worker->getAIQ( dst.ip );
                        Q->setTZero( Q->tZero() - dst.tAbs + srcTAbs );
                    }

                    vS.remove( is );
                }
            }

            if( getTime() - tSync0 > 10 * p.sync.sourcePeriod ) {
                QString err = "Sync pulser signal not detected.";
                Warning() << err;
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
// Run termination initiated externally
// ------------------------------------

wait_external_kill:
    baseSleep();
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
    thread = new QThread;

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


