#ifndef TRIGTCP_H
#define TRIGTCP_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTCP : public TrigBase
{
private:
    double          trigHiT,
                    trigLoT;
    volatile bool   trigHi;

public:
    TrigTCP(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ )
    : TrigBase( p, gw, imQ, niQ ), trigHiT(-1), trigHi(false)   {}

    void rgtSetTrig( bool hi );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    bool isTrigHi() const       {QMutexLocker ml( &runMtx ); return trigHi;}
    double getTrigHiT() const   {QMutexLocker ml( &runMtx ); return trigHiT;}
    double getTrigLoT() const   {QMutexLocker ml( &runMtx ); return trigLoT;}

    bool bothWriteSome( quint64 &imNextCt, quint64 &niNextCt );

    bool eachWriteSome(
        DstStream   dst,
        const AIQ   *aiQ,
        quint64     &nextCt );

    bool bothFinalWrite( quint64 &imNextCt, quint64 &niNextCt );

    bool eachWriteRem(
        DstStream   dst,
        const AIQ   *aiQ,
        quint64     &nextCt,
        double      tlo );
};

#endif  // TRIGTCP_H


