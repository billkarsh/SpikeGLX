#ifndef CNIACQ_H
#define CNIACQ_H

#include "NIReader.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for NI-DAQ data acquisition
//
class CniAcq : public QObject
{
    Q_OBJECT

protected:
    NIReaderWorker          *owner;
    const DAQ::Params       &p;
    quint64                 totPts;
    mutable QMutex          runMtx;
    mutable QWaitCondition  condRun;
    volatile bool           _canSleep,
                            ready,
                            pleaseStop;

public:
    CniAcq( NIReaderWorker *owner, const DAQ::Params &p )
    :   QObject(0), owner(owner), p(p),
        totPts(0ULL), _canSleep(true),
        ready(false), pleaseStop(false) {}
    virtual ~CniAcq()                   {}

    virtual void run() = 0;

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

#endif  // CNIACQ_H


