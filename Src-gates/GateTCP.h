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
        const DAQ::Params   &p,
        IMReader            *im,
        NIReader            *ni,
        TrigBase            *trg  )
    :   GateBase( p, im, ni, trg )  {}

    void rgtSetGate( bool hi );

public slots:
    virtual void run();
};

#endif  // GATETCP_H


