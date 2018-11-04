
#include "CimAcqSim.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QThread>

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

// T0FUDGE used to sync IM and NI stream tZero values.
// SINEWAVES generates sine on all channels, else zeros.
#define MAX10BIT        512
#define T0FUDGE         0.0
#define MAXS            288
#define LOOPSECS        0.003
#define SINEWAVES
//#define PROFILE

/* ---------------------------------------------------------------- */
/* ImSimShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimShared::ImSimShared()
    :   awake(0), asleep(0), stop(false)
{
}

/* ---------------------------------------------------------------- */
/* ImSimProbe ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimProbe::ImSimProbe(
    const CimCfg::ImProbeTable  &T,
    const DAQ::Params           &p,
    int                         ip )
    :   peakDT(0), sumTot(0),
        totPts(0ULL), ip(ip),
        sumN(0)
{
#ifdef PROFILE
    sumGet  = 0;
    sumEnq  = 0;
    sumLok  = 0;
    sumWrk  = 0;
    sumPts  = 0;
#endif

    const CimCfg::AttrEach  &E      = p.im.each[ip];
    const int               *cum    = E.imCumTypCnt;

    srate   = E.srate;
    nAP     = cum[CimCfg::imTypeAP];
    nLF     = cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP];
    nSY     = cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF];
    nCH     = nAP + nLF + nSY;

    const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
    slot = P.slot;
    port = P.port;

// Gain table

    int nNeu = nAP + nLF;

    gain.resize( nNeu );

    for( int c = 0; c < nNeu; ++c )
        gain[c] = E.chanGain( c );
}

/* ---------------------------------------------------------------- */
/* ImSimWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ImSimWorker::run()
{
// Size buffers
// ------------
// - i16Buf[][]: sized for each probe.
//

    std::vector<std::vector<qint16> >   i16Buf;

    const int   nID = probes.size();

    i16Buf.resize( nID );

    for( int iID = 0; iID < nID; ++iID )
        i16Buf[iID].resize( MAXS * probes[iID].nCH );

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

            ImSimProbe  &P = probes[iID];

            if( !P.totPts )
                imQ[P.ip]->setTZero( loopT + T0FUDGE );

            double  dtTot = getTime();

            if( !doProbe( i16Buf[iID], P ) )
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

        // Yielding back some measured 'balance of expected time'
        // T > 0 via usleep( T ) can significantly reduce CPU load
        // but this comes at the expense of latency.

        double  dt = getTime() - loopT;

        if( dt < LOOPSECS )
            QThread::usleep( qMin( 1e6 * 0.5*(LOOPSECS - dt), 1000.0 ) );

        // ---------------
        // Rate statistics
        // ---------------

        if( loopT - lastCheckT >= 5.0 ) {

            for( int iID = 0; iID < nID; ++iID ) {

                ImSimProbe  &P = probes[iID];

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


bool ImSimWorker::doProbe( vec_i16 &dst1D, ImSimProbe &P )
{
#ifdef PROFILE
    double  prbT0 = getTime();
#endif

    qint16* dst = &dst1D[0];
    int     nS;

// -----
// Fetch
// -----

    nS = acq->fetchE( dst, P, loopT );

#ifdef PROFILE
    P.sumPts += nS;
#endif

    if( !nS )
        return true;

#ifdef PROFILE
    P.sumGet += getTime() - prbT0;
#endif

// -------
// Enqueue
// -------

#ifdef PROFILE
    double  dtEnq = getTime();
#endif

    double  tLock, tWork;

    imQ[P.ip]->enqueueProfile( tLock, tWork, dst, nS );
    P.totPts += nS;

#ifdef PROFILE
    P.sumEnq += getTime() - dtEnq;
    P.sumLok += tLock;
    P.sumWrk += tWork;
#endif

    return true;
}


// sumN is the number of loop executions in the 5 sec check
// interval. The minimum value is 5*srate/MAXS.
//
// sumTot/sumN is the average loop time to process the samples.
// The maximum value is MAXS/srate.
//
// Pts measures pts per fetch.
// Get measures the time spent fetching/making the data.
// Enq measures total time spent enquing stream data.
// Lok measures the time spent waiting for queue lock.
// Wrk measures the time spent allocating and writing.
//
// Required values header is written at run start.
//
void ImSimWorker::profile( ImSimProbe &P )
{
#ifndef PROFILE
    Q_UNUSED( P )
#else
    Log() <<
        QString(
        "imec %1 loop ms <%2> pts <%3> gen<%4>"
        " enq<%5> lok<%6> wrk<%7> n(%8)")
        .arg( P.ip, 2, 10, QChar('0') )
        .arg( 1000*P.sumTot/P.sumN, 0, 'f', 3 )
        .arg( P.sumPts/P.sumN )
        .arg( 1000*P.sumGet/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumEnq/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumLok/P.sumN, 0, 'f', 3 )
        .arg( 1000*P.sumWrk/P.sumN, 0, 'f', 3 )
        .arg( P.sumN );

    P.sumGet    = 0;
    P.sumEnq    = 0;
    P.sumLok    = 0;
    P.sumWrk    = 0;
    P.sumPts    = 0;
#endif
}

/* ---------------------------------------------------------------- */
/* ImSimThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimThread::ImSimThread(
    CimAcqSim           *acq,
    QVector<AIQ*>       &imQ,
    ImSimShared         &shr,
    QVector<ImSimProbe> &probes )
{
    thread  = new QThread;
    worker  = new ImSimWorker( acq, imQ, shr, probes );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImSimThread::~ImSimThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* CimAcqSim ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// MS: loopSecs for ThinkPad T450 (2 core)
// MS: [[ Core i7-5600U @ 2.6Ghz, 8GB, Win7Pro-64bit ]]
// MS: 1 probe 0.004 with both audio and shankview
//
CimAcqSim::CimAcqSim( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ),
        T(mainApp()->cfgCtl()->prbTab),
        maxV(p.im.all.range.rmax), nThd(0)
{
}

/* ---------------------------------------------------------------- */
/* ~CimAcqSim ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqSim::~CimAcqSim()
{
// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete imT[iThd];
}

/* ---------------------------------------------------------------- */
/* CimAcqSim::run ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqSim::run()
{
// ---------
// Configure
// ---------

// Create worker threads

// MS: With a variable number of probes handled per thread here,
// MS: and in the trigger, we see that balance is important. More
// MS: threads here allows more stable sample rate, but steals
// MS: time from writing, so some stream files get fewer samples.
// MS: Giving everyone more threads allows better interleaving of
// MS: operations, but pushes demands higher on CPU...If too high,
// MS: there's insufficient headroom to weather disruption from
// MS: service tasks or other user software. Generation thread
// MS: count should be adjusted to keep up with real imec hardware,
// MS: then trigger thread count should be increased to get even
// MS: file lengths. This informs either the limit of probe count
// MS: on given computer hardware, or the hardware requirements to
// MS: support a target probe count.

// @@@ FIX Tune probes per thread here and in triggers
    const int   nPrbPerThd = 3;

    for( int ip0 = 0, np = p.im.get_nProbes(); ip0 < np; ip0 += nPrbPerThd ) {

        QVector<ImSimProbe> probes;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < np )
                probes.push_back( ImSimProbe( T, p, ip0 + id ) );
            else
                break;
        }

        imT.push_back( new ImSimThread( this, owner->imQ, shr, probes ) );
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

    if( isStopped() )
        return;

// ---
// Run
// ---

#ifdef PROFILE
    // Table header, see profile discussion
    Log() <<
        QString("Require loop ms < [[ %1 ]] n > [[ %2 ]] MAXS %3")
        .arg( 1000*MAXS/p.im.each[0].srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.each[0].srate/MAXS ) )
        .arg( MAXS );
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
}

/* ---------------------------------------------------------------- */
/* fetchE --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef SINEWAVES

// Give each analog channel a sin wave of period T.
// Amp = 100 uV.
// Sync words get zeros.
//
static void genNPts(
    qint16          *dst,
    const double    *gain,
    double          maxV,
    double          srate,
    int             nNeu,
    int             nCH,
    quint64         cumSamp,
    int             nPts )
{
    const double    Tsec        = 1.0,
                    sampPerT    = Tsec * srate,
                    f           = 2*M_PI / sampPerT,
                    A           = MAX10BIT * 100e-6 / maxV;

    for( int s = 0; s < nPts; ++s ) {

        double  V = A * sin( f * (cumSamp + s) );

        for( int c = 0; c < nNeu; ++c ) {

            dst[c + s*nCH] =
                qBound( -MAX10BIT, int(gain[c] * V), MAX10BIT-1 );
        }

        for( int c = nNeu; c < nCH; ++c )
            dst[c + s*nCH] = 0;
    }
}

#endif


// Return sample count nS.
//
int CimAcqSim::fetchE(
    qint16              *dst,
    const ImSimProbe    &P,
    double              loopT )
{
// @@@ FIX Experiment to report large fetch cycle times.
#if 1
    static double tLastFetch[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    double tFetch = getTime();
    if( tLastFetch[P.ip] ) {
        if( tFetch - tLastFetch[P.ip] > LOOPSECS * 2 ) {
            Log() <<
                QString("       IM %1  dt %2")
                .arg( P.ip ).arg( int(1000*(tFetch - tLastFetch[P.ip])) );
        }
    }
    tLastFetch[P.ip] = tFetch;
#endif

    int nS = 0;

    double  t0          = owner->imQ[P.ip]->tZero();
    quint64 targetCt    = (loopT+LOOPSECS - t0) * p.im.each[P.ip].srate;

    if( targetCt > P.totPts ) {

        nS = qMin( int((targetCt - P.totPts)), MAXS );

        if( nS <= 0 )
            return nS;

#ifdef SINEWAVES
        genNPts(
            dst, &P.gain[0], maxV, P.srate,
            P.nAP + P.nLF, P.nCH, P.totPts, nS );
#else
        memset( dst, 0, nS * P.nCH * sizeof(qint16) );
#endif
    }

    return nS;
}

/* ---------------------------------------------------------------- */
/* runError ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqSim::runError( QString err )
{
    Error() << err;
    emit owner->daqError( err );
}


