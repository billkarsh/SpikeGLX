
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
    :   QObject(0), dfImAp(0), dfImLf(0), dfNi(0),
        ovr(p), startT(-1), gateHiT(-1), gateLoT(-1), trigHiT(-1),
        iGate(-1), iTrig(-1), gateHi(false), pleaseStop(false),
        p(p), gw(gw), imQ(imQ), niQ(niQ), statusT(-1)
{
}


bool TrigBase::allFilesClosed() const
{
    QMutexLocker    ml( &dfMtx );

    return !dfImAp && !dfImLf && !dfNi;
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
        if( dfImAp )
            dfImAp = (DataFileIMAP*)dfImAp->closeAsync( kvmRmt );
        if( dfImLf )
            dfImLf = (DataFileIMLF*)dfImLf->closeAsync( kvmRmt );
        if( dfNi )
            dfNi = (DataFileNI*)dfNi->closeAsync( kvmRmt );
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

    dfMtx.lock();
        if( imQ ) {
            if( p.apSaveChanCount() )
                dfImAp = new DataFileIMAP();
            if( p.lfSaveChanCount() )
                dfImLf = new DataFileIMLF();
        }
        if( niQ )
            dfNi = new DataFileNI();
    dfMtx.unlock();

    if( !openFile( dfImAp, ig, it )
        || !openFile( dfImLf, ig, it )
        || !openFile( dfNi, ig, it ) ) {

        return false;
    }

    trigHiT = getTime();

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
    if( dfImAp )
        dfImAp->setAsyncWriting( false );

    if( dfImLf )
        dfImLf->setAsyncWriting( false );

    if( dfNi )
        dfNi->setAsyncWriting( false );
}


// This function dispatches ALL stream writing to the
// proper DataFile(s).
//
bool TrigBase::writeAndInvalVB(
    DstStream                   dst,
    std::vector<AIQ::AIQBlock>  &vB )
{
    if( dst == DstImec )
        return writeVBIM( vB );
    else
        return writeVBNI( vB );
}


quint64 TrigBase::scanCount( DstStream dst )
{
    QMutexLocker    ml( &dfMtx );
    DataFile        *df;

    if( dst == DstImec ) {

        if( dfImAp )
            df = dfImAp;
        else
            df = dfImLf;
    }
    else
        df = dfNi;

    if( !df )
        return 0;

    return df->scanCount();
}


void TrigBase::endRun()
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

        if( dfNi ) {
            dfNi->setRemoteParams( kvmRmt );
            dfNi->closeAndFinalize();
            delete dfNi;
            dfNi = 0;
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

// Statusbar

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
    if( dfImAp || dfImLf || dfNi ) {

        // report worst case values

        double  imFull  = 0.0,
                niFull  = 0.0,
                wbps    = 0.0,
                rbps    = 0.0;

        if( dfImAp ) {
            imFull  = dfImAp->percentFull();
            wbps    = dfImAp->writeSpeedBps();
            rbps    = dfImAp->requiredBps();
        }

        if( dfImLf ) {
            imFull  = qMax( imFull, dfImLf->percentFull() );
            wbps   += dfImLf->writeSpeedBps();
            rbps   += dfImLf->requiredBps();
        }

        if( dfNi ) {
            niFull  = dfNi->percentFull();
            wbps   += dfNi->writeSpeedBps();
            rbps   += dfNi->requiredBps();
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


bool TrigBase::openFile( DataFile *df, int ig, int it )
{
    if( !df )
        return true;

    QString name = QString("%1/%2_g%3_t%4.%5.bin")
                    .arg( mainApp()->runDir() )
                    .arg( p.sns.runName )
                    .arg( ig )
                    .arg( it )
                    .arg( df->subtypeFromObj() );

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
bool TrigBase::writeVBIM( std::vector<AIQ::AIQBlock> &vB )
{
    if( !dfImAp && !dfImLf )
        return true;

    int nb = (int)vB.size();

    for( int i = 0; i < nb; ++i ) {

        if( !dfImLf ) {

            // Just save (AP+SY)

            if( !dfImAp->writeAndInvalSubset( p, vB[i].data ) )
                return false;
        }
        else if( !dfImAp ) {

            // Just save (LF+SY)
            // Downsample X12 in place

writeLF:
            vec_i16 &data   = vB[i].data;
            int     R       = vB[i].headCt % 12,
                    nCh     = p.im.imCumTypCnt[CimCfg::imSumAll],
                    nTp     = (int)data.size() / nCh;
            qint16  *D, *S;

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
                memcpy( D, S, nCh * sizeof(qint16) );

            data.resize( D - &data[0] );

            if( data.size() && !dfImLf->writeAndInvalSubset( p, data ) )
                return false;
        }
        else {

            // Save both
            // Make a copy for AP
            // Use the LF code above

            vec_i16 cpy = vB[i].data;

            if( !dfImAp->writeAndInvalSubset( p, cpy ) )
                return false;

            goto writeLF;
        }
    }

    return true;
}


bool TrigBase::writeVBNI( std::vector<AIQ::AIQBlock> &vB )
{
    if( !dfNi )
        return true;

    int nb = (int)vB.size();

    for( int i = 0; i < nb; ++i ) {

        if( !dfNi->writeAndInvalSubset( p, vB[i].data ) )
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


