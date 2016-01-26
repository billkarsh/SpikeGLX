#ifndef TRIGSPIKE_H
#define TRIGSPIKE_H

#include "TrigBase.h"

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigSpike : public TrigBase
{
private:
    struct HiPassFnctr : public AIQ::T_AIQBlockFilter {
        Biquad  *flt;
        int     nchans,
                ichan,
                nzero;
        HiPassFnctr( const DAQ::Params &p );
        virtual ~HiPassFnctr();
        void reset();
        void operator()( vec_i16 &data );
    };

private:
    HiPassFnctr     *usrFlt;
    const qint64    nCycMax;
    const quint64   periEvtCt,
                    refracCt,
                    latencyCt;
    quint64         edgeCt;
    int             nS,
                    state;

public:
    TrigSpike( DAQ::Params &p, GraphsWindow *gw, const AIQ *niQ );
    virtual ~TrigSpike()    {delete usrFlt;}

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void initState();
    bool getEdge();
    bool writeSome( quint64 &nextCt, qint64 &remCt );
};

#endif  // TRIGSPIKE_H


