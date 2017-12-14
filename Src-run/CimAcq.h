#ifndef CIMACQ_H
#define CIMACQ_H

#include "IMReader.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for IMEC data acquisition
//
class CimAcq : public QObject
{
    Q_OBJECT

protected:
    IMReaderWorker          *owner;
    const DAQ::Params       &p;
    quint64                 totPts;
    mutable QMutex          runMtx;
    mutable QWaitCondition  condRun;
    volatile bool           _canSleep,
                            ready,
                            pleaseStop;

public:
    CimAcq( IMReaderWorker *owner, const DAQ::Params &p )
    :   QObject(0), owner(owner), p(p),
        totPts(0ULL), _canSleep(true),
        ready(false), pleaseStop(false) {}

    virtual void run() = 0;
    virtual void update() = 0;

    void atomicSleepWhenReady()
    {
        runMtx.lock();
        ready = true;
        if( _canSleep )
            condRun.wait( &runMtx );
        runMtx.unlock();
    }

    void wake()             {condRun.wakeAll();}
    void stayAwake()        {QMutexLocker ml( &runMtx ); _canSleep = false;}
    bool isReady() const    {QMutexLocker ml( &runMtx ); return ready;}
    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}
};

#endif  // CIMACQ_H


