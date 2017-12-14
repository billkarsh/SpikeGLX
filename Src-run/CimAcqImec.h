#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/NeuropixAPI.h"
#include "IMEC/ElectrodePacket.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ImAcqShared {
    const DAQ::Params           &p;
    QVector<ElectrodePacket>    E;
    QVector<double>             sumScl,
                                sumEnq;
    quint64                     totPts;
    QVector<int>                apPerTpnt,
                                lfPerTpnt,
                                syPerTpnt,
                                chnPerTpnt;
    QMutex                      runMtx;
    QWaitCondition              condWake;
    const int                   maxE;
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
    const CimCfg::ImProbeTable  &T;
    NeuropixAPI                 IM;
    const double                loopSecs;
    ImAcqShared                 shr;
    QVector<ImAcqThread*>       imT;
    int                         nThd;
    volatile bool               paused,
                                pauseAck;

public:
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqImec();

    virtual void run();
    virtual void update( int ip );

private:
    void setPause( bool pause )  {QMutexLocker ml( &runMtx ); paused = pause;}
    bool isPaused() const        {QMutexLocker ml( &runMtx ); return paused;}
    void setPauseAck( bool ack ) {QMutexLocker ml( &runMtx );pauseAck = ack;}
    bool isPauseAck()            {QMutexLocker ml( &runMtx );return pauseAck;}

    bool fetchE( double loopT );
    int fifoPct();

    void SETLBL( const QString &s, bool zero = false );
    void SETVAL( int val );
    void SETVALBLOCKING( int val );

    bool _open( const CimCfg::ImProbeTable &T );

    bool _openProbe( const CimCfg::ImProbeDat &P );
    bool _calibrateADC( const CimCfg::ImProbeDat &P );
    bool _calibrateGain( const CimCfg::ImProbeDat &P );
    bool _dataGenerator( const CimCfg::ImProbeDat &P );
    bool _setLEDs( const CimCfg::ImProbeDat &P );
    bool _selectElectrodes( const CimCfg::ImProbeDat &P );
    bool _setReferences( const CimCfg::ImProbeDat &P );
    bool _setGains( const CimCfg::ImProbeDat &P );
    bool _setHighPassFilter( const CimCfg::ImProbeDat &P );
    bool _setStandby( const CimCfg::ImProbeDat &P );
    bool _writeProbe( const CimCfg::ImProbeDat &P );

    bool _setTrigger( bool software = false );
    bool _setArm();

    bool _arm();
    bool _softStart();
    bool _pauseAcq();
    bool _resumeAcq( int ip );

    bool configure();
    bool startAcq();
    void close();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


