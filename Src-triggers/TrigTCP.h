#ifndef TRIGTCP_H
#define TRIGTCP_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrTCPShared {
    const DAQ::Params       &p;
    std::vector<quint64>    imNextCt;
    double                  tRem;
    QMutex                  runMtx;
    QWaitCondition          condWake;
    int                     awake,
                            asleep,
                            errors;
    bool                    stop;

    TrTCPShared( const DAQ::Params &p )
    :   p(p), tRem(-1), awake(0), asleep(0), stop(false)    {}

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


class TrTCPWorker : public QObject
{
    Q_OBJECT

private:
    TrTCPShared         &shr;
    const QVector<AIQ*> &imQ;
    std::vector<int>    vID;

public:
    TrTCPWorker(
        TrTCPShared         &shr,
        const QVector<AIQ*> &imQ,
        std::vector<int>    &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~TrTCPWorker()              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool writeSomeIM( int ip );
    bool writeRemIM( int ip, double tlo );
};


class TrTCPThread
{
public:
    QThread     *thread;
    TrTCPWorker *worker;

public:
    TrTCPThread(
        TrTCPShared         &shr,
        const QVector<AIQ*> &imQ,
        std::vector<int>    &vID );
    virtual ~TrTCPThread();
};


class TrigTCP : public TrigBase
{
    Q_OBJECT

    friend class TrTCPWorker;

private:
    double          _trigHiT,
                    _trigLoT;
    volatile bool   _trigHi;
    int             nThd;

public:
    TrigTCP(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const AIQ           *niQ )
    :   TrigBase( p, gw, imQ, niQ ), _trigHiT(-1), _trigHi(false) {}

    void rgtSetTrig( bool hi );

public slots:
    virtual void run();

private:
    bool isTrigHi() const       {QMutexLocker ml( &runMtx ); return _trigHi;}
    double getTrigHiT() const   {QMutexLocker ml( &runMtx ); return _trigHiT;}
    double getTrigLoT() const   {QMutexLocker ml( &runMtx ); return _trigLoT;}

    bool alignFiles(
        std::vector<quint64>    &imNextCt,
        quint64                 &niNextCt,
        QString                 &err );

    bool writeSomeNI( quint64 &nextCt );
    bool writeRemNI( quint64 &nextCt, double tlo );

    bool xferAll(
        TrTCPShared &shr,
        quint64     &niNextCt,
        double      tRem,
        QString     &err );
    bool allWriteSome(
        TrTCPShared &shr,
        quint64     &niNextCt,
        QString     &err );
    bool allFinalWrite(
        TrTCPShared &shr,
        quint64     &niNextCt,
        QString     &err );
};

#endif  // TRIGTCP_H


