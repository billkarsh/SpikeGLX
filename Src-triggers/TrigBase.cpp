
#include "TrigImmed.h"
#include "TrigSpike.h"
#include "TrigTimed.h"
#include "TrigTCP.h"
#include "TrigTTL.h"
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"

#include <QDir>
#include <QFileInfo>
#include <QThread>

//#define PERFMON
#ifdef PERFMON
#include <windows.h>
#include <psapi.h>
#endif


/* ---------------------------------------------------------------- */
/* TrigBase ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TrigBase::TrigBase(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
    :   QObject(0), dfImAp(0), dfImLf(0), dfNi(0),
        ovr(p), startT(-1), gateHiT(-1), gateLoT(-1), trigHiT(-1),
        firstCtIm(0), firstCtNi(0), iGate(-1), iTrig(-1), gateHi(false),
        pleaseStop(false), p(p), gw(gw), imQ(imQ), niQ(niQ), statusT(-1)
{
    imS.init( imQ,  0, p );
    niS.init( niQ, -1, p );
}


bool TrigBase::allFilesClosed() const
{
    QMutexLocker    ml( &dfMtx );

    return !dfNi && !dfImAp && !dfImLf;
}


bool TrigBase::isInUse( const QFileInfo &fi ) const
{
    QMutexLocker    ml( &dfMtx );

    if( dfImAp && fi == QFileInfo( dfImAp->binFileName() ) )
        return true;

    if( dfImLf && fi == QFileInfo( dfImLf->binFileName() ) )
        return true;

    if( dfNi && fi == QFileInfo( dfNi->binFileName() ) )
        return true;

    return false;
}


// BK: This should probably be deprecated
//
QString TrigBase::curNiFilename() const
{
    QMutexLocker    ml( &dfMtx );

    return (dfNi ? dfNi->binFileName() : QString::null);
}


quint64 TrigBase::curImFileStart() const
{
    QMutexLocker    ml( &dfMtx );

    return firstCtIm;
}


quint64 TrigBase::curNiFileStart() const
{
    QMutexLocker    ml( &dfMtx );

    return firstCtNi;
}


void TrigBase::setStartT()
{
    startTMtx.lock();
    startT = nowCalibrated();
    startTMtx.unlock();
}


void TrigBase::setGateEnabled( bool enabled )
{
    runMtx.lock();
    gateHiT         = nowCalibrated();
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


// Best estimator of time during run.
//
double TrigBase::nowCalibrated() const
{
    const SyncStream    &S = (niQ ? niS : imS);

    return S.Ct2TAbs( S.Q->endCount() );
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

        gateHiT = nowCalibrated();

        if( ovr.forceGT )
            ovr.get( iGate, iTrig );
        else {
            ++iGate;
            iTrig = -1;
        }
    }
    else
        gateLoT = nowCalibrated();

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
        if( dfImAp )
            dfImAp = (DataFileIMAP*)dfImAp->closeAsync( kvmRmt );
        if( dfImLf )
            dfImLf = (DataFileIMLF*)dfImLf->closeAsync( kvmRmt );
        firstCtIm = 0;
        if( dfNi )
            dfNi = (DataFileNI*)dfNi->closeAsync( kvmRmt );
        firstCtNi = 0;
    dfMtx.unlock();

    trigHiT = -1;

    QMetaObject::invokeMethod(
        gw, "setTriggerLED",
        Qt::QueuedConnection,
        Q_ARG(bool, false) );
}


bool TrigBase::newTrig( int &ig, int &it, bool trigLED )
{
    endTrig();

    it = incTrig( ig );

// Create folder

    QString runDir = QString("%1/%2_g%3")
                        .arg( mainApp()->dataDir() )
                        .arg( p.sns.runName )
                        .arg( ig );

    if( runDir != lastRunDir ) {

        QDir().mkdir( runDir );
        lastRunDir = runDir;
    }

// Create files

    dfMtx.lock();
        if( imQ ) {
            firstCtIm = 0;
            if( p.apSaveChanCount() )
                dfImAp = new DataFileIMAP;
            if( p.lfSaveChanCount() )
                dfImLf = new DataFileIMLF;
        }
        if( niQ ) {
            firstCtNi   = 0;
            dfNi        = new DataFileNI;
        }
    dfMtx.unlock();

// Open files

    if( !openFile( dfImAp, ig, it )
        || !openFile( dfImLf, ig, it )
        || !openFile( dfNi, ig, it ) ) {

        return false;
    }

// Reset state tracking

    trigHiT = nowCalibrated();

    if( trigLED ) {
        QMetaObject::invokeMethod(
            gw, "setTriggerLED",
            Qt::QueuedConnection,
            Q_ARG(bool, true) );
    }

    QString sGT = QString("<G%1 T%2>").arg( ig ).arg( it );

    QMetaObject::invokeMethod(
        gw, "updateGT",
        Qt::QueuedConnection,
        Q_ARG(QString, sGT) );

    return true;
}


void TrigBase::setSyncWriteMode()
{
    if( dfImAp )
        dfImAp->setAsyncWriting( false );

    if( dfImLf )
        dfImLf->setAsyncWriting( false );

    if( dfNi )
        dfNi->setAsyncWriting( false );
}


// Usage:
// Once, after new file(s) are opened...
// Use trigger logic and mapping to determine starting
// counts for both streams. Then call this to bump the
// counts to the nearest X12 imec boundary.
//
void TrigBase::alignX12( quint64 &imCt, quint64 &niCt, bool testFile )
{
    if( testFile && !dfImLf )
        return;

    int del = imCt % 12;

    if( !del )
        return;

    if( del <= 6 )
        del = -del;
    else
        del = 12 - del;

    imCt += del;

    if( niQ )
        niCt += qRound( del * p.ni.srate / p.im.srate );
}


// Usage:
// Similar adjustment to above, but files not yet open.
//
void TrigBase::alignX12( const AIQ *qA, quint64 &cA, quint64 &cB )
{
// Nothing to do if no LFP recording

    if( !imQ || !p.lfSaveChanCount() )
        return;

    if( qA == niQ )
        alignX12( cB, cA, false );
    else
        alignX12( cA, cB, false );
}


// A positive nMax value is the number of samples to retrieve.
// A negative nMax is negative of LOOP_MS.
//
// Return ok.
//
bool TrigBase::nScansFromCt(
    const AIQ   *Q,
    vec_i16     &data,
    quint64     fromCt,
    int         nMax )
{
    int ret;

    if( nMax < 0 ) {
        // 4X-overfetch * loop_sec * rate
        nMax = 4.0 * 0.001 * -nMax * Q->sRate();
    }

    try {
        data.reserve( nMax * Q->nChans() );
    }
    catch( const std::exception& ) {
        Error() << "Trigger low mem";
        return false;
    }

    ret = Q->getNScansFromCt( data, fromCt, nMax );

    if( ret > 0 )
        return true;

    if( ret < 0 ) {

        QString who = (Q == imQ ? "IM" : "NI");
        double  lag = double(Q->qHeadCt() - fromCt) / Q->sRate();

        Error() <<
            QString("%1 recording lagging %2 seconds.")
            .arg( who )
            .arg( lag, 0, 'f', 3 );
    }

    return false;
}


// This function dispatches ALL stream writing to the
// proper DataFile(s).
//
bool TrigBase::writeAndInvalData(
    DstStream   dst,
    vec_i16     &data,
    quint64     headCt )
{
    if( dst == DstImec )
        return writeDataIM( data, headCt );
    else
        return writeDataNI( data, headCt );
}


quint64 TrigBase::scanCount( DstStream dst )
{
    QMutexLocker    ml( &dfMtx );
    DataFile        *df = 0;

    if( dst == DstImec ) {

        if( dfImAp )
            df = dfImAp;
        else
            df = dfImLf;
    }
    else
        df = dfNi;

    return (df ? df->scanCount() : 0);
}


void TrigBase::endRun( const QString &err )
{
    QMetaObject::invokeMethod(
        gw, "setTriggerLED",
        Qt::QueuedConnection,
        Q_ARG(bool, false) );

    dfMtx.lock();
        if( dfImAp ) {
            dfImAp->setRemoteParams( kvmRmt );
            dfImAp->closeAndFinalize();
            delete dfImAp;
            dfImAp = 0;
        }

        if( dfImLf ) {
            dfImLf->setRemoteParams( kvmRmt );
            dfImLf->closeAndFinalize();
            delete dfImLf;
            dfImLf = 0;
        }
        firstCtIm = 0;

        if( dfNi ) {
            dfNi->setRemoteParams( kvmRmt );
            dfNi->closeAndFinalize();
            delete dfNi;
            dfNi        = 0;
            firstCtNi   = 0;
        }
    dfMtx.unlock();

    Debug() << "Trigger thread stopped.";

    if( !err.isEmpty() ) {
        QString s = "Trigger thread error; stopping run.";
        Error() << s;
        emit daqError( s );
    }

    emit finished();
}


void TrigBase::statusOnSince( QString &s )
{
    double  t, nowT = nowCalibrated();
    int     h, m;

    startTMtx.lock();
    t = (startT >= 0 ? nowT - startT : 0);
    startTMtx.unlock();

    h = int(t / 3600);
    t = t - h * 3600;
    m = t / 60;
    t = t - m * 60;

// Statusbar

    QString ch, chim, chni;

    if( p.im.enabled ) {
        chim = QString("%1CH@%2kHz")
                .arg( p.im.imCumTypCnt[CimCfg::imSumAll] )
                .arg( p.im.srate / 1e3, 0, 'f', 3 );
    }

    if( p.ni.enabled ) {
        chni = QString("%1CH@%2kHz")
                .arg( p.ni.niCumTypCnt[CniCfg::niSumAll] )
                .arg( p.ni.srate / 1e3, 0, 'f', 3 );
    }

    if( p.im.enabled && p.ni.enabled )
        ch = QString("{%1, %2}").arg( chim ).arg( chni );
    else if( p.im.enabled )
        ch = chim;
    else
        ch = chni;

#ifdef PERFMON
#if 1
//---------------------------------------------------------------
// Monitor memory usage; fields:
// just before queue filling : current usage : peak usage.
//
// Add to that a file tracking (seconds, current mem) every 5 secs.
//
{
    static double   lastMonT = 0, lastFileT = 0, base = 0;
    static bool     baseSet = false;
    if( nowT - lastMonT > 0.1 ) {
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof(info) );
        lastMonT = nowT;
        if( !baseSet ) {
            base = double(info.WorkingSetSize)/(1024.0*1024*1024);
            if( niQ && niQ->endCount() )
                baseSet = true;
            if( imQ && imQ->endCount() )
                baseSet = true;
        }

        s = QString("ON %1h%2m%3s   %4   %5   %6  ")
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( t, 0, 'f', 1 )
            .arg( base, 0, 'f', 4 )
            .arg( double(info.WorkingSetSize)/(1024.0*1024*1024), 0, 'f', 4 )
            .arg( double(info.PeakWorkingSetSize)/(1024.0*1024*1024), 0, 'f', 4 );

        if( nowT - lastFileT > 5 ) {
            lastFileT = nowT;
            QFile f( QString("%1/mem.txt")
                    .arg( mainApp()->dataDir() ) );
            f.open( QIODevice::Append | QIODevice::Text );
            QTextStream ts( &f );
            ts
            <<(startT >= 0 ? nowT - startT : 0)<<" "
            <<QString("%1")
            .arg( double(info.WorkingSetSize)/(1024.0*1024*1024), 0, 'f', 6 )
            <<"\n";
        }
    }
}
//---------------------------------------------------------------
#else
//---------------------------------------------------------------
// Monitor memory usage; fields:
// just before queue filling : current usage : peak usage.
//
{
    static double   lastMonT = 0, base = 0;
    static bool     baseSet = false;
    if( nowT - lastMonT > 0.1 ) {
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof(info) );
        lastMonT = nowT;
        if( !baseSet ) {
            base = double(info.WorkingSetSize)/(1024.0*1024*1024);
            if( niQ && niQ->endCount() )
                baseSet = true;
            if( imQ && imQ->endCount() )
                baseSet = true;
        }

    s = QString("ON %1h%2m%3s   %4   %5   %6  ")
        .arg( h, 2, 10, QChar('0') )
        .arg( m, 2, 10, QChar('0') )
        .arg( t, 0, 'f', 1 )
        .arg( base, 0, 'f', 4 )
        .arg( double(info.WorkingSetSize)/(1024.0*1024*1024), 0, 'f', 4 )
        .arg( double(info.PeakWorkingSetSize)/(1024.0*1024*1024), 0, 'f', 4 );
    }
}
//---------------------------------------------------------------
#endif
#else
    int ig, it;

    getGT( ig, it );

    s = QString("ON %1h%2m%3s %4 <G%5 T%6>")
        .arg( h, 2, 10, QChar('0') )
        .arg( m, 2, 10, QChar('0') )
        .arg( t, 0, 'f', 1 )
        .arg( ch )
        .arg( ig )
        .arg( it );
#endif  // PERFMON

// RunToolbar::On-time

    QString sGW;

    if( t ) {
        sGW = QString("%1:%2:%3")
                .arg( h, 2, 10, QChar('0') )
                .arg( m, 2, 10, QChar('0') )
                .arg( (int)t, 2, 10, QChar('0') );
    }
    else
        sGW = "--:--:--";

    QMetaObject::invokeMethod(
        gw, "updateOnTime",
        Qt::QueuedConnection,
        Q_ARG(QString, sGW) );

// RunToolbar::Rec-time

    if( trigHiT >= 0 ) {

        t = nowT - trigHiT;
        h = int(t / 3600);
        t = t - h * 3600;
        m = t / 60;
        t = t - m * 60;

        sGW = QString("%1:%2:%3")
                .arg( h, 2, 10, QChar('0') )
                .arg( m, 2, 10, QChar('0') )
                .arg( (int)t, 2, 10, QChar('0') );
    }
    else
        sGW = "--:--:--";

    QMetaObject::invokeMethod(
        gw, "updateRecTime",
        Qt::QueuedConnection,
        Q_ARG(QString, sGW) );
}


void TrigBase::statusWrPerf( QString &s )
{
    if( dfNi || dfImAp || dfImLf ) {

        // report worst case values

        static double   tLastReport = 0;

        double  tReport = getTime(),
                imFull  = 0.0,
                niFull  = 0.0,
                wbps    = 0.0,
                rbps    = 0.0;

        if( dfImAp ) {
            imFull  = dfImAp->percentFull();
            wbps   += dfImAp->writtenBytes();
            rbps   += dfImAp->requiredBps();
        }

        if( dfImLf ) {
            imFull  = qMax( imFull, dfImLf->percentFull() );
            wbps   += dfImLf->writtenBytes();
            rbps   += dfImLf->requiredBps();
        }

        if( dfNi ) {
            niFull  = dfNi->percentFull();
            wbps   += dfNi->writtenBytes();
            rbps   += dfNi->requiredBps();
        }

        wbps /= (tReport - tLastReport);
        tLastReport = tReport;

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
// Loop no more often than every loopPeriod_us

    loopT = 1e6 * (getTime() - loopT);  // microsec

    if( loopT < loopPeriod_us )
        QThread::usleep( loopPeriod_us - loopT );
    else
        QThread::usleep( 1000 * 10 );
}


bool TrigBase::openFile( DataFile *df, int ig, int it )
{
    if( !df )
        return true;

    QString name = QString("%1_g%2_t%3.%4.bin")
                    .arg( p.sns.runName )
                    .arg( ig )
                    .arg( it )
                    .arg( df->fileLblFromObj() );

    if( !df->openForWrite( p, name ) ) {
        Error()
            << "Error opening file: ["
            << name
            << "].";
        return false;
    }

    return true;
}


// Split the data into (AP+SY) and (LF+SY) components,
// directing each to the appropriate data file.
//
// Triggers (callers) are responsible for aligning the
// file data to a X12 imec boundary.
//
// Here, all AP data are written, but only LF samples
// on X12-boundary (sample%12==0) are written.
//
bool TrigBase::writeDataIM( vec_i16 &data, quint64 headCt )
{
    if( !dfImAp && !dfImLf )
        return true;

    int size = (int)data.size();

    if( size && !firstCtIm ) {

        firstCtIm = headCt;

        if( dfImAp )
            dfImAp->setFirstSample( headCt );

        if( dfImLf )
            dfImLf->setFirstSample( headCt / 12 );
    }

    if( !dfImLf ) {

        // Just save (AP+SY)

        if( !dfImAp->writeAndInvalSubset( p, data ) )
            return false;
    }
    else if( !dfImAp ) {

        // Just save (LF+SY)
        // Downsample X12 in place

writeLF:
        qint16  *D, *S;
        int     R       = headCt % 12,
                nCh     = p.im.imCumTypCnt[CimCfg::imSumAll],
                nAP     = p.im.imCumTypCnt[CimCfg::imSumAP],
                nTp     = size / nCh;

        if( R ) {
            // first Tp needs copy to data[0]
            R   = 12 - R;
            D   = &data[0];
        }
        else {
            // first Tp already in place
            R   = 12;
            D   = &data[nCh];
        }

        S = &data[R*nCh];

        for( int it = R; it < nTp; it += 12, D += nCh, S += 12*nCh )
            memcpy( D + nAP, S + nAP, (nCh - nAP) * sizeof(qint16) );

        data.resize( D - &data[0] );

        if( data.size() && !dfImLf->writeAndInvalSubset( p, data ) )
            return false;
    }
    else {

        // Save both
        // Make a copy for AP
        // Use the LF code above

        vec_i16 cpy = data;

        if( !dfImAp->writeAndInvalSubset( p, cpy ) )
            return false;

        goto writeLF;
    }

    return true;
}


bool TrigBase::writeDataNI( vec_i16 &data, quint64 headCt )
{
    if( !dfNi )
        return true;

    if( !firstCtNi && data.size() ) {
        firstCtNi = headCt;
        dfNi->setFirstSample( headCt );
    }

    return dfNi->writeAndInvalSubset( p, data );
}

/* ---------------------------------------------------------------- */
/* Trigger -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Trigger::Trigger(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
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


