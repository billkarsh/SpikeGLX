#ifndef TRIGIMMED_H
#define TRIGIMMED_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrImmShared {
    const DAQ::Params       &p;
    std::vector<quint64>    iqNextCt;
    QMutex                  runMtx;
    QWaitCondition          condWake;
    int                     awake,
                            asleep,
                            errors;
    bool                    stop;

    TrImmShared( const DAQ::Params &p )
    :   p(p), awake(0), asleep(0), stop(false)  {}

    bool wake( bool ok )
    {
        bool    run;
        runMtx.lock();
            errors += !ok;
            ++asleep;
            condWake.wait( &runMtx );
            ++awake;
            run = !stop;
        runMtx.unlock();
        return run;
    }

    void kill()
    {
        runMtx.lock();
            stop = true;
        runMtx.unlock();
        condWake.wakeAll();
    }
};


class TrImmWorker : public QObject
{
    Q_OBJECT

private:
    TrImmShared             &shr;
    std::vector<SyncStream> &vS;
    std::vector<int>        viq;

public:
    TrImmWorker(
        TrImmShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq )
    :   QObject(0), shr(shr), vS(vS), viq(viq)  {}

signals:
    void finished();

public slots:
    void run();

public:
    bool writeSome( int iq );
};


class TrImmThread
{
public:
    QThread     *thread;
    TrImmWorker *worker;

public:
    TrImmThread(
        TrImmShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq );
    virtual ~TrImmThread();
};


class TrigImmed : public TrigBase
{
    Q_OBJECT

    friend class TrImmWorker;

private:
    TrImmWorker *locWorker;
    int         iqMax,
                nThd;

public:
    TrigImmed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ )
    :   TrigBase(p, gw, imQ, obQ, niQ)  {}

public slots:
    virtual void run();

private:
    bool alignFiles( std::vector<quint64> &iqNextCt, QString &err );
    bool xferAll( TrImmShared &shr, QString &err );
    bool allWriteSome( TrImmShared &shr, QString  &err );
};

#endif  // TRIGIMMED_H


