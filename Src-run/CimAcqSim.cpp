
#include "CimAcqSim.h"
#include "Util.h"

#define _USE_MATH_DEFINES
#include <math.h>




#define MAX10BIT    512


// Give each analog channel a sin wave of period T.
// Amp = 100 uV
// Sync words get zeros.
//
static void genNPts(
    vec_i16         &data,
    const Params    &p,
    const double    *gain,
    int             nPts,
    quint64         cumSamp )
{
    const double    Tsec        = 1.0;
    const double    sampPerT    = Tsec * p.im.srate,
                    A           = MAX10BIT*100e-6/0.6;

    int n16     = p.im.imCumTypCnt[CimCfg::imSumAll],
        nNeu    = p.im.imCumTypCnt[CimCfg::imSumNeural];

    data.resize( n16 * nPts );

    for( int s = 0; s < nPts; ++s ) {

        double  V = A * sin( 2*M_PI * (cumSamp + s) / sampPerT );

        for( int c = 0; c < nNeu; ++c ) {

            int v = gain[c] * V;

            if( v < -MAX10BIT )
                v = -MAX10BIT;
            else if( v >= MAX10BIT )
                v = MAX10BIT-1;

            data[c + s*n16] = v;
        }

        for( int c = nNeu; c < n16; ++c )
            data[c + s*n16] = 0;
    }
}


// Give each analog channel zeros.
// Sync words get zeros.
//
static void genZero(
    vec_i16         &data,
    const Params    &p,
    int             nPts )
{
    data.resize( p.im.imCumTypCnt[CimCfg::imSumAll] * nPts, 0 );
}


// Alternately:
// (1) Generate pts at the sample rate.
// (2) Sleep 0.01 sec.
//
void CimAcqSim::run()
{
// ---------
// Configure
// ---------

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

    const double    sleepSecs   = 0.01;
    const quint64   maxPts      = 20 * sleepSecs * p.im.srate;

    double  t0 = getTime();

    while( !isStopped() ) {

        double  t           = getTime();
        quint64 targetCt    = (t - t0) * p.im.srate;

        // Make some more pts?

        if( targetCt > totalTPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totalTPts, maxPts );

            if( !isPaused() )
                genNPts( data, p, &gain[0], nPts, totalTPts );
            else
                genZero( data, p, nPts );

            owner->imQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        usleep( 1e6 * sleepSecs );
    }
}


