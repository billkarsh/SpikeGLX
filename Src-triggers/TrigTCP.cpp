
#include "TrigTCP.h"
#include "Util.h"
#include "DataFile.h"




void TrigTCP::rgtSetTrig( bool hi )
{
    runMtx.lock();

    if( hi ) {

        if( trigHi )
            Error() << "SetTrig(HI) twice in a row...ignoring second.";
        else
            trigHiT = getTime();
    }
    else {
        trigLoT = getTime();

        if( !trigHi )
            Warning() << "SetTrig(LO) twice in a row.";
    }

    trigHi = hi;

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

    setYieldPeriod_ms( 100 );

    quint64 imNextCt    = 0,
            niNextCt    = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        // -------
        // Active?
        // -------

        if( isPaused() ) {

            endTrig();
            goto next_loop;
        }

        // ---------------
        // If finishing up
        // ---------------

        if( !isGateHi() || !isTrigHi() ) {

            if( !dfim && !dfni )
                goto next_loop;

            // Stopping due to gate or trigger going low.
            // Set tlo to the shorter time span from thi.

            double  glo = getGateLoT(),
                    tlo = getTrigLoT(),
                    thi = getTrigHiT();

            if( glo > thi )
                glo -= thi;
            else
                glo = 48*3600;  // arb time > AIQ capacity

            if( tlo > thi )
                tlo -= thi;
            else
                tlo = 48*3600;  // arb time > AIQ capacity

            if( tlo > glo )
                tlo = glo;

            // If our current count is short, fetch remainder...
            // but end file in either case.

            if( !writeRem( dfim, imQ, imNextCt, tlo )
                || !writeRem( dfni, niQ, niNextCt, tlo ) ) {

                break;
            }

            endTrig();
            goto next_loop;
        }

        // -------------
        // If trigger ON
        // -------------

        if( !bothWriteSome( imNextCt, niNextCt ) )
            break;

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


// Return true if no errors.
//
bool TrigTCP::bothWriteSome( quint64 &imNextCt, quint64 &niNextCt )
{
// -------------------
// Open files together
// -------------------

    if( needNewFiles() ) {

        int ig, it;

        imNextCt = 0;
        niNextCt = 0;

        if( !newTrig( ig, it ) )
            return false;
    }

// ---------------
// Fetch from each
// ---------------

    return eachWriteSome( dfim, imQ, imNextCt )
            && eachWriteSome( dfni, niQ, niNextCt );
}


// Return true if no errors.
//
bool TrigTCP::eachWriteSome(
    DataFile    *df,
    const AIQ   *aiQ,
    quint64     &nextCt )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    if( !nextCt )
        nb = aiQ->getAllScansFromT( vB, getTrigHiT() );
    else
        nb = aiQ->getAllScansFromCt( vB, nextCt );

    if( !nb )
        return true;

    nextCt = aiQ->nextCt( vB );

    return writeAndInvalVB( df, vB );
}


// Return true if no errors.
//
bool TrigTCP::writeRem(
    DataFile    *df,
    const AIQ   *aiQ,
    quint64     &nextCt,
    double      tlo )
{
    if( !aiQ )
        return true;

    quint64 spnCt = tlo * aiQ->SRate(),
            curCt = df->scanCount();

    if( curCt >= spnCt )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = aiQ->getNScansFromCt( vB, nextCt, spnCt - curCt );

    if( !nb )
        return true;

    return writeAndInvalVB( df, vB );
}


