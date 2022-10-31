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
    TrSpkShared             &shr;
    std::vector<SyncStream> &vS;
    std::vector<int>        viq;

public:
    TrSpkWorker(
        TrSpkShared             &shr,
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


class TrSpkThread
{
public:
    QThread     *thread;
    TrSpkWorker *worker;

public:
    TrSpkThread(
        TrSpkShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq );
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
        virtual void operator()( int nflt );
    };

    struct Counts {
        // variable -------------------
        std::vector<quint64>    nextCt;
        std::vector<qint64>     remCt;
        // const ----------------------
        std::vector<quint64>    periEvtCt,
                                refracCt,
                                latencyCt;
        const int               nq;

        Counts( const DAQ::Params &p );

        void setupWrite( const std::vector<quint64> &vEdge );
        quint64 minCt( int iq );
        bool remCtDone();
    };

private:
    TrSpkWorker             *locWorker;
    HiPassFnctr             *usrFlt;
    Counts                  cnt;
    std::vector<quint64>    vEdge;
    const qint64            spikesMax;
    quint64                 aEdgeCtNext;
    const int               thresh;
    int                     iqMax,
                            nThd,
                            nSpikes,
                            state;

public:
    TrigSpike(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );
    virtual ~TrigSpike()    {delete usrFlt;}

public slots:
    virtual void run();

private:
    void SETSTATE_GetEdge();
    void SETSTATE_Write();
    void SETSTATE_Done();
    void initState();

    bool getEdge();
    bool xferAll( TrSpkShared &shr, QString &err );
};

#endif  // TRIGSPIKE_H


