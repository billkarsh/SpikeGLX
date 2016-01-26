
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

    quint64 nextCt  = 0;
    int     ig      = -1,
            it      = -1;

    while( !isStopped() && niQ ) {

        double  loopT = getTime();

        // ---------
        // If paused
        // ---------

        if( isPaused() ) {

            endTrig();
            goto next_loop;
        }

        // ---------------
        // If finishing up
        // ---------------

        if( !isGateHi() || !isTrigHi() ) {

            if( !df )
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

            quint64 spnCt = tlo * p.ni.srate,
                    curCt = df->scanCount();

            if( spnCt > curCt ) {
                if( !writeRem( nextCt, spnCt - curCt ) )
                    break;
            }

            endTrig();
            goto next_loop;
        }

        // -------------
        // If trigger ON
        // -------------

        if( !writeSome( ig, it, nextCt ) )
            break;

        // ------
        // Status
        // ------

next_loop:
       if( loopT - statusT > 0.25 ) {

            QString sOn, sWr;

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

    endTrig();

    Debug() << "Trigger thread stopped.";

    emit finished();
}


// Return true if no errors.
//
bool TrigTCP::writeSome( int &ig, int &it, quint64 &nextCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// -----
// Fetch
// -----

    if( !df ) {

        // Starting new file
        // Get all since trig hi sent

        nb = niQ->getAllScansFromT( vB, getTrigHiT() );

        if( !nb )
            return true;

        if( !newTrig( ig, it ) )
            return false;
    }
    else {

        // File in progress
        // Get all blocks since last fetch

        nb = niQ->getAllScansFromCt( vB, nextCt );

        if( !nb )
            return true;
    }

// ---------------
// Update counting
// ---------------

    nextCt = niQ->nextCt( vB );

// -----
// Write
// -----

    return writeAndInvalVB( vB );
}


// Return true if no errors.
//
bool TrigTCP::writeRem( quint64 &nextCt, int nMax )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    nb = niQ->getNScansFromCt( vB, nextCt, nMax );

    if( !nb )
        return true;

    return writeAndInvalVB( vB );
}


