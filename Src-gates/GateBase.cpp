
#include "IMReader.h"
#include "NIReader.h"
#include "TrigBase.h"
#include "GateImmed.h"
#include "GateTCP.h"
#include "Util.h"
#include "DAQ.h"
#include "GraphsWindow.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* GateBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

GateBase::GateBase(
    IMReader        *im,
    NIReader        *ni,
    TrigBase        *trg,
    GraphsWindow    *gw )
    :   QObject(0), im(im), ni(ni), trg(trg), gw(gw),
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

// BK: Need nominal startup time
        if( getTime() - tStart > 4*60.0 ) {
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


void GateBase::baseSetGate( bool hi )
{
    trg->setGate( hi );

    QMetaObject::invokeMethod(
        gw, "setGateLED",
        Qt::QueuedConnection,
        Q_ARG(bool, hi) );
}

/* ---------------------------------------------------------------- */
/* Gate ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Gate::Gate(
    DAQ::Params     &p,
    IMReader        *im,
    NIReader        *ni,
    TrigBase        *trg,
    GraphsWindow    *gw )
{
    thread  = new QThread;

    if( p.mode.mGate == DAQ::eGateImmed )
        worker = new GateImmed( im, ni, trg, gw );
    else if( p.mode.mGate == DAQ::eGateTCP )
        worker = new GateTCP( im, ni, trg, gw );

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


