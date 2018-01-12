
#include "TrigImmed.h"
#include "Util.h"
#include "DataFile.h"




void TrigImmed::setGate( bool hi )
{
    runMtx.lock();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigImmed::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    runMtx.unlock();
}


// Immediate mode triggering simply follows the gate. When the
// gate is high we are saving; when the gate is low we aren't.
//
void TrigImmed::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( 100 );

    QString err;
    quint64 imNextCt    = 0,
            niNextCt    = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        // -------
        // Active?
        // -------

        if( !isGateHi() ) {

            endTrig();
            goto next_loop;
        }

        if( !bothWriteSome( imNextCt, niNextCt ) ) {
            err = "Generic error";
            break;
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

    endRun( err );
}


// Next file @ X12 boundary
//
// One-time concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. It's not likely too old since
// immediate triggering starts as soon as the gate goes
// high. Rather, the target time might be too new, which
// is fixed by retrying on another loop iteration.
//
bool TrigImmed::alignFiles( quint64 &imNextCt, quint64 &niNextCt )
{
    if( (imQ && !imNextCt) || (niQ && !niNextCt) ) {

        double  gateT = getGateHiT();
        quint64 imNext, niNext;

        if( niQ && (0 != niQ->mapTime2Ct( niNext, gateT )) )
            return false;

        if( imQ && (0 != imQ->mapTime2Ct( imNext, gateT )) )
            return false;

        if( niQ && imQ )
            imNext = imS.TAbs2Ct( syncDstTAbs( niNext, &niS, &imS, p ) );

        if( imQ ) {
            alignX12( imNext, niNext );
            imNextCt = imNext;
        }

        niNextCt = niNext;
    }

    return true;
}


// Return true if no errors.
//
bool TrigImmed::bothWriteSome( quint64 &imNextCt, quint64 &niNextCt )
{
// -------------------
// Open files together
// -------------------

    if( allFilesClosed() ) {

        int ig, it;

        // reset tracking
        imNextCt = 0;
        niNextCt = 0;

        if( !newTrig( ig, it ) )
            return false;
    }

// ---------------------
// Seek common sync time
// ---------------------

    if( !alignFiles( imNextCt, niNextCt ) )
        return true;    // too early

// ----------------------
// Fetch from all streams
// ----------------------

    return eachWriteSome( DstImec, imQ, imNextCt )
            && eachWriteSome( DstNidq, niQ, niNextCt );
}


// Return true if no errors.
//
bool TrigImmed::eachWriteSome(
    DstStream   dst,
    const AIQ   *aiQ,
    quint64     &nextCt )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;

    if( !aiQ->getAllScansFromCt( vB, nextCt ) )
        return false;

    if( !vB.size() )
        return true;

    nextCt = aiQ->nextCt( vB );

    return writeAndInvalVB( dst, vB );
}


