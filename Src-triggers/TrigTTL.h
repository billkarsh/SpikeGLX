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
        const double    srate;
        const qint64    hiCtMax,
                        marginCt,
                        refracCt;
        const int       maxFetch;
        quint64         edgeCt,
                        fallCt,
                        nextCt;
        qint64          remCt;
        bool            enabled;

        Counts( const DAQ::Params &p, double srate, bool enabled )
        :   srate(srate),
            hiCtMax(
                p.trgTTL.mode == DAQ::TrgTTLTimed ?
                p.trgTTL.tH * srate
                : std::numeric_limits<qlonglong>::max()),
            marginCt(p.trgTTL.marginSecs * srate),
            refracCt(p.trgTTL.refractSecs * srate),
            maxFetch(0.110 * srate),
            edgeCt(0), fallCt(0),
            nextCt(0), remCt(0), enabled(enabled)   {}

        void setPreMarg()
            {
                // Set from and rem for universal premargin.
                if( enabled ) {
                    remCt   = marginCt;
                    nextCt  = edgeCt - remCt;
                }
                else
                    remCt = 0;
            }
        void setH( DAQ::TrgTTLMode mode )
            {
                // Set from, rem and fall (if applicable) for H phase.
                if( enabled ) {

                    nextCt = edgeCt;

                    if( mode == DAQ::TrgTTLLatch ) {

                        remCt = hiCtMax;
                        // fallCt N.A.
                    }
                    else if( mode == DAQ::TrgTTLTimed ) {

                        remCt   = hiCtMax;
                        fallCt  = edgeCt + remCt;
                    }
                    else {

                        // remCt must be set within H-writer which seeks
                        // and sets true fallCt. Here we must zero fallCt.

                        fallCt = 0;
                    }
                }
                else {
                    fallCt  = 0;
                    remCt   = 0;
                }
            }
        void setPostMarg()
            {
                // Set from and rem for postmargin;
                // not applicable in latched mode.
                if( enabled ) {
                    remCt   = marginCt;
                    nextCt  = fallCt + marginCt - remCt;
                }
                else
                    remCt = 0;
            }
        void advanceByTime()
            {
                nextCt = edgeCt + qMax( hiCtMax, refracCt );
            }
        void advancePastFall()
            {
                nextCt = qMax( edgeCt + refracCt, fallCt + 1 );
            }
        double L_progress()
            {
                return (nextCt - edgeCt) / srate;
            }
        double H_progress()
            {
                return (marginCt + nextCt - edgeCt) / srate;
            }
    };

private:
    Counts          imCnt,
                    niCnt;
    const qint64    highsMax;
    quint64         aEdgeCtNext,
                    aFallCtNext;
    const int       thresh,
                    digChan;
    int             nHighs,
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
    void SETSTATE_PreMarg();
    void SETSTATE_H();
    void SETSTATE_PostMarg();
    void SETSTATE_Done();
    void initState();

    bool getRiseEdge(
        Counts              &cA,
        const SyncStream    &sA,
        Counts              &cB,
        const SyncStream    &sB );

    void getFallEdge(
        Counts              &cA,
        const SyncStream    &sA,
        Counts              &cB,
        const SyncStream    &sB );

    bool writePreMargin( DstStream dst, Counts &C, const AIQ *aiQ );
    bool writePostMargin( DstStream dst, Counts &C, const AIQ *aiQ );
    bool doSomeH( DstStream dst, Counts &C, const AIQ *aiQ );

    void statusProcess( QString &sT, bool inactive );
};

#endif  // TRIGTTL_H


