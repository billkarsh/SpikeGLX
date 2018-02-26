#ifndef CIMACQSIM_H
#define CIMACQSIM_H

#include "CimAcq.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImSimShared {
    const DAQ::Params           &p;
    QVector<QVector<double> >   gain;
    QVector<quint64>            maxPts,
                                totPts;
    QMutex                      runMtx;
    QWaitCondition              condWake;
    double                      tElapsed;
    int                         awake,
                                asleep,
                                errors;
    bool                        stop;

    ImSimShared( const DAQ::Params &p );

    quint64 sElapsed( int ip )
    {
        return tElapsed * p.im.each[ip].srate;
    }

    int nGenPts( int ip )
    {
        return qMin( sElapsed( ip ) - totPts[ip], maxPts[ip] );
    }

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


class ImSimWorker : public QObject
{
    Q_OBJECT

private:
    ImSimShared     &shr;
    QVector<AIQ*>   &imQ;
    QVector<int>    vID;

public:
    ImSimWorker(
        ImSimShared     &shr,
        QVector<AIQ*>   &imQ,
        QVector<int>    &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~ImSimWorker()              {}

signals:
    void finished();

public slots:
    void run();
};


class ImSimThread
{
public:
    QThread     *thread;
    ImSimWorker *worker;

public:
    ImSimThread(
        ImSimShared     &shr,
        QVector<AIQ*>   &imQ,
        QVector<int>    &vID );
    virtual ~ImSimThread();
};


// Simulated IMEC input
//
class CimAcqSim : public CimAcq
{
public:
    CimAcqSim( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p )      {}

    virtual void run();
    virtual void update( int )  {}
};

#endif  // CIMACQSIM_H


