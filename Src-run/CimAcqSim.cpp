
#include "CimAcqSim.h"
#include "Util.h"

#define	_USE_MATH_DEFINES
#include <math.h>




// Give each analog channel a sin wave of period T.
// Sync words get zeros.
//
static void genNPts(
    vec_i16         &data,
    const Params    &p,
    int             nPts,
    quint64         cumSamp )
{
    const double    Tsec        = 1.0;
    const double    sampPerT    = Tsec * p.im.srate;

    int n16     = p.im.imCumTypCnt[CimCfg::imSumAll],
        nNeu    = p.im.imCumTypCnt[CimCfg::imSumNeural];

    data.resize( n16 * nPts );

    for( int s = 0; s < nPts; ++s ) {

        double  V = 16000 * sin( 2*M_PI * (cumSamp + s) / sampPerT );

        for( int c = 0; c < nNeu; ++c )
            data[c + s*n16] = V;

        for( int c = nNeu; c < n16; ++c )
            data[c + s*n16] = 0;
    }
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

            genNPts( data, p, nPts, totalTPts );

            owner->imQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        usleep( 1e6 * sleepSecs );
    }
}


