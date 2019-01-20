
#include "TrigTTL.h"
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "DataFile.h"




TrigTTL::TrigTTL(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        imCnt( p, p.im.srate, p.im.enabled ),
        niCnt( p, p.ni.srate, p.ni.enabled ),
        highsMax(p.trgTTL.isNInf ? UNSET64 : p.trgTTL.nH),
        aEdgeCtNext(0),
        thresh(p.trigThreshAsInt()),
        digChan(p.trgTTL.isAnalog ? -1 : p.trigChan())
{
}


void TrigTTL::setGate( bool hi )
{
    runMtx.lock();
    initState();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigTTL::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    initState();
    runMtx.unlock();
}


#define LOOP_MS             100
#define TINYMARG            0.0001

#define ISSTATE_L           (state == 0)
#define ISSTATE_PreMarg     (state == 1)
#define ISSTATE_H           (state == 2)
#define ISSTATE_PostMarg    (state == 3)
#define ISSTATE_Done        (state == 4)

#define ZEROREM ((!niQ || niCnt.remCt <= 0) && (!imQ || imCnt.remCt <= 0))


// TTL logic is driven by TrgTTLParams:
// {marginSecs, refractSecs, tH, mode, inarow, nH, T}.
// Corresponding states defined above.
//
// There are three TTL modes: {latched, timed, follower}.
// -latched  goes high on triggered; stays high for whole gate.
// -timed    goes high on triggered; stays high for tH.
// -follower goes high on triggered; stays high while chan high.
//
void TrigTTL::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( LOOP_MS );

    initState();

    QString err;

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

        if( inactive ) {

            endTrig();
            goto next_loop;
        }

        // ---------------------------------
        // L phase (waiting for rising edge)
        // ---------------------------------

        if( ISSTATE_L ) {

            if( p.trgTTL.stream == "nidq" ) {

                if( !getRiseEdge( niCnt, niS, imCnt, imS ) )
                    goto next_loop;
            }
            else {

                if( !getRiseEdge( imCnt, imS, niCnt, niS ) )
                    goto next_loop;
            }

            if( p.trgTTL.marginSecs > TINYMARG )
                SETSTATE_PreMarg();
            else
                SETSTATE_H();

            // -------------------
            // Open files together
            // -------------------

            {
                int ig, it;

                if( !newTrig( ig, it ) ) {
                    err = "Generic error";
                    break;
                }
            }
        }

        // ----------------
        // Write pre-margin
        // ----------------

        if( ISSTATE_PreMarg ) {

            if( !writePreMargin( DstImec, imCnt, imQ )
                || !writePreMargin( DstNidq, niCnt, niQ ) ) {

                err = "Generic error";
                break;
            }

            if( ZEROREM )
                SETSTATE_H();
        }

        // -------
        // Write H
        // -------

        if( ISSTATE_H ) {

            // Set falling edge

            if( p.trgTTL.mode == DAQ::TrgTTLFollowV ) {

                if( p.trgTTL.stream == "nidq" ) {

                    if( !niCnt.fallCt )
                        getFallEdge( niCnt, niS, imCnt, imS );
                }
                else {

                    if( !imCnt.fallCt )
                        getFallEdge( imCnt, imS, niCnt, niS );
                }
            }

            // Write

            if( !doSomeH( DstImec, imCnt, imQ )
                || !doSomeH( DstNidq, niCnt, niQ ) ) {

                err = "Generic error";
                break;
            }

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            if( p.trgTTL.mode == DAQ::TrgTTLFollowV ) {

                // In this mode, remCt isn't set until fallCt is set.

                if( p.trgTTL.stream == "nidq" ) {

                    if( !niCnt.fallCt )
                        goto next_loop;
                }
                else {

                    if( !imCnt.fallCt )
                        goto next_loop;
                }
            }

            // Check for zero remainder

            if( ZEROREM ) {

                if( p.trgTTL.marginSecs <= TINYMARG )
                    goto check_done;
                else
                    SETSTATE_PostMarg();
            }
        }

        // -----------------
        // Write post-margin
        // -----------------

        if( ISSTATE_PostMarg ) {

            if( !writePostMargin( DstImec, imCnt, imQ )
                || !writePostMargin( DstNidq, niCnt, niQ ) ) {

                err = "Generic error";
                break;
            }

            // Done?

            if( ZEROREM ) {

check_done:
                if( ++nHighs >= highsMax ) {
                    SETSTATE_Done();
                    inactive = true;
                }
                else {

                    if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

                        imCnt.advanceByTime();
                        niCnt.advanceByTime();
                    }
                    else {
                        imCnt.advancePastFall();
                        niCnt.advancePastFall();
                    }

                    // Strictly for L status message
                    imCnt.edgeCt = imCnt.nextCt;
                    niCnt.edgeCt = niCnt.nextCt;
                    SETSTATE_L();
                }

                endTrig();
            }
        }

        // ------
        // Status
        // ------

next_loop:
        if( loopT - statusT > 1.0 ) {

            QString sOn, sT, sWr;

            statusOnSince( sOn );
            statusProcess( sT, inactive );
            statusWrPerf( sWr );

            Status() << sOn << sT << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

    endRun( err );
}


void TrigTTL::SETSTATE_L()
{
    aEdgeCtNext = 0;
    aFallCtNext = 0;

    state = 0;
}


// Set from and rem for universal premargin.
//
void TrigTTL::SETSTATE_PreMarg()
{
    imCnt.setPreMarg();
    niCnt.setPreMarg();

    state = 1;
}


// Set from, rem and fall (if applicable) for H phase.
//
void TrigTTL::SETSTATE_H()
{
    imCnt.setH( DAQ::TrgTTLMode(p.trgTTL.mode) );
    niCnt.setH( DAQ::TrgTTLMode(p.trgTTL.mode) );

    state = 2;
}


// Set from and rem for postmargin;
// not applicable in latched mode.
//
void TrigTTL::SETSTATE_PostMarg()
{
    if( p.trgTTL.mode != DAQ::TrgTTLLatch ) {

        imCnt.setPostMarg();
        niCnt.setPostMarg();
    }

    state = 3;
}


void TrigTTL::SETSTATE_Done()
{
    state = 4;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigTTL::initState()
{
    imCnt.nextCt    = 0;
    niCnt.nextCt    = 0;
    nHighs          = 0;
    SETSTATE_L();
}


// Find rising edge in stream A...but translate time-points to stream B.
//
bool TrigTTL::getRiseEdge(
    Counts              &cA,
    const SyncStream    &sA,
    Counts              &cB,
    const SyncStream    &sB )
{
// First edge is the gate edge for (state L) status report
// wherein edgeCt is just time we started seeking an edge.

    if( !cA.nextCt ) {
        if( 0 != sA.Q->mapTime2Ct( cA.nextCt, getGateHiT() ) )
            return false;
    }

// It may take several tries to achieve pulser sync for multi streams.
// aEdgeCtNext saves us from costly refinding of edge-A while hunting.

    bool    found;

    if( aEdgeCtNext )
        found = true;
    else {

        if( digChan < 0 ) {

            found = sA.Q->findRisingEdge(
                        aEdgeCtNext,
                        cA.nextCt,
                        p.trgTTL.chan,
                        thresh,
                        p.trgTTL.inarow );
        }
        else {
            found = sA.Q->findBitRisingEdge(
                        aEdgeCtNext,
                        cA.nextCt,
                        digChan,
                        p.trgTTL.bit % 16,
                        p.trgTTL.inarow );
        }

        if( !found ) {
            cA.nextCt   = aEdgeCtNext;  // pick up search here
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
        cA.edgeCt   = aEdgeCtNext;
        cA.nextCt   = aEdgeCtNext;  // for status tracking
    }

    return found;
}


// Find falling edge in stream A...but translate time-points to stream B.
//
// Set fallCt(s) if edge found, set remCt(s) whether found or not.
//
// nextCt tracks where we are writing, aFallCtNext tracks edge hunting.
//
void TrigTTL::getFallEdge(
    Counts              &cA,
    const SyncStream    &sA,
    Counts              &cB,
    const SyncStream    &sB )
{
    bool    found;

    if( !aFallCtNext )
        aFallCtNext = cA.edgeCt;

    if( digChan < 0 ) {

        found = sA.Q->findFallingEdge(
                    aFallCtNext,
                    aFallCtNext,
                    p.trgTTL.chan,
                    thresh,
                    p.trgTTL.inarow );
    }
    else {
        found = sA.Q->findBitFallingEdge(
                    aFallCtNext,
                    aFallCtNext,
                    digChan,
                    p.trgTTL.bit % 16,
                    p.trgTTL.inarow );
    }

    if( found )
        cA.fallCt = aFallCtNext;

    if( found && sB.Q ) {

        cB.fallCt =
        sB.TAbs2Ct( syncDstTAbs( cA.fallCt, &sA, &sB, p ) );
    }

    if( found ) {

        cA.remCt = cA.fallCt - cA.nextCt;

        if( sB.Q )
            cB.remCt = cB.fallCt - cB.nextCt;
    }
    else {
        // If we didn't yet find the falling edge we have to
        // limit the next read so we don't go too far...

        cA.remCt = aFallCtNext - cA.nextCt;

        if( sB.Q ) {
            cB.remCt =
                sB.TAbs2Ct( syncDstTAbs( aFallCtNext, &sA, &sB, p ) )
                - cB.nextCt;
        }
        else
            cB.remCt = 0;
    }
}


// Write margin up to but not including rising edge.
//
// Return true if no errors.
//
bool TrigTTL::writePreMargin( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ || C.remCt <= 0 )
        return true;

    vec_i16 data;
    quint64 headCt  = C.nextCt;
    int     nMax    = (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch);

    if( !nScansFromCt( aiQ, data, headCt, nMax ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(margin + next - edge).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt -= size / aiQ->nChans();
    C.nextCt = C.edgeCt - C.remCt;

    return writeAndInvalData( dst, data, headCt );
}


// Write margin, including falling edge.
//
// Return true if no errors.
//
bool TrigTTL::writePostMargin( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ || C.remCt <= 0 )
        return true;

    vec_i16 data;
    quint64 headCt  = C.nextCt;
    int     nMax    = (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch);

    if( !nScansFromCt( aiQ, data, headCt, nMax ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(margin + next - edge)
// = margin + (fall-edge) + margin - rem = correct.

    C.remCt -= size / aiQ->nChans();
    C.nextCt = C.fallCt + C.marginCt - C.remCt;

    return writeAndInvalData( dst, data, headCt );
}


// Write from rising edge up to but not including falling edge.
//
// Return true if no errors.
//
bool TrigTTL::doSomeH( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ )
        return true;

    vec_i16 data;
    quint64 headCt = C.nextCt;
    bool    ok;

// ---------------
// Fetch a la mode
// ---------------

    if( p.trgTTL.mode == DAQ::TrgTTLLatch )
        ok = nScansFromCt( aiQ, data, headCt, -LOOP_MS );
    else if( C.remCt <= 0 )
        return true;
    else {

        int nMax = (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch);

        ok = nScansFromCt( aiQ, data, headCt, nMax );
    }

    if( !ok )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

// ------------------------
// Write/update all H cases
// ------------------------

    C.nextCt    += size / aiQ->nChans();
    C.remCt     -= C.nextCt - headCt;

    return writeAndInvalData( dst, data, headCt );
}


void TrigTTL::statusProcess( QString &sT, bool inactive )
{
    bool trackNI = p.trgTTL.stream == "nidq";

    if( inactive
        || (trackNI  && !niCnt.nextCt)
        || (!trackNI && !imCnt.nextCt) ) {

        sT = " TX";
    }
    else if( ISSTATE_L ) {

        double  dt = (trackNI ? niCnt : imCnt).L_progress();

        sT = QString(" T-%1s").arg( dt, 0, 'f', 1 );
    }
    else {

        double  dt = (trackNI ? niCnt : imCnt).H_progress();

        sT = QString(" T+%1s").arg( dt, 0, 'f', 1 );
    }
}


