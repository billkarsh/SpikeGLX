#ifndef TRIGSPIKE_H
#define TRIGSPIKE_H

#include "TrigBase.h"

#include <QWaitCondition>

class Biquad;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrSpkShared {
    const DAQ::Params   &p;
    QMutex              runMtx;
    QWaitCondition      condWake;
    int                 awake,
                        asleep,
                        errors;
    bool                stop;

    TrSpkShared( const DAQ::Params &p )
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


class TrSpkWorker : public QObject
{
    Q_OBJECT

private:
    TrSpkShared         &shr;
    const QVector<AIQ*> &imQ;
    QVector<int>        vID;

public:
    TrSpkWorker(
        TrSpkShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~TrSpkWorker()              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool writeSomeIM( int ip );
};


class TrSpkThread
{
public:
    QThread     *thread;
    TrSpkWorker *worker;

public:
    TrSpkThread(
        TrSpkShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID );
    virtual ~TrSpkThread();
};


class TrigSpike : public TrigBase
{
    Q_OBJECT

    friend class TrSpkWorker;

private:
    struct HiPassFnctr : public AIQ::T_AIQFilter {
        Biquad  *flt;
        int     maxInt,
                nzero;
        HiPassFnctr( const DAQ::Params &p );
        virtual ~HiPassFnctr();

        void reset();
        void operator()( int nflt );
    };

    struct CountsIm {
        // variable -------------------
        QVector<quint64>    nextCt;
        QVector<qint64>     remCt;
        // const ----------------------
        QVector<quint64>    periEvtCt,
                            refracCt,
                            latencyCt;
        const int           offset,
                            np;

        CountsIm( const DAQ::Params &p );

        void setupWrite( const QVector<quint64> &vEdge );
        quint64 minCt( int ip );
        bool remCtDone();
    };

    struct CountsNi {
        // variable -------------------
        quint64             nextCt;
        qint64              remCt;
        // const ----------------------
        const quint64       periEvtCt,
                            refracCt,
                            latencyCt;

        CountsNi( const DAQ::Params &p );

        void setupWrite(
            const QVector<quint64>  &vEdge,
            bool                    enabled );

        quint64 minCt();
    };

private:
    HiPassFnctr         *usrFlt;
    CountsIm            imCnt;
    CountsNi            niCnt;
    QVector<quint64>    vEdge;
    const qint64        spikesMax;
    quint64             aEdgeCtNext;
    const int           thresh;
    int                 nThd,
                        nSpikes,
                        state;

public:
    TrigSpike(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const AIQ           *niQ );
    virtual ~TrigSpike()    {delete usrFlt;}

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_GetEdge();
    void SETSTATE_Write();
    void SETSTATE_Done();
    void initState();

    bool getEdge( int iSrc );

    bool writeSomeNI();

    bool xferAll( TrSpkShared &shr, QString &err );
};

#endif  // TRIGSPIKE_H


