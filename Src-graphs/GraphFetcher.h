#ifndef GRAPHFETCHER_H
#define GRAPHFETCHER_H

#include <QObject>
#include <QMutex>

class GraphsWindow;
class AIQ;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GFWorker : public QObject
{
    Q_OBJECT

private:
    GraphsWindow    *gw;
    const AIQ       *aiQ;
    mutable QMutex  runMtx;
    volatile bool   paused,
                    pleaseStop;

public:
    GFWorker( GraphsWindow *gw, const AIQ *aiQ )
    :   QObject(0), gw(gw), aiQ(aiQ),
        paused(false), pleaseStop(false)    {}
    virtual ~GFWorker()                     {}

    void pause( bool pause ) {QMutexLocker ml( &runMtx ); paused = pause;}
    void stop()         {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isPaused()     {QMutexLocker ml( &runMtx ); return paused;}
    bool isStopped()    {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();
};


class GraphFetcher
{
private:
    QThread     *thread;
    GFWorker    *worker;

public:
    GraphFetcher( GraphsWindow *gw, const AIQ *aiQ );
    virtual ~GraphFetcher();

    void pause( bool pause )    {worker->pause( pause );}
};

#endif  // GRAPHFETCHER_H


