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
    mutable QMutex          runMtx;
    mutable QWaitCondition  condRun;
    volatile bool           _canSleep,
                            ready,
                            pleaseStop;

public:
    CimAcq( IMReaderWorker *owner, const DAQ::Params &p )
    :   QObject(0), owner(owner), p(p), _canSleep(true),
        ready(false), pleaseStop(false) {}
    virtual ~CimAcq()                   {}

    virtual void run() = 0;
    virtual void update( int ip ) = 0;
    virtual QString opto_getAttens( int ip, int color ) = 0;
    virtual QString opto_emit( int ip, int color, int site ) = 0;

    void atomicSleepWhenReady()
    {
        runMtx.lock();
        ready = true;
        if( _canSleep && !pleaseStop )
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


