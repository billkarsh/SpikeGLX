
#include "CimAcqSim.h"
#include "Util.h"

#define _USE_MATH_DEFINES
#include <math.h>




#define MAX10BIT    512
//#define PROFILE


// Give each analog channel a sin wave of period T.
// Amp = 100 uV
// Sync words get zeros.
//
static void genNPts(
    vec_i16             &data,
    const DAQ::Params   &p,
    const double        *gain,
    int                 nPts,
    quint64             cumSamp )
{
    const double    Tsec        = 1.0;
    const double    sampPerT    = Tsec * p.im.srate,
                    f           = 2*M_PI/sampPerT,
                    A           = MAX10BIT*100e-6/0.6;

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


// Give each analog channel zeros.
// Sync words get zeros.
//
static void genZero(
    vec_i16             &data,
    const DAQ::Params   &p,
    int                 nPts )
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

    const double    loopSecs    = 0.02;
    const quint64   maxPts      = 10 * loopSecs * p.im.srate;

    double  t0 = getTime();

    while( !isStopped() ) {

        double  t           = getTime(),
                tf;
        quint64 targetCt    = (t+loopSecs - t0) * p.im.srate;

#ifdef PROFILE
        double  t1 = 0;
#endif

        // Make some more pts?

        if( targetCt > totalTPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totalTPts, maxPts );

            if( !isPaused() )
                genNPts( data, p, &gain[0], nPts, totalTPts );
            else
                genZero( data, p, nPts );

#ifdef PROFILE
        t1 = getTime();
#endif

            owner->imQ->enqueue( data, nPts, totalTPts );
            totalTPts += nPts;
        }

        tf = getTime();

#ifdef PROFILE
// The actual rate should be ~p.im.srate = [[ 30000 ]].
// The total T should be <= loopSecs = [[ 20.00 ]] ms.

        Log() <<
            QString("im rate %1    tot %2    gen %3    enq %4")
            .arg( int(totalTPts/(tf-t0)) )
            .arg( 1000*(tf-t),  5, 'f', 2, '0' )
            .arg( 1000*(t1-t),  5, 'f', 2, '0' )
            .arg( 1000*(tf-t1), 5, 'f', 2, '0' );
#endif

        if( (tf -= t) < loopSecs )
            usleep( 1e6 * (loopSecs - tf) );
    }
}


