
#include "TrigTimed.h"
#include "Util.h"
#include "DataFile.h"

#include <limits>




TrigTimed::TrigTimed(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        nCycMax(
            p.trgTim.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTim.nH),
        hiCtMax(
            p.trgTim.isHInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTim.tH * p.ni.srate),
        loCt(p.trgTim.tL * p.ni.srate),
        maxFetch(0.110 * p.ni.srate)
{
}


void TrigTimed::setGate( bool hi )
{
    runMtx.lock();
    initState();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigTimed::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    initState();
    runMtx.unlock();
}


// Timed logic is driven by TrgTimParams: {tL0, tH, tL, nH}.
// There are four corresponding states {0=L0, 1=H, 2=L, 3=done}.
//
void TrigTimed::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( 100 );

    initState();

    quint64 nextCt  = 0,
            hiCtCur = 0;
    int     ig      = -1,
            it      = -1;

    while( !isStopped() ) {

        double  loopT   = getTime(),
                gHiT,
                delT    = 0;
        bool    inactive;

        // --------
        // Inactive
        // --------

        inactive = state == 3 || isPaused() || !isGateHi();

        if( inactive ) {

            endTrig();
            goto next_loop;
        }

        // ------------------
        // Current gate start
        // ------------------

        gHiT = getGateHiT();

        // ----------------------------
        // L0 phase (waiting for start)
        // ----------------------------

        if( state == 0 )
            delT = remainingL0( loopT, gHiT );

        // -----------------
        // H phase (writing)
        // -----------------

        if( state == 1 ) {

next_H:
            if( !doSomeH( ig, it, gHiT, hiCtCur, nextCt ) )
                break;

            // Done?

            if( hiCtCur >= hiCtMax ) {

                if( nH >= nCycMax ) {
                    state       = 3;
                    inactive    = true;
                }
                else
                    state = 2;

                endTrig();
            }
        }

        // ----------------------------
        // L phase (waiting for next H)
        // ----------------------------

        if( state == 2 ) {

            delT = remainingL( nextCt );

            if( state == 1 )
                goto next_H;
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

            if( inactive )
                sT = " TX";
            else if( state == 0 || state == 2 )
                sT = QString(" T-%1s").arg( delT, 0, 'f', 1 );
            else
                sT = QString(" T+%1s").arg( hiCtCur/p.ni.srate, 0, 'f', 1 );

            Status() << sOn << sT << sWr;

            statusT = loopT;
        }

        // -------------------
        // Moderate fetch rate
        // -------------------

        yield( loopT );
    }

    endTrig();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


void TrigTimed::initState()
{
    nH      = 0;
    state   = (p.trgTim.tL0 > 0 ? 0 : 1);
}


// Return time remaining in L0 phase.
//
double TrigTimed::remainingL0( double loopT, double gHiT )
{
    double  elapsedT = loopT - gHiT;

    if( elapsedT < p.trgTim.tL0 )
        return p.trgTim.tL0 - elapsedT;  // remainder

    state = 1;

    return 0;
}


// Return time remaining in L phase.
//
double TrigTimed::remainingL( quint64 &nextCt )
{
    quint64 elapsedCt = niQ->curCount();

    if( elapsedCt < nextCt + loCt )
        return (nextCt + loCt - elapsedCt) / p.ni.srate;

    state   = 1;
    nextCt += loCt;

    return 0;
}


// Return true if no errors.
//
bool TrigTimed::doSomeH(
    int     &ig,
    int     &it,
    double  gHiT,
    quint64 &hiCtCur,
    quint64 &nextCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// -----
// Fetch
// -----

    if( !df ) {

        // Starting new file

        hiCtCur = 0;

        if( !nH ) {
            // H0 initial fetch based on time
            nb = niQ->getNScansFromT(
                    vB,
                    gHiT + p.trgTim.tL0,
                    (hiCtMax <= maxFetch ? hiCtMax : maxFetch) );
        }
        else {
            // Hk initial fetch based on nextCt
            nb = niQ->getNScansFromCt(
                    vB,
                    nextCt,
                    (hiCtMax <= maxFetch ? hiCtMax : maxFetch) );
        }

        if( !nb )
            return true;

        if( !newTrig( ig, it ) )
            return false;

        ++nH;
    }
    else {

        // File in progress

        uint    remCt = hiCtMax - hiCtCur;

        nb = niQ->getNScansFromCt(
                vB,
                nextCt,
                (remCt <= maxFetch ? remCt : maxFetch) );

        if( !nb )
            return true;
    }

// ---------------
// Update counting
// ---------------

    nextCt   = niQ->nextCt( vB );
    hiCtCur += nextCt - vB[0].headCt;

// -----
// Write
// -----

    return writeAndInvalVB( vB );
}


