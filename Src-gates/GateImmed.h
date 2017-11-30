#ifndef GATEIMMED_H
#define GATEIMMED_H

#include "GateBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateImmed : public GateBase
{
public:
    GateImmed(
        const DAQ::Params   &p,
        IMReader            *im,
        NIReader            *ni,
        TrigBase            *trg  )
    :   GateBase( p, im, ni, trg )  {}

public slots:
    virtual void run();
};

#endif  // GATEIMMED_H


