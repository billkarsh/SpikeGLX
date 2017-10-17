#ifndef TRIGIMMED_H
#define TRIGIMMED_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrImmShared {
    const DAQ::Params   &p;
    QVector<quint64>    imNextCt;
    QMutex              runMtx;
    QWaitCondition      condWake;
    int                 awake,
                        asleep,
                        errors;
    bool                stop;

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
    TrImmShared         &shr;
    const QVector<AIQ*> &imQ;
    QVector<int>        vID;

public:
    TrImmWorker(
        TrImmShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~TrImmWorker()              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool writeSomeIM( int ip );
};


class TrImmThread
{
public:
    QThread     *thread;
    TrImmWorker *worker;

public:
    TrImmThread(
        TrImmShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID );
    virtual ~TrImmThread();
};


class TrigImmed : public TrigBase
{
    Q_OBJECT

    friend class TrImmWorker;

private:
    int     nThd;

public:
    TrigImmed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ )   {}

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    bool alignFiles(
        QVector<quint64>    &imNextCt,
        quint64             &niNextCt );

    bool writeSomeNI( quint64 &nextCt );

    bool xferAll( TrImmShared &shr, quint64 &niNextCt );
    bool allWriteSome( TrImmShared &shr, quint64 &niNextCt );
};

#endif  // TRIGIMMED_H


