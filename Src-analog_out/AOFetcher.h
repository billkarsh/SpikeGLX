#ifndef AOFETCHER_H
#define AOFETCHER_H

#include <QObject>
#include <QMutex>

class AOCtl;
class AIQ;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AOWorker : public QObject
{
    Q_OBJECT

private:
    AOCtl           *aoC;
    const AIQ       *niQ;
    mutable QMutex  runMtx;
    volatile bool   paused,
                    pleaseStop;

public:
    AOWorker( AOCtl *aoC, const AIQ *niQ )
    :   QObject(0), aoC(aoC), niQ(niQ),
        paused(false), pleaseStop(false)    {}
    virtual ~AOWorker()                     {}

    void pause( bool pause ) {QMutexLocker ml( &runMtx ); paused = pause;}
    void stop()         {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isPaused()     {QMutexLocker ml( &runMtx ); return paused;}
    bool isStopped()    {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();
};


class AOFetcher
{
private:
    QThread     *thread;
    AOWorker    *worker;

public:
    AOFetcher( AOCtl *aoC, const AIQ *niQ );
    virtual ~AOFetcher();
};

#endif  // AOFETCHER_H


