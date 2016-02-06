
#include "AOFetcher.h"
#include "Util.h"
#include "AIQ.h"
#include "AOCtl.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* AOWorker ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AOWorker::run()
{
    bool    devStarted = aoC->devStart();

    if( !devStarted )
        Error() << "AO could not start hardware.";

    const double    maxLateS        = 0.100;
    const int       loopPeriod_us   = int(0.10 * 1e6 * aoC->bufSecs()),
                    nFetch          = int(aoC->bufSecs() * niQ->SRate());

    quint64 fromCt = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        if( !isPaused() && devStarted && aoC->readyForScans() ) {

            std::vector<AIQ::AIQBlock>  vB;
            vec_i16                     cat;
            int                         nb;

            if( !fromCt ) {

                nb = niQ->getNewestNScans( vB, nFetch );

                if( nb ) {

                    vec_i16 *data;

                    if( !niQ->catBlocks( data, cat, vB ) ) {

                        Warning() << "AOFetcher mem failure; restart.";
                        aoC->restart();
                        goto next_loop;
                    }

                    if( (int)data->size() < nFetch )
                        goto next_loop;

                    aoC->putScans( *data );

                    fromCt = niQ->nextCt( data, vB );
                }
            }
            else {

                nb = niQ->getNScansFromCt( vB, fromCt, nFetch );

                if( nb ) {

                    vec_i16 *data;

                    if( !niQ->catBlocks( data, cat, vB ) ) {

                        Warning() << "AOFetcher mem failure; restart.";
                        aoC->restart();
                        goto next_loop;
                    }

                    aoC->putScans( *data );

                    if( getTime() - vB[nb-1].tailT < maxLateS )
                        fromCt = niQ->nextCt( data, vB );
                    else if( isStopped() )
                        break;
                    else {
                        Debug() << "AOFetcher restart";
                        aoC->restart();
                    }
                }
            }
        }

next_loop:
        // Fetch no more often than every loopPeriod_us.

        loopT = 1e6*(getTime() - loopT);	// microsec

        if( loopT < loopPeriod_us )
            usleep( loopPeriod_us - loopT );
    }

    if( devStarted )
        aoC->devStop();

    emit finished();
}

/* ---------------------------------------------------------------- */
/* AOFetcher ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

AOFetcher::AOFetcher( AOCtl *aoC, const AIQ *niQ )
{
    thread  = new QThread;
    worker  = new AOWorker( aoC, niQ );

    worker->moveToThread( thread );

// Important:
// ----------
// Worker signals thread to quit() via a direct connection!!
// Reason:
// If a queued connection were specified, the QThread::quit()
// <slot> would be invoked and would execute in the GUI thread,
// that is, the thread in which ~AOFetcher() executes. That's
// not a problem per se. However, a deadlock ensues if the GUI
// thread needs to use thread->wait() for synchronization.

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


AOFetcher::~AOFetcher()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}


