
#include "GraphFetcher.h"
#include "Util.h"
#include "AIQ.h"
#include "SVGrafsM.h"

#include <QThread>


#define PERIOD_SECS 0.1


/* ---------------------------------------------------------------- */
/* GFWorker ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GFWorker::setStreams( const QVector<GFStream> &gfs )
{
    QMutexLocker    ml( &gfsMtx );

    this->gfs = gfs;

    for( int is = 0, ns = gfs.size(); is < ns; ++is ) {

        GFStream    &G = this->gfs[is];

        G.setCts = PERIOD_SECS * G.aiQ->sRate();
        G.nextCt = 0;
    }
}


void GFWorker::run()
{
    Debug() << "Graph fetching started.";

    const int   loopPeriod_us = 1e6 * PERIOD_SECS;

    while( !isStopped() ) {

        double  loopT = getTime();

        if( !isPaused() ) {

            gfsMtx.lock();

            for( int is = 0, ns = gfs.size(); is < ns; ++is )
                fetch( gfs[is] );

            gfsMtx.unlock();
        }

        // Fetch no more often than every loopPeriod_us

        loopT = 1e6*(getTime() - loopT);    // microsec

        if( loopT < loopPeriod_us )
            QThread::usleep( loopPeriod_us - loopT );
        else
            QThread::usleep( 1000 * 10 );
    }

    Debug() << "Graph fetching stopped.";

    emit finished();
}


void GFWorker::fetch( GFStream &S )
{
    quint64 endCt = S.aiQ->endCount();

// Just wait if fetching too soon

    if( S.nextCt && S.nextCt >= endCt )
        return;

// Reset the count if not set or lagging 0.50 secs

    if( !S.nextCt || S.nextCt < endCt - 0.50 * S.aiQ->sRate() ) {

        if( endCt > S.setCts )
            S.nextCt = endCt - S.setCts;
        else {
            S.nextCt = 0;
            return;
        }
    }

// Fetch from last count

    vec_i16 data;

    try {
        data.reserve( 1.05 * S.aiQ->nChans() * S.setCts );
    }
    catch( const std::exception& ) {

        Warning()
            << "GraphFetcher low mem; dropped "
            << S.stream
            << " scans.";
    }

    if( 1 != S.aiQ->getAllScansFromCt( data, S.nextCt ) ) {

        Warning()
            << "GraphFetcher low mem; dropped "
            << S.stream
            << " scans.";
    }

    S.W->putScans( data, S.nextCt );

// putScans() is allowed to resize the data block to make
// downsampling smoother. The result of that tells us where
// to fetch the next contiguous block.

    S.nextCt += data.size() / S.aiQ->nChans();
}

/* ---------------------------------------------------------------- */
/* GraphFetcher --------------------------------------------------- */
/* ---------------------------------------------------------------- */

GraphFetcher::GraphFetcher()
{
    thread  = new QThread;
    worker  = new GFWorker;

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


GraphFetcher::~GraphFetcher()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}


