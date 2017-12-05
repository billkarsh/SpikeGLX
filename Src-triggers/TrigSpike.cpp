
#include "TrigSpike.h"
#include "Util.h"
#include "Biquad.h"
#include "MainApp.h"
#include "Run.h"
#include "DataFile.h"
#include "GraphsWindow.h"

#include <QTimer>




/* ---------------------------------------------------------------- */
/* struct HiPassFnctr --------------------------------------------- */
/* ---------------------------------------------------------------- */

// IMPORTANT!!
// -----------
// Here's the strategy to combat filter transients...
// We look for edges using findFltFallingEdge(), starting from the
// position 'edgeCt'. Every time we modify edgeCt we will tell the
// filter to reset its 'zero' counter to BIQUAD_TRANS_WIDE. We'll
// have the filter zero that many leading data points.

TrigSpike::HiPassFnctr::HiPassFnctr( const DAQ::Params &p )
    :   flt(0), nchans(0), ichan(p.trgSpike.aiChan)
{
    if( p.trgSpike.stream == "nidq" ) {

        if( ichan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.ni.srate );
            nchans  = p.ni.niCumTypCnt[CniCfg::niSumAll];
            maxInt  = 32768;
        }
    }
    else {

        // Highpass filtering in the Imec AP band is primarily
        // used to remove DC offsets, rather than LFP.

        if( ichan < p.im.imCumTypCnt[CimCfg::imSumAP] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.im.srate );
            nchans  = p.im.imCumTypCnt[CimCfg::imSumAll];
            maxInt  = 512;
        }
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

        flt->apply1BlockwiseMem1( &data[0], maxInt, ntpts, nchans, ichan );

        if( nzero > 0 ) {

            // overwrite with zeros

            if( ntpts > nzero )
                ntpts = nzero;

            short   *d      = &data[ichan],
                    *dlim   = &data[ichan + ntpts*nchans];

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
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        usrFlt(new HiPassFnctr( p )),
        imCnt( p, p.im.srate ),
        niCnt( p, p.ni.srate ),
        spikesMax(
            p.trgSpike.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgSpike.nS),
        aEdgeCtNext(0),
        thresh(
            p.trgSpike.stream == "nidq" ?
            p.ni.vToInt16( p.trgSpike.T, p.trgSpike.aiChan )
            : p.im.vToInt10( p.trgSpike.T, p.trgSpike.aiChan ))
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


#define SETSTATE_GetEdge    (state = 0)

#define ISSTATE_GetEdge     (state == 0)
#define ISSTATE_Write       (state == 1)
#define ISSTATE_Done        (state == 2)


// Spike logic is driven by TrgSpikeParams:
// {periEvtSecs, refractSecs, inarow, nS, T}.
// Corresponding states defined above.
//
void TrigSpike::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( 100 );

    initState();

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive )
            goto next_loop;

        // -------------
        // If gate start
        // -------------

        // Set gateHiT as rough place to start getEdge() search,
        // subject to periEvent criteria. Precision not needed
        // here; sync only applied to getEdge() results.

        if( (imQ && !imCnt.edgeCt) || (niQ && !niCnt.edgeCt) ) {

            usrFlt->reset();

            double  gateT = getGateHiT();

            imCnt.edgeCt =
            qMax(
                imS.TAbs2Ct( gateT ),
                imQ->qHeadCt() + imCnt.periEvtCt + imCnt.latencyCt );

            niCnt.edgeCt =
            qMax(
                niS.TAbs2Ct( gateT ),
                niQ->qHeadCt() + niCnt.periEvtCt + niCnt.latencyCt );
        }

        // --------------
        // Seek next edge
        // --------------

        if( ISSTATE_GetEdge ) {

            if( p.trgSpike.stream == "nidq" ) {

                if( !getEdge( niCnt, niS, imCnt, imS ) )
                    goto next_loop;
            }
            else {

                if( !getEdge( imCnt, imS, niCnt, niS ) )
                    goto next_loop;
            }

            QMetaObject::invokeMethod(
                gw, "blinkTrigger",
                Qt::QueuedConnection );

            // ---------------
            // Start new files
            // ---------------

            imCnt.remCt = -1;
            niCnt.remCt = -1;

            {
                int ig, it;

                if( !newTrig( ig, it, false ) )
                    break;

                setSyncWriteMode();
            }

            SETSTATE_Write();
        }

        // ----------------
        // Handle this edge
        // ----------------

        if( ISSTATE_Write ) {

            if( !writeSome( DstImec, imQ, imCnt )
                || !writeSome( DstNidq, niQ, niCnt ) ) {

                break;
            }

            // -----
            // Done?
            // -----

            if( imCnt.remCt <= 0 && niCnt.remCt <= 0 ) {

                endTrig();

                usrFlt->reset();
                imCnt.edgeCt += imCnt.refracCt;
                niCnt.edgeCt += niCnt.refracCt;

                if( ++nSpikes >= spikesMax )
                    SETSTATE_Done();
                else
                    SETSTATE_GetEdge;
            }
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


void TrigSpike::SETSTATE_Write()
{
    state       = 1;
    aEdgeCtNext = 0;
}


void TrigSpike::SETSTATE_Done()
{
    state = 2;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigSpike::initState()
{
    usrFlt->reset();
    imCnt.edgeCt    = 0;
    niCnt.edgeCt    = 0;
    aEdgeCtNext     = 0;
    nSpikes         = 0;
    SETSTATE_GetEdge;
}


// Find edge in stream A...but translate time-points to stream B.
//
// Return true if found edge later than full premargin.
//
bool TrigSpike::getEdge(
    Counts              &cA,
    const SyncStream    &sA,
    Counts              &cB,
    const SyncStream    &sB )
{
// For multistream, we need mappable data for both A and B.
// aEdgeCtNext saves us from costly refinding of A in cases
// where A already succeeded but B not yet.

    bool    found;

    if( aEdgeCtNext )
        found = true;
    else {
        found = sA.Q->findFltFallingEdge(
                    aEdgeCtNext,
                    cA.edgeCt,
                    p.trgSpike.aiChan,
                    thresh,
                    p.trgSpike.inarow,
                    *usrFlt );

        if( !found ) {
            cA.edgeCt   = aEdgeCtNext;  // pick up search here
            aEdgeCtNext = 0;
        }
    }

    if( found && sB.Q ) {

        cB.edgeCt =
        sB.TAbs2Ct( syncDstTAbs( aEdgeCtNext, &sA, &sB, p ) );
    }

    if( found ) {

        alignX12( sA.Q, aEdgeCtNext, cB.edgeCt );

        cA.edgeCt   = aEdgeCtNext;
        aEdgeCtNext = 0;
    }

    return found;
}


bool TrigSpike::writeSome(
    DstStream   dst,
    const AIQ   *aiQ,
    Counts      &cnt )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// ---------------------------------------
// Fetch immediately (don't miss old data)
// ---------------------------------------

    if( cnt.remCt == -1 ) {
        cnt.nextCt  = cnt.edgeCt - cnt.periEvtCt;
        cnt.remCt   = 2 * cnt.periEvtCt + 1;
    }

    nb = aiQ->getNScansFromCt( vB, cnt.nextCt, cnt.remCt );

// ---------------
// Update tracking
// ---------------

    if( !nb )
        return true;

    cnt.nextCt = aiQ->nextCt( vB );
    cnt.remCt -= cnt.nextCt - vB[0].headCt;

// -----
// Write
// -----

    return writeAndInvalVB( dst, vB );
}


