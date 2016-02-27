
#include "GateImmed.h"




// Immed mode sets gate high once.
// Sleep until run ended externally.
//
void GateImmed::run()
{
    if( !baseStartReaders() )
        goto done;

    baseSetGate( true );
    baseSleep();

done:
    emit finished();
}


