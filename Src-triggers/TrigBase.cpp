
#include "TrigImmed.h"
#include "TrigSpike.h"
#include "TrigTimed.h"
#include "TrigTCP.h"
#include "TrigTTL.h"
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* TrigBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigBase::TrigBase(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   QObject(0), dfim(0), dfni(0), ovr(p),
        startT(-1), gateHiT(-1), gateLoT(-1),
        iGate(-1), iTrig(-1), gateHi(false), pleaseStop(false),
        p(p), gw(gw), imQ(imQ), niQ(niQ), statusT(-1)
{
}


bool TrigBase::needNewFiles() const
{
    QMutexLocker    ml( &dfMtx );

    return (imQ && !dfim) || (niQ && !dfni);
}


bool TrigBase::isInUse( const QFileInfo &fi ) const
{
    QMutexLocker    ml( &dfMtx );

    if( dfim && fi == QFileInfo( dfim->binFileName() ) )
        return true;

    if( dfni && fi == QFileInfo( dfni->binFileName() ) )
        return true;

    return false;
}


// BK: This should probably be deprecated
//
QString TrigBase::curNiFilename() const
{
    QMutexLocker    ml( &dfMtx );

    return (dfni ? dfni->binFileName() : QString::null);
}


void TrigBase::setStartT()
{
    startTMtx.lock();
    startT = getTime();
    startTMtx.unlock();
}


void TrigBase::setGateEnabled( bool enabled )
{
    runMtx.lock();
    gateHiT         = getTime();
    ovr.gateEnab    = enabled;
    runMtx.unlock();

    if( enabled ) {

        if( p.mode.mGate == DAQ::eGateImmed )
            setGate( true );
    }
    else
        setGate( false );
}


void TrigBase::forceGTCounters( int g, int t )
{
    runMtx.lock();
    ovr.set( g, t );
    runMtx.unlock();
}


// All callers must manage runMtx around this.
//
void TrigBase::baseSetGate( bool hi )
{
    if( hi ) {

        if( !ovr.gateEnab )
            return;

        bool    started = false;

        startTMtx.lock();
        started = (startT >= 0);
        startTMtx.unlock();

        if( !started ) {
            Warning()
                <<  "Received setGate(hi) before acquisition started"
                    " -- IGNORED.";
            return;
        }

        gateHiT = getTime();

        if( ovr.forceGT )
            ovr.get( iGate, iTrig );
        else {
            ++iGate;
            iTrig = -1;
        }
    }
    else
        gateLoT = getTime();

    gateHi = hi;

    QMetaObject::invokeMethod(
        gw, "setGateLED",
        Qt::QueuedConnection,
        Q_ARG(bool, hi) );
}


// All callers must manage runMtx around this.
//
void TrigBase::baseResetGTCounters()
{
    ovr.reset();
    iGate = -1;
    iTrig = -1;
}


void TrigBase::endTrig()
{
    dfMtx.lock();
        if( dfim )
            dfim = (DataFileIM*)dfim->closeAsync( kvmRmt );
        if( dfni )
            dfni = (DataFileNI*)dfni->closeAsync( kvmRmt );
    dfMtx.unlock();

    QMetaObject::invokeMethod(
        gw, "setTriggerLED",
        Qt::QueuedConnection,
        Q_ARG(bool, false) );
}


bool TrigBase::newTrig( int &ig, int &it, bool trigLED )
{
    endTrig();

    it = incTrig( ig );

    dfMtx.lock();
        if( imQ )
            dfim = new DataFileIM();
        if( niQ )
            dfni = new DataFileNI();
    dfMtx.unlock();

    if( !openFile( dfim, ig, it, "imec" )
        || !openFile( dfni, ig, it, "nidq" ) ) {

        return false;
    }

    if( trigLED ) {
        QMetaObject::invokeMethod(
            gw, "setTriggerLED",
            Qt::QueuedConnection,
            Q_ARG(bool, true) );
    }

    return true;
}


void TrigBase::setSyncWriteMode()
{
    if( dfim )
        dfim->setAsyncWriting( false );

    if( dfni )
        dfni->setAsyncWriting( false );
}


bool TrigBase::writeAndInvalVB(
    DstStream                   dst,
    std::vector<AIQ::AIQBlock>  &vB )
{
    DataFile    *df;

    if( dst == DstImec )
        df = dfim;
    else
        df = dfni;

    if( !df )
        return true;

    int nb = (int)vB.size();

    for( int i = 0; i < nb; ++i ) {

        if( !df->writeAndInvalSubset( p, vB[i].data ) )
            return false;
    }

    return true;
}


quint64 TrigBase::scanCount( DstStream dst )
{
    DataFile    *df;

    if( dst == DstImec )
        df = dfim;
    else
        df = dfni;

    if( !df )
        return 0;

    return df->scanCount();
}


bool TrigBase::allFilesClosed()
{
    QMutexLocker    ml( &dfMtx );

    return !dfim && !dfni;
}


void TrigBase::endRun()
{
    QMetaObject::invokeMethod(
        gw, "setTriggerLED",
        Qt::QueuedConnection,
        Q_ARG(bool, false) );

    dfMtx.lock();
        if( dfim ) {
            dfim->setRemoteParams( kvmRmt );
            dfim->closeAndFinalize();
            delete dfim;
            dfim = 0;
        }

        if( dfni ) {
            dfni->setRemoteParams( kvmRmt );
            dfni->closeAndFinalize();
            delete dfni;
            dfni = 0;
        }
    dfMtx.unlock();
}


void TrigBase::statusOnSince( QString &s, double nowT, int ig, int it )
{
    double  t;
    int     h, m;

    startTMtx.lock();
    t = (startT >= 0 ? nowT - startT : 0);
    startTMtx.unlock();

    h = int(t / 3600);
    t = t - h * 3600;
    m = t / 60;
    t = t - m * 60;

    QString ch, chim, chni;

    if( p.im.enabled ) {
        chim = QString("%4CH@%5kHz")
                .arg( p.im.imCumTypCnt[CimCfg::imSumAll] )
                .arg( p.im.srate / 1e3, 0, 'f', 3 );
    }

    if( p.ni.enabled ) {
        chni = QString("%4CH@%5kHz")
                .arg( p.ni.niCumTypCnt[CniCfg::niSumAll] )
                .arg( p.ni.srate / 1e3, 0, 'f', 3 );
    }

    if( p.im.enabled && p.ni.enabled )
        ch = QString("{%1, %2}").arg( chim ).arg( chni );
    else if( p.im.enabled )
        ch = chim;
    else
        ch = chni;

    s = QString("ON %1h%2m%3s %4 <G%5 T%6>")
        .arg( h, 2, 10, QChar('0') )
        .arg( m, 2, 10, QChar('0') )
        .arg( t, 0, 'f', 1 )
        .arg( ch )
        .arg( ig )
        .arg( it );

    QString sGW = QString("%1:%2:%3")
        .arg( h, 2, 10, QChar('0') )
        .arg( m, 2, 10, QChar('0') )
        .arg( (int)t, 2, 10, QChar('0') );

    QMetaObject::invokeMethod(
        gw, "updateOnTime",
        Qt::QueuedConnection,
        Q_ARG(QString, sGW) );
}


void TrigBase::statusWrPerf( QString &s )
{
    if( dfim || dfni ) {

        // report worst case values

        double  imFull  = 0.0,
                niFull  = 0.0,
                wbps    = 0.0,
                rbps    = 0.0;

        if( dfim ) {
            imFull  = dfim->percentFull();
            wbps    = dfim->writeSpeedBps();
            rbps    = dfim->requiredBps();
        }

        if( dfni ) {
            niFull = dfni->percentFull();
            wbps  += dfni->writeSpeedBps();
            rbps  += dfni->requiredBps();
        }

        s = QString(" FileQFill%=(%1,%2) MB/s=%3 (%4 req)")
            .arg( imFull, 0, 'f', 1 )
            .arg( niFull, 0, 'f', 1 )
            .arg( wbps/(1024*1024), 0, 'f', 1 )
            .arg( rbps/(1024*1024), 0, 'f', 1 );
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
    loopT = 1e6 * (getTime() - loopT);  // microsec

    if( loopT < loopPeriod_us )
        usleep( loopPeriod_us - loopT );
    else
        usleep( 1000 * 10 );
}


bool TrigBase::openFile(
    DataFile    *df,
    int         ig,
    int         it,
    const char  *type )
{
    if( !df )
        return true;

    QString name = QString("%1/%2_g%3_t%4.%5.bin")
                    .arg( mainApp()->runDir() )
                    .arg( p.sns.runName )
                    .arg( ig )
                    .arg( it )
                    .arg( type );

    if( !df->openForWrite( p, name ) ) {
        Error()
            << "Error opening file: ["
            << name
            << "].";
        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* Trigger -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Trigger::Trigger(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
{
    thread  = new QThread;

    if( p.mode.mTrig == DAQ::eTrigImmed )
        worker = new TrigImmed( p, gw, imQ, niQ );
    else if( p.mode.mTrig == DAQ::eTrigTimed )
        worker = new TrigTimed( p, gw, imQ, niQ );
    else if( p.mode.mTrig == DAQ::eTrigTTL )
        worker = new TrigTTL( p, gw, imQ, niQ );
    else if( p.mode.mTrig == DAQ::eTrigSpike )
        worker = new TrigSpike( p, gw, imQ, niQ );
    else
        worker = new TrigTCP( p, gw, imQ, niQ );

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


