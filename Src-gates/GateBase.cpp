
#include "IMReader.h"
#include "NIReader.h"
#include "GateImmed.h"
#include "GateTCP.h"
#include "TrigBase.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

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
    QString err;
    int     nIM         = (im ? im->worker->nAIQ( jsIM ) : 0),
            nOB         = (im ? im->worker->nAIQ( jsOB ) : 0);
    bool    softTrig    = (p.im.prbAll.trgSource == 0);

    if( im )
        im->configure();

    if( ni )
        ni->configure();

// ------------------------
// Watch for all configured
// ------------------------

// Config allowance: nIM probes X 20 sec each + 20 sec others.

    while( !isStopped() ) {

        if( getTime() - tStart > nIM*20.0 + 20.0 ) {
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

        err         = "Gate not detecting samples on streams:";
        int nbad    = 0;

        if( im ) {

            for( int ip = 0; ip < nIM; ++ip ) {

                if( !im->worker->getAIQ( jsIM, ip )->endCount() ) {
                    if( !nbad )
                        err += "\n    { ";
                    else if( !(nbad % 8) )
                        err += "\n      ";
                    else
                        err += ", ";
                    ++nbad;
                    err += QString("IM%1").arg( ip );
                }
            }

#ifdef HAVE_IMEC
            for( int ip = 0; ip < nOB; ++ip ) {

                if( !im->worker->getAIQ( jsOB, ip )->endCount() ) {
                    if( !nbad )
                        err += "\n    { ";
                    else if( !(nbad % 8) )
                        err += "\n      ";
                    else
                        err += ", ";
                    ++nbad;
                    err += QString("OB%1").arg( ip );
                }
            }
#endif
        }

        if( ni && !ni->worker->getAIQ()->endCount() ) {
            if( !nbad )
                err += "\n    { ";
            else if( !(nbad % 8) )
                err += "\n      ";
            else
                err += ", ";
            ++nbad;
            err += "NI";
        }

        err += " }.";

        if( nbad )
            goto samples_loop_again;

        break;

samples_loop_again:;
        // MS: We might consider sleeping here to allow acquisition
        // threads more cycles to get going. That needs to be tested
        // on several machines because sleeps are not well controlled,
        // and we might introduce long startup latency.
    }

// ------------------------------------------------------
// Allow 10ms so immediate gateT falls within each stream
// ------------------------------------------------------

    {
        double  tAcq0 = getTime();
        while( getTime() - tAcq0 < 0.01 )
            ;
    }

// ---------
// Adjust t0
// ---------

    if( p.sync.sourceIdx != DAQ::eSyncSourceNone
        && (ni != 0) + nOB + nIM > 1 ) {

        double  tSync0 = getTime();

        const CimCfg::ImProbeTable  &T = mainApp()->cfgCtl()->prbTab;
        std::vector<SyncStream>     vS( (ni != 0) + nOB + nIM );
        int                         nq = 0;

        if( ni )
            vS[nq++].init( ni->worker->getAIQ(), jsNI, 0, p );

        for( int ip = 0; ip < nOB; ++ip )
            vS[nq++].init( im->worker->getAIQ( jsOB, ip ), jsOB, ip, p );

        for( int ip = 0; ip < nIM; ++ip )
            vS[nq++].init( im->worker->getAIQ( jsIM, ip ), jsIM, ip, p );

        // Use entry zero as ref, delete others as completed

        while( !isStopped() && vS.size() > 1 ) {

            QThread::msleep( 1000*p.sync.sourcePeriod/8 );

            syncDstTAbsMult( vS[0].Q->endCount(), 0, vS, p );

            double  srcTAbs = vS[0].tAbs;

            for( int is = vS.size() - 1; is > 0; --is ) {

                SyncStream  &dst = vS[is];

                if( dst.js == jsIM ) {
                    const CimCfg::ImProbeDat    &P = T.get_iProbe( dst.ip );
                    if( T.simprb.isSimProbe( P.slot, P.port, P.dock ) )
                        dst.bySync = true;
                }

                if( dst.bySync ) {

                    if( dst.tAbs != srcTAbs )
                        dst.Q->setTZero( dst.Q->tZero() - dst.tAbs + srcTAbs );

                    vS.erase( vS.begin() + is );
                }
            }

            if( getTime() - tSync0 > 10 * p.sync.sourcePeriod ) {

                    err     = "Sync signal not detected/matched on streams:";
                int nbad    = 0;

                foreach( const SyncStream &S, vS ) {

                    if( !nbad )
                        err += "\n    { ";
                    else if( !(nbad % 8) )
                        err += "\n      ";
                    else
                        err += ", ";
                    ++nbad;
                    switch( S.js ) {
                        case jsNI: err += "NI"; break;
                        case jsOB: err += QString("OB%1").arg( S.ip ); break;
                        case jsIM: err += QString("IM%1").arg( S.ip ); break;
                    }
                }

                err += " }.";

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


