
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
    quint64             cumSamp )
{
    const double    Tsec        = 1.0,
                    sampPerT    = Tsec * p.im.srate,
                    f           = 2*M_PI/sampPerT,
                    A           = MAX10BIT*100e-6/p.im.range.rmax;

    int n16     = p.im.imCumTypCnt[CimCfg::imSumAll],
        nNeu    = p.im.imCumTypCnt[CimCfg::imSumNeural];

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

// Init gain table

    int             nNeu = p.im.imCumTypCnt[CimCfg::imSumNeural];
    QVector<double> gain( nNeu );

    for( int c = 0; c < nNeu; ++c )
        gain[c] = p.im.chanGain( c );

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
    const quint64   maxPts      = 10 * loopSecs * p.im.srate;

    double  t0 = getTime();

    owner->imQ->setTZero( t0 );

    while( !isStopped() ) {

        double  tGen,
                t           = getTime(),
                tElapse     = t + loopSecs - t0;
        quint64 targetCt    = tElapse * p.im.srate;

        // Make some more pts?

        if( targetCt > totPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totPts, maxPts );

            genNPts( data, p, &gain[0], nPts, totPts );

            if( !owner->imQ->enqueue( data, totPts, nPts ) ) {
                QString e = "IM simulator enqueue low mem.";
                Error() << e;
                owner->daqError( e );
                return;
            }

            totPts += nPts;
        }

        tGen = getTime() - t;

        static double   tLastBreath = t;

        if( t - tLastBreath > 1.0 ) {

            guiBreathe();

#ifdef PROFILE
// The actual rate should be ~p.im.srate = [[ 30000 ]].
// The generator T should be <= loopSecs = [[ 10.00 ]] ms.

            static quint64  lastPts = 0;

            Log() <<
                QString("im rate %1    tot %2")
                .arg( (totPts-lastPts)/(t-tLastBreath), 0, 'f', 0 )
                .arg( 1000*tGen, 5, 'f', 2, '0' );

            lastPts = totPts;
#endif
            tLastBreath = t;
        }

        if( tGen < loopSecs )
            QThread::usleep( 1e6 * (loopSecs - tGen) );
        else
            QThread::usleep( 1000 * 10 );
    }
}


