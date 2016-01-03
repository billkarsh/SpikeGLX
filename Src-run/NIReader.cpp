
#include "NIReader.h"
#include "Util.h"
#include "CniInDmx.h"
#include "CniInSim.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* NIReaderWorker ------------------------------------------------- */
/* ---------------------------------------------------------------- */

NIReaderWorker::NIReaderWorker( const Params &p, AIQ *aiQ )
    :   QObject(0), aiQ(aiQ)
{
#ifdef HAVE_NIDAQmx
    niin = new CniInDmx( this, p );
#else
    niin = new CniInSim( this, p );
#endif
}


NIReaderWorker::~NIReaderWorker()
{
    delete niin;
}


void NIReaderWorker::stop()
{
    niin->stop();
}


void NIReaderWorker::run()
{
    niin->run();

    emit finished();
}

/* ---------------------------------------------------------------- */
/* NIReader ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

NIReader::NIReader( const Params &p, AIQ *aiQ )
{
    thread  = new QThread;
    worker  = new NIReaderWorker( p, aiQ );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Thread manually started by run manager.
//    thread->start();
}


NIReader::~NIReader()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}


void NIReader::start()
{
    thread->start();
}


