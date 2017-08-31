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
// MS: loopSecs: 1probe=0.003, 5probe=0.003, 6probe=0.003, 8probe=0.02
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ), loopSecs(0.003), shr( p, loopSecs ),
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
    bool _manualProbeSettings();
    bool _calibrateADC_fromFiles();
    bool _calibrateADC_fromEEPROM();
    bool _selectElectrodes();
    bool _setReferences();
    bool _setStandby();
    bool _setGains();
    bool _setHighPassFilter();
    bool _correctGain_fromFiles();
    bool _correctGain_fromEEPROM();
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


