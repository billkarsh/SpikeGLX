#ifndef GATETCP_H
#define GATETCP_H

#include "GateBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateTCP : public GateBase
{
public:
    GateTCP(
        IMReader        *im,
        NIReader        *ni,
        TrigBase        *trg,
        GraphsWindow    *gw )
    : GateBase( im, ni, trg, gw )   {}

    void rgtSetGate( bool hi );

public slots:
    virtual void run();
};

#endif  // GATETCP_H


