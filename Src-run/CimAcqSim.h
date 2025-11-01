#ifndef CIMACQSIM_H
#define CIMACQSIM_H

#include "CimAcq.h"

class CimAcqSim;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImSimAcqShared {
    double          startT;
    QMutex          runMtx;
    QWaitCondition  condWake;
    int             awake,
                    asleep;
    bool            stop;

    ImSimAcqShared();

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
        QMutexLocker    ml( &runMtx );
        return stop;
    }

    void kill()
    {
        runMtx.lock();
            stop = true;
        runMtx.unlock();
        condWake.wakeAll();
    }
};


struct ImSimAcqProbe {
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

    ImSimAcqProbe(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        AIQ                         *Q,
        int                         ip );
};


class ImSimAcqWorker : public QObject
{
    Q_OBJECT

private:
    CimAcqSim                   *acq;
    ImSimAcqShared              &shr;
    std::vector<ImSimAcqProbe>  probes;
    double                      loopT,
                                lastCheckT;

public:
    ImSimAcqWorker(
        CimAcqSim                   *acq,
        ImSimAcqShared              &shr,
        std::vector<ImSimAcqProbe>  &probes )
    :   QObject(0), acq(acq), shr(shr), probes(probes)  {}

signals:
    void finished();

public slots:
    void run();

private:
    bool doProbe( vec_i16 &dst1D, ImSimAcqProbe &P );
    void profile( ImSimAcqProbe &P );
};


class ImSimAcqThread
{
public:
    QThread         *thread;
    ImSimAcqWorker  *worker;

public:
    ImSimAcqThread(
        CimAcqSim                   *acq,
        ImSimAcqShared              &shr,
        std::vector<ImSimAcqProbe>  &probes );
    virtual ~ImSimAcqThread();
};


// Simulated IMEC input
//
class CimAcqSim : public CimAcq
{
    friend class ImSimAcqWorker;

private:
    const CimCfg::ImProbeTable      &T;
    ImSimAcqShared                  shr;
    std::vector<ImSimAcqThread*>    imT;
    const double                    maxV;

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
        const ImSimAcqProbe &P,
        double              loopT );

    void runError( QString err );
};

#endif  // CIMACQSIM_H


