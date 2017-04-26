
#include "CniAcqSim.h"
#include "Util.h"

#define _USE_MATH_DEFINES
#include <math.h>




// Give each analog channel a sin wave of period T.
// Digital words/channels get zeros.
//
static void genNPts(
    vec_i16         &data,
    const Params    &p,
    int             nPts,
    quint64         cumSamp )
{
    const double    Tsec        = 1.0;
    const double    sampPerT    = Tsec * p.ni.srate;

    int n16     = p.ni.niCumTypCnt[CniCfg::niSumAll],
        nAna    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

    data.resize( n16 * nPts );

    for( int s = 0; s < nPts; ++s ) {

        double  V = 16000 * sin( 2*M_PI * (cumSamp + s) / sampPerT );

        for( int c = 0; c < nAna; ++c )
            data[c + s*n16] = V;

        for( int c = nAna; c < n16; ++c )
            data[c + s*n16] = 0;
    }
}


// Alternately:
// (1) Generate pts at the sample rate.
// (2) Sleep 0.01 sec.
//
void CniAcqSim::run()
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

// Moderators prevent crashes by limiting how often and how many
// points are made. Such trouble can happen under high channel
// counts or in debug mode where everything is running slowly.
// The penalty is a reduction in actual sample rate.

    const double    sleepSecs   = 0.01;
    const quint64   maxPts      = 10 * sleepSecs * p.ni.srate;

    double  t0 = getTime();

    while( !isStopped() ) {

        double  t           = getTime();
        quint64 targetCt    = (t - t0) * p.ni.srate;

        // Make some more pts?

        if( targetCt > totalTPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totalTPts, maxPts );

            genNPts( data, p, nPts, totalTPts );

            owner->niQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        usleep( 1e6 * sleepSecs );
    }
}


