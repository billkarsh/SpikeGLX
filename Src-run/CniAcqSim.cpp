
#include "CniAcqSim.h"
#include "Util.h"

#include <QThread>

#define _USE_MATH_DEFINES
#include <math.h>
#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif


#define MAX16BIT    32768
//#define PROFILE


/* ---------------------------------------------------------------- */
/* Generator functions -------------------------------------------- */
/* ---------------------------------------------------------------- */

// Give each analog channel a sin wave of period T.
// Neu amp = 100 uV.
// Aux amp = 2.2 V.
// Digital words/channels get zeros.
//
static void genNPts(
    vec_i16             &data,
    const DAQ::Params   &p,
    const double        *gain,
    int                 nPts,
    quint64             cumSamp )
{
    const double    Tsec        = 1.0,
                    sampPerT    = Tsec * p.ni.srate,
                    f           = 2*M_PI/sampPerT,
                    An          = MAX16BIT*100e-6/p.ni.range.rmax,
                    Ax          = MAX16BIT*2.2/p.ni.range.rmax;

    int n16     = p.ni.niCumTypCnt[CniCfg::niSumAll],
        nNeu    = p.ni.niCumTypCnt[CniCfg::niSumNeural],
        nAna    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];

    data.resize( n16 * nPts );

    qint16  *dst = &data[0];

    for( int s = 0; s < nPts; ++s ) {

        double  S = sin( f * (cumSamp + s) );

        for( int c = 0; c < nNeu; ++c ) {

            dst[c + s*n16] =
                qBound( -MAX16BIT, int(gain[c] * An * S), MAX16BIT-1 );
        }

        for( int c = nNeu; c < nAna; ++c ) {

            dst[c + s*n16] =
                qBound( -MAX16BIT, int(gain[c] * Ax * S), MAX16BIT-1 );

//----------------------------------------------------------------------
// Force 1-sec pulse; amplitude 3.1V; when [10..11)-sec; chans {0,192}.
// Useful for testing TTL trigger.
#if 0
if( c == 192 ) {
if( cumSamp+s >= 10*p.ni.srate && cumSamp+s < 11*p.ni.srate )
    dst[s*n16] = dst[c + s*n16] = int(gain[c] * MAX16BIT*3.1/p.ni.range.rmax);
else
    dst[s*n16] = dst[c + s*n16] = 0;
}
#endif
//----------------------------------------------------------------------
        }

        for( int c = nAna; c < n16; ++c )
            dst[c + s*n16] = 0;
    }
}

/* ---------------------------------------------------------------- */
/* CniAcqSim::run() ----------------------------------------------- */
/* ---------------------------------------------------------------- */

// Alternately:
// (1) Generate pts at the sample rate.
// (2) Sleep balance of time, up to loopSecs.
//
void CniAcqSim::run()
{
// ---------
// Configure
// ---------

// Init gain table

    int             nAna = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
    QVector<double> gain( nAna );

    for( int c = 0; c < nAna; ++c )
        gain[c] = p.ni.chanGain( c );

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

    owner->niQ->setTZero( t0 );

    while( !isStopped() ) {

        double  tGen,
                t           = getTime(),
                tElapse     = t + loopSecs - t0;
        quint64 targetCt    = tElapse * p.ni.srate;

        // Make some more pts?

        if( targetCt > totPts ) {

            vec_i16 data;
            int     nPts = qMin( targetCt - totPts, maxPts );

            genNPts( data, p, &gain[0], nPts, totPts );

            if( !owner->niQ->enqueue( data, totPts, nPts ) ) {
                QString e = "NI simulator enqueue low mem.";
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
// The actual rate should be ~p.ni.srate = [[ 19737 ]].
// The generator T should be <= loopSecs = [[ 20.00 ]] ms.

            static quint64  lastPts = 0;

            Log() <<
                QString("ni rate %1    tot %2")
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


