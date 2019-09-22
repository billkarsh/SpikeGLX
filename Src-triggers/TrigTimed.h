#ifndef TRIGTIMED_H
#define TRIGTIMED_H

#include "TrigBase.h"

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
        :   hiCtMax(p.trgTim.isHInf ? UNSET64 :
                (p.trgTim.tH - (p.trgTim.tL < 0.0 ? p.trgTim.tL : 0.0))
                * srate),
            loCt(p.trgTim.tL * srate),
            maxFetch(0.400 * srate),
            nextCt(0), hiCtCur(0), enabled(enabled) {}

        bool isReset()
            {
                return !enabled || !hiCtCur;
            }
        bool hDone()
            {
                return !enabled || hiCtCur >= hiCtMax;
            }
        void hNext()
            {
                nextCt += loCt;
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

public slots:
    virtual void run();

private:
    void SETSTATE_Done();
    void initState();

    double remainingL0( double loopT, double gHiT );
    double remainingL( const AIQ *aiQ, const Counts &C );

    bool alignFiles( double gHiT );
    void advanceNext();

    bool bothDoSomeH( double gHiT );
    bool eachDoSomeH( DstStream dst, const AIQ *aiQ, Counts &C );
};

#endif  // TRIGTIMED_H


