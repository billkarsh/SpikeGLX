
#include "TrigTimed.h"
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "DataFile.h"




TrigTimed::TrigTimed(
    const DAQ::Params   &p,
    GraphsWindow        *gw,
    const AIQ           *imQ,
    const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ),
        imCnt( p, p.im.srate, p.im.enabled ),
        niCnt( p, p.ni.srate, p.ni.enabled ),
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


#define SETSTATE_L0     (state = 0)
#define SETSTATE_H      (state = 1)
#define SETSTATE_L      (state = 2)

#define ISSTATE_L0      (state == 0)
#define ISSTATE_H       (state == 1)
#define ISSTATE_L       (state == 2)
#define ISSTATE_Done    (state == 3)


// Timed logic is driven by TrgTimParams: {tL0, tH, tL, nH}.
// There are four corresponding states defined above.
//
void TrigTimed::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( 100 );

    initState();

    QString err;

    while( !isStopped() ) {

        double  loopT   = getTime(),
                gHiT,
                delT    = 0;
        bool    inactive;

        // -------
        // Active?
        // -------

        inactive = ISSTATE_Done || !isGateHi();

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

        if( ISSTATE_L0 )
            delT = remainingL0( loopT, gHiT );

        // -----------------
        // H phase (writing)
        // -----------------

        if( ISSTATE_H ) {

            if( !bothDoSomeH( gHiT ) ) {
                err = "Generic error";
                break;
            }

            // Done?

            if( niCnt.hDone() && imCnt.hDone() ) {

                if( ++nH >= nCycMax ) {
                    SETSTATE_Done();
                    inactive = true;
                }
                else {
                    alignNextFiles();
                    SETSTATE_L;
                }

                endTrig();
            }
        }

        // ----------------------------
        // L phase (waiting for next H)
        // ----------------------------

        if( ISSTATE_L ) {

            if( niQ )
                delT = remainingL( niQ, niCnt );
            else
                delT = remainingL( imQ, imCnt );
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
            statusWrPerf( sWr );

            if( inactive )
                sT = " TX";
            else if( ISSTATE_L0 || ISSTATE_L )
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

    endRun( err );
}


void TrigTimed::SETSTATE_Done()
{
    state = 3;
    mainApp()->getRun()->dfSetRecordingEnabled( false, true );
}


void TrigTimed::initState()
{
    nH = 0;

    imCnt.nextCt = 0;
    niCnt.nextCt = 0;

    if( p.trgTim.tL0 > 0 )
        SETSTATE_L0;
    else
        SETSTATE_H;
}


// Return time remaining in L0 phase.
//
double TrigTimed::remainingL0( double loopT, double gHiT )
{
    double  elapsedT = loopT - gHiT;

    if( elapsedT < p.trgTim.tL0 )
        return p.trgTim.tL0 - elapsedT;  // remainder

    SETSTATE_H;

    return 0;
}


// Return time remaining in L phase.
//
double TrigTimed::remainingL( const AIQ *aiQ, const Counts &C )
{
    quint64 endCt = aiQ->endCount();

    if( endCt < C.nextCt )
        return (C.nextCt - endCt) / aiQ->sRate();

    SETSTATE_H;

    return 0;
}


// Next file @ X12 boundary
//
// One-time concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. The most likely failure mode is
// that the target time is too new, which is fixed by
// retrying on another loop iteration.
//
bool TrigTimed::alignFirstFiles( double gHiT )
{
    if( (imQ && !imCnt.nextCt) || (niQ && !niCnt.nextCt) ) {

        double  startT = gHiT + p.trgTim.tL0;
        quint64 imNext, niNext;

        if( niQ && (0 != niQ->mapTime2Ct( niNext, startT )) )
            return false;

        if( imQ && (0 != imQ->mapTime2Ct( imNext, startT )) )
            return false;

        if( niQ && imQ )
            imNext = imS.TAbs2Ct( syncDstTAbs( niNext, &niS, &imS, p ) );

        if( imQ ) {
            alignX12( imNext, niNext );
            imCnt.nextCt = imNext;
        }

        niCnt.nextCt = niNext;
    }

    return true;
}


// Next file @ X12 boundary
//
void TrigTimed::alignNextFiles()
{
    quint64 niNext = niCnt.hNext();

    if( imQ ) {

        quint64 imNext = imCnt.hNext();

        alignX12( imNext, niNext );
        imCnt.nextCt = imNext;
    }

    niCnt.nextCt = niNext;
}


// Return true if no errors.
//
bool TrigTimed::bothDoSomeH( double gHiT )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        imCnt.hiCtCur   = 0;
        niCnt.hiCtCur   = 0;

        if( !newTrig( ig, it ) )
            return false;
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFirstFiles( gHiT ) )
        return true;    // too early

// ----------------------
// Fetch from all streams
// ----------------------

    return eachDoSomeH( DstImec, imQ, imCnt )
            && eachDoSomeH( DstNidq, niQ, niCnt );
}


// Return true if no errors.
//
bool TrigTimed::eachDoSomeH( DstStream dst, const AIQ *aiQ, Counts &C )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    uint                        remCt = C.hiCtMax - C.hiCtCur;

    if( !aiQ->getNScansFromCt(
            vB,
            C.nextCt,
            (remCt <= C.maxFetch ? remCt : C.maxFetch) ) ) {

        return false;
    }

    if( !vB.size() )
        return true;

// ---------------
// Update counting
// ---------------

    C.nextCt   = aiQ->nextCt( vB );
    C.hiCtCur += C.nextCt - vB[0].headCt;

// -----
// Write
// -----

    return writeAndInvalVB( dst, vB );
}


