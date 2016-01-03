#ifndef GATEBASE_H
#define GATEBASE_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

namespace DAQ {
struct Params;
}

class TrigBase;
class GraphsWindow;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GateBase : public QObject
{
    Q_OBJECT

protected:
    TrigBase                *trg;
    GraphsWindow            *gw;
    mutable QMutex          runMtx;
    mutable QWaitCondition  condWake;
    volatile bool           pleaseStop,
                            _canSleep;
public:
    GateBase( TrigBase *trg, GraphsWindow *gw );
    virtual ~GateBase() {}

    void stop()         {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped()    {QMutexLocker ml( &runMtx ); return pleaseStop;}

    void wake()         {condWake.wakeAll();}
    void stayAwake()    {QMutexLocker ml( &runMtx ); _canSleep = false;}
    bool canSleep()     {QMutexLocker ml( &runMtx ); return _canSleep;}

signals:
    void finished();

public slots:
    virtual void run() = 0;

protected:
    void baseSetGate( bool hi );
};


class Gate
{
public:
    QThread     *thread;
    GateBase    *worker;

public:
    Gate( DAQ::Params &p, TrigBase *trg, GraphsWindow *gw );
    virtual ~Gate();
};

#endif  // GATEBASE_H


