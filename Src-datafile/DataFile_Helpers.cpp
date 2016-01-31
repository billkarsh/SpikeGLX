
#include "DataFile_Helpers.h"
#include "DataFile.h"
#include "Util.h"

#include <QThread>


/* ---------------------------------------------------------------- */
/* DFWriterWorker ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DFWriterWorker::run()
{
    Debug() << "DFWriter started for " << d->binFileName();

    for(;;) {

        vec_i16 buf;
        quint64 firstCt = 0;
        int     n;

        if( (n = dequeue( buf, firstCt, waitData() )) )
            write( buf );
        else if( isStopped() )
            break;
    }

    Debug() << "DFWriter stopped for " << d->binFileName();

    emit finished();
}


bool DFWriterWorker::write( const vec_i16 &scans )
{
    if( !d )
        return false;

    return d->doFileWrite( scans );
}

/* ---------------------------------------------------------------- */
/* DFWriter ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DFWriter::DFWriter( DataFile *df, int maxQSize )
{
    thread  = new QThread;
    worker  = new DFWriterWorker( df, maxQSize );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


DFWriter::~DFWriter()
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

/* ---------------------------------------------------------------- */
/* DFCloseAsyncWorker --------------------------------------------- */
/* ---------------------------------------------------------------- */

void DFCloseAsyncWorker::run()
{
    if( d->mode == DataFile::Output ) {

        d->setRemoteParams( kvm );
        d->closeAndFinalize();
        delete d;
    }

    emit finished();
}

/* ---------------------------------------------------------------- */
/* DFCloseAsync --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DFCloseAsync( DataFile *df, const KeyValMap &kvm )
{
    QThread             *thread  = new QThread;
    DFCloseAsyncWorker  *worker  = new DFCloseAsyncWorker( df, kvm );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );
    Connect( thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

    thread->start();
}


