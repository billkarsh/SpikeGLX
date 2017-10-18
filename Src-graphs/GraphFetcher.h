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
    struct Stream {
        const AIQ   *aiQ;
        quint64     nextCt;
        Stream( const AIQ *aiQ ) : aiQ(aiQ), nextCt(0) {}
    };

private:
    GraphsWindow    *gw;
    Stream          imS,
                    niS;
    mutable QMutex  runMtx;
    volatile bool   hardPaused, // Pause button
                    softPaused, // Window state
                    pleaseStop;

public:
    GFWorker( GraphsWindow *gw, const AIQ *imQ, const AIQ *niQ )
    :   QObject(0), gw(gw),
        imS(imQ), niS(niQ),
        hardPaused(false), softPaused(false),
        pleaseStop(false)                   {}
    virtual ~GFWorker()                     {}

    void hardPause( bool pause )
        {QMutexLocker ml( &runMtx ); hardPaused = pause;}
    void softPause( bool pause )
        {QMutexLocker ml( &runMtx ); softPaused = pause;}
    bool isPaused() const
        {QMutexLocker ml( &runMtx ); return hardPaused || softPaused;}

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();

private:
    void fetch( Stream &S, double loopT, double oldestSecs );
};


class GraphFetcher
{
private:
    QThread     *thread;
    GFWorker    *worker;

public:
    GraphFetcher(
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ );
    virtual ~GraphFetcher();

    void hardPause( bool pause )    {worker->hardPause( pause );}
    void softPause( bool pause )    {worker->softPause( pause );}
};

#endif  // GRAPHFETCHER_H


