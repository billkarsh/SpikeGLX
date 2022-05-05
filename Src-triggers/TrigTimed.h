#ifndef TRIGTIMED_H
#define TRIGTIMED_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrTimShared {
    const DAQ::Params   &p;
    QMutex              runMtx;
    QWaitCondition      condWake;
    int                 awake,
                        asleep,
                        errors;
    bool                stop;

    TrTimShared( const DAQ::Params &p )
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


class TrTimWorker : public QObject
{
    Q_OBJECT

private:
    TrTimShared             &shr;
    std::vector<SyncStream> &vS;
    std::vector<int>        viq;

public:
    TrTimWorker(
        TrTimShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq )
    :   shr(shr), vS(vS), viq(viq)  {}

signals:
    void finished();

public slots:
    void run();

public:
    bool doSomeH( int iq );
};


class TrTimThread
{
public:
    QThread     *thread;
    TrTimWorker *worker;

public:
    TrTimThread(
        TrTimShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq );
    virtual ~TrTimThread();
};


class TrigTimed : public TrigBase
{
    Q_OBJECT

    friend class TrTimWorker;

private:
    struct Counts {
        // variable -------------------
        std::vector<quint64>    nextCt,
                                hiCtCur;
        // const ----------------------
        std::vector<quint64>    hiCtMax;
        std::vector<qint64>     loCt;
        std::vector<uint>       maxFetch;
        const int               nq;

        Counts( const DAQ::Params &p );

        bool isReset();
        bool hDone();
        void hNext();
    };

private:
    TrTimWorker     *locWorker;
    Counts          cnt;
    const qint64    nCycMax;
    int             iqMax,
                    nThd,
                    nH,
                    state;

public:
    TrigTimed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );

public slots:
    virtual void run();

private:
    void SETSTATE_Done();
    void initState();

    double remainingL0( double loopT, double gHiT );
    double remainingL();

    bool alignFiles( double gHiT, QString &err );
    bool xferAll( TrTimShared &shr, QString &err );
    bool allDoSomeH( TrTimShared &shr, double gHiT, QString &err );
};

#endif  // TRIGTIMED_H


