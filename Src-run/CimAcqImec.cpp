
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDir>
#include <QThread>


// T0FUDGE used to sync IM and NI stream tZero values.
// TPNTPERFETCH reflects the AP/LF sample rate ratio.
// OVERFETCH enables fetching more than loopSecs generates.
#define T0FUDGE         0.0
#define TPNTPERFETCH    12
// @@@ FIX Tuning required {LOOPSECS, OVERFETCH}
#define LOOPSECS        0.01    // 0.004
#define OVERFETCH       20.0    // 2.0, [10.0]
#define PROFILE
//#define TUNE            0

static int error15 = 0;

//------------------------------------------------------------------
// Experiment to histogram successive timestamp differences.
#if 1
static QVector<quint64> bins( 34, 0 );
#endif
//------------------------------------------------------------------

/* ---------------------------------------------------------------- */
/* ImAcqShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqShared::ImAcqShared( double tPntPerLoop )
    :   maxE(ceil(OVERFETCH*qMax(tPntPerLoop/TPNTPERFETCH,1.0))),
        awake(0), asleep(0), stop(false)
{
}

/* ---------------------------------------------------------------- */
/* ImAcqProbe ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqProbe::ImAcqProbe(
    const CimCfg::ImProbeTable  &T,
    const DAQ::Params           &p,
    int                         ip )
    :   peakDT(0), sumTot(0),
        totPts(0ULL), ip(ip),
        fetchType(0), sumN(0)
{
#ifdef PROFILE
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

/* ---------------------------------------------------------------- */
/* ImAcqWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ImAcqWorker::run()
{
// Size buffers
// ------------
// - lfLast[][]: each probe must retain the prev LF for all channels.
// - i16Buf[][]: sized for each probe.
// - E[]: max sized over {fetchType, maxE}; reused each iID.
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
        i16Buf[iID].resize( shr.maxE * TPNTPERFETCH * P.nCH );

        if( P.fetchType == 0 ) {
            if( sizeof(electrodePacket) > EbytMax )
                EbytMax = sizeof(electrodePacket);
        }
    }

    E.resize( shr.maxE * EbytMax );

// -------------
// @@@ FIX Mod for no packets
_rawAP.resize( shr.maxE * TPNTPERFETCH * 384 );
_rawLF.resize( shr.maxE * 384 );
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

            ImAcqProbe  &P = probes[iID];

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

        // On some machines we can successfully yield back some
        // measured 'balance of expected time' T > 0 via usleep( T )
        // and still keep pace with the data rate. However, on other
        // machines the sleeps prove to be much longer than T and
        // we rapidly overflow the FIFO. The only universally safe
        // practice is therefore to never yield from this loop.

// @@@ FIX Experiment to yield more from imAcq loop
// @@@ FIX Note using 0.5 on this yield compared to sim.

        double  dt = getTime() - loopT;

        if( dt < LOOPSECS )
            QThread::usleep( 1e6 * 0.5*(LOOPSECS - dt) );

        // ---------------
        // Rate statistics
        // ---------------

        if( loopT - lastCheckT >= 5.0 ) {

            for( int iID = 0; iID < nID; ++iID ) {

                ImAcqProbe  &P = probes[iID];

                if( !keepingUp( P ) )
                    goto exit;

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


bool ImAcqWorker::doProbe( float *lfLast, vec_i16 &dst1D, ImAcqProbe &P )
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
//    if( !acq->fetchE( nE, &E[0], P, loopT, &_rawAP[0], &_rawLF[0] ) )
    if( !acq->fetchE( nE, &E[0], P, loopT ) )
        return false;

    if( !nE ) {

// @@@ FIX Adjust sample waiting for trigger type

        // BK: Allow up to 5 seconds for (external) trigger.
        // BK: Tune with experience.

        if( !P.totPts && loopT - shr.startT >= 5.0 ) {
            acq->runError(
                QString("Imec probe %1 getting no samples.").arg( P.ip ),
                false );
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
#if 1
{
static quint32  lastVal[8] = {0,0,0,0,0,0,0,0};

quint32 firstVal = ((electrodePacket*)&E[0])[0].timestamp[0];

if( firstVal < lastVal[P.ip] || firstVal > lastVal[P.ip] + 4 ) {
    Log() << QString("~~~~~~~~ TSTAMP GAP IM %1  val %2")
                .arg( P.ip )
                .arg( qint32(firstVal - lastVal[P.ip]) );
}

lastVal[P.ip] = ((electrodePacket*)&E[0])[nE-1].timestamp[11];
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

            srcLF = (qint16*)pE->lfpData;
            srcSY = pE->Trigger;
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

//------------------------------------------------------------------
// Experiment to histogram successive timestamp differences.
// One can collect just difs within packets, or just between
// packets, or both.
//
#if 1
qint64  dif = -999999;
if( it > 0 )        // intra-packet
    dif = ((electrodePacket*)&E[0])[ie].timestamp[it] -
        ((electrodePacket*)&E[0])[ie].timestamp[it-1];
else if( ie > 0 )   // inter-packet
    dif = ((electrodePacket*)&E[0])[ie].timestamp[0] -
        ((electrodePacket*)&E[0])[ie-1].timestamp[11];

if( dif == 0 ) {
    Log()<<QString("ZERO TSTAMP DIF: stamp %1 samples %2")
    .arg( ((electrodePacket*)&E[0])[ie].timestamp[it] )
    .arg( P.totPts );
}

if( dif > 31 ) {
    Log()<<QString("BIGDIF: ip %1 dif %2 stamp %3 npts %4")
    .arg( P.ip )
    .arg( dif )
    .arg( ((electrodePacket*)&E[0])[ie].timestamp[0] )
    .arg( P.totPts );
}

if( dif == -999999 )
    ;
else if( dif < 0 )
    ++bins[32];
else if( dif > 31 )
    ++bins[33];
else
    ++bins[dif];
#endif
//------------------------------------------------------------------

//------------------------------------------------------------------
// Experiment to visualize timestamps as sawtooth in channel 16.
#if 0
dst[16] = ((electrodePacket*)&E[0])[ie].timestamp[it] % 8000 - 4000;
#endif
//------------------------------------------------------------------

//------------------------------------------------------------------
// Experiment to visualize counter as sawtooth in channel 16.
#if 0
static uint count[8] = {0,0,0,0,0,0,0,0};
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

#ifdef PROFILE
    double  dtEnq = getTime();
#endif

    imQ[P.ip]->enqueue( &dst1D[0], TPNTPERFETCH * nE );
    P.totPts += TPNTPERFETCH * nE;

#ifdef PROFILE
    P.sumEnq += getTime() - dtEnq;
#endif

    return true;
}


bool ImAcqWorker::keepingUp( const ImAcqProbe &P )
{
    int qf = acq->fifoPct( P );

 // @@@ FIX Tune threshold for FIFO warnings after bigger FIFO provided

    if( qf >= 10 ) { // 5% standard

        Warning() <<
            QString("IMEC FIFO queue %1 fill% %2, loop ms <%3> peak %4")
            .arg( P.ip )
            .arg( qf, 2, 10, QChar('0') )
            .arg( 1000*P.sumTot/P.sumN, 0, 'f', 3 )
            .arg( 1000*P.peakDT, 0, 'f', 3 );

        if( qf >= 95 ) {
            acq->runError(
                QString("IMEC FIFO queue %1 overflow; stopping run.")
                .arg( P.ip ),
                false );
            return false;
        }
    }

    return true;
}


// sumN is the number of loop executions in the 5 sec check
// interval. The minimum value is 5*srate/(TPNTPERFETCH*maxE).
//
// sumTot/sumN is the average loop time to process the samples.
// The maximum value is TPNTPERFETCH*maxE/srate.
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
//
// Required values header is written at run start.
//
void ImAcqWorker::profile( ImAcqProbe &P )
{
#ifndef PROFILE
    Q_UNUSED( P )
#else
    Log() <<
        QString(
        "imec %1 loop ms <%2> get<%3> scl<%4> enq<%5> n(%6) %(%7)")
        .arg( P.ip, 2, 10, QChar('0') )
        .arg( 1000*P.sumTot/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumGet/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumScl/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumEnq/P.sumN, 0, 'f', 3 )
//        .arg( error15 )
        .arg( P.sumN )
        .arg( acq->fifoPct( P ), 2, 10, QChar('0') );

error15=0;
    P.sumGet    = 0;
    P.sumScl    = 0;
    P.sumEnq    = 0;
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqThread::ImAcqThread(
    CimAcqImec          *acq,
    QVector<AIQ*>       &imQ,
    ImAcqShared         &shr,
    QVector<ImAcqProbe> &probes )
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

// MS: loopSecs for ThinkPad T450 (2 core)
// MS: [[ Core i7-5600U @ 2.6Ghz, 8GB, Win7Pro-64bit ]]
// MS: 1 probe 0.004 with both audio and shankview
//
CimAcqImec::CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ),
        T(mainApp()->cfgCtl()->prbTab),
        shr( LOOPSECS * p.im.each[0].srate ),
        pausPortsRequired(0), pausSlot(-1), nThd(0)
{
}

/* ---------------------------------------------------------------- */
/* ~CimAcqImec ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::~CimAcqImec()
{
// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete imT[iThd];

    _close();
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

        QVector<ImAcqProbe> probes;

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
        QString("Require loop ms < [[ %1 ]] n > [[ %2 ]] maxE %3")
        .arg( 1000*TPNTPERFETCH*shr.maxE/p.im.each[0].srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.each[0].srate/(TPNTPERFETCH*shr.maxE) ) )
        .arg( shr.maxE );
#endif

    shr.startT = getTime();

// Wake all workers

    shr.condWake.wakeAll();

// Sleep main thread until external stop command

    atomicSleepWhenReady();

// --------
// Clean up
// --------

// Force all workers to exit

    shr.kill();

// Try nicely waiting for all threads to finish...
// Time out if not responding...

    double  t0 = getTime();

    do {
        int nRunning = 0;

        for( int iThd = 0; iThd < nThd; ++iThd )
            nRunning += imT[iThd]->thread->isRunning();

        if( !nRunning )
            break;

        QThread::msleep( 200 );

    } while( getTime() - t0 < 2.0 );

// Close hardware; may jostle laggards

    _close();

//------------------------------------------------------------------
// Experiment to histogram successive timestamp differences.
#if 1
for( int i = 0; i < 34; ++i ) {
    Log()<<QString("bin %1  N %2").arg( i ).arg( bins[i] );
}
#endif
//------------------------------------------------------------------
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
        QThread::usleep( 1e6*LOOPSECS/8 );

// ----------------------
// Stop streams this slot
// ----------------------

// @@@ FIX What about stopInfiniteStream? Replace with arm?
//
//    err = stopInfiniteStream( P.slot );

    err = arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ),
            false );
        return;
    }

// --------------------------
// Update settings this probe
// --------------------------

    if( !_selectElectrodes( P, false ) )
        return;

    if( !_setReferences( P, false ) )
        return;

    if( !_setGains( P, false ) )
        return;

    if( !_setHighPassFilter( P, false ) )
        return;

    if( !_setStandby( P, false ) )
        return;

    if( !_writeProbe( P, false ) )
        return;

// -------------------------------
// Set slot to software triggering
// -------------------------------

    err = setTriggerInput( P.slot, TRIGIN_SW );

    if( err != SUCCESS  ) {
        runError(
            QString("IMEC setTriggerInput(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ),
            false );
        return;
    }

// ------------
// Arm the slot
// ------------

    err = arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ),
            false );
        return;
    }

// @@@ FIX startInfiniteStream not needed??
//
//    err = startInfiniteStream( P.slot );
//
//    if( err != SUCCESS ) {
//        runError(
//            QString("IMEC startInfiniteStream(slot %1) error %2 '%3'.")
//            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ),
//            false );
//        return;
//    }

// ----------------
// Restart the slot
// ----------------

    err = setSWTrigger( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setSWTrigger(slot %1) error %2 '%3'.")
            .arg( P.slot ).arg( err ).arg( np_GetErrorMessage( err ) ),
            false );
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


void CimAcqImec::pauseAck( int port )
{
    QMutexLocker    ml( &runMtx );

    pausPortsReported.insert( port );
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
    double              loopT,
    qint16* rawAP, qint16* rawLF )  // @@@ FIX Mod for no packets
{
    nE = 0;

    int out = readAPFifo( P.slot, P.port, 0, rawAP, shr.maxE*TPNTPERFETCH );

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
    // Tune LOOPSECS and OVERFETCH on designated probe
    if( TUNE == P.ip ) {
        static int nnormal = 0;
        if( !nE )
            ;
        else if( nE != uint(loopSecs*p.im.each[P.ip].srate/TPNTPERFETCH) ) {
            Log() << nE << " " << shr.maxE << " " << nnormal;
            nnormal = 0;
        }
        else
            ++nnormal;
    }
#endif

    return true;
}

#endif


#if 1   // The real thing

bool CimAcqImec::fetchE(
    int                 &nE,
    qint8               *E,
    const ImAcqProbe    &P,
    double              loopT )
{
    nE = 0;

// ----------------------------------
// Fill with zeros if hardware paused
// ----------------------------------

    if( pausedSlot() == P.slot ) {

zeroFill:
        pauseAck( P.port );

        double  t0          = owner->imQ[P.ip]->tZero();
        quint64 targetCt    = (loopT+LOOPSECS - t0) * p.im.each[P.ip].srate;

        if( targetCt > P.totPts ) {

            nE = qMin( int((targetCt - P.totPts)/TPNTPERFETCH), shr.maxE );

            if( nE > 0 ) {

                int Ebytes = 0;

                if( P.fetchType == 0 )
                    Ebytes = sizeof(electrodePacket);

                memset( E, 0, nE * Ebytes );
            }
        }

        return true;
    }

// --------------------
// Else fetch real data
// --------------------

// @@@ FIX Experiment to report large fetch cycle times
#if 1
    static double tLastFetch[8] = {0,0,0,0,0,0,0,0};
    double tFetch = getTime();
    if( tLastFetch[P.ip] ) {
        if( tFetch - tLastFetch[P.ip] > LOOPSECS * 2 ) {
            Log() <<
                QString("       IM %1  dt %2  Q% %3")
                .arg( P.ip ).arg( int(1000*(tFetch - tLastFetch[P.ip])) )
                .arg( fifoPct( P ) );
        }
    }
    tLastFetch[P.ip] = tFetch;
#endif

    size_t          out;
    NP_ErrorCode    err = SUCCESS;

    if( P.fetchType == 0 ) {
        err = readElectrodeData(
                P.slot, P.port,
                (electrodePacket*)E,
                &out, shr.maxE );
    }

    if( err != SUCCESS ) {

        if( pausedSlot() == P.slot )
            goto zeroFill;

// @@@ FIX Diagnose bogus error codes

//        if( err == DATA_READ_FAILED ) {
//            Log() << "**** "<< out;
//++error15;
//            nE = 0;
//            return true;
//        }

        runError(
            QString(
            "IMEC readElectrodeData(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ),
            false );
        return false;
    }

    nE = out;

#ifdef TUNE
    // Tune LOOPSECS and OVERFETCH on designated probe
    if( TUNE == P.ip ) {
        static int nnormal = 0;
        if( !out )
            ;
        else if( out != uint(loopSecs*p.im.each[P.ip].srate/TPNTPERFETCH) ) {
            Log() << out << " " << shr.maxE << " " << nnormal;
            nnormal = 0;
        }
        else
            ++nnormal;
    }
#endif

// @@@ FIX Experiment to check error flags
#if 1
    int nCount = 0, nSerdes = 0, nLock = 0, nPop = 0, nSync = 0;

    for( int ie = 0; ie < nE; ++ie ) {

        quint16 *errs = ((electrodePacket*)E)[ie].Trigger;

        for( int i = 0; i < TPNTPERFETCH; ++i ) {

            int err = errs[i];

            if( err & 0x04 ) ++nCount;
            if( err & 0x08 ) ++nSerdes;
            if( err & 0x10 ) ++nLock;
            if( err & 0x20 ) ++nPop;
            if( err & 0x80 ) ++nSync;
        }
    }

    if( nCount + nSerdes + nLock + nPop + nSync ) {
        Log() <<
        QString("ERROR: S %1 P %2 cnt %3 ser %4 lok %5 pop %6 syn %7")
        .arg( P.slot ).arg( P.port )
        .arg( nCount ).arg( nSerdes ).arg( nLock ).arg( nPop ).arg( nSync );
    }
#endif

    return true;
}

#endif  // The real thing

/* ---------------------------------------------------------------- */
/* fifoPct -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int CimAcqImec::fifoPct( const ImAcqProbe &P )
{
    quint8  pct = 0;

    if( pausedSlot() != P.slot ) {

        size_t          nused, nempty;
        NP_ErrorCode    err;

        err =
        getElectrodeDataFifoState( P.slot, P.port, &nused, &nempty );

        if( err == SUCCESS )
            pct = (100*nused) / (nused + nempty);
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
    QThread::msleep( 2000 );
    SETVAL( 100 );
    return ok;
}


bool CimAcqImec::_openProbe( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("open probe %1").arg( P.ip ), true );

    NP_ErrorCode    err = openProbe( P.slot, P.port );
    QThread::msleep( 10 );  // post openProbe

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

    SETVAL( 27 );
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

    QString path = QString("%1/ImecProbeData").arg( appPath() );

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
    Log() << QString("Imec probe %1 ADC calibrated").arg( P.ip );
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

    QString path = QString("%1/ImecProbeData").arg( appPath() );

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

    SETVAL( 56 );
    Log() << QString("Imec probe %1 gains calibrated").arg( P.ip );
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

    Log() << QString("Imec probe %1 generating synthetic data").arg( P.ip );
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
    Log() << QString("Imec probe %1 LED set").arg( P.ip );
    return true;
}


bool CimAcqImec::_selectElectrodes( const CimCfg::ImProbeDat &P, bool kill )
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
                .arg( err ).arg( np_GetErrorMessage( err ) ),
                kill );
            return false;
        }
    }


    SETVAL( 59 );
    Log() << QString("Imec probe %1 electrodes selected").arg( P.ip );
    return true;
}


bool CimAcqImec::_setReferences( const CimCfg::ImProbeDat &P, bool kill )
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
                .arg( err ).arg( np_GetErrorMessage( err ) ),
                kill );
            return false;
        }
    }

    SETVAL( 60 );
    Log() << QString("Imec probe %1 references set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setGains( const CimCfg::ImProbeDat &P, bool kill )
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
// setting every 8th gain high and others low.
#if 0
        int apidx, lfidx;

        if( !(ic % 8) )
            apidx = IMROTbl::gainToIdx( 3000 );
        else
            apidx = IMROTbl::gainToIdx( 50 );

        if( !(ic % 8) )
            lfidx = IMROTbl::gainToIdx( 3000 );
        else
            lfidx = IMROTbl::gainToIdx( 50 );

        err = setGain( P.slot, P.port, ic,
                    apidx,
                    lfidx );
#endif
//---------------------------------------------------------

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setGain(slot %1, port %2) error %3 '%4'.")
                .arg( P.slot ).arg( P.port )
                .arg( err ).arg( np_GetErrorMessage( err ) ),
                kill );
            return false;
        }
    }

    SETVAL( 61 );
    Log() << QString("Imec probe %1 gains set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setHighPassFilter( const CimCfg::ImProbeDat &P, bool kill )
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
                .arg( err ).arg( np_GetErrorMessage( err ) ),
                kill );
            return false;
        }
    }

    SETVAL( 62 );
    Log() << QString("Imec probe %1 filters set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setStandby( const CimCfg::ImProbeDat &P, bool kill )
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
                .arg( err ).arg( np_GetErrorMessage( err ) ),
                kill );
            return false;
        }
    }

    SETVAL( 63 );
    Log() << QString("Imec probe %1 standby chans set").arg( P.ip );
    return true;
}


bool CimAcqImec::_writeProbe( const CimCfg::ImProbeDat &P, bool kill )
{
    SETLBL( QString("writing probe %1...").arg( P.ip ) );

    NP_ErrorCode    err;

    err = writeProbeConfiguration( P.slot, P.port, true );

    if( err != SUCCESS ) {
        runError(
            QString(
            "IMEC writeProbeConfig(slot %1, port %2) error %3 '%4'.")
            .arg( P.slot ).arg( P.port )
            .arg( err ).arg( np_GetErrorMessage( err ) ),
            kill );
        return false;
    }

    SETVAL( 100 );
    return true;
}


bool CimAcqImec::_setTrigger()
{
    SETLBL( "set triggering", true );

    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

// @@@ FIX Need real trigger input options from user params.
//
        int             slot    = T.getEnumSlot( is );
//        NP_ErrorCode    err     = setTriggerInput( slot, p.im.all.trgSource );
        NP_ErrorCode    err     = setTriggerInput( slot, TRIGIN_SW );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerInput(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }

// @@@ FIX Need trigger out options from user params.

        SETVAL( (is+1)*33/ns );

        err = setTriggerEdge( slot, p.im.all.trgRising );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

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

// @@@ FIX startInfiniteStream not needed??
//
//    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {
//
//        int             slot    = T.getEnumSlot( is );
//        NP_ErrorCode    err     = startInfiniteStream( slot );
//
//        if( err != SUCCESS ) {
//            runError(
//                QString("IMEC startInfiniteStream(slot %1) error %2 '%3'.")
//                .arg( slot )
//                .arg( err ).arg( np_GetErrorMessage( err ) ) );
//            return false;
//        }
//    }

    SETVAL( 100 );
    Log() << "IMEC Armed";
    return true;
}


bool CimAcqImec::_softStart()
{
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int             slot    = T.getEnumSlot( is );
        NP_ErrorCode    err     = setSWTrigger( slot );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setSWTrigger(slot %1) error %2 '%3'.")
                .arg( slot )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::configure()
{
    STOPCHECK;

    if( !_open( T ) )
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
/* close ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::_close()
{
// @@@ FIX Verify best closing practice.
    for( int is = 0, ns = T.nLogSlots(); is < ns; ++is ) {

        int slot = T.getEnumSlot( is );

// @@@ FIX What about stopInfiniteStream? Replace with arm?
//
//        stopInfiniteStream( slot );

        arm( slot );
//        close( slot, -1 );
        closeBS( slot );
    }

    QThread::msleep( 2000 );    // post closeBS
}

/* ---------------------------------------------------------------- */
/* runError ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::runError( QString err, bool kill )
{
    if( kill )
        _close();

    Error() << err;
    emit owner->daqError( err );
}

#endif  // HAVE_IMEC


