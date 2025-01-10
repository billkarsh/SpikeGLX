
#include "CimAcqSim.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QThread>

#include <math.h>

// SINEWAVES generates sine on all channels, else zeros.
#define MAX10BIT        512
#define MAXVOLTS        0.6
#define MAXS            288
#define LOOPSECS        0.003
#define SINEWAVES
#define PROFILE

/* ---------------------------------------------------------------- */
/* ImSimAcqShared ------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimAcqShared::ImSimAcqShared()
    :   awake(0), asleep(0), stop(false)
{
}

/* ---------------------------------------------------------------- */
/* ImSimAcqProbe -------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimAcqProbe::ImSimAcqProbe(
    const CimCfg::ImProbeTable  &T,
    const DAQ::Params           &p,
    AIQ                         *Q,
    int                         ip )
    :   peakDT(0), sumTot(0), totPts(0ULL), Q(Q), ip(ip), sumN(0)
{
#ifdef PROFILE
    sumGet  = 0;
    sumEnq  = 0;
    sumLok  = 0;
    sumWrk  = 0;
    sumPts  = 0;
#endif

    const CimCfg::PrbEach   &E      = p.im.prbj[ip];
    const int               *cum    = E.imCumTypCnt;

    srate   = E.srate;
    nAP     = cum[CimCfg::imTypeAP];
    nLF     = cum[CimCfg::imTypeLF] - nAP;
    nCH     = cum[CimCfg::imSumAll];

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
/* ImSimAcqWorker ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ImSimAcqWorker::run()
{
// Size buffers
// ------------
// - i16Buf[][]: sized for each probe.
//
    std::vector<vec_i16 >   i16Buf;

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

            ImSimAcqProbe   &P = probes[iID];

            if( !P.totPts )
                P.Q->setTZero( loopT );

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

        double  dt = getTime() - loopT;

        if( dt < LOOPSECS )
            QThread::usleep( 250 );

        // ---------------
        // Rate statistics
        // ---------------

        if( loopT - lastCheckT >= 5.0 ) {

            for( int iID = 0; iID < nID; ++iID ) {

                ImSimAcqProbe   &P = probes[iID];

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


bool ImSimAcqWorker::doProbe( vec_i16 &dst1D, ImSimAcqProbe &P )
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

    P.Q->enqueueProfile( tLock, tWork, dst, nS );
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
void ImSimAcqWorker::profile( ImSimAcqProbe &P )
{
#ifndef PROFILE
    Q_UNUSED( P )
#else
    Log() <<
        QString(
        "imec %1 loop ms <%2> lag <%3> gen<%4>"
        " enq<%5> lok<%6> wrk<%7> n(%8)")
        .arg( P.ip, 2, 10, QChar('0') )
        .arg( 1000*P.sumTot/P.sumN, 0, 'f', 3 )
        .arg( 1000*(getTime() - P.Q->endTime()), 0, 'f', 3 )
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
/* ImSimAcqThread ------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimAcqThread::ImSimAcqThread(
    CimAcqSim                   *acq,
    ImSimAcqShared              &shr,
    std::vector<ImSimAcqProbe>  &probes )
{
    thread  = new QThread;
    worker  = new ImSimAcqWorker( acq, shr, probes );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImSimAcqThread::~ImSimAcqThread()
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
    :   CimAcq(owner, p), T(mainApp()->cfgCtl()->prbTab), maxV(MAXVOLTS)
{
}

/* ---------------------------------------------------------------- */
/* ~CimAcqSim ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqSim::~CimAcqSim()
{
    shr.kill();

    for( int iThd = 0, nThd = imT.size(); iThd < nThd; ++iThd ) {
        imT[iThd]->thread->wait( 10000/nThd );
        delete imT[iThd];
    }
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

    for( int i = 0; i < 3; ++i ) {
        QMetaObject::invokeMethod(
            mainApp(), "rsAuxStep",
            Qt::QueuedConnection );
    }

    {
        // @@@ FIX Tune streams per thread here and in triggers
        const int                   nStrPerThd = 3;
        std::vector<ImSimAcqProbe>  probes;

        for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

            probes.push_back( ImSimAcqProbe( T, p, owner->imQ[ip], ip ) );

            for( int i = 0; i < 11; ++i ) {
                QMetaObject::invokeMethod(
                    mainApp(), "rsProbeStep",
                    Qt::QueuedConnection );
            }

            if( probes.size() >= nStrPerThd ) {
                imT.push_back( new ImSimAcqThread( this, shr, probes ) );
                probes.clear();
            }
        }

        if( probes.size() )
            imT.push_back( new ImSimAcqThread( this, shr, probes ) );
    }

    for( int i = 0; i < 2; ++i ) {
        QMetaObject::invokeMethod(
            mainApp(), "rsStartStep",
            Qt::QueuedConnection );
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
    {
        int nThd = imT.size();

        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                QThread::usleep( 10 );
            shr.runMtx.lock();
        }
    }
    shr.runMtx.unlock();

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( isStopped() )
        return;

    QMetaObject::invokeMethod(
        mainApp(), "rsStartStep",
        Qt::QueuedConnection );

// ---
// Run
// ---

#ifdef PROFILE
    // Table header, see profile discussion
    Log() <<
        QString("Require loop ms < [[ %1 ]] n > [[ %2 ]] MAXS %3")
        .arg( 1000*MAXS/p.im.prbj[0].srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.prbj[0].srate/MAXS ) )
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
    const ImSimAcqProbe &P,
    double              loopT )
{
    int nS = 0;

    double  t0          = owner->imQ[P.ip]->tZero();
    quint64 targetCt    = (loopT+LOOPSECS - t0) * p.im.prbj[P.ip].srate;

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


