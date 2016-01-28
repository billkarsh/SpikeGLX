
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

    quint64 nextCt  = 0;
    int     ig      = -1,
            it      = -1;

    while( !isStopped() ) {

        double  loopT = getTime();

        // --------------
        // If trigger OFF
        // --------------

        if( isPaused() || !isGateHi() ) {

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
bool TrigImmed::writeSome( int &ig, int &it, quint64 &nextCt )
{
    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

// -----
// Fetch
// -----

    if( !df ) {

        // Starting new file
        // Get all since gate opened

        nb = niQ->getAllScansFromT( vB, getGateHiT() );

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


