
#include "GateTCP.h"
#include "TrigBase.h"




// This does the work (not run()).
//
void GateTCP::rgtSetGate( bool hi )
{
    trg->setGate( hi );
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


