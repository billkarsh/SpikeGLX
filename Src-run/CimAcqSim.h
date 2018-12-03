#ifndef CIMACQSIM_H
#define CIMACQSIM_H

#include "CimAcq.h"

class CimAcqSim;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImSimShared {
    double          startT;
    QMutex          runMtx;
    QWaitCondition  condWake;
    int             awake,
                    asleep;
    bool            stop;

    ImSimShared();

    bool wait()
    {
        bool    run;
        runMtx.lock();
            ++asleep;
            condWake.wait( &runMtx );
            ++awake;
            run = !stop;
        runMtx.unlock();
        return run;
    }

    bool stopping()
    {
        bool    _stop;
        runMtx.lock();
            _stop = stop;
        runMtx.unlock();
        return _stop;
    }

    void kill()
    {
        runMtx.lock();
            stop = true;
        runMtx.unlock();
        condWake.wakeAll();
    }
};


struct ImSimProbe {
    double          srate,
                    peakDT,
                    sumTot,
                    sumGet,
                    sumEnq,
                    sumLok,
                    sumWrk;
    quint64         sumPts,
                    totPts;
    QVector<double> gain;
    int             ip,
                    nAP,
                    nLF,
                    nSY,
                    nCH,
                    slot,
                    port,
                    sumN;

    ImSimProbe()    {}
    ImSimProbe(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        int                         ip );
};


class ImSimWorker : public QObject
{
    Q_OBJECT

private:
    CimAcqSim           *acq;
    QVector<AIQ*>       &imQ;
    ImSimShared         &shr;
    QVector<ImSimProbe> probes;
    double              loopT,
                        lastCheckT;

public:
    ImSimWorker(
        CimAcqSim           *acq,
        QVector<AIQ*>       &imQ,
        ImSimShared         &shr,
        QVector<ImSimProbe> &probes )
    :   acq(acq), imQ(imQ), shr(shr), probes(probes)    {}
    virtual ~ImSimWorker()                              {}

signals:
    void finished();

public slots:
    void run();

private:
    bool doProbe( vec_i16 &dst1D, ImSimProbe &P );
    void profile( ImSimProbe &P );
};


class ImSimThread
{
public:
    QThread     *thread;
    ImSimWorker *worker;

public:
    ImSimThread(
        CimAcqSim           *acq,
        QVector<AIQ*>       &imQ,
        ImSimShared         &shr,
        QVector<ImSimProbe> &probes );
    virtual ~ImSimThread();
};


// Simulated IMEC input
//
class CimAcqSim : public CimAcq
{
    friend class ImSimWorker;

private:
    const CimCfg::ImProbeTable  &T;
    ImSimShared                 shr;
    QVector<ImSimThread*>       imT;
    const double                maxV;
    int                         nThd;

public:
    CimAcqSim( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqSim();

    virtual void run();
    virtual void update( int )  {}

private:
    int fetchE(
        qint16              *dst,
        const ImSimProbe    &P,
        double              loopT );

    void runError( QString err );
};

#endif  // CIMACQSIM_H


