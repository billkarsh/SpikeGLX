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
    QVector<int>        vID;

public:
    TrTimWorker(
        TrTimShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID )
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
        QVector<int>        &vID );
    virtual ~TrTimThread();
};


class TrigTimed : public TrigBase
{
    Q_OBJECT

    friend class TrTimWorker;

private:
    struct Counts {
        const quint64   hiCtMax;
        const qint64    loCt;
        const uint      maxFetch;

        Counts( const DAQ::Params &p, double srate )
        :   hiCtMax(
                p.trgTim.isHInf ?
                std::numeric_limits<qlonglong>::max()
                : p.trgTim.tH * srate),
            loCt(p.trgTim.tL * srate),
            maxFetch(0.110 * srate)     {}
    };

    struct CountsIm : public Counts {
        QVector<quint64>    nextCt,
                            hiCtCur;
        int                 np;

        CountsIm( const DAQ::Params &p, double srate )
        :   Counts( p, srate ), np(p.im.nProbes)    {}

        bool hDone()
            {
                for( int ip = 0; ip < np; ++ip ) {
                    if( hiCtCur[ip] < hiCtMax )
                        return false;
                }
                return true;
            }
        void hNext()
            {
                for( int ip = 0; ip < np; ++ip )
                    nextCt[ip] += loCt;
            }
    };

    struct CountsNi : public Counts {
        quint64             nextCt,
                            hiCtCur;
        bool                enabled;

        CountsNi( const DAQ::Params &p, double srate )
        :   Counts( p, srate ),
            nextCt(0), hiCtCur(0), enabled(p.ni.enabled)    {}

        bool hDone()
            {
                return !enabled || hiCtCur >= hiCtMax;
            }
        void hNext()
            {
                 nextCt += loCt;
            }
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

    bool alignFirstFiles( double gHiT );
    void alignNextFiles();

    bool doSomeHNi();

    bool xferAll( TrTimShared &shr );
    bool allDoSomeH( TrTimShared &shr, double gHiT );
};

#endif  // TRIGTIMED_H


