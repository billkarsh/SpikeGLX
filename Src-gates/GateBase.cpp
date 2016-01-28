
#include "GateImmed.h"
#include "GateTCP.h"
#include "TrigBase.h"
#include "Util.h"
#include "DAQ.h"
#include "GraphsWindow.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* GateBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

GateBase::GateBase( TrigBase *trg, GraphsWindow *gw )
    :   QObject(0), trg(trg), gw(gw),
        pleaseStop(false), _canSleep(true)
{
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

Gate::Gate( DAQ::Params &p, TrigBase *trg, GraphsWindow *gw )
{
    thread  = new QThread;

    if( p.mode.mGate == DAQ::eGateImmed )
        worker = new GateImmed( trg, gw );
    else if( p.mode.mGate == DAQ::eGateTCP )
        worker = new GateTCP( trg, gw );

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


