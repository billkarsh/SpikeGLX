
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
// The Biquad filter functions have internal memory, so if there's
// a discontinuity in a filter's input stream, transients ensue.
// Here's the strategy to combat filter transients...
// We look for edges using findFltFallingEdge(), starting from the
// position 'edgeCt'. Every time we modify edgeCt we will tell the
// filter to reset its 'zero' counter to BIQUAD_TRANS_WIDE. We'll
// have the filter zero that many leading data points.

TrigSpike::HiPassFnctr::HiPassFnctr( const DAQ::Params &p )
{
    fltbuf.resize( nmax = 256 );

    chan = p.trgSpike.aiChan;

    if( p.trgSpike.stream == "nidq" ) {

        if( chan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.ni.srate );
            maxInt  = 32768;
        }
    }
    else {

        // Highpass filtering in the Imec AP band is primarily
        // used to remove DC offsets, rather than LFP.

        if( chan < p.im.imCumTypCnt[CimCfg::imSumAP] ) {

            flt     = new Biquad( bq_type_highpass, 300/p.im.srate );
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


void TrigSpike::HiPassFnctr::operator()( int nflt )
{
    if( flt ) {

        flt->apply1BlockwiseMem1( &fltbuf[0], maxInt, nflt, 1, 0 );

        if( nzero > 0 ) {

            // overwrite with zeros

            if( nflt > nzero )
                nflt = nzero;

            memset( &fltbuf[0], 0, nflt*sizeof(qint16) );
            nzero -= nflt;
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
        spikesMax(p.trgSpike.isNInf ? UNSET64 : p.trgSpike.nS),
        aEdgeCtNext(0),
        thresh(p.trigThreshAsInt())
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

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive )
            goto next_loop;

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

            {
                int ig, it;

                if( !newTrig( ig, it, false ) ) {
                    err = "Generic error";
                    break;
                }

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

                err = "Generic error";
                break;
            }

            // -----
            // Done?
            // -----

            if( imCnt.remCt <= 0 && niCnt.remCt <= 0 ) {

                endTrig();

                if( ++nSpikes >= spikesMax )
                    SETSTATE_Done();
                else {

                    usrFlt->reset();
                    imCnt.advanceEdge();
                    niCnt.advanceEdge();

                    SETSTATE_GetEdge();
                }
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
            statusOnSince( sOn, ig, it );
            statusWrPerf( sWr );

            Status() << sOn << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

    endRun( err );
}


void TrigSpike::SETSTATE_GetEdge()
{
    aEdgeCtNext = 0;
    state       = 0;
}


void TrigSpike::SETSTATE_Write()
{
    imCnt.setupWrite( imQ != 0 );
    niCnt.setupWrite( niQ != 0 );

    state = 1;
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
    nSpikes         = 0;
    SETSTATE_GetEdge();
}


// Find edge in stream A...but translate time-points to stream B.
//
// Return true if found.
//
bool TrigSpike::getEdge(
    Counts              &cA,
    const SyncStream    &sA,
    Counts              &cB,
    const SyncStream    &sB )
{
// Start getEdge() search at gate edge, subject to
// periEvent criteria. Precision not needed here;
// sync only applied to getEdge() results.

    if( !cA.edgeCt ) {
        usrFlt->reset();
        cA.setGateEdge( sA, getGateHiT() );
    }

// It may take several tries to achieve pulser sync for multi streams.
// aEdgeCtNext saves us from costly refinding of edge-A while hunting.

    bool    found;

    if( aEdgeCtNext )
        found = true;
    else {
        found = sA.Q->findFltFallingEdge(
                    aEdgeCtNext,
                    cA.edgeCt,
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

        if( p.sync.sourceIdx != DAQ::eSyncSourceNone && !sB.bySync )
            return false;
    }

    if( found ) {

        alignX12( sA.Q, aEdgeCtNext, cB.edgeCt );

        cA.edgeCt = aEdgeCtNext;
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

    vec_i16 data;
    quint64 headCt = cnt.nextCt;

    if( !nScansFromCt( aiQ, data, headCt, cnt.remCt ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// ---------------
// Update tracking
// ---------------

    cnt.nextCt  += size / aiQ->nChans();
    cnt.remCt   -= cnt.nextCt - headCt;

// -----
// Write
// -----

    return writeAndInvalData( dst, data, headCt );
}


