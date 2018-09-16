#ifndef AIQ_H
#define AIQ_H

/* ---------------------------------------------------------------- */
/* Includes ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "SGLTypes.h"

#include <QMutex>
#include <deque>

// BK: FIFO Notes.
// Can switch between std::list <-> std::deque.
// Makes no difference in performance.

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

        AIQBlock( const vec_i16 &src, int len, quint64 headCt );

        AIQBlock(
            const vec_i16   &src,
            int             offset,
            int             len,
            quint64         headCt );
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
    const int               nchans;
    std::deque<AIQBlock>    Q;
    mutable QMutex          QMtx;
    double                  tzero;
    quint64                 curCts,
                            endCt;

/* ------- */
/* Methods */
/* ------- */

public:
    AIQ( double srate, int nchans, int capacitySecs );

    double sRate() const        {return srate;}
    double chanRate() const     {return nchans * srate;}
    int nChans() const          {return nchans;}

    void setTZero( double t0 )  {tzero = t0;}
    double tZero() const        {return tzero;}

    bool enqueue( const vec_i16 &src, quint64 headCt, int nWhole );

    bool enqueueSim(
        double          &tLock,
        double          &tWork,
        const vec_i16   &src,
        quint64         headCt,
        int             nWhole );

    quint64 qHeadCt() const;
    quint64 endCount() const;
    int mapTime2Ct( quint64 &ct, double t ) const;
    int mapCt2Time( double &t, quint64 ct ) const;

    bool catBlocks(
        vec_i16*                &dst,
        vec_i16                 &cat,
        std::vector<AIQBlock>   &vB ) const;

    quint64 sumCt( std::vector<AIQBlock> &vB ) const;
    quint64 nextCt( std::vector<AIQBlock> &vB ) const;
    quint64 nextCt( vec_i16 *data, std::vector<AIQBlock> &vB ) const;

    int getAllScansFromT(
        std::vector<AIQBlock>   &dest,
        double                  fromT ) const;

    int getAllScansFromCt(
        std::vector<AIQBlock>   &dest,
        quint64                 fromCt ) const;

    int getAllScansFromCt(
        vec_i16                 &dest,
        quint64                 fromCt ) const;

    int getNScansFromT(
        std::vector<AIQBlock>   &dest,
        double                  fromT,
        int                     nMax ) const;

    int getNScansFromCt(
        std::vector<AIQBlock>   &dest,
        quint64                 fromCt,
        int                     nMax ) const;

    int getNScansFromCt(
        vec_i16                 &dest,
        quint64                 fromCt,
        int                     nMax ) const;

    qint64 getNScansFromCtMono(
        qint16                  *dst,
        quint64                 fromCt,
        int                     nScans,
        int                     chan ) const;

    qint64 getNScansFromCtStereo(
        qint16                  *dst,
        quint64                 fromCt,
        int                     nScans,
        int                     chan1,
        int                     chan2 ) const;

    bool getNewestNScans(
        std::vector<AIQBlock>   &dest,
        int                     nMax ) const;

    qint64 getNewestNScansMono(
        qint16                  *dst,
        int                     nScans,
        int                     chan ) const;

    qint64 getNewestNScansStereo(
        qint16                  *dst,
        int                     nScans,
        int                     chan1,
        int                     chan2 ) const;

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

    bool findBitRisingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        int                     bit,
        int                     inarow ) const;

    bool findFallingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        qint16                  T,
        int                     inarow ) const;

    bool findFltFallingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        qint16                  T,
        int                     inarow,
        T_AIQBlockFilter        &usrFlt ) const;

    bool findBitFallingEdge(
        quint64                 &outCt,
        quint64                 fromCt,
        int                     chan,
        int                     bit,
        int                     inarow ) const;

private:
    void updateQCts( int nWhole );
};

#endif  // AIQ_H


