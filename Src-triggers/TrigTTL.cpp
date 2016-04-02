
#include "TrigTTL.h"
#include "Util.h"
#include "DataFile.h"

#include <limits>




TrigTTL::TrigTTL(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        nCycMax(
            p.trgTTL.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTTL.nH),
        hiCtMax(
            p.trgTTL.mode == DAQ::TrgTTLTimed ?
            p.trgTTL.tH * p.ni.srate
            : std::numeric_limits<qlonglong>::max()),
        marginCt(p.trgTTL.marginSecs * p.ni.srate),
        refracCt(p.trgTTL.refractSecs * p.ni.srate),
        maxFetch(0.110 * p.ni.srate)
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


// TTL logic is driven by TrgTTLParams:
// {marginSecs, refractSecs, tH, mode, inarow, nH, T}.
// Corresponding states {0=L, 1=premarg, 2=H, 3=postmarg, 4=done}.
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

    quint64 edgeCt  = 0,
            fallCt  = 0;
    qint64  remCt   = 0;
    int     ig      = -1,
            it      = -1;

    while( !isStopped() ) {

        double  loopT = getTime();
        bool    inactive;

        // --------
        // Inactive
        // --------

        inactive = state == 4 || isPaused() || !isGateHi();

        if( inactive ) {

            endTrig();
            goto next_loop;
        }

        // ---------------------------------
        // L phase (waiting for rising edge)
        // ---------------------------------

        if( state == 0 )
            seekNextEdge( edgeCt, fallCt, remCt );

        // ----------------
        // Write pre-margin
        // ----------------

        if( state == 1 ) {

            if( !writePreMargin( ig, it, remCt, edgeCt ) )
                break;
        }

        // -------
        // Write H
        // -------

        if( state == 2 ) {

            if( !doSomeH( ig, it, remCt, fallCt, edgeCt ) )
                break;

            // Done?

            if( p.trgTTL.mode == DAQ::TrgTTLLatch )
                goto next_loop;

            if( remCt <= 0 ) {

                if( !marginCt )
                    goto check_done;
                else {
                    remCt   = marginCt;
                    state   = 3;
                }
            }
        }

        // -----------------
        // Write post-margin
        // -----------------

        if( state == 3 ) {

            if( !writePostMargin( remCt, fallCt ) )
                break;

            // Done?

            if( remCt <= 0 ) {

check_done:
                if( nH >= nCycMax ) {
                    state       = 4;
                    inactive    = true;
                }
                else {

                    if( p.trgTTL.mode == DAQ::TrgTTLTimed )
                        nextCt = edgeCt + std::max( hiCtMax, refracCt );
                    else
                        nextCt = std::max( edgeCt + refracCt, fallCt + 1 );

                    edgeCt  = nextCt;
                    state   = 0;
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

            getGT( ig, it );
            statusOnSince( sOn, loopT, ig, it );
            statusWrPerf( sWr );

            if( inactive || !nextCt )
                sT = " TX";
            else if( !state ){
                sT =
                QString(" T-%1s")
                .arg( (nextCt - edgeCt)/p.ni.srate, 0, 'f', 1 );
            }
            else {
                sT =
                QString(" T+%1s")
                .arg( (nextCt - edgeCt + marginCt)/p.ni.srate, 0, 'f', 1 );
            }

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


void TrigTTL::initState()
{
    nextCt  = 0;
    nH      = 0;
    state   = 0;
}


void TrigTTL::seekNextEdge(
    quint64 &edgeCt,
    quint64 &fallCt,
    qint64  &remCt )
{
// First edge is the gate edge for status report

    if( !nextCt ) {

        if( !niQ->mapTime2Ct( nextCt, getGateHiT() ) )
            return;

        edgeCt = nextCt;
    }

    int thresh = p.ni.vToInt16( p.trgTTL.T, p.trgTTL.aiChan );

    if( !niQ->findRisingEdge(
            nextCt,
            nextCt,
            p.trgTTL.aiChan,
            thresh,
            p.trgTTL.inarow ) ) {

        return;
    }

    edgeCt  = nextCt;
    fallCt  = 0;
    remCt   = marginCt;
    state   = (marginCt ? 1 : 2);
}


// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePreMargin(
    int     &ig,
    int     &it,
    qint64  &remCt,
    quint64 edgeCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getNScansFromCt(
            vB,
            edgeCt - remCt,
            (remCt <= maxFetch ? remCt : maxFetch) );

    if( !nb )
        return true;

    if( !isDataFileNI() && !newTrig( ig, it ) )
        return false;

    if( (remCt -= niQ->sumCt( vB )) <= 0 )
        state = 2;

    nextCt = edgeCt - remCt;    // for state-2 & status

    return writeAndInvalVB( dfni, vB );
}


// Decrement remCt...
// remCt must previously be inited to marginCt.
//
// Return true if no errors.
//
bool TrigTTL::writePostMargin( qint64 &remCt, quint64 fallCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getNScansFromCt(
            vB,
            fallCt + 1 + marginCt - remCt,
            (remCt <= maxFetch ? remCt : maxFetch) );

    if( !nb )
        return true;

    remCt -= niQ->sumCt( vB );
    nextCt = fallCt + 1 + marginCt - remCt; // for status

    return writeAndInvalVB( dfni, vB );
}


// Return true if no errors.
//
bool TrigTTL::doSomeH(
    int     &ig,
    int     &it,
    qint64  &remCt,
    quint64 &fallCt,
    quint64 edgeCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// -----------------
// Starting new file
// -----------------

    if( !isDataFileNI() ) {

        if( !newTrig( ig, it ) )
            return false;

        ++nH;
    }

// ---------------
// Fetch a la mode
// ---------------

    if( p.trgTTL.mode == DAQ::TrgTTLLatch ) {

        // Latched case
        // Get all since last fetch

        nb = niQ->getAllScansFromCt( vB, nextCt );
    }
    else if( p.trgTTL.mode == DAQ::TrgTTLTimed ) {

        // Timed case
        // In-progress H fetch using counts

        fallCt  = edgeCt + hiCtMax - 1;

        remCt   = hiCtMax - (nextCt - edgeCt);

        nb = niQ->getNScansFromCt(
                vB,
                nextCt,
                (remCt <= maxFetch ? remCt : maxFetch) );
    }
    else {

        // Follower case
        // Fetch up to falling edge

        if( !fallCt ) {

            quint64 outCt;
            int     thresh = p.ni.vToInt16( p.trgTTL.T, p.trgTTL.aiChan );

            if( niQ->findFallingEdge(
                    outCt,
                    nextCt,
                    p.trgTTL.aiChan,
                    thresh,
                    p.trgTTL.inarow ) ) {

                fallCt  = outCt;
                remCt   = fallCt - nextCt + 1;
            }
            else
                remCt = hiCtMax;    // infinite for now
        }
        else
            remCt = fallCt - nextCt + 1;

        nb = niQ->getNScansFromCt(
                vB,
                nextCt,
                (remCt <= maxFetch ? remCt : maxFetch) );
    }

// ------------------------
// Write/update all H cases
// ------------------------

    if( !nb )
        return true;

    nextCt = niQ->nextCt( vB );
    remCt -= nextCt - vB[0].headCt;

    return writeAndInvalVB( dfni, vB );
}


