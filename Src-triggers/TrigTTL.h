#ifndef TRIGTTL_H
#define TRIGTTL_H

#include "TrigBase.h"

#include <QWaitCondition>

#include <limits>

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
    struct Counts {
        const qint64    hiCtMax,
                        marginCt,
                        refracCt;
        const int       maxFetch;

        Counts( const DAQ::Params &p, double srate )
        :   hiCtMax(
                p.trgTTL.mode == DAQ::TrgTTLTimed ?
                p.trgTTL.tH * srate
                : std::numeric_limits<qlonglong>::max()),
            marginCt(p.trgTTL.marginSecs * srate),
            refracCt(p.trgTTL.refractSecs * srate),
            maxFetch(0.110 * srate) {}
    };

    struct CountsIm : public Counts {
        QVector<quint64>    edgeCt,
                            fallCt,
                            nextCt;
        QVector<qint64>     remCt;

        CountsIm( const DAQ::Params &p, double srate )
        :   Counts( p, srate )  {}
        bool remCtDone()
            {
                for( int ip = 0, np = remCt.size(); ip < np; ++ip ) {
                    if( remCt[ip] > 0 )
                        return false;
                }
                return true;
            }
        void advanceByTime()
            {
                qint64  adv = std::max( hiCtMax, refracCt );

                for( int ip = 0, np = nextCt.size(); ip < np; ++ip )
                    nextCt[ip] = edgeCt[ip] + adv;
            }
        void advancePastFall()
            {
                for( int ip = 0, np = nextCt.size(); ip < np; ++ip ) {
                    nextCt[ip] =
                        std::max( edgeCt[ip] + refracCt, fallCt[ip] + 1 );
                }
            }
    };

    struct CountsNi : public Counts {
        quint64             edgeCt,
                            fallCt,
                            nextCt;
        qint64              remCt;

        CountsNi( const DAQ::Params &p, double srate )
        :   Counts( p, srate ),
            edgeCt(0), fallCt(0), nextCt(0), remCt(0)   {}
        void advanceByTime()
            {
                nextCt = edgeCt + std::max( hiCtMax, refracCt );
            }
        void advancePastFall()
            {
                nextCt = std::max( edgeCt + refracCt, fallCt + 1 );
            }
    };

private:
    CountsIm        imCnt;
    CountsNi        niCnt;
    const qint64    nCycMax;
    quint64         aEdgeCtNext,
                    aFallCtNext;
    const int       thresh,
                    digChan;
    int             nThd,
                    nH,
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
    void SETSTATE_H();
    void SETSTATE_Done();
    void initState();

    bool _getRiseEdge(
        quint64     &aNextCt,
        const AIQ   *qA,
        quint64     &bNextCt,
        const AIQ   *qB );

    bool _getFallEdge(
        quint64     &aFallCt,
        const AIQ   *qA,
        quint64     &bFallCt,
        const AIQ   *qB );

    bool getRiseEdge();
    void getFallEdge();

    bool writePreMarginNi();
    bool writePostMarginNi();
    bool doSomeHNi();

    bool xferAll( TrTTLShared &shr, int preMidPost );

    void statusProcess( QString &sT, bool inactive );
};

#endif  // TRIGTTL_H


