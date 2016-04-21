#ifndef DATAFILE_HELPERS_H
#define DATAFILE_HELPERS_H

#include "SampleBufQ.h"
#include "KVParams.h"

#include <QObject>
#include <QString>

class DataFile;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* DFWriter ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DFWriterWorker : public QObject, public SampleBufQ
{
    Q_OBJECT

private:
    DataFile        *d;
    mutable QMutex  runMtx;
    volatile bool   _waitData,
                    pleaseStop;

public:
    DFWriterWorker( DataFile *df, int maxQSize )
    :   QObject(0), SampleBufQ(maxQSize),
        d(df), _waitData(true),
        pleaseStop(false)           {}
    virtual ~DFWriterWorker()       {}

    void stayAwake()        {QMutexLocker ml( &runMtx ); _waitData = false;}
    bool waitData() const   {QMutexLocker ml( &runMtx ); return _waitData;}
    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();

private:
    bool write( const vec_i16 &scans );
};


class DFWriter
{
public:
    QThread         *thread;
    DFWriterWorker  *worker;

public:
    DFWriter( DataFile *df, int maxQSize );
    virtual ~DFWriter();
};

/* ---------------------------------------------------------------- */
/* DFCloseAsync --------------------------------------------------- */
/* ---------------------------------------------------------------- */

class DFCloseAsyncWorker : public QObject
{
    Q_OBJECT

private:
    DataFile    *d;
    KeyValMap   kvm;

public:
    DFCloseAsyncWorker( DataFile *df, const KeyValMap &kvm )
    : QObject(0), d(df), kvm(kvm)   {}

signals:
    void finished();

public slots:
    void run();
};


void DFCloseAsync( DataFile *df, const KeyValMap &kvm );

#endif // DATAFILE_HELPERS_H


