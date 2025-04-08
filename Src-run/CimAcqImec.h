#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/NeuropixAPI.h"
#include "CAR.h"

class CimAcqImec;
class Biquad;

class QFile;

using namespace Neuropixels;

// Older method: Do update( int ip ) only if whole slot paused
//#define PAUSEWHOLESLOT

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImSimLfDat {
    QVector<uint>   ig2ic;
    vec_i16         ibuf_ig,
                    ibuf_ic;
    QFile           *f;
    qint64          smpEOF,
                    smpCur;
    int             acq[3],
                    nG,
                    nC,
                    inbuf;
    ImSimLfDat() : f(0), smpCur(0), inbuf(0)    {}
    virtual ~ImSimLfDat();
    bool init( QString &err, const QString &pfName );
    void load1();
    void get_ie( struct electrodePacket* E, int ie, int nC );
    void retireN( int n );
};


struct ImSimApDat {
    QVector<uint>   ig2ic;
    vec_i16         ibuf_ig,
                    ibuf_ic;
    QFile           *f;
    qint64          smpEOF,
                    smpCur,
                    tstamp;
    int             acq[3],
                    fetchType,
                    nG,
                    nC,
                    inbuf;
    ImSimApDat() : f(0), smpCur(0), tstamp(0), inbuf(0) {}
    virtual ~ImSimApDat();
    bool init( QString &err, const QString &pfName );
    bool load1();
    void fetchT0( struct electrodePacket* E, int* out, ImSimLfDat &LF );
    void fetchT2( struct PacketInfo* H, int16_t* D, int is, int nAP, int smpMax, int* out );
};


// One per probe file
//
struct ImSimDat {
    QMutex      *bufMtx;
    ImSimApDat  AP;
    ImSimLfDat  LF;

    ImSimDat() : bufMtx(0)  {}
    virtual ~ImSimDat();
    bool init( QString &err, const QString &pfName );
    void loadToN( qint64 N );
    void fifo( int *packets, int *empty ) const;
    void fetchT0( struct electrodePacket* E, int* out );
    void fetchT2( struct PacketInfo* H, int16_t* D, int is, int nAP, int smpMax, int* out );
};


// Fetches samples for all ImProbeFileDat.
//
class ImSimPrbWorker : public QObject
{
    Q_OBJECT

private:
    std::vector<ImSimDat>   &simDat;
    mutable QMutex          runMtx;
    volatile bool           pleaseStop;

public:
    ImSimPrbWorker( std::vector<ImSimDat> &simDat )
        :   QObject(0), simDat(simDat), pleaseStop(false)   {}

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

signals:
    void finished();

public slots:
    void run();
};


class ImSimPrbThread
{
private:
    QThread         *thread;
    ImSimPrbWorker  *worker;

public:
    ImSimPrbThread( std::vector<ImSimDat> &simDat );
    virtual ~ImSimPrbThread();
};


// Record if any config worker had an error.
//
struct ImCfgShared {
    QMutex              runMtx;
    QVector<QString>    error;

    ImCfgShared()   {}

    void seterror( QString e )
    {
        runMtx.lock();
            error.push_back( e );
        runMtx.unlock();
    }
};


// Configure array of probes (vip).
//
class ImCfgWorker : public QObject
{
    Q_OBJECT

private:
    CimAcqImec                  *acq;
    const CimCfg::ImProbeTable  &T;
    std::vector<int>            vip;
    ImCfgShared                 &shr;

public:
    ImCfgWorker(
        CimAcqImec                  *acq,
        const CimCfg::ImProbeTable  &T,
        const std::vector<int>      &vip,
        ImCfgShared                 &shr )
    :   QObject(0), acq(acq), T(T), vip(vip), shr(shr)  {}

signals:
    void finished();

public slots:
    void run();

private:
    bool _mt_simProbe( const CimCfg::ImProbeDat &P );
    bool _mt_openProbe( const CimCfg::ImProbeDat &P );
    bool _mt_calibrateADC( const CimCfg::ImProbeDat &P );
    bool _mt_calibrateGain( const CimCfg::ImProbeDat &P );
    bool _mt_calibrateOpto( const CimCfg::ImProbeDat &P );
    bool _mt_setLEDs( const CimCfg::ImProbeDat &P );
    bool _mt_selectElectrodes( const CimCfg::ImProbeDat &P );
    bool _mt_selectReferences( const CimCfg::ImProbeDat &P );
    bool _mt_selectGains( const CimCfg::ImProbeDat &P );
    bool _mt_selectAPFiltes( const CimCfg::ImProbeDat &P );
    bool _mt_setStandby( const CimCfg::ImProbeDat &P );
    bool _mt_writeProbe( const CimCfg::ImProbeDat &P );
};


class ImCfgThread
{
public:
    QThread     *thread;
    ImCfgWorker *worker;

public:
    ImCfgThread(
        CimAcqImec                  *acq,
        const CimCfg::ImProbeTable  &T,
        const std::vector<int>      &vip,
        ImCfgShared                 &shr );
    virtual ~ImCfgThread();
};


// There is one of these managing run state over all workers.
//
struct ImAcqShared {
    double                  startT;
// Experiment to histogram successive timestamp differences.
    std::vector<quint64>    tStampBins,
                            tStampEvtByPrb;
    QMutex                  runMtx,
                            carMtx;
    QWaitCondition          condWake;
    int                     awake,
                            asleep,
                            ip_CAR;
    bool                    stop;

    ImAcqShared();

// Experiment to histogram successive timestamp differences.
    virtual ~ImAcqShared();
    void tStampHist_EPack( const electrodePacket* E, int ip, int ie, int it );
    void tStampHist_PInfo( const PacketInfo* H, int ip, int it );

    void updateCAR( int ip )
    {
        carMtx.lock();
            ip_CAR = ip;
        carMtx.unlock();
    }

    int checkCAR()
    {
        int ip;
        carMtx.lock();
            ip = ip_CAR;
        carMtx.unlock();
        return ip;
    }

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


struct ImAcqQFlt {
    AIQ             *Qf;
    Biquad          *hipass,
                    *lopass;
    CAR             car;
    mutable QMutex  carMtx;
    int             maxInt,
                    nC,
                    nAP;

    ImAcqQFlt( const DAQ::Params &p, AIQ *Qf, int ip );
    virtual ~ImAcqQFlt();
    void mapChanged( const DAQ::Params &p, int ip );
    void enqueue( qint16 *D, int ntpts ) const;
    void enqueueZero( int ntpts ) const;
};


struct ImAcqStream {
// @@@ FIX Experiment to report large fetch cycle times.
    double      tLastFetch,
                tLastErrFlagsReport,
                tLastFifoReport,
                tPreEnq,
                tPostEnq,
                peakDT,
                sumTot,
                sumLag,
                sumGet,
                sumScl,
                sumEnq;
    quint64     totPts;
    AIQ         *Q;
    ImAcqQFlt   *QFlt;
    QVector<uint>           vXA;
    QMap<quint64,int>       mtStampMiss;
    std::vector<quint32>    vtStampMiss;
    std::vector<quint16>    vstatusMiss;
    quint32     tStampLastFetch,
                errCOUNT[4],
                errSERDES[4],
                errLOCK[4],
                errPOP[4],
                errSYNC[4],
                errMISS[4];
    int         fifoAve,
                fifoDqb,
                fifoN,
                sumN;
    int         js,
                ip,
                nAP,
                nLF,
                nXA,
                nXD,
                nCH,
                slot,
                port,
                dock,
                fetchType;  // {0=1.0, 2=2.0, 4=2020, 9=obx}
    quint16     statusLastFetch;
    bool        simType;
#ifdef PAUSEWHOLESLOT
    bool        zeroFill;
#endif

    ImAcqStream(
        const CimCfg::ImProbeTable  &T,
        const DAQ::Params           &p,
        AIQ                         *Q,
        int                         js,
        int                         ip );
    virtual ~ImAcqStream();

    QString metricsName( int shank = -1 ) const;
    void sendErrMetrics() const;
    void fileMissReport() const;
    void checkErrs_EPack( electrodePacket* E, int nE );
    void checkErrs_PInfo( PacketInfo* H, int nT, int shank );
    bool checkFifo( int *packets, CimAcqImec *acq );
};


// Handles several streams of mixed type.
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
    ImAcqShared                 &shr;
    std::vector<ImAcqStream>    streams;
    std::vector<PacketInfo>     H;
    std::vector<qint32>         D;

public:
    ImAcqWorker(
        CimAcqImec                  *acq,
        ImAcqShared                 &shr,
        std::vector<ImAcqStream>    &streams );

signals:
    void finished();

public slots:
    void run();

private:
    bool doProbe_T0( qint16 *lfLast, vec_i16 &dst1D, ImAcqStream &S );
    bool doProbe_T2( vec_i16 &dst1D, ImAcqStream &S );
    bool do_obx( vec_i16 &dst1D, ImAcqStream &S );
    bool workerYield();
    void profile( const ImAcqStream &S );
};


class ImAcqThread
{
public:
    QThread     *thread;
    ImAcqWorker *worker;

public:
    ImAcqThread(
        CimAcqImec                  *acq,
        ImAcqShared                 &shr,
        std::vector<ImAcqStream>    &streams );
    virtual ~ImAcqThread();
};


// Hardware IMEC input
//
class CimAcqImec : public CimAcq
{
    friend struct ImAcqStream;
    friend class  ImCfgWorker;
    friend class  ImAcqWorker;

private:
    const CimCfg::ImProbeTable  &T;
    ImCfgShared                 cfgShr;
    ImAcqShared                 acqShr;
    std::vector<ImSimDat>       simDat;
    std::vector<ImCfgThread*>   cfgThd;
    std::vector<ImAcqThread*>   acqThd;
    std::vector<int>            ip2simdat;
    ImSimPrbThread              *simThd;
#ifdef PAUSEWHOLESLOT
    QSet<int>                   pausStreamsReported;
    int                         pausStreamsRequired,
                                pausSlot;
#endif

public:
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqImec();

    virtual void run();
    virtual void update( int ip );
    virtual QString opto_getAttens( int ip, int color );
    virtual QString opto_emit( int ip, int color, int site );

private:
// -------
// Acquire
// -------

#ifdef PAUSEWHOLESLOT
    void pauseSlot( int slot );
    int  pausedSlot() const {QMutexLocker ml( &runMtx ); return pausSlot;}
    bool pauseAck( int port, int dock );
    bool pauseAllAck() const;
#endif

    bool fetchE_T0( int &nE, electrodePacket* E, ImAcqStream &S );
    bool fetchD_T2(
        int             &nT,
        PacketInfo*     H,
        qint16*         D,
        ImAcqStream     &S,
        int             nC,
        int             smpMax,
        streamsource_t  shank = SourceAP );
    bool fetch_obx( int &nT, PacketInfo* H, qint16* D, ImAcqStream &S );
    int fifoPct( int *packets, int *dqb, const ImAcqStream &S ) const;

// ----------------
// Config and start
// ----------------

// Config progress dialog
    void STEPAUX();
    void STEPPROBE();
    void STEPSTART();

// Config common aux
    bool _aux_sizeStreamBufs();
    bool _aux_initObxSlot( int slot );
    void _aux_doneObxSlot( int slot );
    bool _aux_open( const CimCfg::ImProbeTable &T );
    bool _aux_clrObxSync( int slot );
    bool _aux_setObxSyncAsOutput( int slot );
    bool _aux_setObxSyncAsInput( int slot );
    bool _aux_clrPXISync( int slot );
    bool _aux_setPXISyncAsOutput( int slot );
    bool _aux_setPXISyncAsInput( int slot );
    bool _aux_setPXISyncAsListener( int slot );
    bool _aux_setPXITrigBus( const QVector<int> &vslots, int srcSlot );
    bool _aux_setSync( const CimCfg::ImProbeTable &T );
    bool _aux_config();

// Config each probe (single threaded)
    bool _1t_simProbe( const CimCfg::ImProbeDat &P );
    bool _1t_openProbe( const CimCfg::ImProbeDat &P );
    bool _1t_calibrateADC( const CimCfg::ImProbeDat &P );
    bool _1t_calibrateGain( const CimCfg::ImProbeDat &P );
    bool _1t_calibrateOpto( const CimCfg::ImProbeDat &P );
    bool _1t_setLEDs( const CimCfg::ImProbeDat &P );
    bool _1t_selectElectrodes( const CimCfg::ImProbeDat &P );
    bool _1t_selectReferences( const CimCfg::ImProbeDat &P );
    bool _1t_selectGains( const CimCfg::ImProbeDat &P );
    bool _1t_selectAPFilters( const CimCfg::ImProbeDat &P );
    bool _1t_setStandby( const CimCfg::ImProbeDat &P );
    bool _1t_writeProbe( const CimCfg::ImProbeDat &P );
    bool _1t_configProbes( const CimCfg::ImProbeTable &T );

// Config each probe (multithreaded)
    bool _mt_configProbes( const CimCfg::ImProbeTable &T );

// Config common trigger/start
    bool _st_setHwrTriggers();
    bool _st_setObxTriggers();
    bool _st_setPXITriggers();
    bool _st_setTriggers();
    bool _st_setArm();
    bool _st_softStart();
    bool _st_config();

    bool configure();
    void createAcqWorkerThreads();
    bool startAcq();

// -----
// Error
// -----

    void runError( QString err );
    QString makeErrorString( NP_ErrorCode err ) const;
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


