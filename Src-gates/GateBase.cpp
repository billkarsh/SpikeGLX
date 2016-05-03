
#include "IMReader.h"
#include "NIReader.h"
#include "GateImmed.h"
#include "GateTCP.h"
#include "TrigBase.h"
#include "Util.h"
#include "DAQ.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* GateBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

GateBase::GateBase( IMReader *im, NIReader *ni, TrigBase *trg )
    :   QObject(0), im(im), ni(ni), trg(trg),
        _canSleep(true), pleaseStop(false)
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
            QString err = "Gate startup synchronizer timed out.";
            Error() << err;
            emit daqError( err );
            goto wait_external_kill;
        }

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
    DAQ::Params     &p,
    IMReader        *im,
    NIReader        *ni,
    TrigBase        *trg )
{
    thread  = new QThread;

    if( p.mode.mGate == DAQ::eGateImmed )
        worker = new GateImmed( im, ni, trg );
    else if( p.mode.mGate == DAQ::eGateTCP )
        worker = new GateTCP( im, ni, trg );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
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


