
#include "GraphFetcher.h"
#include "Util.h"
#include "AIQ.h"
#include "GraphsWindow.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* GFWorker ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GFWorker::run()
{
    Debug() << "Graph fetching started.";

    const double    oldestSecs      = 0.1;
    const int       loopPeriod_us   = 1000 * 100;

    while( !isStopped() ) {

        double  loopT = getTime();

        if( !isPaused() && !gw->isHidden() ) {

            fetch( imS, loopT, oldestSecs );
            fetch( niS, loopT, oldestSecs );
        }

        // Fetch no more often than every loopPeriod_us.

        loopT = 1e6*(getTime() - loopT);    // microsec

        if( loopT < loopPeriod_us )
            usleep( loopPeriod_us - loopT );
        else
            usleep( 1000 * 10 );
    }

    Debug() << "Graph fetching stopped.";

    emit finished();
}


void GFWorker::fetch( Stream &S, double loopT, double oldestSecs )
{
    if( !S.aiQ )
        return;

    std::vector<AIQ::AIQBlock>  vB;
    double                      testT;
    int                         nb;

// mapCt2Time fails if nextCt >= curCount

    if( S.nextCt && S.nextCt >= S.aiQ->curCount() )
        return;

// Reset the count if not set or lagging 1.0 secs.
// 1.0s * (30000samp/s) / (100samp/block) = 300 blocks.

    if( !S.nextCt
        || !S.aiQ->mapCt2Time( testT, S.nextCt )
        || testT < loopT - 1.0 ) {

        if( !S.aiQ->mapTime2Ct( S.nextCt, loopT - oldestSecs ) )
            return;
    }

// Fetch from last count

    nb = S.aiQ->getAllScansFromCt( vB, S.nextCt );

    if( !nb )
        return;

    vec_i16 cat;
    vec_i16 *data;

    if( !S.aiQ->catBlocks( data, cat, vB ) ) {

// MS: Might want to name extended stream here
        Warning()
            << "GraphFetcher mem failure; dropped "
            << (&S == &imS ? "imec" : "nidq")
            << " scans.";
    }

    if( &S == &imS )
        gw->imPutScans( *data, vB[0].headCt );
    else
        gw->niPutScans( *data, vB[0].headCt );

// putScans() is allowed to resize the data block to make
// downsampling smoother. The result of that tells us where
// to fetch the next contiguous block.

    S.nextCt = S.aiQ->nextCt( data, vB );
}

/* ---------------------------------------------------------------- */
/* GraphFetcher --------------------------------------------------- */
/* ---------------------------------------------------------------- */

GraphFetcher::GraphFetcher(
    GraphsWindow    *gw,
    const AIQ       *imQ,
    const AIQ       *niQ )
{
    thread  = new QThread;
    worker  = new GFWorker( gw, imQ, niQ );

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


