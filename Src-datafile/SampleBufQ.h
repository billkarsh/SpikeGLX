#ifndef SAMPLEBUFQ_H
#define SAMPLEBUFQ_H

#include "SGLTypes.h"

#include <QMutex>
#include <QWaitCondition>
#include <deque>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SampleBufQ
{
/* ----- */
/* Types */
/* ----- */

private:
    struct SampleBuf {
        vec_i16 data;
        quint64 firstCt;

        SampleBuf( vec_i16 &src, quint64 firstCt )
        : firstCt(firstCt)
        {
            data.swap( src );
            src.clear();
        }
    };

/* ---- */
/* Data */
/* ---- */

private:
    std::deque<SampleBuf>   dataQ;
    mutable QMutex          dataQMtx;
    mutable QWaitCondition  condBufQIsEntry,
                            condBufQIsEmpty;
    const uint              maxQSize;

/* ------- */
/* Methods */
/* ------- */

public:
    SampleBufQ( int maxQSize ) : maxQSize(maxQSize) {}

    void wake()
    {
        condBufQIsEntry.wakeOne();
        condBufQIsEmpty.wakeAll();
    }

    double percentFull() const
    {
        QMutexLocker ml( &dataQMtx );
        return (100.0 * dataQ.size()) / maxQSize;
    }

    void enqueue( vec_i16 &src, quint64 firstCt );
    bool dequeue( vec_i16 &dst, quint64 &firstCt, bool wait = false );
    bool waitForEmpty( int ms = -1 );

protected:
    virtual void overflowWarning();
};

#endif  // SAMPLEBUFQ_H


