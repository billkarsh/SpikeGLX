#ifndef CNISIM_H
#define CNISIM_H

#include "CniIn.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Simulated NI-DAQ input
//
class CniInSim : public CniIn
{
public:
    CniInSim( NIReaderWorker *owner, const Params &p )
    : CniIn( owner, p ) {}

    void run();
};

#endif  // CNISIM_H


