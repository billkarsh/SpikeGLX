
#include "CimAcqSim.h"
#include "Util.h"

#include <QThread>

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif

#define MAX10BIT    512
//#define PROFILE


/* ---------------------------------------------------------------- */
/* Generator functions -------------------------------------------- */
/* ---------------------------------------------------------------- */

// Give each analog channel a sin wave of period T.
// Amp = 100 uV.
// Sync words get zeros.
//
static void genNPts(
    vec_i16             &data,
    const DAQ::Params   &p,
    const double        *gain,
    int                 nPts,
    int                 ip,
    quint64             cumSamp )
{
    const double    Tsec        = 1.0,
                    sampPerT    = Tsec * p.im.all.srate,
                    f           = 2*M_PI/sampPerT,
                    A           = MAX10BIT*100e-6/p.im.all.range.rmax;

    int n16     = p.im.each[ip].imCumTypCnt[CimCfg::imSumAll],
        nNeu    = p.im.each[ip].imCumTypCnt[CimCfg::imSumNeural];

    data.resize( n16 * nPts );

    qint16  *dst = &data[0];

    for( int s = 0; s < nPts; ++s ) {

        double  V = A * sin( f * (cumSamp + s) );

        for( int c = 0; c < nNeu; ++c ) {

            dst[c + s*n16] =
                qBound( -MAX10BIT, int(gain[c] * V), MAX10BIT-1 );
        }

        for( int c = nNeu; c < n16; ++c )
            dst[c + s*n16] = 0;
    }
}


// Give each analog channel zeros.
// Sync words get zeros.
//
static void genZero(
    vec_i16             &data,
    const DAQ::Params   &p,
    int                 nPts,
    int                 ip )
{
    data.resize( p.im.each[ip].imCumTypCnt[CimCfg::imSumAll] * nPts, 0 );
}

/* ---------------------------------------------------------------- */
/* ImSimShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimShared::ImSimShared( const DAQ::Params &p )
    :   p(p), totPts(0ULL),
        awake(0), asleep(0), stop(false)
{
// Init gain table

    gain.resize( p.im.nProbes );

    for( int ip = 0; ip < p.im.nProbes; ++ip ) {

        const CimCfg::AttrEach  &E = p.im.each[ip];

        QVector<double> &G = gain[ip];

        int nNeu = E.imCumTypCnt[CimCfg::imSumNeural];

        G.resize( nNeu );

        for( int c = 0; c < nNeu; ++c )
            G[c] = E.chanGain( c );
    }
}

/* ---------------------------------------------------------------- */
/* ImSimWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ImSimWorker::run()
{
    const int   nID = vID.size();

    for(;;) {

        if( !shr.wake() )
            break;

        for( int iID = 0; iID < nID; ++iID ) {

            vec_i16 data;
            int     ip = vID[iID];

            if( !shr.zeros ) {

                genNPts( data, shr.p, &shr.gain[ip][0],
                    shr.nPts, ip, shr.totPts );
            }
            else
                genZero( data, shr.p, shr.nPts, ip );

            imQ[ip]->enqueue( data, shr.totPts, shr.nPts );
        }
    }

    emit finished();
}

/* ---------------------------------------------------------------- */
/* ImSimThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimThread::ImSimThread(
    ImSimShared     &shr,
    QVector<AIQ*>   &imQ,
    QVector<int>    &vID )
{
    thread  = new QThread;
    worker  = new ImSimWorker( shr, imQ, vID );

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
/* CimAcqSim::run() ----------------------------------------------- */
/* ---------------------------------------------------------------- */

// Alternately:
// (1) Generate pts at the sample rate.
// (2) Sleep balance of time, up to loopSecs.
//
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

    const int               nPrbPerThd = 3;

    QVector<ImSimThread*>   imT;
    ImSimShared             shr( p );
    int                     nThd = 0;

    for( int ip0 = 0; ip0 < p.im.nProbes; ip0 += nPrbPerThd ) {

        QVector<int>    vID;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < p.im.nProbes )
                vID.push_back( ip0 + id );
            else
                break;
        }

        imT.push_back( new ImSimThread( shr, owner->imQ, vID ) );
        ++nThd;
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    atomicSleepWhenReady();

// -----
// Fetch
// -----

// Moderators prevent crashes by limiting how often and how many
// points are made. Such trouble can happen under high channel
// counts or in debug mode where everything is running slowly.
// The penalty is a reduction in actual sample rate.

    const double    loopSecs    = 0.01;
    const quint64   maxPts      = 10 * loopSecs * p.im.all.srate;

    double  t0 = getTime();

    for( int ip = 0; ip < p.im.nProbes; ++ip )
        owner->imQ[ip]->setTZero( t0 );

    while( !isStopped() ) {

        double  tGen,
                t           = getTime(),
                tElapse     = t + loopSecs - t0;
        quint64 targetCt    = tElapse * p.im.all.srate;

        // Make some more pts?

        if( targetCt > shr.totPts ) {

            // Chunk params

            shr.awake   = 0;
            shr.asleep  = 0;
            shr.nPts    = qMin( targetCt - shr.totPts, maxPts );
            shr.zeros   = isPaused();

            // Wake all threads

            shr.condWake.wakeAll();

            // Wait all threads started, and all done

            shr.runMtx.lock();
                while( shr.awake  < nThd
                    || shr.asleep < nThd ) {

                    shr.runMtx.unlock();
                        usleep( 1e6*loopSecs/8 );
                    shr.runMtx.lock();
                }
            shr.runMtx.unlock();

            // Update counts

            shr.totPts += shr.nPts;
        }

        tGen = getTime() - t;

#ifdef PROFILE
// The actual rate should be ~p.im.all.srate = [[ 30000 ]].
// The generator T should be <= loopSecs = [[ 10.00 ]] ms.

        Log() <<
            QString("im rate %1    tot %2")
            .arg( shr.totPts/tElapse, 0, 'f', 0 )
            .arg( 1000*tGen, 5, 'f', 2, '0' );
#endif

        if( tGen < loopSecs )
            usleep( 1e6 * (loopSecs - tGen) );
    }

// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete imT[iThd];
}


