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
    TrTimShared         &shr;
    const QVector<AIQ*> &imQ;
    std::vector<int>    vID;

public:
    TrTimWorker(
        TrTimShared         &shr,
        const QVector<AIQ*> &imQ,
        std::vector<int>    &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~TrTimWorker()              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool doSomeHIm( int ip );
};


class TrTimThread
{
public:
    QThread     *thread;
    TrTimWorker *worker;

public:
    TrTimThread(
        TrTimShared         &shr,
        const QVector<AIQ*> &imQ,
        std::vector<int>    &vID );
    virtual ~TrTimThread();
};


class TrigTimed : public TrigBase
{
    Q_OBJECT

    friend class TrTimWorker;

private:
    struct CountsIm {
        // variable -------------------
        std::vector<quint64>    nextCt,
                                hiCtCur;
        // const ----------------------
        std::vector<quint64>    hiCtMax,
                                loCt;
        std::vector<uint>       maxFetch;
        const int               np;

        CountsIm( const DAQ::Params &p );

        bool hDone();
        void hNext();
    };

    struct CountsNi {
        // variable -------------------
        quint64             nextCt,
                            hiCtCur;
        // const ----------------------
        const quint64       hiCtMax,
                            loCt;
        const uint          maxFetch;
        const bool          enabled;

        CountsNi( const DAQ::Params &p );

        bool hDone();
        void hNext();
    };

private:
    CountsIm        imCnt;
    CountsNi        niCnt;
    const qint64    nCycMax;
    int             nThd,
                    nH,
                    state;

public:
    TrigTimed(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const AIQ           *niQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_Done();
    void initState();

    double remainingL0( double loopT, double gHiT );
    double remainingL( const AIQ *aiQ, quint64 nextCt );

    bool alignFirstFiles( double gHiT, QString &err );
    void alignNextFiles();

    bool doSomeHNi();

    bool xferAll( TrTimShared &shr, QString &err );
    bool allDoSomeH( TrTimShared &shr, double gHiT, QString &err );
};

#endif  // TRIGTIMED_H


