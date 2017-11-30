#ifndef SYNC_H
#define SYNC_H

#include "qglobal.h"

namespace DAQ {
struct Params;
}

class AIQ;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SyncStream
{
    double          srate,
                    tZero;
    mutable double  tAbs;   // output
    const AIQ       *Q;
    int             ip,
                    chan,
                    bit,    // -1=analog
                    thresh;
    mutable bool    bySync; // output

    SyncStream() : Q(0), ip(-2) {}
    void init( const AIQ *Q, int ip, const DAQ::Params &p );

    double Ct2TRel( quint64 ct ) const      {return ct/srate;}
    double Ct2TAbs( quint64 ct ) const      {return tZero + ct/srate;}
    quint64 TRel2Ct( double tRel ) const    {return tRel*srate;}
    quint64 TAbs2Ct( double tAbs ) const    {return (tAbs - tZero)*srate;}

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
    quint64                     srcCt,
    int                         iSrc,
    const QVector<SyncStream>   &vS,
    const DAQ::Params           &p );

#endif  // SYNC_H


