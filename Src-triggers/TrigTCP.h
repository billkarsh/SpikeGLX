#ifndef TRIGTCP_H
#define TRIGTCP_H

#include "TrigBase.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigTCP : public TrigBase
{
private:
    double          _trigHiT,
                    _trigLoT;
    volatile bool   _trigHi;

public:
    TrigTCP(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const AIQ           *imQ,
        const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ), _trigHiT(-1), _trigHi(false) {}

    void rgtSetTrig( bool hi );

public slots:
    virtual void run();

private:
    bool isTrigHi() const       {QMutexLocker ml( &runMtx ); return _trigHi;}
    double getTrigHiT() const   {QMutexLocker ml( &runMtx ); return _trigHiT;}
    double getTrigLoT() const   {QMutexLocker ml( &runMtx ); return _trigLoT;}

    bool alignFiles( quint64 &imNextCt, quint64 &niNextCt );

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


