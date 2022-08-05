#ifndef CNIACQSIM_H
#define CNIACQSIM_H

#include "CniAcq.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Simulated NI-DAQ input
//
class CniAcqSim : public CniAcq
{
public:
    CniAcqSim( NIReaderWorker *owner, const DAQ::Params &p )
    :   CniAcq(owner, p)    {}

    virtual void run();
};

#endif  // CNIACQSIM_H


