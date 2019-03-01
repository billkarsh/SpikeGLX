#ifndef TRIGIMMED_H
#define TRIGIMMED_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigImmed : public TrigBase
{
public:
    TrigImmed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ) {}

public slots:
    virtual void run();

private:
    bool alignFiles( quint64 &imNextCt, quint64 &niNextCt );

    bool bothWriteSome( quint64 &imNextCt, quint64 &niNextCt );

    bool eachWriteSome(
        DstStream   dst,
        const AIQ   *aiQ,
        quint64     &nextCt );
};

#endif  // TRIGIMMED_H


