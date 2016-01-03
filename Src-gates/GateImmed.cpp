
#include "GateImmed.h"




// Immed mode sets gate high once.
// Sleep until run ended externally.
//
void GateImmed::run()
{
    baseSetGate( true );

    runMtx.lock();
    condWake.wait( &runMtx );
    runMtx.unlock();

    emit finished();
}


