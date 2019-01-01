
#include "TrigTCP.h"
#include "Util.h"
#include "DataFile.h"

#define LOOP_MS     100




void TrigTCP::rgtSetTrig( bool hi )
{
    runMtx.lock();

    if( hi ) {

        if( _trigHi )
            Error() << "SetTrig(HI) twice in a row...ignoring second.";
        else
            _trigHiT = nowCalibrated();
    }
    else {
        _trigLoT = nowCalibrated();

        if( !_trigHi )
            Error() << "SetTrig(LO) twice in a row.";
    }

    _trigHi = hi;

    runMtx.unlock();
}


void TrigTCP::setGate( bool hi )
{
    runMtx.lock();
    baseSetGate( hi );
    runMtx.unlock();
}


void TrigTCP::resetGTCounters()
{
    runMtx.lock();
    baseResetGTCounters();
    runMtx.unlock();
}


// Remote mode triggering is turned on/off by remote app.
//
void TrigTCP::run()
{
    Debug() << "Trigger thread started.";

    setYieldPeriod_ms( LOOP_MS );

    QString err;
    quint64 imNextCt    = 0,
            niNextCt    = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        // ---------------
        // If finishing up
        // ---------------

        if( !isGateHi() || !isTrigHi() ) {

            if( allFilesClosed() )
                goto next_loop;

            if( !bothFinalWrite( imNextCt, niNextCt ) ) {
                err = "Generic error";
                break;
            }

            endTrig();
            goto next_loop;
        }

        // -------------
        // If trigger ON
        // -------------

        if( !bothWriteSome( imNextCt, niNextCt ) ) {
            err = "Generic error";
            break;
        }

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 1.0 ) {

            QString sOn, sWr;

            statusOnSince( sOn );
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
// Per-trigger concurrent setting of tracking data.
// mapTime2Ct may return false if the sought time mark
// isn't in the stream. It's not likely too old since
// trigger high command was just received. Rather, the
// target time might be too new, which is fixed by
// retrying on another loop iteration.
//
bool TrigTCP::alignFiles( quint64 &imNextCt, quint64 &niNextCt )
{
    if( (imQ && !imNextCt) || (niQ && !niNextCt) ) {

        double  trigT = getTrigHiT();
        quint64 imNext, niNext;

        if( niQ && (0 != niQ->mapTime2Ct( niNext, trigT )) )
            return false;

        if( imQ && (0 != imQ->mapTime2Ct( imNext, trigT )) )
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
bool TrigTCP::bothWriteSome( quint64 &imNextCt, quint64 &niNextCt )
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
bool TrigTCP::eachWriteSome(
    DstStream   dst,
    const AIQ   *aiQ,
    quint64     &nextCt )
{
    if( !aiQ )
        return true;

    vec_i16 data;
    quint64 headCt = nextCt;

    if( !nScansFromCt( aiQ, data, headCt, -LOOP_MS ) )
        return false;

    uint    size = data.size();

    if( !size )
        return true;

    nextCt += size / aiQ->nChans();

    return writeAndInvalData( dst, data, headCt );
}


// Return true if no errors.
//
bool TrigTCP::bothFinalWrite( quint64 &imNextCt, quint64 &niNextCt )
{
// Stopping due to gate or trigger going low.
// Set tlo to the shorter time span from thi.

    double  glo = getGateLoT(),
            tlo = getTrigLoT(),
            thi = getTrigHiT();

    if( glo > thi )
        glo -= thi;
    else
        glo = 48*3600;  // arb time (48 hrs) > AIQ capacity

    if( tlo > thi )
        tlo -= thi;
    else
        tlo = 48*3600;  // arb time (48 hrs) > AIQ capacity

    if( tlo > glo )
        tlo = glo;

// If our current count is short, fetch remainder.

    return eachWriteRem( DstImec, imQ, imNextCt, tlo )
            && eachWriteRem( DstNidq, niQ, niNextCt, tlo );
}


// Return true if no errors.
//
bool TrigTCP::eachWriteRem(
    DstStream   dst,
    const AIQ   *aiQ,
    quint64     &nextCt,
    double      tlo )
{
    if( !aiQ )
        return true;

    quint64 spnCt = tlo * aiQ->sRate(),
            curCt = scanCount( dst );

    if( curCt >= spnCt )
        return true;

    vec_i16 data;
    int     nMax = spnCt - curCt;

    if( !nScansFromCt( aiQ, data, nextCt, nMax ) )
        return false;

    if( !data.size() )
        return true;

    return writeAndInvalData( dst, data, nextCt );
}


