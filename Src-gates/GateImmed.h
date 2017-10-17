#ifndef GATEIMMED_H
#define GATEIMMED_H

#include "GateBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateImmed : public GateBase
{
public:
    GateImmed( IMReader *im, NIReader *ni, TrigBase *trg )
    :   GateBase( im, ni, trg ) {}

public slots:
    virtual void run();
};

#endif  // GATEIMMED_H


