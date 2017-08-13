#ifndef TRIGTTL_H
#define TRIGTTL_H

#include "TrigBase.h"

#include <limits>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTTL : public TrigBase
{
private:
    struct Counts {
        const qint64    hiCtMax,
                        marginCt,
                        refracCt;
        const int       maxFetch;
        quint64         edgeCt,
                        fallCt,
                        nextCt;
        qint64          remCt;

        Counts( const DAQ::Params &p, double srate )
        :   hiCtMax(
                p.trgTTL.mode == DAQ::TrgTTLTimed ?
                p.trgTTL.tH * srate
                : std::numeric_limits<qlonglong>::max()),
            marginCt(p.trgTTL.marginSecs * srate),
            refracCt(p.trgTTL.refractSecs * srate),
            maxFetch(0.110 * srate),
            edgeCt(0), fallCt(0), nextCt(0), remCt(0)   {}
        void advanceByTime()
            {
                nextCt = edgeCt + std::max( hiCtMax, refracCt );
            }
        void advancePastFall()
            {
                nextCt = std::max( edgeCt + refracCt, fallCt + 1 );
            }
    };

private:
    Counts          imCnt,
                    niCnt;
    const qint64    nCycMax;
    quint64         aEdgeCtNext,
                    aFallCtNext;
    const int       thresh,
                    digChan;
    int             nH,
                    state;

public:
    TrigTTL(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_L();
    void SETSTATE_H();
    void SETSTATE_Done();
    void initState();

    bool getRiseEdge(
        Counts      &cA,
        const AIQ   *qA,
        Counts      &cB,
        const AIQ   *qB );

    void getFallEdge(
        Counts      &cA,
        const AIQ   *qA,
        Counts      &cB,
        const AIQ   *qB );

    bool writePreMargin( DstStream dst, Counts &C, const AIQ *aiQ );
    bool writePostMargin( DstStream dst, Counts &C, const AIQ *aiQ );
    bool doSomeH( DstStream dst, Counts &C, const AIQ *aiQ );

    void statusProcess( QString &sT, bool inactive );
};

#endif  // TRIGTTL_H


