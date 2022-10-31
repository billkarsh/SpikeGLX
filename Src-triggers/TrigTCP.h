#ifndef TRIGTCP_H
#define TRIGTCP_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrTCPShared {
    const DAQ::Params       &p;
    std::vector<quint64>    iqNextCt;
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
    TrTCPShared             &shr;
    std::vector<SyncStream> &vS;
    std::vector<int>        viq;

public:
    TrTCPWorker(
        TrTCPShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq )
    :   QObject(0), shr(shr), vS(vS), viq(viq)  {}

signals:
    void finished();

public slots:
    void run();

public:
    bool writeSome( int iq );
    bool writeRem( int iq, double tlo );
};


class TrTCPThread
{
public:
    QThread     *thread;
    TrTCPWorker *worker;

public:
    TrTCPThread(
        TrTCPShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq );
    virtual ~TrTCPThread();
};


class TrigTCP : public TrigBase
{
    Q_OBJECT

    friend class TrTCPWorker;

private:
    TrTCPWorker     *locWorker;
    double          _trigHiT,
                    _trigLoT;
    volatile bool   _trigHi;
    int             iqMax,
                    nThd;

public:
    TrigTCP(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ )
    :   TrigBase(p, gw, imQ, obQ, niQ),
        _trigHiT(-1), _trigHi(false)    {}

    void rgtSetTrig( bool hi );

public slots:
    virtual void run();

private:
    bool isTrigHi() const       {QMutexLocker ml( &runMtx ); return _trigHi;}
    double getTrigHiT() const   {QMutexLocker ml( &runMtx ); return _trigHiT;}
    double getTrigLoT() const   {QMutexLocker ml( &runMtx ); return _trigLoT;}

    bool alignFiles( std::vector<quint64> &iqNextCt, QString &err );
    bool xferAll( TrTCPShared &shr, double tRem, QString &err );
    bool allWriteSome( TrTCPShared &shr, QString &err );
    bool allFinalWrite( TrTCPShared &shr, QString &err );
};

#endif  // TRIGTCP_H


