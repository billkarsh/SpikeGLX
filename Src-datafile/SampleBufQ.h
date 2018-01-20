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
        SampleBuf( vec_i16 &src )   {data.swap( src );}
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

    void enqueue( vec_i16 &src );
    bool dequeue( vec_i16 &dst, bool wait = false );
    bool waitForEmpty( int ms = -1 );

protected:
    virtual void overflowWarning();
};

#endif  // SAMPLEBUFQ_H


