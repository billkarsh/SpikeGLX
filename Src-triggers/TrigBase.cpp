
#include "TrigImmed.h"
#include "TrigSpike.h"
#include "TrigTimed.h"
#include "TrigTCP.h"
#include "TrigTTL.h"
#include "Util.h"
#include "MainApp.h"
#include "DataFileNI.h"
#include "GraphsWindow.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* TrigBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigBase::TrigBase( DAQ::Params &p, GraphsWindow *gw, const AIQ *aiQ  )
    :   QObject(0), p(p), df(0), gw(gw), aiQ(aiQ),
        runDir(mainApp()->runDir()), statusT(-1), startT(getTime()),
        gateHiT(-1), gateLoT(-1), iGate(-1), iTrig(-1), gateHi(false),
        paused(p.mode.trgInitiallyOff), pleaseStop(false)
{
}


QString TrigBase::curFilename()
{
    QMutexLocker    ml( &dfMtx );

    return (df ? df->binFileName() : QString::null);
}


void TrigBase::pause( bool pause )
{
    runMtx.lock();

    gateHiT = getTime();
    paused  = pause;

    runMtx.unlock();
}


void TrigBase::forceGTCounters( int g, int t )
{
    runMtx.lock();

    iGate   = g;
    iTrig   = t - 1;

    runMtx.unlock();
}


void TrigBase::baseSetGate( bool hi )
{
    if( hi ) {
        gateHiT = getTime();
        ++iGate;
        iTrig   = -1;
    }
    else
        gateLoT = getTime();

    gateHi  = hi;
}


// Called when the run name has been changed, effectively beginning
// a new run. This does not affect the hi/lo state of the gate, but
// if the gate count is higher than zero we push it back to zero.
//
void TrigBase::baseResetGTCounters()
{
    if( iGate > 0 )
        iGate = 0;

    iTrig = -1;
}


void TrigBase::endTrig()
{
    if( df ) {
        dfMtx.lock();
        df = df->closeAsync( kvmRmt );
        dfMtx.unlock();
    }

    if( gw ) {
        QMetaObject::invokeMethod(
            gw, "setTriggerLED",
            Qt::QueuedConnection,
            Q_ARG(bool, false) );
    }
}


bool TrigBase::newTrig( int &ig, int &it, bool trigLED )
{
    endTrig();

    it = incTrig( ig );

    dfMtx.lock();
    df = new DataFileNI();
    dfMtx.unlock();

    QString name = QString("%1/%2_g%3_t%4.nidq.bin")
                    .arg( runDir )
                    .arg( p.sns.runName )
                    .arg( ig )
                    .arg( it );

    if( !df->openForWrite( p, name ) ) {
        Error()
            << "Error opening file: ["
            << name
            << "].";
        return false;
    }

    if( trigLED && gw ) {
        QMetaObject::invokeMethod(
            gw, "setTriggerLED",
            Qt::QueuedConnection,
            Q_ARG(bool, true) );
    }

    return true;
}


bool TrigBase::writeAndInvalVB( std::vector<AIQ::AIQBlock> &vB )
{
    int nb = (int)vB.size();

    for( int i = 0; i < nb; ++i ) {

        if( !df->writeAndInvalSubset( p, vB[i].data ) )
            return false;
    }

    return true;
}


void TrigBase::statusOnSince( QString &s, double nowT, int ig, int it )
{
    double  t = nowT - startT;
    int     h, m;

    h = int(t / 3600);
    t = t - h * 3600;
    m = t / 60;
    t = t - m * 60;

    s = QString("ON %1h%2m%3s %4CH %5kHz <G%6 T%7>")
        .arg( h, 2, 10, QChar('0') )
        .arg( m, 2, 10, QChar('0') )
        .arg( t, 0, 'f', 1 )
        .arg( p.ni.niCumTypCnt[CniCfg::niSumAll] )
        .arg( p.ni.srate / 1e3, 0, 'f', 3 )
        .arg( ig )
        .arg( it );
}


void TrigBase::statusWrPerf( QString &s )
{
    if( df ) {

        s = QString(" QFill=%1% MB/s=%2 (%3 req)")
            .arg( df->percentFull(), 0, 'f', 2 )
            .arg( df->writeSpeedBytesSec()/1e6, 0, 'f', 1 )
            .arg( df->minimalWriteSpeedRequired()/1e6, 0, 'f', 1 );
    }
    else
        s = QString::null;
}


void TrigBase::setYieldPeriod_ms( int loopPeriod_ms )
{
    if( loopPeriod_ms > 0 )
        loopPeriod_us = 1000 * loopPeriod_ms;
    else
        loopPeriod_us = 1000 * daqAIFetchPeriodMillis();
}


void TrigBase::yield( double loopT )
{
    loopT = 1e6 * (getTime() - loopT);	// microsec

    if( loopT < loopPeriod_us )
        usleep( loopPeriod_us - loopT );
    else
        usleep( 1000 * 10 );
}

/* ---------------------------------------------------------------- */
/* Trigger -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Trigger::Trigger( DAQ::Params &p, GraphsWindow *gw, const AIQ *aiQ )
{
    thread  = new QThread;

    if( p.mode.mTrig == DAQ::eTrigImmed )
        worker = new TrigImmed( p, gw, aiQ );
    else if( p.mode.mTrig == DAQ::eTrigTimed )
        worker = new TrigTimed( p, gw, aiQ );
    else if( p.mode.mTrig == DAQ::eTrigTTL )
        worker = new TrigTTL( p, gw, aiQ );
    else if( p.mode.mTrig == DAQ::eTrigSpike )
        worker = new TrigSpike( p, gw, aiQ );
    else
        worker = new TrigTCP( p, gw, aiQ );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


Trigger::~Trigger()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}


