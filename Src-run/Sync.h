#ifndef SYNC_H
#define SYNC_H

#include "AIQ.h"

namespace DAQ {
struct Params;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SyncStream
{
    mutable double  tAbs;   // output
    const AIQ       *Q;
    int             js,
                    ip,
                    chan,
                    bit,    // -1=analog
                    thresh;
    mutable bool    bySync; // output

    SyncStream() : Q(0), js(-1), ip(0)  {}
    virtual ~SyncStream()               {}

    void init( const AIQ *Q, int js, int ip, const DAQ::Params &p );

    double Ct2TRel( quint64 ct ) const
        {return ct / Q->sRate();}

    double Ct2TAbs( quint64 ct ) const
        {return Q->tZero() + ct / Q->sRate();}

    quint64 TRel2Ct( double tRel ) const
        {return tRel * Q->sRate();}

    quint64 TAbs2Ct( double tAbs ) const
        {return (tAbs - Q->tZero()) * Q->sRate();}

    bool findEdge(
        quint64             &outCt,
        quint64             fromCt,
        const DAQ::Params   &p ) const;
};

/* ---------------------------------------------------------------- */
/* Functions ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

double syncDstTAbs(
    quint64             srcCt,
    const SyncStream    *src,
    const SyncStream    *dst,
    const DAQ::Params   &p );

void syncDstTAbsMult(
    quint64                         srcCt,
    int                             iSrc,
    const std::vector<SyncStream>   &vS,
    const DAQ::Params               &p );

#endif  // SYNC_H


