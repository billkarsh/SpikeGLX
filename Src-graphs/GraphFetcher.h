#ifndef GRAPHFETCHER_H
#define GRAPHFETCHER_H

#include <QObject>
#include <QMutex>

#include <vector>

class SVGrafsM;
class AIQ;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct GFStream {
    QString     stream;
    SVGrafsM    *W;
    AIQ         *aiQ;
    quint64     setCts,
                nextCt;

    GFStream()
        :   W(0), aiQ(0), setCts(0), nextCt(0)                  {}
    GFStream( const QString &stream, SVGrafsM *W )
        :   stream(stream), W(W), aiQ(0), setCts(0), nextCt(0)  {}
};

class GFWorker : public QObject
{
    Q_OBJECT

private:
    std::vector<GFStream>   gfs;
    mutable QMutex          gfsMtx,
                            runMtx;
    volatile bool           hardPaused, // Pause button
                            softPaused, // Window state
                            pleaseStop;

public:
    GFWorker()
    :   QObject(0),
        hardPaused(false), softPaused(false),
        pleaseStop(false)   {}

    void setStreams( const std::vector<GFStream> &gfs );

    bool hardPause( bool pause )
        {
            QMutexLocker ml( &runMtx );
            bool was = hardPaused;
            hardPaused = pause;
            return was;
        }
    void softPause( bool pause )
        {QMutexLocker ml( &runMtx ); softPaused = pause;}
    bool isPaused() const
        {QMutexLocker ml( &runMtx ); return hardPaused || softPaused;}
    void waitPaused()
        {
            QMutexLocker ml( &runMtx );
            if( hardPaused || softPaused )
                QMutexLocker ml2( &gfsMtx );
        }

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();

private:
    void fetch( GFStream &S );
};


class GraphFetcher
{
private:
    QThread     *thread;
    GFWorker    *worker;

public:
    GraphFetcher();
    virtual ~GraphFetcher();

    void setStreams( const std::vector<GFStream> &gfs )
        {worker->setStreams( gfs );}

    bool hardPause( bool pause )    {return worker->hardPause( pause );}
    void softPause( bool pause )    {worker->softPause( pause );}
    void waitPaused()               {worker->waitPaused();}
};

#endif  // GRAPHFETCHER_H


