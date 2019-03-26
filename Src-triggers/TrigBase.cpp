
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
    const QVector<AIQ*> &imQ,
    const AIQ           *niQ )
    :   QObject(0), dfNi(0),
        ovr(p), startT(-1), gateHiT(-1), gateLoT(-1), trigHiT(-1),
        firstCtNi(0), iGate(-1), iTrig(-1), gateHi(false),
        pleaseStop(false), p(p), gw(gw), imQ(imQ), niQ(niQ), statusT(-1),
        nImQ(imQ.size())
{
    if( nImQ ) {
        vS.resize( nImQ );
        for( int ip = 0; ip < nImQ; ++ip )
            vS[ip].init( imQ[ip], ip, p );
    }

    if( niQ ) {
        vS.insert( vS.begin(), SyncStream() );
        vS[0].init( niQ, -1, p );
    }

    tLastReport = getTime();
}


bool TrigBase::allFilesClosed() const
{
    QMutexLocker    ml( &dfMtx );

    return !dfNi && !firstCtIm.size();
}


bool TrigBase::isInUse( const QFileInfo &fi ) const
{
    QMutexLocker    ml( &dfMtx );

    for( int ip = 0, np = firstCtIm.size(); ip < np; ++ip ) {

        if( dfImAp[ip] && fi == QFileInfo( dfImAp[ip]->binFileName() ) )
            return true;

        if( dfImLf[ip] && fi == QFileInfo( dfImLf[ip]->binFileName() ) )
            return true;
    }

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


quint64 TrigBase::curImFileStart( uint ip ) const
{
    QMutexLocker    ml( &dfMtx );

    if( ip < (uint)firstCtIm.size() )
        return firstCtIm[ip];

    return 0;
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


void TrigBase::setGate( bool hi )
{
    QMutexLocker    ml( &runMtx );

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

        if( ovr.forceGT ) {

            ovr.get( iGate, iTrig );

            if( iGate < 0 ) {
                iGate = 0;
                iTrig = -1;
            }
        }
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
    const SyncStream    &S = vS[0];

    return S.Ct2TAbs( S.Q->endCount() );
}


void TrigBase::endTrig()
{
    dfMtx.lock();
        for( int ip = 0, np = firstCtIm.size(); ip < np; ++ip ) {

            if( dfImAp[ip] )
                dfImAp[ip]->closeAsync( kvmRmt );

            if( dfImLf[ip] )
                dfImLf[ip]->closeAsync( kvmRmt );
        }
        dfImAp.clear();
        dfImLf.clear();
        firstCtIm.clear();

        if( dfNi )
            dfNi = (DataFileNI*)dfNi->closeAsync( kvmRmt );
        firstCtNi = 0;
    dfMtx.unlock();

    if( trigHiT >= 0 ) {

        trigHiT = -1;

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

// Create folder

    if( forceName.isEmpty() ) {

        QString runDir = QString("%1/%2_g%3")
                            .arg( mainApp()->dataDir() )
                            .arg( p.sns.runName )
                            .arg( ig );

        if( runDir != lastRunDir ) {

            QDir().mkdir( runDir );
            lastRunDir = runDir;
        }
    }

// Create files

    dfMtx.lock();
        if( nImQ ) {
            for( int ip = 0; ip < nImQ; ++ip ) {

                firstCtIm.push_back( 0 );

                dfImAp.push_back(
                    p.im.each[ip].apSaveChanCount() ?
                    new DataFileIMAP( ip ) : 0 );

                dfImLf.push_back(
                    p.im.each[ip].lfIsSaving() ?
                    new DataFileIMLF( ip ) : 0 );
            }
        }
        if( niQ ) {
            firstCtNi   = 0;
            dfNi        = new DataFileNI;
        }
    dfMtx.unlock();

// Open files

    bool    ok = true;

    for( int ip = 0; ip < nImQ; ++ip ) {

        if( dfImAp[ip] && !openFile( dfImAp[ip], ig, it ) ) {
            ok = false;
            break;
        }

        if( dfImLf[ip] && !openFile( dfImLf[ip], ig, it ) ) {
            ok = false;
            break;
        }
    }

    if( ok )
        ok = openFile( dfNi, ig, it );

    forceName.clear();

    if( !ok )
        return false;

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
    for( int ip = 0, np = firstCtIm.size(); ip < np; ++ip ) {

        if( dfImAp[ip] )
            dfImAp[ip]->setAsyncWriting( false );

        if( dfImLf[ip] )
            dfImLf[ip]->setAsyncWriting( false );
    }

    if( dfNi )
        dfNi->setAsyncWriting( false );
}


// A positive nMax value is the number of samples to retrieve.
// A negative nMax is negative of LOOP_MS.
//
// Return ok.
//
bool TrigBase::nScansFromCt(
    vec_i16     &data,
    quint64     fromCt,
    int         nMax,
    int         ip )
{
    const AIQ   *Q = (ip >= 0 ? imQ[ip] : niQ);
    int         ret;

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

        double  lag = double(Q->qHeadCt() - fromCt) / Q->sRate();
        QString who = (ip >= 0 ? QString("IM%1").arg( ip ) : "NI");

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
    uint        ip,
    vec_i16     &data,
    quint64     headCt )
{
    if( dst == DstImec )
        return writeDataIM( data, headCt, ip );
    else
        return writeDataNI( data, headCt );
}


quint64 TrigBase::scanCount( DstStream dst )
{
    QMutexLocker    ml( &dfMtx );
    DataFile        *df = 0;

    if( dst == DstImec ) {

        for( int ip = 0, np = firstCtIm.size(); ip < np; ++ip ) {

            if( (df = dfImAp[ip]) )
                goto count;

            if( (df = dfImLf[ip]) )
                goto count;
        }
    }
    else
        df = dfNi;

count:
    return (df ? df->scanCount() : 0);
}


void TrigBase::endRun( const QString &err )
{
    QMetaObject::invokeMethod(
        gw, "setTriggerLED",
        Qt::QueuedConnection,
        Q_ARG(bool, false) );

    dfMtx.lock();
        for( int ip = 0, np = firstCtIm.size(); ip < np; ++ip ) {

            if( dfImAp[ip] ) {
                dfImAp[ip]->setRemoteParams( kvmRmt );
                dfImAp[ip]->closeAndFinalize();
                delete dfImAp[ip];
            }

            if( dfImLf[ip] ) {
                dfImLf[ip]->setRemoteParams( kvmRmt );
                dfImLf[ip]->closeAndFinalize();
                delete dfImLf[ip];
            }
        }
        dfImAp.clear();
        dfImLf.clear();
        firstCtIm.clear();

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
        QString s =
        QString("Trigger (writing) error [%1]; stopping run.").arg( err );
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

        int allChans = 0;

        for( int ip = 0; ip < nImQ; ++ip )
            allChans += p.im.each[ip].imCumTypCnt[CimCfg::imSumAll];

        // any representative rate OK

        chim = QString("%1CH@%2kHz")
                .arg( allChans )
                .arg( p.im.each[0].srate / 1e3, 0, 'f', 3 );
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
            for( int ip = 0; ip < nImQ; ++ip ) {
                if( imQ[ip]->endCount() ) {
                    baseSet = true;
                    break;
                }
            }
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
            for( int ip = 0; ip < nImQ; ++ip ) {
                if( imQ[ip]->endCount() ) {
                    baseSet = true;
                    break;
                }
            }
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
    double  tReport,
            imFull  = 0.0,
            niFull  = 0.0,
            wbps    = 0.0,
            rbps    = 0.0;
    int     np      = firstCtIm.size();

    if( dfNi || np ) {

        tReport = getTime();

        // report worst case values

        for( int ip = 0; ip < np; ++ip ) {

            if( dfImAp[ip] ) {
                imFull   = qMax( imFull, dfImAp[ip]->percentFull() );
                wbps    += dfImAp[ip]->writtenBytes();
                rbps    += dfImAp[ip]->requiredBps();
            }

            if( dfImLf[ip] ) {
                imFull  = qMax( imFull, dfImLf[ip]->percentFull() );
                wbps   += dfImLf[ip]->writtenBytes();
                rbps   += dfImLf[ip]->requiredBps();
            }
        }

        if( dfNi ) {
            niFull  = dfNi->percentFull();
            wbps   += dfNi->writtenBytes();
            rbps   += dfNi->requiredBps();
        }

        wbps /= (tReport - tLastReport);
        wbps /= 1024*1024;
        rbps /= 1024*1024;
        tLastReport = tReport;

        s = QString(" FileQFill%=(%1,%2) MB/s=%3 (%4 req)")
            .arg( imFull, 0, 'f', 1 )
            .arg( niFull, 0, 'f', 1 )
            .arg( wbps, 0, 'f', 1 )
            .arg( rbps, 0, 'f', 1 );
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

    if( !df->openForWrite( p, ig, it, forceName ) ) {

        if( forceName.isEmpty() ) {
            Error()
                << QString("Error opening file: [%1_g%2_t%3.%4.bin].")
                    .arg( p.sns.runName )
                    .arg( ig )
                    .arg( it )
                    .arg( df->fileLblFromObj() );
        }
        else {
            Error()
                << QString("Error opening file: [%1.%2.bin].")
                    .arg( forceName )
                    .arg( df->fileLblFromObj() );
        }

        return false;
    }

    return true;
}


// Write LF samples on X12 boundaries (sample%12==0).
//
// - inplace true means data param will not be used
// for AP, so we can write the X12 samples into it.
// Otherwise we allocate an alternate dst.
//
// - xtra true means that the first sample in the file
// is not an X12, so we will need to construct the prior
// X12 LF data by extrapolating from the nearest forward
// X12 and the timepoint preceding it. The constructed
// sync data are a copy of the first timepoint values.
//
bool TrigBase::writeDataLF(
    vec_i16     &data,
    quint64     headCt,
    uint        ip,
    bool        inplace,
    bool        xtra )
{
    vec_i16 dstAlt;
    vec_i16 *dst;
    qint16  *D, *S;
    int     R       = headCt % 12,
            nCh     = p.im.each[ip].imCumTypCnt[CimCfg::imSumAll],
            nAP     = p.im.each[ip].imCumTypCnt[CimCfg::imSumAP],
            nLF     = p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural] - nAP,
            size    = (int)data.size(),
            nTp     = size / nCh;

// Set up dst = destination workspace
// D points to first destination for X12 copies

    if( inplace )
        dst = &data;
    else {
        dstAlt.resize( ((xtra ? 2 : 1) + nTp) * nCh );
        dst = &dstAlt;
    }

    D = &dst->front();

// S points to first source timepoint

    if( R )
        R = 12 - R;

    S = &data[R*nCh];

// Extrapolate extra first timepoint if needed

    if( xtra ) {

        // Point p2 to the LF data for the first X12 timepoint.
        // Point p1 to the LF data for the previous timepoint.

        qint16  *p2 = S  + nAP,
                *p1 = p2 - nCh;

        D += nAP;   // D offset temporarily to LF channels

        for( int lf = 0; lf < nLF; ++lf )
            D[lf] = p2[lf] - (p2[lf] - p1[lf]) * 12;

        D -= nAP;   // D normalized

        // sync channels

        for( int is = nAP + nLF; is < nCh; ++is )
            D[is] = data[is];

        D += nCh;
    }

// S to D X12 copies

    for( int it = R; it < nTp; it += 12, D += nCh, S += 12*nCh )
        memcpy( D + nAP, S + nAP, (nCh - nAP) * sizeof(qint16) );

    dst->resize( D - &dst->front() );

    if( dst->size() && !dfImLf[ip]->writeAndInvalSubset( p, *dst ) )
        return false;

    return true;
}


// Split the data into (AP+SY) and (LF+SY) components,
// directing each to the appropriate data file.
//
// Here, all AP data are written, but only LF samples
// on X12-boundary (sample%12==0) are written.
//
bool TrigBase::writeDataIM( vec_i16 &data, quint64 headCt, uint ip )
{
    uint    np      = firstCtIm.size();
    bool    isAP    = (ip < np && dfImAp[ip]),
            isLF    = (ip < np && dfImLf[ip]),
            xtra    = false;

    if( !(isAP || isLF) )
        return true;

    int size = (int)data.size();

    if( size && !firstCtIm[ip] ) {

        firstCtIm[ip] = headCt;

        if( isAP )
            dfImAp[ip]->setFirstSample( headCt );

        if( isLF ) {

            if( headCt % 12 ) {

                // need enough data to extrapolate

                int nCh = p.im.each[ip].imCumTypCnt[CimCfg::imSumAll];

                if( data.size() / nCh > 12 - (headCt % 12) )
                    xtra = true;
            }

            dfImLf[ip]->setFirstSample( headCt / 12 );
        }
    }

    if( isLF && !writeDataLF( data, headCt, ip, !isAP, xtra ) )
        return false;

    if( isAP && !dfImAp[ip]->writeAndInvalSubset( p, data ) )
        return false;

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
    const QVector<AIQ*> &imQ,
    const AIQ           *niQ )
{
    thread = new QThread;

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


