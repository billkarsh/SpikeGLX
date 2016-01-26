#ifndef CIMACQ_H
#define CIMACQ_H

#include "IMReader.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for IMEC data acquisition
//
class CimAcq : public QObject
{
    Q_OBJECT

protected:
    IMReaderWorker  *owner;
    const Params    &p;
    quint64         totalTPts;
    mutable QMutex  runMtx;
    volatile bool   pleaseStop;

public:
    CimAcq( IMReaderWorker *owner, const Params &p )
    :   QObject(0), owner(owner), p(p),
        totalTPts(0ULL), pleaseStop(false)  {}

    virtual void run() = 0;

    void stop()         {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped()    {QMutexLocker ml( &runMtx ); return pleaseStop;}
};

#endif  // CIMACQ_H


