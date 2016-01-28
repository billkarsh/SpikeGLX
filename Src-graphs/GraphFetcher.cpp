
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

    quint64 nextCt = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        if( !isPaused() && !gw->isHidden() ) {

            std::vector<AIQ::AIQBlock>  vB;
            double                      testT;
            int                         nb;

            // mapCt2Time fails if nextCt >= curCount

            if( nextCt && nextCt >= niQ->curCount() )
                goto next_loop;

            // Reset the count if not set or lagging

            if( !nextCt
                || !niQ->mapCt2Time( testT, nextCt )
                || testT < loopT - 3 * oldestSecs ) {

                if( !niQ->mapTime2Ct( nextCt, loopT - oldestSecs ) )
                    goto next_loop;
            }

            // Fetch from last count

            nb = niQ->getAllScansFromCt( vB, nextCt );

            if( !nb )
                goto next_loop;

            vec_i16 cat;
            vec_i16 &data = niQ->catBlocks( cat, vB );

            gw->putScans( data, vB[0].headCt );

            nextCt = niQ->nextCt( vB );
        }

next_loop:
        // Fetch no more often than every loopPeriod_us.

        loopT = 1e6*(getTime() - loopT);	// microsec

        if( loopT < loopPeriod_us )
            usleep( loopPeriod_us - loopT );
        else
            usleep( 1000 * 10 );
    }

    Debug() << "Graph fetching stopped.";

    emit finished();
}

/* ---------------------------------------------------------------- */
/* GraphFetcher --------------------------------------------------- */
/* ---------------------------------------------------------------- */

GraphFetcher::GraphFetcher( GraphsWindow *gw, const AIQ *niQ )
{
    thread  = new QThread;
    worker  = new GFWorker( gw, niQ );

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


