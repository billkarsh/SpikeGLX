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
    TrTTLShared         &shr;
    const QVector<AIQ*> &imQ;
    QVector<int>        vID;

public:
    TrTTLWorker(
        TrTTLShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~TrTTLWorker()              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool writePreMarginIm( int ip );
    bool writePostMarginIm( int ip );
    bool doSomeHIm( int ip );
};


class TrTTLThread
{
public:
    QThread     *thread;
    TrTTLWorker *worker;

public:
    TrTTLThread(
        TrTTLShared         &shr,
        const QVector<AIQ*> &imQ,
        QVector<int>        &vID );
    virtual ~TrTTLThread();
};


class TrigTTL : public TrigBase
{
    Q_OBJECT

    friend class TrTTLWorker;

private:
    struct CountsIm {
        // variable -------------------
        QVector<quint64>    edgeCt,
                            fallCt,
                            nextCt;
        QVector<qint64>     remCt;
        // const ----------------------
        QVector<double>     srate;
        QVector<qint64>     hiCtMax,
                            marginCt,
                            refracCt;
        QVector<int>        maxFetch;
        const int           iTrk,
                            np;

        CountsIm( const DAQ::Params &p );

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

    struct CountsNi {
        // variable -------------------
        quint64             edgeCt,
                            fallCt,
                            nextCt;
        qint64              remCt;
        // const ----------------------
        const double        srate;
        const qint64        hiCtMax,
                            marginCt,
                            refracCt;
        const int           maxFetch;
        const bool          enabled;

        CountsNi( const DAQ::Params &p );

        void setPreMarg();
        void setH( DAQ::TrgTTLMode mode );
        void setPostMarg();
        void advanceByTime();
        void advancePastFall();
        double L_progress();
        double H_progress();
    };

private:
    CountsIm            imCnt;
    CountsNi            niCnt;
    QVector<quint64>    vEdge;
    const qint64        highsMax;
    quint64             aEdgeCtNext,
                        aFallCtNext;
    const int           thresh,
                        digChan;
    int                 nThd,
                        nHighs,
                        state;

public:
    TrigTTL(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const AIQ           *niQ );

    virtual void setGate( bool hi );
    virtual void resetGTCounters();

public slots:
    virtual void run();

private:
    void SETSTATE_L();
    void SETSTATE_PreMarg();
    void SETSTATE_H();
    void SETSTATE_PostMarg();
    void SETSTATE_Done();
    void initState();

    bool _getRiseEdge( quint64 &srcNextCt, int iSrc );
    bool _getFallEdge( quint64 srcEdgeCt, int iSrc );

    bool getRiseEdge();
    void getFallEdge();

    bool writePreMarginNi();
    bool writePostMarginNi();
    bool doSomeHNi();

    bool xferAll( TrTTLShared &shr, int preMidPost );

    void statusProcess( QString &sT, bool inactive );
};

#endif  // TRIGTTL_H


