#ifndef GATEIMMED_H
#define GATEIMMED_H

#include "GateBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateImmed : public GateBase
{
public:
    GateImmed( TrigBase *trg, GraphsWindow *gw )
    : GateBase( trg, gw )    {}

public slots:
    virtual void run();
};

#endif  // GATEIMMED_H


