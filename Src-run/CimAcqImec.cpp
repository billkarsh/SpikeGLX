
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "MetricsWindow.h"

#include <QDir>
#include <QThread>


// T0FUDGE used to sync IM and NI stream tZero values.
// TPNTPERFETCH reflects the AP/LF sample rate ratio.
#define T0FUDGE         0.0
#define TPNTPERFETCH    12
#define AVEE            5
#define MAXE            24
//#define PROFILE
//#define TUNE            0


/* ---------------------------------------------------------------- */
/* ImAcqShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqShared::ImAcqShared()
    :   awake(0), asleep(0), stop(false)
{
// Experiment to histogram successive timestamp differences.
    tStampBins.assign( 34, 0 );
    tStampEvtByPrb.assign( 32, 0 );
}


// Experiment to histogram successive timestamp differences.
//
ImAcqShared::~ImAcqShared()
{
#if 0
    Log()<<"------ Intrafetch timestamp diffs ------";
    for( int i = 0; i < 34; ++i )
        Log()<<QString("bin %1  N %2").arg( i ).arg( tStampBins[i] );
    Log()<<"---- Timestamp incidents each probe ----";
    for( int i = 0; i < 32; ++i )
        Log()<<QString("probe %1  N %2").arg( i ).arg( tStampEvtByPrb[i] );
    Log()<<"----------------------------------------";
#endif
}


// Experiment to histogram successive timestamp differences.
// One can collect just difs within packets, or just between
// packets, or both.
//
void ImAcqShared::tStampHist(
    const electrodePacket*  E,
    int                     ip,
    int                     ie,
    int                     it )
{
#if 0
    qint64  dif = -999999;
    if( it > 0 )        // intra-packet
        dif = E[ie].timestamp[it] - E[ie].timestamp[it-1];
    else if( ie > 0 )   // inter-packet
        dif = E[ie].timestamp[0] - E[ie-1].timestamp[11];

// @@@ Fix Experiment to report the zero delta value.
#if 0
    if( dif == 0 ) {
        if( !tStampEvtByPrb[ip] ) {
            Warning()<<
            QString("ZERO TS probe %1, value %2")
            .arg( ip ).arg( E[0].timestamp[0] );
        }
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 0
    if( dif == 0 ) {
        Log()<<QString("ZERO TSTAMP DIF: stamp %1")
        .arg( E[ie].timestamp[it] );
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 0
    if( dif > 31 ) {
        Log()<<QString("BIGDIF: ip %1 dif %2 stamp %3")
        .arg( ip ).arg( dif ).arg( E[ie].timestamp[it] );
    }
#endif

    if( dif != -999999 ) {

        if( dif < 0 )
            ++tStampBins[32];
        else if( dif > 31 )
            ++tStampBins[33];
        else
            ++tStampBins[dif];

        if( dif < 3 || dif > 4 )
            ++tStampEvtByPrb[ip];
    }
#else
    Q_UNUSED( E )
    Q_UNUSED( ip )
    Q_UNUSED( ie )
    Q_UNUSED( it )
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqProbe ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqProbe::ImAcqProbe(
    const CimCfg::ImProbeTable  &T,
    const DAQ::Params           &p,
    int                         ip )
    :   tLastErrReport(0), tLastFifoReport(0),
        peakDT(0), sumTot(0), totPts(0ULL),
        errCOUNT(0), errSERDES(0), errLOCK(0), errPOP(0), errSYNC(0),
        fifoAve(0), fifoN(0), sumN(0), ip(ip),
        fetchType(0), zeroFill(false)
{
// @@@ FIX Experiment to report large fetch cycle times.
    tLastFetch      = 0;

// Experiment to detect gaps in timestamps across fetches.
    tStampLastFetch = 0;

#ifdef PROFILE
    sumLag  = 0;
    sumGet  = 0;
    sumScl  = 0;
    sumEnq  = 0;
#endif

    const int   *cum = p.im.each[ip].imCumTypCnt;
    nAP = cum[CimCfg::imTypeAP];
    nLF = cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP];
    nSY = cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF];
    nCH = nAP + nLF + nSY;

    const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
    slot = P.slot;
    port = P.port;
}


ImAcqProbe::~ImAcqProbe()
{
    sendErrMetrics();
}


// Policy: Send only if nonzero.
//
void ImAcqProbe::sendErrMetrics() const
{
    if( errCOUNT + errSERDES + errLOCK + errPOP + errSYNC ) {

        QMetaObject::invokeMethod(
            mainApp()->metrics(),
            "errUpdateFlags",
            Qt::QueuedConnection,
            Q_ARG(int, ip),
            Q_ARG(quint32, errCOUNT),
            Q_ARG(quint32, errSERDES),
            Q_ARG(quint32, errLOCK),
            Q_ARG(quint32, errPOP),
            Q_ARG(quint32, errSYNC) );
    }
}


void ImAcqProbe::checkErrFlags( qint8 *E, int nE ) const
{
    for( int ie = 0; ie < nE; ++ie ) {

        quint16 *errs = ((electrodePacket*)E)[ie].Status;

        for( int i = 0; i < TPNTPERFETCH; ++i ) {

            int err = errs[i];

            if( err & ELECTRODEPACKET_STATUS_ERR_COUNT )    ++errCOUNT;
            if( err & ELECTRODEPACKET_STATUS_ERR_SERDES )   ++errSERDES;
            if( err & ELECTRODEPACKET_STATUS_ERR_LOCK )     ++errLOCK;
            if( err & ELECTRODEPACKET_STATUS_ERR_POP )      ++errPOP;
            if( err & ELECTRODEPACKET_STATUS_ERR_SYNC )     ++errSYNC;
        }
    }

    double  tProf = getTime();

    if( tProf - tLastErrReport >= 2.0 ) {

        sendErrMetrics();
        tLastErrReport = tProf;
    }
}


bool ImAcqProbe::checkFifo( size_t *packets, CimAcqImec *acq ) const
{
    double  tFifo = getTime();

    fifoAve += acq->fifoPct( packets, *this );
    ++fifoN;

    if( tFifo - tLastFifoReport >= 5.0 ) {

        if( fifoN > 0 )
            fifoAve /= fifoN;

        QMetaObject::invokeMethod(
            mainApp()->metrics(),
            "prfUpdateFifo",
            Qt::QueuedConnection,
            Q_ARG(int, ip),
            Q_ARG(int, fifoAve) );

        if( fifoAve >= 5 ) {    // 5% standard

            Warning() <<
                QString("IMEC FIFO queue %1 fill% %2")
                .arg( ip )
                .arg( fifoAve, 2, 10, QChar('0') );

            if( fifoAve >= 95 ) {
                acq->runError(
                    QString("IMEC FIFO queue %1 overflow; stopping run.")
                    .arg( ip ) );
                return false;
            }
        }

        fifoAve         = 0;
        fifoN           = 0;
        tLastFifoReport = tFifo;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* ImAcqWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqWorker::ImAcqWorker(
    CimAcqImec              *acq,
    QVector<AIQ*>           &imQ,
    ImAcqShared             &shr,
    std::vector<ImAcqProbe> &probes )
    :   acq(acq), imQ(imQ), shr(shr), probes(probes),
        tLastYieldReport(getTime()), yieldSum(0)
{
}


void ImAcqWorker::run()
{
// Size buffers
// ------------
// - lfLast[][]: each probe must retain the prev LF for all channels.
// - i16Buf[][]: sized for each probe.
// - E[]: max sized over {fetchType, MAXE}; reused each iID.
//

    std::vector<std::vector<float> >    lfLast;
    std::vector<std::vector<qint16> >   i16Buf;

    const int   nID     = probes.size();
    uint        EbytMax = 0;

    lfLast.resize( nID );
    i16Buf.resize( nID );

    for( int iID = 0; iID < nID; ++iID ) {

        const ImAcqProbe    &P = probes[iID];

        lfLast[iID].assign( P.nLF, 0.0F );
        i16Buf[iID].resize( MAXE * TPNTPERFETCH * P.nCH );

        if( P.fetchType == 0 ) {
            if( sizeof(electrodePacket) > EbytMax )
                EbytMax = sizeof(electrodePacket);
        }
    }

    E.resize( MAXE * EbytMax );

// -------------
// @@@ FIX Mod for no packets
//_rawAP.resize( MAXE * TPNTPERFETCH * 384 );
//_rawLF.resize( MAXE * 384 );
// -------------

    if( !shr.wait() )
        goto exit;

// -----------------------
// Fetch : Scale : Enqueue
// -----------------------

    lastCheckT = shr.startT;

    while( !acq->isStopped() && !shr.stopping() ) {

        loopT = getTime();

        // ------------
        // Do my probes
        // ------------

        for( int iID = 0; iID < nID; ++iID ) {

            const ImAcqProbe    &P = probes[iID];

            if( !P.totPts )
                imQ[P.ip]->setTZero( loopT + T0FUDGE );

            double  dtTot = getTime();

            if( !doProbe( &lfLast[iID][0], i16Buf[iID], P ) )
                goto exit;

            dtTot = getTime() - dtTot;

            if( dtTot > P.peakDT )
                P.peakDT = dtTot;

            P.sumTot += dtTot;
            ++P.sumN;
        }

        // -----
        // Yield
        // -----

        if( !workerYield() )
            goto exit;

        // ---------------
        // Rate statistics
        // ---------------

        if( loopT - lastCheckT >= 5.0 ) {

            for( int iID = 0; iID < nID; ++iID ) {

                const ImAcqProbe    &P = probes[iID];

#ifdef PROFILE
                profile( P );
#endif
                P.peakDT    = 0;
                P.sumTot    = 0;
                P.sumN      = 0;
            }

            lastCheckT  = getTime();
        }
    }

exit:
    emit finished();
}


bool ImAcqWorker::doProbe(
    float               *lfLast,
    vec_i16             &dst1D,
    const ImAcqProbe    &P )
{
#ifdef PROFILE
    double  prbT0 = getTime();
#endif

    qint16* dst = &dst1D[0];
    int     nE;

// -----
// Fetch
// -----

// @@@ FIX Mod for no packets
//    if( !acq->fetchE( nE, &E[0], P, &_rawAP[0], &_rawLF[0] ) )
    if( !acq->fetchE( nE, &E[0], P ) )
        return false;

    if( !nE ) {

// @@@ FIX Adjust sample waiting for trigger type

        // BK: Allow up to 5 seconds for (external) trigger.
        // BK: Tune with experience.

        if( !P.totPts && loopT - shr.startT >= 5.0 ) {
            acq->runError(
                QString("IMEC probe %1 getting no samples.").arg( P.ip ) );
            return false;
        }

        return true;
    }

#ifdef PROFILE
    P.sumGet += getTime() - prbT0;
#endif

// -----
// Scale
// -----

//------------------------------------------------------------------
// Experiment to detect gaps in timestamps across fetches.
#if 0
{
quint32 firstVal = ((electrodePacket*)&E[0])[0].timestamp[0];
if( P.tStampLastFetch
    && (firstVal < P.tStampLastFetch
    ||  firstVal > P.tStampLastFetch + 4) ) {

    Log() <<
        QString("~~ TSTAMP GAP IM %1  val %2")
        .arg( P.ip )
        .arg( qint32(firstVal - P.tStampLastFetch) );
}
P.tStampLastFetch = ((electrodePacket*)&E[0])[nE-1].timestamp[11];
}
#endif
//------------------------------------------------------------------

#ifdef PROFILE
    double  dtScl = getTime();
#endif

    for( int ie = 0; ie < nE; ++ie ) {

        const qint16    *srcLF = 0;
        const quint16   *srcSY = 0;

        if( P.fetchType == 0 ) {

            electrodePacket *pE = &((electrodePacket*)&E[0])[ie];

            srcLF = pE->lfpData;
            srcSY = pE->Status;
        }

        for( int it = 0; it < TPNTPERFETCH; ++it ) {

            // ----------
            // ap - as is
            // ----------

            if( P.fetchType == 0 ) {
                memcpy( dst,
                    ((electrodePacket*)&E[0])[ie].apData[it],
                    P.nAP * sizeof(qint16) );
            }

            shr.tStampHist( (electrodePacket*)&E[0], P.ip, ie, it );

//------------------------------------------------------------------
// Experiment to visualize timestamps as sawtooth in channel 16.
#if 0
dst[16] = ((electrodePacket*)&E[0])[ie].timestamp[it] % 8000 - 4000;
#endif
//------------------------------------------------------------------

//------------------------------------------------------------------
// Experiment to visualize counter as sawtooth in channel 16.
#if 0
static uint count[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
count[P.ip] += 3;
dst[16] = count[P.ip] % 8000 - 4000;
#endif
//------------------------------------------------------------------

            dst += P.nAP;

            // -----------------
            // lf - interpolated
            // -----------------

#if 1
// Standard linear interpolation
            float slope = float(it)/TPNTPERFETCH;

            for( int lf = 0, nlf = P.nLF; lf < nlf; ++lf )
                *dst++ = lfLast[lf] + slope*(srcLF[lf]-lfLast[lf]);
#else
// Raw data for diagnostics
            for( int lf = 0, nlf = P.nLF; lf < nlf; ++lf )
                *dst++ = srcLF[lf];
#endif

            // ----
            // sync
            // ----

            *dst++ = srcSY[it];

        }   // it

        // ---------------
        // update saved lf
        // ---------------

        for( int lf = 0, nlf = P.nLF; lf < nlf; ++lf )
            lfLast[lf] = srcLF[lf];
    }   // ie

#ifdef PROFILE
    P.sumScl += getTime() - dtScl;
#endif

// -------
// Enqueue
// -------

    P.tPreEnq = getTime();

    if( P.zeroFill ) {
        imQ[P.ip]->enqueueZero( P.tPostEnq, P.tPreEnq );
        P.zeroFill = false;
    }

    imQ[P.ip]->enqueue( &dst1D[0], TPNTPERFETCH * nE );
    P.tPostEnq = getTime();
    P.totPts  += TPNTPERFETCH * nE;

#ifdef PROFILE
    P.sumLag += mainApp()->getRun()->getStreamTime() -
                (imQ[P.ip]->tZero() + P.totPts / imQ[P.ip]->sRate());
    P.sumEnq += P.tPostEnq - P.tPreEnq;
#endif

    return true;
}


bool ImAcqWorker::workerYield()
{
// Get maximum outstanding packets for this worker thread

    size_t  maxQPkts    = 0;
    int     nID         = probes.size();

    for( int iID = 0; iID < nID; ++iID ) {

        const ImAcqProbe    &P = probes[iID];
        size_t              packets;

        if( !P.checkFifo( &packets, acq ) )
            return false;

        if( packets > maxQPkts )
            maxQPkts = packets;
    }

// Yield time if fewer than the average fetched packet count.

    double  t = getTime();

    if( maxQPkts < AVEE ) {
        QThread::usleep( 250 );
        yieldSum += getTime() - t;
    }

    if( t - tLastYieldReport >= 5.0 ) {

        QMetaObject::invokeMethod(
            mainApp()->metrics(),
            "prfUpdateAwake",
            Qt::QueuedConnection,
            Q_ARG(int, probes[0].ip),
            Q_ARG(int, probes[nID-1].ip),
            Q_ARG(int, qMax( 0, int(100.0*(1.0 - yieldSum/5.0))) ) );

        yieldSum            = 0;
        tLastYieldReport    = t;
    }

    return true;
}


// sumN is the number of loop executions in the 5 sec check
// interval. The minimum value is 5*srate/(MAXE*TPNTPERFETCH).
//
// sumTot/sumN is the average loop time to process the samples.
// The maximum value is MAXE*TPNTPERFETCH/srate.
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
//
// Required values header is written at run start.
//
void ImAcqWorker::profile( const ImAcqProbe &P )
{
#ifndef PROFILE
    Q_UNUSED( P )
#else
    Log() <<
        QString(
        "imec %1 loop ms <%2> lag<%3> get<%4> scl<%5> enq<%6> n(%7) %(%8)")
        .arg( P.ip, 2, 10, QChar('0') )
        .arg( 1000*P.sumTot/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumLag/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumGet/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumScl/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumEnq/P.sumN, 0, 'f', 3 )
        .arg( P.sumN )
        .arg( acq->fifoPct( 0, P ), 2, 10, QChar('0') );

    P.sumLag    = 0;
    P.sumGet    = 0;
    P.sumScl    = 0;
    P.sumEnq    = 0;
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqThread::ImAcqThread(
    CimAcqImec              *acq,
    QVector<AIQ*>           &imQ,
    ImAcqShared             &shr,
    std::vector<ImAcqProbe> &probes )
{
    thread  = new QThread;
    worker  = new ImAcqWorker( acq, imQ, shr, probes );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImAcqThread::~ImAcqThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* CimAcqImec ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ),
        T(mainApp()->cfgCtl()->prbTab),
        pausPortsRequired(0), pausSlot(-1), nThd(0)
{
}

/* ---------------------------------------------------------------- */
/* ~CimAcqImec ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::~CimAcqImec()
{
    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd ) {
        imT[iThd]->thread->wait( 10000/nThd );
        delete imT[iThd];
    }

    QThread::msleep( 2000 );

// Close hardware

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is )
        closeBS( T.getEnumSlot( is ) );
}

/* ---------------------------------------------------------------- */
/* CimAcqImec::run ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimAcqImec::run()
{
// ---------
// Configure
// ---------

// Hardware

    if( !configure() )
        return;

// Create worker threads

// @@@ FIX Tune probes per thread here and in triggers
    const int   nPrbPerThd = 3;

    for( int ip0 = 0, np = p.im.get_nProbes(); ip0 < np; ip0 += nPrbPerThd ) {

        std::vector<ImAcqProbe> probes;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < np )
                probes.push_back( ImAcqProbe( T, p, ip0 + id ) );
            else
                break;
        }

        imT.push_back( new ImAcqThread( this, owner->imQ, shr, probes ) );
        ++nThd;
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                QThread::usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( isStopped() || !startAcq() )
        return;

// ---
// Run
// ---

#ifdef PROFILE
    // Table header, see profile discussion
    Log() <<
        QString("Require loop ms < [[ %1 ]] n > [[ %2 ]] MAXE %3")
        .arg( 1000*MAXE*TPNTPERFETCH/p.im.each[0].srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.each[0].srate/(MAXE*TPNTPERFETCH) ) )
        .arg( MAXE );
#endif

    shr.startT = getTime();

// Wake all workers

    shr.condWake.wakeAll();

// Sleep main thread until external stop command

    atomicSleepWhenReady();

// --------
// Clean up
// --------
}

/* ---------------------------------------------------------------- */
/* update --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Updating settings affects all ports on that slot.
//
void CimAcqImec::update( int ip )
{
    const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
    NP_ErrorCode                err;

    pauseSlot( P.slot );

    while( !pauseAllAck() )
        QThread::usleep( 100 );

// ----------------------
// Stop streams this slot
// ----------------------

    err = arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return;
    }

// --------------------------
// Update settings this probe
// --------------------------

    if( !_selectElectrodes( P ) )
        return;

    if( !_setReferences( P ) )
        return;

    if( !_setGains( P ) )
        return;

    if( !_setHighPassFilter( P ) )
        return;

    if( !_setStandby( P ) )
        return;

    if( !_writeProbe( P ) )
        return;

// -------------------------------
// Set slot to software triggering
// -------------------------------

    err = setTriggerInput( P.slot, TRIGIN_SW );

    if( err != SUCCESS  ) {
        runError(
            QString("IMEC setTriggerInput(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return;
    }

// ------------
// Arm the slot
// ------------

    err = arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return;
    }

// ----------------
// Restart the slot
// ----------------

    err = setSWTrigger( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setSWTrigger(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return;
    }

// -----------------
// Reset pause flags
// -----------------

    pauseSlot( -1 );
}

/* ---------------------------------------------------------------- */
/* Pause controls ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::pauseSlot( int slot )
{
    QMutexLocker    ml( &runMtx );

    pausSlot            = slot;
    pausPortsRequired   = (slot >= 0 ? T.nQualPortsThisSlot( slot ) : 0);
    pausPortsReported.clear();
}


bool CimAcqImec::pauseAck( int port )
{
    QMutexLocker    ml( &runMtx );
    bool            wasAck = pausPortsReported.contains( port );

    pausPortsReported.insert( port );
    return wasAck;
}


bool CimAcqImec::pauseAllAck() const
{
    QMutexLocker    ml( &runMtx );

    return pausPortsReported.count() >= pausPortsRequired;
}

/* ---------------------------------------------------------------- */
/* fetchE --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#if 0   // without packets

bool CimAcqImec::fetchE(
    int                 &nE,
    qint8               *E,
    const ImAcqProbe    &P,
    qint16* rawAP, qint16* rawLF )  // @@@ FIX Mod for no packets
{
    nE = 0;

    int out = readAPFifo( P.slot, P.port, 0, rawAP, MAXE*TPNTPERFETCH );

    if( out < TPNTPERFETCH ) {
        Log() << "tiny partial fetch " << out;
        return true;
    }

    nE = out / TPNTPERFETCH;

    if( nE*TPNTPERFETCH < out )
        Log() << "big fetch with excess " << out - nE*TPNTPERFETCH;

    int nLF = readLFPFifo( P.slot, P.port, 0, rawLF, nE );

    if( nLF != nE )
        Log()<< "MISMATCH nE nLF " << nE << " " << nLF;

    for( int ie = 0; ie < nE; ++ie ) {

        electrodePacket *K = &((electrodePacket*)E)[ie];

        memcpy( K->apData, rawAP, TPNTPERFETCH*384*sizeof(qint16) );
        rawAP += TPNTPERFETCH*384;

        memcpy( K->lfpData, rawLF, 384*sizeof(qint16) );
        rawLF += 384;

        memset( K->SYNC, 0, TPNTPERFETCH*sizeof(qint16) );
    }

#ifdef TUNE
    // Tune AVEE and MAXE on designated probe
    if( TUNE == P.ip ) {
        static std::vector<uint> pkthist( 1 + MAXE, 0 ); // 0 + [1..MAXE]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- ave %1  max %2")
                .arg( AVEE ).arg( MAXE );
            for( int i = 0; i <= MAXE; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.assign( 1 + MAXE, 0 );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[out];
    }
#endif

    return true;
}

#endif


#if 1   // The real thing

bool CimAcqImec::fetchE( int &nE, qint8 *E, const ImAcqProbe &P )
{
    nE = 0;

// --------------------------------
// Hardware pause acknowledged here
// --------------------------------

    if( pausedSlot() == P.slot ) {

ackPause:
        if( !pauseAck( P.port ) )
            P.zeroFill = true;

        return true;
    }

// --------------------
// Else fetch real data
// --------------------

// @@@ FIX Experiment to report large fetch cycle times.
#if 0
    double tFetch = getTime();
    if( P.tLastFetch ) {
        if( tFetch - P.tLastFetch > 0.010 ) {
            Log() <<
                QString("       IM %1  dt %2  Q% %3")
                .arg( P.ip ).arg( int(1000*(tFetch - P.tLastFetch)) )
                .arg( fifoPct( P ) );
        }
    }
    P.tLastFetch = tFetch;
#endif

    size_t          out;
    NP_ErrorCode    err = SUCCESS;

    if( P.fetchType == 0 ) {
        err = readElectrodeData(
                P.slot, P.port,
                (electrodePacket*)E,
                &out, MAXE );
    }

// @@@ FIX Experiment to report fetched packet count vs time.
#if 0
if( P.ip == 0 ) {
    static double q0 = getTime();
    static QFile f;
    static QTextStream ts( &f );
    double qq = getTime() - q0;
    if( qq >= 5.0 && qq < 6.0 ) {
        if( !f.isOpen() ) {
            f.setFileName( "pace.txt" );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
        }
        ts<<QString("%1\t%2\n").arg( qq ).arg( out );
        if( qq >= 6.0 )
            f.close();
    }
}
#endif

    if( err != SUCCESS ) {

        if( pausedSlot() == P.slot )
            goto ackPause;

        runError(
            QString(
            "IMEC readElectrodeData(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    nE = out;

#ifdef TUNE
    // Tune AVEE and MAXE on designated probe
    if( TUNE == P.ip ) {
        static std::vector<uint> pkthist( 1 + MAXE, 0 ); // 0 + [1..MAXE]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- ave %1  max %2")
                .arg( AVEE ).arg( MAXE );
            for( int i = 0; i <= MAXE; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.assign( 1 + MAXE, 0 );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[out];
    }
#endif

// -----------------
// Check error flags
// -----------------

    P.checkErrFlags( E, nE );

    return true;
}

#endif  // The real thing

/* ---------------------------------------------------------------- */
/* fifoPct -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int CimAcqImec::fifoPct( size_t *packets, const ImAcqProbe &P ) const
{
    quint8  pct = 0;

    if( pausedSlot() != P.slot ) {

        size_t          nused, nempty;
        NP_ErrorCode    err;

        if( !packets )
            packets = &nused;

        err =
        getElectrodeDataFifoState( P.slot, P.port, packets, &nempty );

        if( err == SUCCESS )
            pct = quint8((100 * *packets) / (*packets + nempty));
        else {
            Warning() <<
                QString("IMEC getElectrodeDataFifoState(slot %1, port %2)"
                " error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) );
        }
    }

    return pct;
}

/* ---------------------------------------------------------------- */
/* configure ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define STOPCHECK   if( isStopped() ) return false;


void CimAcqImec::SETLBL( const QString &s, bool zero )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetLabel",
        Qt::QueuedConnection,
        Q_ARG(QString, s),
        Q_ARG(bool, zero) );
}


void CimAcqImec::SETVAL( int val )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetValue",
        Qt::QueuedConnection,
        Q_ARG(int, val) );
}


void CimAcqImec::SETVALBLOCKING( int val )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetValue",
        Qt::BlockingQueuedConnection,
        Q_ARG(int, val) );
}


// @@@ FIX Leave buffers at defaults until understand better.
// @@@ FIX Need to scale buf size with probe count.
// NP_PARAM_BUFFERSIZE:     default 128K
// NP_PARAM_BUFFERCOUNT:    default 64
//
bool CimAcqImec::_allProbesSizeStreamBufs()
{
#if 0
    NP_ErrorCode    err;

    err = setParameter( NP_PARAM_BUFFERSIZE, 128*1024 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( BUFSIZE ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }
#endif

#if 0
    err = setParameter( NP_PARAM_BUFFERCOUNT, 64 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( BUFCOUNT ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }
#endif

    return true;
}


bool CimAcqImec::_open( const CimCfg::ImProbeTable &T )
{
    SETLBL( "open session", true );

    bool    ok = false;

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int             slot    = T.getEnumSlot( is );
        NP_ErrorCode    err     = openBS( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC openBS( %1 ) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            goto exit;
        }
    }

    ok = true;

exit:
    SETVAL( 25 );
    return ok;
}


// User designated slot set as master.
// Imec source selected and programmed.
// Master SMA configured for output.
// Non-masters automatically get shared signal.
//
bool CimAcqImec::_setSyncAsOutput( int slot )
{
    NP_ErrorCode    err;

    err = setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCMASTER ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = setParameter( NP_PARAM_SYNCSOURCE, TRIGIN_SYNCCLOCK );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCSOURCE ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = setParameter( NP_PARAM_SYNCPERIOD_MS, 1000 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCPERIOD ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = setTriggerOutput( slot, TRIGOUT_SMA, TRIGIN_SHAREDSYNC );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setTriggerOutput(slot %1, SYNC) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    return true;
}


// User designated slot set as master.
// External source selected.
// Master SMA configured for input.
// Non-masters automatically get shared signal.
//
bool CimAcqImec::_setSyncAsInput( int slot )
{
    NP_ErrorCode    err;

    err = setParameter( NP_PARAM_SYNCMASTER, slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCMASTER ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = setParameter( NP_PARAM_SYNCSOURCE, TRIGIN_SMA );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setParameter( SYNCSOURCE ) error %1 '%2'.")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_setSync( const CimCfg::ImProbeTable &T )
{
    SETLBL( "program sync" );

    bool    ok = true;

    if( p.sync.sourceIdx == DAQ::eSyncSourceNone ) {
        Log() << "IMEC syncing set to: DISABLED";
        goto exit;
    }

    if( p.sync.sourceIdx >= DAQ::eSyncSourceIM ) {

        ok = _setSyncAsOutput(
                T.getEnumSlot( p.sync.sourceIdx - DAQ::eSyncSourceIM ) );
        Log() << "IMEC syncing set to: ENABLED/OUTPUT";
    }
    else {
        ok = _setSyncAsInput( p.sync.imInputSlot );
        Log() << "IMEC syncing set to: ENABLED/INPUT";
    }

exit:
    SETVAL( 100 );
    return ok;
}


bool CimAcqImec::_openProbe( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("open probe %1").arg( P.ip ), true );

    NP_ErrorCode    err = openProbe( P.slot, P.port );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC openProbe(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = init( P.slot, P.port );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC init(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    SETVAL( 50 );
    return true;
}


bool CimAcqImec::_calibrateADC( const CimCfg::ImProbeDat &P )
{
    if( p.im.all.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 ADC calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( p.im.all.calPolicy == 1 )
            goto warn;
        else {
            runError(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.sn ).arg( P.ip ) );
            return false;
        }
    }

    SETLBL( QString("calibrate probe %1 ADC").arg( P.ip )  );

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_ADCCalibration.csv").arg( path ).arg( P.sn ) ;

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = setADCCalibration( P.slot, P.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC setADCCalibration(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    SETVAL( 53 );
    Log() << QString("IMEC probe %1 ADC calibrated").arg( P.ip );
    return true;
}


bool CimAcqImec::_calibrateGain( const CimCfg::ImProbeDat &P )
{
    if( p.im.all.calPolicy == 2 ) {

warn:
        Warning() <<
            QString("IMEC Skipping probe %1 gain calibration").arg( P.ip );
        return true;
    }

    if( P.cal < 1 ) {

        if( p.im.all.calPolicy == 1 )
            goto warn;
        else {
            runError(
                QString("Can't find calibration folder '%1' for probe %2.")
                .arg( P.sn ).arg( P.ip ) );
            return false;
        }
    }

    SETLBL( QString("calibrate probe %1 gains").arg( P.ip ) );

    QString path = calibPath();

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_gainCalValues.csv").arg( path ).arg( P.sn ) ;

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file '%1'.").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    NP_ErrorCode    err;

    err = setGainCalibration( P.slot, P.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC setGainCalibration(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    SETVAL( 57 );
    Log() << QString("IMEC probe %1 gains calibrated").arg( P.ip );
    return true;
}


#if 0   // selectDataSource now private in NeuropixAPI.h
// Synthetic data generation for testing:
// 0 = normal
// 1 = each chan is ADC id num
// 2 = each chan is chan id num
// 3 = each chan linear ramping
//
bool CimAcqImec::_dataGenerator( const CimCfg::ImProbeDat &P )
{
    NP_ErrorCode    err = IM.selectDataSource( P.slot, P.port, 3 );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC selectDataSource(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    Log() << QString("IMEC probe %1 generating synthetic data").arg( P.ip );
    return true;
}
#endif


bool CimAcqImec::_setLEDs( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 LED").arg( P.ip ) );

    NP_ErrorCode    err;

    err = setHSLed( P.slot, P.port, p.im.each[P.ip].LEDEnable );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setHSLed(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    SETVAL( 58 );
    Log() << QString("IMEC probe %1 LED set").arg( P.ip );
    return true;
}


bool CimAcqImec::_selectElectrodes( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("select probe %1 electrodes").arg( P.ip ) );

    const IMROTbl   &T = p.im.each[P.ip].roTbl;
    int             nC = T.nChan();
    NP_ErrorCode    err;

// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        if( T.chIsRef( ic ) )
            continue;

        err = selectElectrode( P.slot, P.port, ic, T.e[ic].bank );

        if( err != SUCCESS ) {
            runError(
                QString(
                "IMEC selectElectrode(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 59 );
    Log() << QString("IMEC probe %1 electrodes selected").arg( P.ip );
    return true;
}


bool CimAcqImec::_setReferences( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 references").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan();
    NP_ErrorCode    err;

// ------------------------------------
// Connect all according to table refid
// ------------------------------------

// refid    (ref,bnk)   who
// -----    ---------   ---
//   0        (0,0)     ext
//   1        (1,0)     tip
//   2        (2,0)     192
//   3        (2,1)     576
//   4        (2,2)     960
//
    for( int ic = 0; ic < nC; ++ic ) {

        int rid = R.e[ic].refid,
            ref = (rid < 2 ? rid : 2),
            bnk = (rid > 1 ? rid - 2 : 0);

        err = setReference( P.slot, P.port, ic,
                channelreference_t(ref), bnk );

        if( err != SUCCESS ) {
            runError(
                QString(
                "IMEC setReference(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 60 );
    Log() << QString("IMEC probe %1 references set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setGains( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 gains").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan();
    NP_ErrorCode    err;

// --------------------------------
// Set all according to table gains
// --------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        const IMRODesc  &E = R.e[ic];

        err = setGain( P.slot, P.port, ic,
                    IMROTbl::gainToIdx( E.apgn ),
                    IMROTbl::gainToIdx( E.lfgn ) );

//---------------------------------------------------------
// Experiment to visualize LF scambling on shankviewer by
// setting every nth gain high and others low.
#if 0
        int apidx, lfidx;

        if( !(ic % 10) ) {
            apidx = IMROTbl::gainToIdx( 3000 );
            lfidx = IMROTbl::gainToIdx( 3000 );
        }
        else {
            apidx = IMROTbl::gainToIdx( 50 );
            lfidx = IMROTbl::gainToIdx( 50 );
        }

        err = setGain( P.slot, P.port, ic,
                    apidx,
                    lfidx );
#endif
//---------------------------------------------------------

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setGain(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 61 );
    Log() << QString("IMEC probe %1 gains set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setHighPassFilter( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 filters").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = setAPCornerFrequency( P.slot, P.port, ic, !R.e[ic].apflt );

        if( err != SUCCESS ) {
            runError(
                QString(
                "IMEC setAPCornerFrequency(slot %1, port %2)"
                " error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 62 );
    Log() << QString("IMEC probe %1 filters set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setStandby( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 standby").arg( P.ip ) );

// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = p.im.each[P.ip].roTbl.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = setStdb( P.slot, P.port, ic,
                p.im.each[P.ip].stdbyBits.testBit( ic ) );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setStandby(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 63 );
    Log() << QString("IMEC probe %1 standby chans set").arg( P.ip );
    return true;
}


bool CimAcqImec::_writeProbe( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("writing probe %1...").arg( P.ip ) );

    NP_ErrorCode    err;

    err = writeProbeConfiguration( P.slot, P.port, true );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC writeProbeConfig(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    SETVAL( 100 );
    return true;
}


// Set lowest slot for software trigger.
// Pass to others on PXI_TRIG<0> with rising edge sensitivity.
//
bool CimAcqImec::_setTrigger()
{
    SETLBL( "set triggering", true );

    int             ns = T.nLogSlots(),
                    slot;
    NP_ErrorCode    err;

// Lowest slot gets software input

    slot    = T.getEnumSlot( 0 );
    err     = setTriggerInput( slot, TRIGIN_SW );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setTriggerInput(slot %1) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

// Done if no others

    if( ns <= 1 )
        goto exit;

// Set slot zero output to PXI_TRIG<0>

    err = setTriggerOutput( slot, TRIGOUT_PXI1, TRIGIN_SW );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setTriggerOutput(slot %1) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = setTriggerEdge( slot, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setTriggerEdge(slot %1) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

// Set other inputs to PXI_TRIG<0>

    for( int is = 1; is < ns; ++is ) {

        SETVAL( is*66/ns );

        slot    = T.getEnumSlot( is );
        err     = setTriggerInput( slot, TRIGIN_PXI1 );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerInput(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }

        err = setTriggerEdge( slot, true );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

exit:
    SETVAL( 66 );
    Log()
        << "IMEC Trigger source: "
        << (p.im.all.trgSource ? "hardware" : "software");
    return true;
}


bool CimAcqImec::_setArm()
{
    SETLBL( "arm system" );

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int             slot    = T.getEnumSlot( is );
        NP_ErrorCode    err     = arm( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC arm(slot %1) error %2 '%3'.")
                .arg( slot )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    SETVAL( 100 );
    Log() << "IMEC Armed";
    return true;
}


bool CimAcqImec::_softStart()
{
    int             slot    = T.getEnumSlot( 0 );
    NP_ErrorCode    err     = setSWTrigger( slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setSWTrigger(slot %1) error %2 '%3'.")
            .arg( slot )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    return true;
}


bool CimAcqImec::configure()
{
    STOPCHECK;

    if( !_allProbesSizeStreamBufs() )
        return false;

    STOPCHECK;

    if( !_open( T ) )
        return false;

    STOPCHECK;

    if( !_setSync( T ) )
        return false;

    STOPCHECK;

    for( int ip = 0, np = p.im.get_nProbes(); ip < np; ++ip ) {

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );

        if( !_openProbe( P ) )
            return false;

        STOPCHECK;

        if( !_calibrateADC( P ) )
            return false;

        STOPCHECK;

        if( !_calibrateGain( P ) )
            return false;

//        STOPCHECK;

//        if( !_dataGenerator( P ) )
//            return false;

        STOPCHECK;

        if( !_setLEDs( P ) )
            return false;

        STOPCHECK;

        if( !_selectElectrodes( P ) )
            return false;

        STOPCHECK;

        if( !_setReferences( P ) )
            return false;

        STOPCHECK;

        if( !_setGains( P ) )
            return false;

        STOPCHECK;

        if( !_setHighPassFilter( P ) )
            return false;

        STOPCHECK;

        if( !_setStandby( P ) )
            return false;

        STOPCHECK;

        if( !_writeProbe( P ) )
            return false;

        STOPCHECK;
    }

    if( !_setTrigger() )
        return false;

    if( !_setArm() )
        return false;

// Flush all progress messages
    SETVALBLOCKING( 100 );

    return true;
}

/* ---------------------------------------------------------------- */
/* startAcq ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::startAcq()
{
    STOPCHECK;

    if( !p.im.all.trgSource ) {

        if( !_softStart() )
            return false;

        Log() << "IMEC Acquisition started";
    }
    else
        Log() << "IMEC Waiting for external trigger";

    return true;
}

/* ---------------------------------------------------------------- */
/* runError ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::runError( QString err )
{
    Error() << err;
    emit owner->daqError( err );
}

#endif  // HAVE_IMEC


