
#include "TrigSpike.h"
#include "Util.h"
#include "Biquad.h"
#include "DataFile.h"
#include "GraphsWindow.h"

#include <QTimer>




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
    if( p.trgSpike.stream == "imec" ) {

        if( ichan < p.im.imCumTypCnt[CimCfg::imSumAP] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.im.srate );
            nchans  = p.im.imCumTypCnt[CimCfg::imSumAll];
            maxInt  = 512;
        }
    }
    else {

        if( ichan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.ni.srate );
            nchans  = p.ni.niCumTypCnt[CniCfg::niSumAll];
            maxInt  = 32768;
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
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        usrFlt(new HiPassFnctr( p )),
        imCnt( p, p.im.srate ),
        niCnt( p, p.ni.srate ),
        nCycMax(
            p.trgSpike.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgSpike.nS)
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
#define SETSTATE_Write      (state = 1)
#define SETSTATE_Done       (state = 2)

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

            if( p.trgSpike.stream == "imec" ) {

                if( !getEdge( imCnt, imQ, niCnt, niQ ) )
                    goto next_loop;
            }
            else {

                if( !getEdge( niCnt, niQ, imCnt, imQ ) )
                    goto next_loop;
            }

            QMetaObject::invokeMethod(
                gw, "blinkTrigger",
                Qt::QueuedConnection );

// BK: Absorb pause into gate control...
// BK: This goes away.
//            if( isPaused() ) {
//
//                usrFlt->reset();
//                edgeCt += refracCt;
//                goto next_loop;
//            }

            // ---------------
            // Start new files
            // ---------------

            imCnt.remCt = -1;
            niCnt.remCt = -1;

            {
                int ig, it;

                if( !newTrig( ig, it, false ) )
                    break;

                if( dfim )
                    dfim->setAsyncWriting( false );

                if( dfni )
                    dfni->setAsyncWriting( false );
            }

            SETSTATE_Write;
        }

        // ----------------
        // Handle this edge
        // ----------------

        if( ISSTATE_Write ) {

            if( !writeSome( dfim, imQ, imCnt )
                || !writeSome( dfni, niQ, niCnt ) ) {

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
                    SETSTATE_Done;
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
    int     thresh;
    bool    found;

    if( qA == imQ )
        thresh = p.im.vToInt10( p.trgSpike.T, p.trgSpike.aiChan );
    else
        thresh = p.ni.vToInt16( p.trgSpike.T, p.trgSpike.aiChan );

    if( cA.edgeCt < minCt ) {

        usrFlt->reset();
        cA.edgeCt = minCt;
    }

    found = qA->findFltFallingEdge(
                cA.edgeCt,
                cA.edgeCt,
                p.trgSpike.aiChan,
                thresh,
                p.trgSpike.inarow,
                *usrFlt );

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, cA.edgeCt );
        qB->mapTime2Ct( cB.edgeCt, wallT );
    }

    return found;
}


bool TrigSpike::writeSome(
    DataFile    *df,
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

    return writeAndInvalVB( df, vB );
}


