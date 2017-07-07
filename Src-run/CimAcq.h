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
    quint64                 totalTPts;
    mutable QMutex          runMtx;
    mutable QWaitCondition  condRun;
    volatile bool           _canSleep,
                            ready,
                            paused,
                            pleaseStop;

public:
    CimAcq( IMReaderWorker *owner, const DAQ::Params &p )
    :   QObject(0), owner(owner), p(p),
        totalTPts(0ULL), _canSleep(true),
        ready(false), paused(false),
        pleaseStop(false) {}

    virtual void run() = 0;
    virtual bool pause( bool pause, bool changed ) = 0;

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
    bool canSleep() const   {QMutexLocker ml( &runMtx ); return _canSleep;}
    void setReady()         {QMutexLocker ml( &runMtx ); ready = true;}
    bool isReady() const    {QMutexLocker ml( &runMtx ); return ready;}
    void setPause( bool pause )
        {QMutexLocker ml( &runMtx ); paused = pause;}
    bool isPaused() const   {QMutexLocker ml( &runMtx ); return paused;}
    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}
};

#endif  // CIMACQ_H


