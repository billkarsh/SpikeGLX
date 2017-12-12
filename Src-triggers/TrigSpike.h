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
    struct HiPassFnctr : public AIQ::T_AIQBlockFilter {
        Biquad  *flt;
        int     nchans,
                ichan,
                maxInt,
                nzero;
        HiPassFnctr( const DAQ::Params &p );
        virtual ~HiPassFnctr();
        void reset();
        void operator()( vec_i16 &data );
    };

    struct Counts {
        const quint64   periEvtCt,
                        refracCt,
                        latencyCt;

        Counts( const DAQ::Params &p, double srate )
        :   periEvtCt(p.trgSpike.periEvtSecs * srate),
            refracCt(qMax( p.trgSpike.refractSecs * srate, 5.0 )),
            latencyCt(0.25 * srate) {}

        quint64 minCt()             {return periEvtCt + latencyCt;}
    };

    struct CountsIm : public Counts {
        QVector<quint64>    nextCt;
        QVector<qint64>     remCt;
        int                 offset,
                            np;

        CountsIm( const DAQ::Params &p, double srate )
        :   Counts( p, srate ),
            offset(p.ni.enabled ? 1 : 0),
            np(p.im.nProbes)
            {
                nextCt.resize( np );
                remCt.resize( np );
            }

        void setupWrite( const QVector<quint64> &vEdge )
            {
                for( int ip = 0; ip < np; ++ip ) {
                    nextCt[ip]  = vEdge[offset+ip] - periEvtCt;
                    remCt[ip]   = 2 * periEvtCt + 1;
                }
            }
        bool remCtDone()
            {
                for( int ip = 0; ip < np; ++ip ) {
                    if( remCt[ip] > 0 )
                        return false;
                }
                return true;
            }
    };

    struct CountsNi : public Counts {
        quint64 nextCt;
        qint64  remCt;

        CountsNi( const DAQ::Params &p, double srate )
        :   Counts( p, srate ), nextCt(0), remCt(0) {}

        void setupWrite( const QVector<quint64> &vEdge )
            {
                nextCt  = vEdge[0] - periEvtCt;
                remCt   = 2 * periEvtCt + 1;
            }
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

    bool xferAll( TrSpkShared &shr );
};

#endif  // TRIGSPIKE_H


