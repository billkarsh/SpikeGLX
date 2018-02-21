#ifndef TRIGSPIKE_H
#define TRIGSPIKE_H

#include "TrigBase.h"

#include <limits>

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
                maxInt,
                nzero;
        HiPassFnctr( const DAQ::Params &p );
        virtual ~HiPassFnctr();
        void reset();
        void operator()( vec_i16 &data );
    };

    struct Counts {
        const quint64   periEvtCt,
                        refracCt,
                        latencyCt;
        quint64         edgeCt,
                        nextCt;
        qint64          remCt;

        Counts( const DAQ::Params &p, double srate )
        :   periEvtCt(p.trgSpike.periEvtSecs * srate),
            refracCt(qMax( p.trgSpike.refractSecs * srate, 5.0 )),
            latencyCt(0.25 * srate),
            edgeCt(0), nextCt(0), remCt(0)  {}

        void setGateEdge( const SyncStream &S, double gateT )
            {
                edgeCt = qMax(
                    S.TAbs2Ct( gateT ),
                    S.Q->qHeadCt() + periEvtCt + latencyCt );
            }
        void setupWrite( bool enabled )
            {
                nextCt  = edgeCt - periEvtCt;
                remCt   = (enabled ? 2 * periEvtCt + 1 : 0);
            }
        void advanceEdge()
            {
                edgeCt += refracCt;
            }
    };

private:
    HiPassFnctr     *usrFlt;
    Counts          imCnt,
                    niCnt;
    const qint64    spikesMax;
    quint64         aEdgeCtNext;
    const int       thresh;
    int             nSpikes,
                    state;

public:
    TrigSpike(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ );
    virtual ~TrigSpike()    {delete usrFlt;}

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_GetEdge();
    void SETSTATE_Write();
    void SETSTATE_Done();
    void initState();

    bool getEdge(
        Counts              &cA,
        const SyncStream    &sA,
        Counts              &cB,
        const SyncStream    &sB );

    bool writeSome(
        DstStream   dst,
        const AIQ   *aiQ,
        Counts      &cnt );
};

#endif  // TRIGSPIKE_H


