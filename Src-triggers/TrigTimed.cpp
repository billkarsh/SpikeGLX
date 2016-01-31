
#include "TrigTimed.h"
#include "Util.h"
#include "DataFile.h"




TrigTimed::TrigTimed(
    DAQ::Params     &p,
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        imCnt( p, p.im.srate),
        niCnt( p, p.ni.srate),
        nCycMax(
            p.trgTim.isNInf ?
            std::numeric_limits<qlonglong>::max()
            : p.trgTim.nH)
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

    int ig  = -1,
        it  = -1;

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
            if( !bothDoSomeH( ig, it, gHiT ) )
                break;

            // Done?

            if( (!niQ || niCnt.hiCtCur >= niCnt.hiCtMax)
                && (!imQ || imCnt.hiCtCur >= imCnt.hiCtMax) ) {

                if( ++nH >= nCycMax ) {
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

            if( niQ )
                delT = remainingL( niQ, niCnt );
            else
                delT = remainingL( imQ, imCnt );

            if( state == 1 ) {

                imCnt.nextCt += imCnt.loCt;
                niCnt.nextCt += niCnt.loCt;
                goto next_H;
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

            if( inactive )
                sT = " TX";
            else if( state == 0 || state == 2 )
                sT = QString(" T-%1s").arg( delT, 0, 'f', 1 );
            else {

                double  hisec;

                if( niQ )
                    hisec = niCnt.hiCtCur / p.ni.srate;
                else
                    hisec = imCnt.hiCtCur / p.im.srate;

                sT = QString(" T+%1s").arg( hisec, 0, 'f', 1 );
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
double TrigTimed::remainingL( const AIQ *aiQ, Counts &C )
{
    quint64 elapsedCt = aiQ->curCount();

    if( elapsedCt < C.nextCt + C.loCt )
        return (C.nextCt + C.loCt - elapsedCt) / aiQ->SRate();

    state = 1;

    return 0;
}


// Return true if no errors.
//
bool TrigTimed::bothDoSomeH( int &ig, int &it, double gHiT )
{
// -------------------
// Open files together
// -------------------

    if( (imQ && !dfim) || (niQ && !dfni) ) {

        imCnt.hiCtCur = 0;
        niCnt.hiCtCur = 0;

        if( !newTrig( ig, it ) )
            return false;
    }

// ---------------
// Fetch from each
// ---------------

    return eachDoSomeH( dfim, imQ, imCnt, gHiT )
            && eachDoSomeH( dfni, niQ, niCnt, gHiT );
}


// Return true if no errors.
//
bool TrigTimed::eachDoSomeH(
    DataFile    *df,
    const AIQ   *aiQ,
    Counts      &C,
    double      gHiT )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    if( !nH ) {
        // H0 initial fetch based on time
        nb = aiQ->getNScansFromT(
                vB,
                gHiT + p.trgTim.tL0,
                (C.hiCtMax <= C.maxFetch ? C.hiCtMax : C.maxFetch) );
    }
    else {
        // Hk fetch based on nextCt

        uint    remCt = C.hiCtMax - C.hiCtCur;

        nb = aiQ->getNScansFromCt(
                vB,
                C.nextCt,
                (remCt <= C.maxFetch ? remCt : C.maxFetch) );
    }

    if( !nb )
        return true;

// ---------------
// Update counting
// ---------------

    C.nextCt   = aiQ->nextCt( vB );
    C.hiCtCur += C.nextCt - vB[0].headCt;

// -----
// Write
// -----

    return writeAndInvalVB( df, vB );
}


