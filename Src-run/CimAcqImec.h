#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/Neuropix_basestation_api.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImAcqShared {
    const DAQ::Params           &p;
    QVector<ElectrodePacket>    E;
    QVector<double>             sumScl,
                                sumEnq;
    double                      tStamp;
    quint64                     totPts;
    QMutex                      runMtx;
    QWaitCondition              condWake;
    const int                   maxE,
                                apPerTpnt,
                                lfPerTpnt,
                                syPerTpnt,
                                chnPerTpnt;
    int                         awake,
                                asleep,
                                nE;
    bool                        stop;

    ImAcqShared( const DAQ::Params &p, double loopSecs );

    bool wake()
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

    void kill()
    {
        runMtx.lock();
            stop = true;
        runMtx.unlock();
        condWake.wakeAll();
    }
};


class ImAcqWorker : public QObject
{
    Q_OBJECT

private:
    ImAcqShared     &shr;
    QVector<AIQ*>   &imQ;
    QVector<int>    vID;

public:
    ImAcqWorker(
        ImAcqShared     &shr,
        QVector<AIQ*>   &imQ,
        QVector<int>    &vID )
    :   shr(shr), imQ(imQ), vID(vID)    {}
    virtual ~ImAcqWorker()              {}

signals:
    void finished();

public slots:
    void run();
};


class ImAcqThread
{
public:
    QThread     *thread;
    ImAcqWorker *worker;

public:
    ImAcqThread(
        ImAcqShared     &shr,
        QVector<AIQ*>   &imQ,
        QVector<int>    &vID );
    virtual ~ImAcqThread();
};


// Hardware IMEC input
//
class CimAcqImec : public CimAcq
{
private:
    Neuropix_basestation_api    IM;
    const double                loopSecs;
    ImAcqShared                 shr;
    QVector<ImAcqThread*>       imT;
    int                         nThd;
    volatile bool               pauseAck;

public:
// MS: loopSecs for ThinkPad T450 (2 core)
// MS: [[ Core i7-5600U @ 2.6Ghz, 8GB, Win7Pro-64bit ]]
// MS: 1 probe 0.005 with both audio and shankview
// MS: 4 probe 0.005 with both audio and shankview
// MS: 5 probe 0.010 if just audio or shankview
// MS: 6 probe 0.050 if no audio or shankview
//
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ), loopSecs(0.005), shr( p, loopSecs ),
        nThd(0), pauseAck(false)    {}
    virtual ~CimAcqImec();

    virtual void run();
    virtual bool pause( bool pause, bool changed );

private:
    void setPauseAck( bool ack ) {QMutexLocker ml( &runMtx );pauseAck = ack;}
    bool isPauseAck()            {QMutexLocker ml( &runMtx );return pauseAck;}

    bool fetchE( double loopT );

    void SETLBL( const QString &s );
    void SETVAL( int val );
    void SETVALBLOCKING( int val );
    bool _open();
    bool _setLEDs();
    bool _manualProbeSettings( int ip );
    bool _calibrateADC_fromFiles( int ip );
    bool _calibrateADC_fromEEPROM( int ip );
    bool _selectElectrodes();
    bool _setReferences();
    bool _setStandby();
    bool _setGains();
    bool _setHighPassFilter();
    bool _correctGain_fromFiles( int ip );
    bool _correctGain_fromEEPROM( int ip );
    bool _setNeuralRecording();
    bool _setElectrodeMode();
    bool _setTriggerMode();
    bool _setStandbyAll();
    bool _setRecording();
    bool _pauseAcq();
    bool _resumeAcq( bool changed );

    bool configure();
    bool startAcq();
    void close();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


