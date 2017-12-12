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
    struct Counts {
        const double    srate;
        const qint64    hiCtMax,
                        marginCt,
                        refracCt;
        const int       maxFetch;

        Counts( const DAQ::Params &p, double srate )
        :   srate(srate),
            hiCtMax(
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
        int                 iTrk,
                            np;

        CountsIm( const DAQ::Params &p, double srate )
        :   Counts( p, srate ),
            iTrk(p.streamID(p.trgTTL.stream)),
            np(p.im.nProbes)
            {
                edgeCt.resize( np );
                fallCt.resize( np );
                nextCt.resize( np );
                remCt.resize( np );
            }

        void setPreMarg()
            {
                // Set from and rem for universal premargin.
                if( np ) {
                    for( int ip = 0; ip < np; ++ip ) {
                        remCt[ip]   = marginCt;
                        nextCt[ip]  = edgeCt[ip] - remCt[ip];
                    }
                }
                else
                    remCt.fill( 0, np );
            }
        void setH( DAQ::TrgTTLMode mode )
            {
                // Set from, rem and fall (if applicable) for H phase.
                if( np ) {

                    nextCt = edgeCt;

                    if( mode == DAQ::TrgTTLLatch ) {

                        remCt.fill( hiCtMax, np );
                        // fallCt N.A.
                    }
                    else if( mode == DAQ::TrgTTLTimed ) {

                        remCt.fill( hiCtMax, np );

                        for( int ip = 0; ip < np; ++ip )
                            fallCt[ip] = edgeCt[ip] + remCt[ip];
                    }
                    else {

                        // remCt must be set within H-writer which seeks
                        // and sets true fallCt. Here we must zero fallCt.

                        fallCt.fill( 0, np );
                    }
                }
                else {
                    fallCt.fill( 0, np );
                    remCt.fill( 0, np );
                }
            }
        void setPostMarg()
            {
                // Set from and rem for postmargin;
                // not applicable in latched mode.
                if( np ) {
                    for( int ip = 0; ip < np; ++ip ) {
                        remCt[ip]   = marginCt;
                        nextCt[ip]  = fallCt[ip] + marginCt - remCt[ip];
                    }
                }
                else
                    remCt.fill( 0, np );
            }
        bool allFallCtSet()
            {
                for( int ip = 0; ip < np; ++ip ) {
                    if( !fallCt[ip] )
                        return false;
                }
                return true;
            }
        bool remCtDone()
            {
                for( int ip = 0; ip < np; ++ip ) {
                    if( remCt[ip] > 0 )
                        return false;
                }
                return true;
            }
        void advanceByTime()
            {
                qint64  adv = qMax( hiCtMax, refracCt );

                for( int ip = 0; ip < np; ++ip )
                    nextCt[ip] = edgeCt[ip] + adv;
            }
        void advancePastFall()
            {
                for( int ip = 0; ip < np; ++ip ) {
                    nextCt[ip] =
                        qMax( edgeCt[ip] + refracCt, fallCt[ip] + 1 );
                }
            }
        bool isTracking()
            {
                return nextCt[iTrk] > 0;
            }
        double L_progress()
            {
                return (nextCt[iTrk] - edgeCt[iTrk]) / srate;
            }
        double H_progress()
            {
                return (marginCt + nextCt[iTrk] - edgeCt[iTrk]) / srate;
            }
    };

    struct CountsNi : public Counts {
        quint64 edgeCt,
                fallCt,
                nextCt;
        qint64  remCt;
        bool    enabled;

        CountsNi( const DAQ::Params &p, double srate )
        :   Counts( p, srate ),
            edgeCt(0), fallCt(0),
            nextCt(0), remCt(0), enabled(p.ni.enabled)  {}

        void setPreMarg()
            {
                // Set from and rem for universal premargin.
                if( enabled ) {
                    remCt   = marginCt;
                    nextCt  = edgeCt - remCt;
                }
                else
                    remCt = 0;
            }
        void setH( DAQ::TrgTTLMode mode )
            {
                // Set from, rem and fall (if applicable) for H phase.
                if( enabled ) {

                    nextCt = edgeCt;

                    if( mode == DAQ::TrgTTLLatch ) {

                        remCt = hiCtMax;
                        // fallCt N.A.
                    }
                    else if( mode == DAQ::TrgTTLTimed ) {

                        remCt   = hiCtMax;
                        fallCt  = edgeCt + remCt;
                    }
                    else {

                        // remCt must be set within H-writer which seeks
                        // and sets true fallCt. Here we must zero fallCt.

                        fallCt = 0;
                    }
                }
                else {
                    fallCt  = 0;
                    remCt   = 0;
                }
            }
        void setPostMarg()
            {
                // Set from and rem for postmargin;
                // not applicable in latched mode.
                if( enabled ) {
                    remCt   = marginCt;
                    nextCt  = fallCt + marginCt - remCt;
                }
                else
                    remCt = 0;
            }
        void advanceByTime()
            {
                nextCt = edgeCt + qMax( hiCtMax, refracCt );
            }
        void advancePastFall()
            {
                nextCt = qMax( edgeCt + refracCt, fallCt + 1 );
            }
        double L_progress()
            {
                return (nextCt - edgeCt) / srate;
            }
        double H_progress()
            {
                return (marginCt + nextCt - edgeCt) / srate;
            }
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


