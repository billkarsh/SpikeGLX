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
    double              srate,
                        peakDT,
                        sumTot,
                        sumGet,
                        sumEnq,
                        sumLok,
                        sumWrk;
    quint64             sumPts,
                        totPts;
    std::vector<double> gain;
    AIQ                 *Q;
    int                 ip,
                        nAP,
                        nLF,
                        nCH,
                        slot,
                        port,
                        sumN;

    ImSimProbe(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        AIQ                         *Q,
        int                         ip );
};


class ImSimWorker : public QObject
{
    Q_OBJECT

private:
    CimAcqSim               *acq;
    ImSimShared             &shr;
    std::vector<ImSimProbe> probes;
    double                  loopT,
                            lastCheckT;

public:
    ImSimWorker(
        CimAcqSim               *acq,
        ImSimShared             &shr,
        std::vector<ImSimProbe> &probes )
    :   acq(acq), shr(shr), probes(probes)    {}

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
        CimAcqSim               *acq,
        ImSimShared             &shr,
        std::vector<ImSimProbe> &probes );
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
    std::vector<ImSimThread*>   imT;
    const double                maxV;

public:
    CimAcqSim( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqSim();

    virtual void run();
    virtual void update( int )                  {}
    virtual QString opto_getAttens( int, int )  {return "0 0 0 0 0 0 0 0 0 0 0 0 0 0 ";}
    virtual QString opto_emit( int, int, int )  {return QString();}

private:
    int fetchE(
        qint16              *dst,
        const ImSimProbe    &P,
        double              loopT );

    void runError( QString err );
};

#endif  // CIMACQSIM_H


