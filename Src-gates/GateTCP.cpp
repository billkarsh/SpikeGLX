
#include "GateTCP.h"




// This does the work (not run()).
//
void GateTCP::rgtSetGate( bool hi )
{
    baseSetGate( hi );
}


// TCP mode doesn't need to track anything.
// Sleep until run ended externally.
//
void GateTCP::run()
{
    if( !baseStartReaders() )
        goto done;

    baseSleep();

done:
    emit finished();
}


