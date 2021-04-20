#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/NeuropixAPI.h"

#include <QSet>

class CimAcqImec;

using namespace Neuropixels;


/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// There is one of these, basically to manage run state.
//
struct ImAcqShared {
    double                  startT;
// Experiment to histogram successive timestamp differences.
    std::vector<quint64>    tStampBins,
                            tStampEvtByPrb;
    QMutex                  runMtx;
    QWaitCondition          condWake;
    int                     awake,
                            asleep;
    bool                    stop;

    ImAcqShared();

// Experiment to histogram successive timestamp differences.
    virtual ~ImAcqShared();
    void tStampHist_T0( const electrodePacket* E, int ip, int ie, int it );
    void tStampHist_T2( const PacketInfo* H, int ip, int it );

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


struct ImAcqProbe {
// @@@ FIX Experiment to report large fetch cycle times.
    mutable double  tLastFetch,
                    tLastErrReport,
                    tLastFifoReport,
                    tPreEnq,
                    tPostEnq,
                    peakDT,
                    sumTot,
                    sumLag,
                    sumGet,
                    sumScl,
                    sumEnq;
    mutable quint64 totPts;
// Experiment to detect gaps in timestamps across fetches.
    mutable quint32 lastTStamp,
                    errCOUNT,
                    errSERDES,
                    errLOCK,
                    errPOP,
                    errSYNC,
                    tStampLastFetch;
    mutable int     fifoAve,
                    fifoN,
                    sumN;
    int             ip,
                    nAP,
                    nLF,
                    nSY,
                    nCH,
                    slot,
                    port,
                    dock,
                    fetchType;  // accommodate custom probe architectures
    mutable bool    zeroFill;

    ImAcqProbe()    {}
    ImAcqProbe(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        int                         ip );
    virtual ~ImAcqProbe();

    void sendErrMetrics() const;
    void checkErrFlags_T0( const electrodePacket* E, int nE ) const;
    void checkErrFlags_T2( const PacketInfo* H, int nT ) const;
    bool checkFifo( int *packets, CimAcqImec *acq ) const;
};


// Handles several probes of mixed type.
//
class ImAcqWorker : public QObject
{
    Q_OBJECT

private:
    double                      tLastYieldReport,
                                yieldSum,
                                loopT,
                                lastCheckT;
    CimAcqImec                  *acq;
    QVector<AIQ*>               &imQ;
    ImAcqShared                 &shr;
    std::vector<ImAcqProbe>     probes;
    std::vector<PacketInfo>     H;
    std::vector<qint32>         D;

public:
    ImAcqWorker(
        CimAcqImec              *acq,
        QVector<AIQ*>           &imQ,
        ImAcqShared             &shr,
        std::vector<ImAcqProbe> &probes );
    virtual ~ImAcqWorker()  {}

signals:
    void finished();

public slots:
    void run();

private:
    bool doProbe_T0(
        float               *lfLast,
        vec_i16             &dst1D,
        const ImAcqProbe    &P );
    bool doProbe_T2(
        vec_i16             &dst1D,
        const ImAcqProbe    &P );
    bool workerYield();
    void profile( const ImAcqProbe &P );
};


class ImAcqThread
{
public:
    QThread     *thread;
    ImAcqWorker *worker;

public:
    ImAcqThread(
        CimAcqImec              *acq,
        QVector<AIQ*>           &imQ,
        ImAcqShared             &shr,
        std::vector<ImAcqProbe> &probes );
    virtual ~ImAcqThread();
};


// Hardware IMEC input
//
class CimAcqImec : public CimAcq
{
    friend struct ImAcqProbe;
    friend class  ImAcqWorker;

private:
    const CimCfg::ImProbeTable  &T;
    ImAcqShared                 shr;
    std::vector<ImAcqThread*>   imT;
    QSet<int>                   pausDocksReported;
    int                         pausDocksRequired,
                                pausSlot,
                                nThd;

public:
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqImec();

    virtual void run();
    virtual void update( int ip );

private:
    void pauseSlot( int slot );
    int  pausedSlot() const {QMutexLocker ml( &runMtx ); return pausSlot;}
    bool pauseAck( int port, int dock );
    bool pauseAllAck() const;

    bool fetchE_T0( int &nE, electrodePacket* E, const ImAcqProbe &P );
    bool fetchD_T2( int &nT, PacketInfo* H, qint16* D, const ImAcqProbe &P );
    int fifoPct( int *packets, const ImAcqProbe &P ) const;

    void SETLBL( const QString &s, bool zero = false );
    void SETVAL( int val );
    void SETVALBLOCKING( int val );

    bool _allProbesSizeStreamBufs();
    bool _open( const CimCfg::ImProbeTable &T );
    bool _setSyncAsOutput( int slot );
    bool _setSyncAsInput( int slot );
    bool _setSync( const CimCfg::ImProbeTable &T );
    bool _openProbe( const CimCfg::ImProbeDat &P );
    bool _calibrateADC( const CimCfg::ImProbeDat &P );
    bool _calibrateGain( const CimCfg::ImProbeDat &P );
    bool _dataGenerator( const CimCfg::ImProbeDat &P );
    bool _setLEDs( const CimCfg::ImProbeDat &P );
    bool _selectElectrodes1( const CimCfg::ImProbeDat &P );
    bool _selectElectrodesN( const CimCfg::ImProbeDat &P );
    bool _setReferences( const CimCfg::ImProbeDat &P );
    bool _setGains( const CimCfg::ImProbeDat &P );
    bool _setHighPassFilter( const CimCfg::ImProbeDat &P );
    bool _setStandby( const CimCfg::ImProbeDat &P );
    bool _writeProbe( const CimCfg::ImProbeDat &P );

    bool _setTrigger();
    bool _setArm();

    bool _softStart();

    bool configure();
    bool startAcq();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


