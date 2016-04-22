#ifndef GATEBASE_H
#define GATEBASE_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

namespace DAQ {
struct Params;
}

class IMReader;
class NIReader;
class TrigBase;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateBase : public QObject
{
    Q_OBJECT

protected:
    IMReader                *im;
    NIReader                *ni;
    TrigBase                *trg;
    mutable QMutex          runMtx;
    mutable QWaitCondition  condWake;
    volatile bool           _canSleep,
                            pleaseStop;

public:
    GateBase( IMReader *im, NIReader *ni, TrigBase *trg );
    virtual ~GateBase() {}

    void wake()             {condWake.wakeAll();}
    void stayAwake()        {QMutexLocker ml( &runMtx ); _canSleep = false;}
    bool canSleep() const   {QMutexLocker ml( &runMtx ); return _canSleep;}
    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void runStarted();
    void daqError( const QString &s );
    void finished();

public slots:
    virtual void run() = 0;

protected:
    bool baseStartReaders();
    void baseSleep();
};


class Gate
{
public:
    QThread     *thread;
    GateBase    *worker;

public:
    Gate(
        DAQ::Params     &p,
        IMReader        *im,
        NIReader        *ni,
        TrigBase        *trg );
    virtual ~Gate();
};

#endif  // GATEBASE_H


