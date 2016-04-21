#ifndef SHA1VERIFIER_H
#define SHA1VERIFIER_H

#include "KVParams.h"
#include <QMutex>

class QProgressDialog;
class ConsoleWindow;

class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Sha1Worker : public QObject
{
    Q_OBJECT

public:
    enum Result {
        Success,
        Failure,
        Canceled
    };

public:
    QString         dataFileName,
                    dataFileNameShort,
                    extendedError;
    KeyValMap       kvm;
    mutable QMutex  runMtx;
    volatile bool   pleaseStop;

public:
    Sha1Worker(
        const QString   &dataFileName,
        const KeyValMap &kvm );

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void progress( int );
    void result( int res );

public slots:
    void run();
};


// Self-deleting verify handler
//
class Sha1Verifier : public QObject
{
    Q_OBJECT

private:
    ConsoleWindow   *cons;
    QProgressDialog *prog;
    QThread         *thread;
    Sha1Worker      *worker;

public:
    Sha1Verifier( const QString &curAcqFile );

public slots:
    void cancel()   {worker->stop();}
    void result( int res );
};

#endif  // SHA1VERIFIER_H


