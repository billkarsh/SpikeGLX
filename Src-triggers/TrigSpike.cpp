
#include "TrigSpike.h"
#include "Util.h"
#include "Biquad.h"
#include "DataFile.h"
#include "GraphsWindow.h"

#include <QTimer>
#include <limits>




/* ---------------------------------------------------------------- */
/* struct HiPassFnctr --------------------------------------------- */
/* ---------------------------------------------------------------- */

// IMPORTANT!!
// -----------
// The Biquad maintains state data from its previous application.
// When applied to contiguous data streams there's no problem. If
// applied for the first time, or after a gap in the data, there
// will be a significant transient oscillation that dies down to
// below-noise levels in (conservatively) 120 timepoints.

// Here's the strategy to combat that...
// We look for edges using findFltRisingEdge(), starting from the
// position 'edgeCt'. Every time we modify edgeCt we will tell the
// filter to reset its 'zero' counter to 120. The filter overwrites
// its output with zeroes up to the zero count.

#define BIQUAD_TRANS_WIDE  120


TrigSpike::HiPassFnctr::HiPassFnctr( const DAQ::Params &p )
    :   flt(0), nchans(0), ichan(p.trgSpike.aiChan)
{
    if( ichan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {

        flt     = new Biquad( bq_type_highpass, 300/p.ni.srate );
        nchans  = p.ni.niCumTypCnt[CniCfg::niSumAll];
    }

    reset();
}


TrigSpike::HiPassFnctr::~HiPassFnctr()
{
    if( flt )
        delete flt;
}


void TrigSpike::HiPassFnctr::reset()
{
    nzero = BIQUAD_TRANS_WIDE;
}


void TrigSpike::HiPassFnctr::operator()( vec_i16 &data )
{
    if( flt ) {

        int ntpts = (int)data.size() / nchans;

        flt->apply1BlockwiseMem1( &data[0], 32768, ntpts, nchans, ichan );

        if( nzero > 0 ) {

            // overwrite with zeros

            if( ntpts > nzero )
                ntpts = nzero;

            short   *d		= &data[ichan],
                    *dlim	= &data[ichan + ntpts*nchans];

            for( ; d < dlim; d += nchans )
                *d = 0;

            nzero -= ntpts;
        }
    }
}

/* ---------------------------------------------------------------- */
/* TrigSpike ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

TrigSpike::TrigSpike(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        usrFlt(new HiPassFnctr( p )),
        nCycMax(
            p.trgSpike.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgSpike.nS),
        periEvtCt(p.trgSpike.periEvtSecs * p.ni.srate),
        refracCt(std::max( p.trgSpike.refractSecs * p.ni.srate, 5.0 )),
        latencyCt(0.25 * p.ni.srate)
{
}


void TrigSpike::setGate( bool hi )
{
    runMtx.lock();
    initState();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigSpike::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    initState();
    runMtx.unlock();
}


// Spike logic is driven by TrgSpikeParams:
// {periEvtSecs, refractSecs, inarow, nS, T}.
// Corresponding states {0=getEdge, 1=write, 2=done}.
//
void TrigSpike::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( 100 );

    initState();

    quint64 nextCt;
    qint64  remCt = 0;

    while( !isStopped() ) {

        double  loopT   = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = state == 2 || !isGateHi();

        if( inactive )
            goto next_loop;

        // -------------
        // If gate start
        // -------------

        if( !edgeCt ) {

            usrFlt->reset();

            if( !niQ->mapTime2Ct( edgeCt, getGateHiT() ) )
                goto next_loop;
        }

        // --------------
        // Seek next edge
        // --------------

        if( state == 0 ) {

            if( !getEdge() )
                goto next_loop;

            QMetaObject::invokeMethod(
                gw, "blinkTrigger",
                Qt::QueuedConnection );

            if( isPaused() ) {

                usrFlt->reset();
                edgeCt += refracCt;
                goto next_loop;
            }

            remCt   = -1;   // open new file
            state   = 1;
        }

        // ----------------
        // Handle this edge
        // ----------------

        if( state == 1 ) {

            if( !writeSome( nextCt, remCt ) )
                break;
        }

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 0.25 ) {

            QString sOn, sWr;
            int     ig, it;

            getGT( ig, it );
            statusOnSince( sOn, loopT, ig, it );
            statusWrPerf( sWr );

            Status() << sOn << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

    endRun();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


void TrigSpike::initState()
{
    usrFlt->reset();
    edgeCt  = 0;
    nS      = 0;
    state   = 0;
}


// Return true if found edge later than full premargin.
//
// Also require edge a little later (latencyCt) to allow
// time to start fetching those data.
//
bool TrigSpike::getEdge()
{
    quint64 minCt   = niQ->qHeadCt() + periEvtCt + latencyCt;

    if( edgeCt < minCt ) {

        usrFlt->reset();
        edgeCt = minCt;
    }

    return niQ->findFltRisingEdge(
                edgeCt,
                edgeCt,
                p.trgSpike.aiChan,
                p.trgSpike.T,
                p.trgSpike.inarow,
                *usrFlt );
}


bool TrigSpike::writeSome( quint64 &nextCt, qint64 &remCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;
    bool                        newFile = false;

// ---------------------------------------
// Fetch immediately (don't miss old data)
// ---------------------------------------

    if( remCt == -1 ) {

        newFile = true;
        nextCt  = edgeCt - periEvtCt;
        remCt   = 2 * periEvtCt + 1;
    }

    nb = niQ->getNScansFromCt( vB, nextCt, remCt );

// -------------------------------------
// Open new file for synchronous writing
// -------------------------------------

    if( newFile ) {

        int ig, it;

        if( !nb || !newTrig( ig, it, false ) )
            return false;

        dfni->setAsyncWriting( false );
    }

// ---------------
// Update tracking
// ---------------

    if( !nb )
        return true;

    nextCt = niQ->nextCt( vB );
    remCt -= nextCt - vB[0].headCt;

// -----
// Write
// -----

    if( !writeAndInvalVB( dfni, vB ) )
        return false;

// -----
// Done?
// -----

    if( remCt == 0 ) {

        endTrig();

        usrFlt->reset();
        edgeCt += refracCt;

        if( ++nS >= nCycMax )
            state = 2;
        else
            state = 0;
    }

    return true;
}


