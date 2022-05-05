#ifndef TRIGTTL_H
#define TRIGTTL_H

#include "TrigBase.h"

#include <QWaitCondition>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct TrTTLShared {
    const DAQ::Params   &p;
    QMutex              runMtx;
    QWaitCondition      condWake;
    int                 preMidPost, // {-1,0,+1}
                        awake,
                        asleep,
                        errors;
    bool                stop;

    TrTTLShared( const DAQ::Params &p )
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


class TrTTLWorker : public QObject
{
    Q_OBJECT

private:
    TrTTLShared             &shr;
    std::vector<SyncStream> &vS;
    std::vector<int>        viq;

public:
    TrTTLWorker(
        TrTTLShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq )
    :   shr(shr), vS(vS), viq(viq)  {}

signals:
    void finished();

public slots:
    void run();

public:
    bool writePreMargin( int iq );
    bool writePostMargin( int iq );
    bool doSomeH( int iq );
};


class TrTTLThread
{
public:
    QThread     *thread;
    TrTTLWorker *worker;

public:
    TrTTLThread(
        TrTTLShared             &shr,
        std::vector<SyncStream> &vS,
        std::vector<int>        &viq );
    virtual ~TrTTLThread();
};


class TrigTTL : public TrigBase
{
    Q_OBJECT

    friend class TrTTLWorker;

private:
    struct Counts {
        // variable -------------------
        std::vector<quint64>    edgeCt,
                                fallCt,
                                nextCt;
        std::vector<qint64>     remCt;
        // const ----------------------
        std::vector<qint64>     hiCtMax,
                                marginCt,
                                refracCt;
        std::vector<int>        maxFetch;
        const int               iTrk,
                                nq;

        Counts( const DAQ::Params &p );

        void setPreMarg();
        void setH( DAQ::TrgTTLMode mode );
        void setPostMarg();
        bool allFallCtSet();
        bool remCtDone();
        void advanceByTime();
        void advancePastFall();
        bool isTracking();
        double L_progress();
        double H_progress();
    };

private:
    TrTTLWorker             *locWorker;
    Counts                  cnt;
    std::vector<quint64>    vEdge;
    const qint64            highsMax;
    quint64                 aEdgeCtNext,
                            aFallCtNext;
    const int               thresh,
                            digChan;
    int                     iqMax,
                            nThd,
                            nHighs,
                            state;

public:
    TrigTTL(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );

public slots:
    virtual void run();

private:
    void SETSTATE_L();
    void SETSTATE_PreMarg();
    void SETSTATE_H();
    void SETSTATE_PostMarg();
    void SETSTATE_Done();
    void initState();

    bool _getRiseEdge();
    bool _getFallEdge();

    bool getRiseEdge();
    void getFallEdge();

    bool xferAll( TrTTLShared &shr, int preMidPost, QString &err );

    void statusProcess( QString &sT, bool inactive );
};

#endif  // TRIGTTL_H


