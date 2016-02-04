#ifndef AIQ_H
#define AIQ_H

/* ---------------------------------------------------------------- */
/* Includes ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "SGLTypes.h"

#include <QMutex>
#include <deque>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AIQ
{
/* ----- */
/* Types */
/* ----- */

public:
    struct AIQBlock {
        vec_i16 data;
        quint64 headCt;
        double  tailT;

        AIQBlock(
            vec_i16 &src,
            int     len,
            quint64 headCt,
            double  tailT );

        AIQBlock(
            vec_i16 &src,
            int     offset,
            int     len,
            quint64 headCt,
            double  tailT );
    };

    // callback functor
    struct T_AIQBlockFilter {
        virtual void operator()( vec_i16 &data ) = 0;
    };

/* ---- */
/* Data */
/* ---- */

private:
    const double            srate;
    const quint64           maxCts;
    const int               nChans;
    std::deque<AIQBlock>    Q;
    mutable QMutex          QMtx;
    quint64                 curCts;

/* ------- */
/* Methods */
/* ------- */

public:
    AIQ( double srate, int nChans, int capacitySecs );

    double SRate() const    {return srate;}
    int NChans() const      {return nChans;}

    void enqueue( vec_i16 &src, int nWhole, quint64 headCt );

    quint64 qHeadCt() const;
    quint64 curCount() const;
    bool mapTime2Ct( quint64 &ct, double t ) const;
    bool mapCt2Time( double &t, quint64 ct ) const;

    bool catBlocks(
        vec_i16*                &dst,
        vec_i16                 &cat,
        std::vector<AIQBlock>   &vB ) const;
    quint64 sumCt( std::vector<AIQBlock> &vB ) const;
    quint64 nextCt( std::vector<AIQBlock> &vB ) const;

    bool copy1BlockFromCt(
        vec_i16 &dest,
        quint64 &headCt,
        quint64 fromCt ) const;

    int getAllScansFromT(
        std::vector<AIQBlock>   &dest,
        double                  fromT ) const;

    int getAllScansFromCt(
        std::vector<AIQBlock>   &dest,
        quint64                 fromCt ) const;

    int getNScansFromT(
        std::vector<AIQBlock>   &dest,
        double                  fromT,
        int                     nMax ) const;

    int getNScansFromCt(
        std::vector<AIQBlock>   &dest,
        quint64                 fromCt,
        int                     nMax ) const;

    int getNewestNScans(
        std::vector<AIQBlock>   &dest,
        int                     nMax ) const;

    bool findRisingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        qint16                  T,
        int                     inarow ) const;

    bool findFltRisingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        qint16                  T,
        int                     inarow,
        T_AIQBlockFilter        &usrFlt ) const;

    bool findFallingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        qint16                  T,
        int                     inarow ) const;

private:
    void updateQCts( int nWhole );
};

#endif  // AIQ_H


