#ifndef TRIGTIMED_H
#define TRIGTIMED_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTimed : public TrigBase
{
private:
    const qint64    nCycMax;
    const quint64   hiCtMax;
    const qint64    loCt;
    const uint      maxFetch;
    int             nH,
                    state;

public:
    TrigTimed( DAQ::Params &p, GraphsWindow *gw, const AIQ *aiQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void initState();
    double remainingL0( double loopT, double gHiT );
    double remainingL( quint64 &nextCt );

    bool doSomeH(
        int     &ig,
        int     &it,
        double  gHiT,
        quint64 &hiCtCur,
        quint64 &nextCt );
};

#endif  // TRIGTIMED_H


