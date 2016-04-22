#ifndef GATETCP_H
#define GATETCP_H

#include "GateBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateTCP : public GateBase
{
public:
    GateTCP( IMReader *im, NIReader *ni, TrigBase *trg )
    : GateBase( im, ni, trg )   {}

    void rgtSetGate( bool hi );

public slots:
    virtual void run();
};

#endif  // GATETCP_H


