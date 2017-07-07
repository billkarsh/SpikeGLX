
#include "CniAcqSim.h"
#include "Util.h"

#define _USE_MATH_DEFINES
#include <math.h>




//#define PROFILE


// Give each analog channel a sin wave of period T.
// Digital words/channels get zeros.
//
static void genNPts(
    vec_i16             &data,
    const DAQ::Params   &p,
    int                 nPts,
    quint64             cumSamp )
{
    const double    Tsec        = 1.0;
    const double    sampPerT    = Tsec * p.ni.srate,
                    f           = 2*M_PI/sampPerT;

    int n16     = p.ni.niCumTypCnt[CniCfg::niSumAll],
        nAna    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

    data.resize( n16 * nPts );

    qint16  *dst = &data[0];

    for( int s = 0; s < nPts; ++s ) {

        double  V = 16000 * sin( f * (cumSamp + s) );

        for( int c = 0; c < nAna; ++c )
            dst[c + s*n16] = V;

        for( int c = nAna; c < n16; ++c )
            dst[c + s*n16] = 0;
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

    const double    loopSecs    = 0.02;
    const quint64   maxPts      = 10 * loopSecs * p.ni.srate;

    double  t0 = getTime();

    while( !isStopped() ) {

        double  t           = getTime(),
                tf;
        quint64 targetCt    = (t+loopSecs - t0) * p.ni.srate;

#ifdef PROFILE
        double  t1 = 0;
#endif

        // Make some more pts?

        if( targetCt > totalTPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totalTPts, maxPts );

            genNPts( data, p, nPts, totalTPts );

#ifdef PROFILE
        t1 = getTime();
#endif

            owner->niQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        tf = getTime();

#ifdef PROFILE
// The actual rate should be ~p.ni.srate = [[ 19737 ]].
// The total T should be <= loopSecs = [[ 20.00 ]] ms.

        Log() <<
            QString("ni rate %1    tot %2    gen %3    enq %4")
            .arg( int(totalTPts/(tf-t0)) )
            .arg( 1000*(tf-t),  5, 'f', 2, '0' )
            .arg( 1000*(t1-t),  5, 'f', 2, '0' )
            .arg( 1000*(tf-t1), 5, 'f', 2, '0' );
#endif

        if( (tf -= t) < loopSecs )
            usleep( 1e6 * (loopSecs - tf) );
    }
}


