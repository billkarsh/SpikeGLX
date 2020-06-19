#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/NeuropixAPI.h"

#include <QSet>

class CimAcqImec;


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
    void tStampHist(
        const electrodePacket*  E,
        int                     ip,
        int                     ie,
        int                     it );

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
    mutable quint32 errCOUNT,
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
                    fetchType;  // accommodate custom probe architectures
    mutable bool    zeroFill;

    ImAcqProbe()    {}
    ImAcqProbe(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        int                         ip );
    virtual ~ImAcqProbe();

    void sendErrMetrics() const;
    void checkErrFlags( qint32 *E, int nE ) const;
    bool checkFifo( size_t *packets, CimAcqImec *acq ) const;
};


// Handles several probes of mixed type.
//
class ImAcqWorker : public QObject
{
    Q_OBJECT

private:
    double              	tLastYieldReport,
                        	yieldSum,
                        	loopT,
                        	lastCheckT;
    CimAcqImec              *acq;
    QVector<AIQ*>           &imQ;
    ImAcqShared             &shr;
    std::vector<ImAcqProbe> probes;
    std::vector<qint32>     E;
// ---------
// @@@ FIX Mod for no packets
std::vector<qint16> _rawAP, _rawLF;
// ---------

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
    bool doProbe(
        float               *lfLast,
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
    QSet<int>                   pausPortsReported;
    int                         pausPortsRequired,
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
    bool pauseAck( int port );
    bool pauseAllAck() const;

//    bool fetchE(
//        int                 &nE,
//        qint32              *E,
//        const ImAcqProbe    &P,
//        qint16* rawAP, qint16* rawLF ); // @@@ FIX Mod for no packets

    bool fetchE( int &nE, qint32 *E, const ImAcqProbe &P );
    int fifoPct( size_t *packets, const ImAcqProbe &P ) const;

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
    bool _setElectrode1( int slot, int port, int ic, int bank );
    bool _selectElectrodes( const CimCfg::ImProbeDat &P );
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


