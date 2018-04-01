
#include "GateImmed.h"
#include "TrigBase.h"




// Immed mode sets gate high once.
// Sleep until run ended externally.
//
void GateImmed::run()
{
    if( baseStartReaders() ) {

        trg->setGate( true );
        baseSleep();
    }

    emit finished();
}


