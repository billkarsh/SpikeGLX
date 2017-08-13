
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
        nCycMax(
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

        // Set gateHiT as place from which to start
        // searching for edge in getEdge().

        if( !imCnt.edgeCt || !niCnt.edgeCt ) {

            usrFlt->reset();

            double  gateT = getGateHiT();

            if( imQ && !imQ->mapTime2Ct( imCnt.edgeCt, gateT ) )
                goto next_loop;

            if( niQ && !niQ->mapTime2Ct( niCnt.edgeCt, gateT ) )
                goto next_loop;
        }

        // --------------
        // Seek next edge
        // --------------

        if( ISSTATE_GetEdge ) {

            if( p.trgSpike.stream == "nidq" ) {

                if( !getEdge( niCnt, niQ, imCnt, imQ ) )
                    goto next_loop;
            }
            else {

                if( !getEdge( imCnt, imQ, niCnt, niQ ) )
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

                if( ++nS >= nCycMax )
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
    nS              = 0;
    SETSTATE_GetEdge;
}


// Find edge in stream A...but translate time-points to stream B.
//
// Return true if found edge later than full premargin.
//
// Also require edge a little later (latencyCt) to allow
// time to start fetching those data.
//
bool TrigSpike::getEdge(
    Counts      &cA,
    const AIQ   *qA,
    Counts      &cB,
    const AIQ   *qB )
{
    quint64 minCt = qA->qHeadCt() + cA.periEvtCt + cA.latencyCt;
    bool    found;

    if( cA.edgeCt < minCt ) {

        usrFlt->reset();
        cA.edgeCt = minCt;
    }

// For multistream, we need mappable data for both A and B.
// aEdgeCtNext saves us from costly refinding of A in cases
// where A already succeeded but B not yet.

    if( aEdgeCtNext )
        found = true;
    else {
        found = qA->findFltFallingEdge(
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

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, aEdgeCtNext );
        found = qB->mapTime2Ct( cB.edgeCt, wallT );
    }

    if( found ) {

        alignX12( qA, aEdgeCtNext, cB.edgeCt );

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


