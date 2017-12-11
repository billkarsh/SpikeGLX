#ifndef TRIGTIMED_H
#define TRIGTIMED_H

#include "TrigBase.h"

#include <limits>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTimed : public TrigBase
{
private:
    struct Counts {
        const quint64   hiCtMax;
        const qint64    loCt;
        const uint      maxFetch;
        quint64         nextCt,
                        hiCtCur;
        bool            enabled;

        Counts( const DAQ::Params &p, double srate, bool enabled )
        :   hiCtMax(
                p.trgTim.isHInf ?
                std::numeric_limits<qlonglong>::max()
                : p.trgTim.tH * srate),
            loCt(p.trgTim.tL * srate),
            maxFetch(0.110 * srate),
            nextCt(0), hiCtCur(0), enabled(enabled) {}

        bool hDone()
            {
                return !enabled || hiCtCur >= hiCtMax;
            }
        quint64 hNext()
            {
                 return nextCt + loCt;
            }
    };

private:
    Counts          imCnt,
                    niCnt;
    const qint64    nCycMax;
    int             nH,
                    state;

public:
    TrigTimed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_Done();
    void initState();

    double remainingL0( double loopT, double gHiT );
    double remainingL( const AIQ *aiQ, const Counts &C );

    bool alignFirstFiles( double gHiT );
    void alignNextFiles();

    bool bothDoSomeH( double gHiT );
    bool eachDoSomeH( DstStream dst, const AIQ *aiQ, Counts &C );
};

#endif  // TRIGTIMED_H


