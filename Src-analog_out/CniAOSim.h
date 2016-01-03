#ifndef CNIAOSIM_H
#define CNIAOSIM_H

#include "CniAO.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Simulated analog output
//
class CniAOSim : public CniAO
{
public:
    CniAOSim( AOCtl *owner, const DAQ::Params &p )
    : CniAO( owner, p )   {}

    void clearDerived()                 {}

    bool doAutoStart()                  {return false;}
    bool readyForScans()                {return false;}
    bool devStart()                     {return false;}
    void devStop()                      {}
    void putScans( vec_i16 & )          {}
};

#endif  // CNIAOSIM_H


