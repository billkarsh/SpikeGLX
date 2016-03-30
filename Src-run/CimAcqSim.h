#ifndef CIMACQSIM_H
#define CIMACQSIM_H

#include "CimAcq.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Simulated IMEC input
//
class CimAcqSim : public CimAcq
{
public:
    CimAcqSim( IMReaderWorker *owner, const Params &p )
    : CimAcq( owner, p ) {}

    virtual void run();
    virtual bool pause( bool pause, bool )  {setPause( pause );return true;}
};

#endif  // CIMACQSIM_H


