
#include "TrigTTL.h"
#include "Util.h"
#include "DataFile.h"




TrigTTL::TrigTTL(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        imCnt( p, p.im.srate ),
        niCnt( p, p.ni.srate ),
        nCycMax(
            p.trgTTL.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTTL.nH),
        aEdgeCt(0),
        thresh(
            p.trgTTL.stream == "imec" ?
            p.im.vToInt10( p.trgTTL.T, p.trgTTL.aiChan )
            : p.ni.vToInt16( p.trgTTL.T, p.trgTTL.aiChan ))
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


#define SETSTATE_PreMarg    (state = 1)
#define SETSTATE_PostMarg   (state = 3)
#define SETSTATE_Done       (state = 4)

#define ISSTATE_L           (state == 0)
#define ISSTATE_PreMarg     (state == 1)
#define ISSTATE_H           (state == 2)
#define ISSTATE_PostMarg    (state == 3)
#define ISSTATE_Done        (state == 4)


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

    setYieldPeriod_ms( 100 );

    initState();

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

            if( p.trgTTL.stream == "imec" ) {

                if( !getRiseEdge( imCnt, imQ, niCnt, niQ ) )
                    goto next_loop;
            }
            else {

                if( !getRiseEdge( niCnt, niQ, imCnt, imQ ) )
                    goto next_loop;
            }

            if( niCnt.marginCt )
                SETSTATE_PreMarg;
            else
                SETSTATE_H();

            // -------------------
            // Open files together
            // -------------------

            if( allFilesClosed() ) {

                int ig, it;

                if( !newTrig( ig, it ) )
                    break;

                ++nH;
            }
        }

        // ----------------
        // Write pre-margin
        // ----------------

        if( ISSTATE_PreMarg ) {

            if( !writePreMargin( DstImec, imCnt, imQ )
                || !writePreMargin( DstNidq, niCnt, niQ ) ) {

                break;
            }

            if( (!imQ || imCnt.remCt <= 0)
                && (!niQ || niCnt.remCt <= 0) ) {

                SETSTATE_H();
            }
        }

        // -------
        // Write H
        // -------

        if( ISSTATE_H ) {

            if( !doSomeH( DstImec, imCnt, imQ )
                || !doSomeH( DstNidq, niCnt, niQ ) ) {

                break;
            }

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            if( p.trgTTL.mode == DAQ::TrgTTLFollowAI ) {

                // In this mode, a zero remainder doesn't mean done.

                if( p.trgTTL.stream == "imec" ) {

                    if( !imCnt.fallCt )
                        goto next_loop;
                }
                else {

                    if( !niCnt.fallCt )
                        goto next_loop;
                }
            }

            // Check for zero remainder

            if( (!imQ || imCnt.remCt <= 0)
                && (!niQ || niCnt.remCt <= 0) ) {

                if( !niCnt.marginCt )
                    goto check_done;
                else {
                    imCnt.remCt = imCnt.marginCt;
                    niCnt.remCt = niCnt.marginCt;
                    SETSTATE_PostMarg;
                }
            }
        }

        // -----------------
        // Write post-margin
        // -----------------

        if( ISSTATE_PostMarg ) {

            if( !writePostMargin( DstImec, imCnt, imQ )
                || !writePostMargin( DstNidq, niCnt, niQ ) ) {

                break;
            }

            // Done?

            if( (!imQ || imCnt.remCt <= 0)
                && (!niQ || niCnt.remCt <= 0) ) {

check_done:
                if( nH >= nCycMax ) {
                    SETSTATE_Done;
                    inactive = true;
                }
                else {

                    if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

                        imCnt.nextCt =
                            imCnt.edgeCt
                            + std::max( imCnt.hiCtMax, imCnt.refracCt );

                        niCnt.nextCt =
                            niCnt.edgeCt
                            + std::max( niCnt.hiCtMax, niCnt.refracCt );
                    }
                    else {

                        imCnt.nextCt =
                            std::max(
                                imCnt.edgeCt + imCnt.refracCt,
                                imCnt.fallCt + 1 );

                        niCnt.nextCt =
                            std::max(
                                niCnt.edgeCt + niCnt.refracCt,
                                niCnt.fallCt + 1 );
                    }

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
        if( loopT - statusT > 0.25 ) {

            QString sOn, sT, sWr;
            int     ig, it;

            getGT( ig, it );
            statusOnSince( sOn, loopT, ig, it );
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

    endRun();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


void TrigTTL::SETSTATE_L()
{
    state           = 0;
    aEdgeCt         = 0;
    imCnt.fallCt    = 0;
    niCnt.fallCt    = 0;
}


void TrigTTL::SETSTATE_H()
{
    state       = 2;
    imCnt.remCt = -1;
    niCnt.remCt = -1;
}


void TrigTTL::initState()
{
    imCnt.nextCt    = 0;
    niCnt.nextCt    = 0;
    nH              = 0;
    SETSTATE_L();
}


// Find rising edge in stream A...but translate time-points to stream B.
//
bool TrigTTL::getRiseEdge(
    Counts      &cA,
    const AIQ   *qA,
    Counts      &cB,
    const AIQ   *qB )
{
    bool    found;

// First edge is the gate edge for (state L) status report
// wherein edgeCt is just time we started seeking an edge.

    if( !cA.nextCt ) {

        double  gateT = getGateHiT();

        if( !qA->mapTime2Ct( cA.nextCt, gateT ) )
            return false;

        cA.edgeCt = cA.nextCt;

        if( qB && qB->mapTime2Ct( cB.nextCt, gateT ) )
            cB.edgeCt = cB.nextCt;
    }

    if( aEdgeCt )
        found = true;
    else {
        found = qA->findRisingEdge(
                    aEdgeCt,
                    cA.nextCt,
                    p.trgTTL.aiChan,
                    thresh,
                    p.trgTTL.inarow );

        if( !found ) {
            cA.nextCt   = aEdgeCt;
            aEdgeCt     = 0;
        }
    }

    quint64 bOutCt;

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, aEdgeCt );
        found = qB->mapTime2Ct( bOutCt, wallT );
    }

    if( found ) {

        alignX12( qA, aEdgeCt, bOutCt );

        cA.nextCt   = aEdgeCt;
        cA.edgeCt   = aEdgeCt;
        cA.remCt    = cA.marginCt;
        aFallCt     = aEdgeCt;
        aEdgeCt     = 0;

        if( qB ) {
            cB.nextCt   = bOutCt;
            cB.edgeCt   = bOutCt;
            cB.remCt    = cB.marginCt;
        }
    }

    return found;
}


// Find falling edge in stream A...but translate time-points to stream B.
//
void TrigTTL::getFallEdge(
    Counts      &cA,
    const AIQ   *qA,
    Counts      &cB,
    const AIQ   *qB )
{
    bool    found;

    if( aEdgeCt )
        found = true;
    else {
        found = qA->findFallingEdge(
                    aFallCt,
                    aFallCt,
                    p.trgTTL.aiChan,
                    thresh,
                    p.trgTTL.inarow );

        if( found )
            aEdgeCt = aFallCt;
    }

    quint64 bOutCt;

    if( found && qB ) {

        double  wallT;

        qA->mapCt2Time( wallT, aEdgeCt );
        found = qB->mapTime2Ct( bOutCt, wallT );
    }

    if( found ) {

        alignX12( qA, aEdgeCt, bOutCt );

        cA.fallCt   = aEdgeCt;
        cA.remCt    = aEdgeCt - cA.nextCt;
        aEdgeCt     = 0;

        if( qB ) {
            cB.fallCt   = bOutCt;
            cB.remCt    = bOutCt - cB.nextCt;
        }
    }
    else {
        // If we didn't yet find the falling edge we have to
        // limit the next read so we don't go too far...
        // For A, don't go farther than we've looked.
        // For B, use the current B count.
        //
        // Since we're setting a finite remCt, exhausting
        // that in a read doesn't mean we're done. Rather,
        // for the follower mode we need to test if the
        // falling edge was found.

        cA.remCt = aFallCt - cA.nextCt;

        if( qB )
            cB.remCt = qB->curCount() - cB.nextCt;
        else
            cB.remCt = 0;
    }
}


// Write margin up to but not including rising edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePreMargin( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ || !C.remCt )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = aiQ->getNScansFromCt(
            vB,
            C.edgeCt - C.remCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be what's done: +(margin - rem).
// If next = edge - rem, then status = +(next - edge + margin).
//
// When rem falls to zero, (next = edge) sets us up for state H.

    C.remCt -= aiQ->sumCt( vB );
    C.nextCt = C.edgeCt - C.remCt;

    return writeAndInvalVB( dst, vB );
}


// Write margin, including falling edge.
//
// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePostMargin( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ || !C.remCt )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = aiQ->getNScansFromCt(
            vB,
            C.fallCt + C.marginCt - C.remCt,
            (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );

    if( !nb )
        return true;

// Status in this state should be: +(margin + H + margin - rem).
// With next defined as below, status = +(next - edge + margin)
// = margin + (fall-edge) + margin - rem = correct.

    C.remCt -= aiQ->sumCt( vB );
    C.nextCt = C.fallCt + C.marginCt - C.remCt;

    return writeAndInvalVB( dst, vB );
}


// Write from rising edge up to but not including falling edge.
//
// Return true if no errors.
//
bool TrigTTL::doSomeH( DstStream dst, Counts &C, const AIQ *aiQ )
{
    if( !aiQ || (p.trgTTL.mode != DAQ::TrgTTLFollowAI && !C.remCt) )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// ---------------
// Fetch a la mode
// ---------------

    if( p.trgTTL.mode == DAQ::TrgTTLLatch ) {

        // Latched case
        // Get all since last fetch

        nb = aiQ->getAllScansFromCt( vB, C.nextCt );
    }
    else if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

        // Timed case
        // In-progress H fetch using counts

        C.fallCt  = C.edgeCt + C.hiCtMax;
        C.remCt   = C.hiCtMax - (C.nextCt - C.edgeCt);

        nb = aiQ->getNScansFromCt(
                vB,
                C.nextCt,
                (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );
    }
    else {

        // Follower case
        // Fetch up to falling edge

        if( !C.fallCt ) {

            if( p.trgTTL.stream == "imec" )
                getFallEdge( imCnt, imQ, niCnt, niQ );
            else
                getFallEdge( niCnt, niQ, imCnt, imQ );
        }
        else
            C.remCt = C.fallCt - C.nextCt;

        nb = aiQ->getNScansFromCt(
                vB,
                C.nextCt,
                (C.remCt <= C.maxFetch ? C.remCt : C.maxFetch) );
    }

// ------------------------
// Write/update all H cases
// ------------------------

    if( !nb )
        return true;

    C.nextCt = aiQ->nextCt( vB );
    C.remCt -= C.nextCt - vB[0].headCt;

    return writeAndInvalVB( dst, vB );
}


void TrigTTL::statusProcess( QString &sT, bool inactive )
{
    if( inactive || !niCnt.nextCt )
        sT = " TX";
    else if( ISSTATE_L ) {

        double  dt;

        if( niQ )
            dt = (niCnt.nextCt - niCnt.edgeCt)/p.ni.srate;
        else
            dt = (imCnt.nextCt - imCnt.edgeCt)/p.im.srate;

        sT = QString(" T-%1s").arg( dt, 0, 'f', 1 );
    }
    else {

        double  dt;

        if( niQ ) {
            dt = (niCnt.nextCt - niCnt.edgeCt + niCnt.marginCt)
                    / p.ni.srate;
        }
        else {
            dt = (imCnt.nextCt - imCnt.edgeCt + imCnt.marginCt)
                    / p.im.srate;
        }

        sT = QString(" T+%1s").arg( dt, 0, 'f', 1 );
    }
}


