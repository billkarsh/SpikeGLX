
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

    quint64 imNextCt    = 0,
            niNextCt    = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        // -------
        // Active?
        // -------

        if( isPaused() || !isGateHi() ) {

            endTrig();
            goto next_loop;
        }

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
bool TrigImmed::bothWriteSome( quint64 &imNextCt, quint64 &niNextCt )
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
bool TrigImmed::eachWriteSome(
    DataFile    *df,
    const AIQ   *aiQ,
    quint64     &nextCt )
{
    if( !aiQ )
        return true;

    std::vector<AIQ::AIQBlock>  vB;
    int                         nb;

    if( !nextCt )
        nb = aiQ->getAllScansFromT( vB, getGateHiT() );
    else
        nb = aiQ->getAllScansFromCt( vB, nextCt );

    if( !nb )
        return true;

    nextCt = aiQ->nextCt( vB );

    return writeAndInvalVB( df, vB );
}


