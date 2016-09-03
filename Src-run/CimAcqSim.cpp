
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

            int v = p.im.chanGain( c ) * V;

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

// -----
// Start
// -----

    atomicSleepWhenReady();

// -----
// Fetch
// -----

    const double    sleepSecs = 0.01;

    double  t0 = getTime();

    while( !isStopped() ) {

        double  t           = getTime();
        quint64 targetCt    = (t - t0) * p.im.srate;

        // Make some more pts?

        if( targetCt > totalTPts ) {

            vec_i16 data;
            int     nPts = targetCt - totalTPts;

            if( !isPaused() )
                genNPts( data, p, nPts, totalTPts );
            else
                genZero( data, p, nPts );

            owner->imQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        usleep( 1e6 * sleepSecs );
    }
}


