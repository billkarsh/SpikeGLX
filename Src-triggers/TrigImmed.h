#ifndef TRIGIMMED_H
#define TRIGIMMED_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigImmed : public TrigBase
{
public:
    TrigImmed( DAQ::Params &p, GraphsWindow *gw, const AIQ *aiQ )
    : TrigBase( p, gw, aiQ ) {}

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    bool writeSome( int &ig, int &it, quint64 &nextCt );
};

#endif  // TRIGIMMED_H


