
#include "IMReader.h"
#include "Util.h"
#include "CimAcqImec.h"
#include "CimAcqSim.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* IMReaderWorker ------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMReaderWorker::IMReaderWorker( const Params &p, AIQ *imQ )
    :   QObject(0), imQ(imQ)
{
#ifdef HAVE_IMEC
    imAcq = new CimAcqImec( this, p );
#else
    imAcq = new CimAcqSim( this, p );
#endif
}


IMReaderWorker::~IMReaderWorker()
{
    delete imAcq;
}


bool IMReaderWorker::isReady() const
{
    return imAcq->isReady();
}


void IMReaderWorker::start()
{
    imAcq->wake();
}


void IMReaderWorker::stayAwake()
{
    imAcq->stayAwake();
}


void IMReaderWorker::stop()
{
    imAcq->stop();
}


void IMReaderWorker::run()
{
    imAcq->run();

    emit finished();
}

/* ---------------------------------------------------------------- */
/* IMReader ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMReader::IMReader( const Params &p, AIQ *imQ )
{
    thread  = new QThread;
    worker  = new IMReaderWorker( p, imQ );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Thread manually started by gate.
//    thread->start();
}


IMReader::~IMReader()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stayAwake();
        worker->wake();
        worker->stop();
        thread->wait();
    }

    delete thread;
}


void IMReader::configure()
{
    thread->start();
}


