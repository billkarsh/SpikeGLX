#ifndef CIMACQIMEC_H
#define CIMACQIMEC_H

#ifdef HAVE_IMEC

#include "CimAcq.h"
#include "IMEC/Neuropix_basestation_api.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Hardware IMEC input
//
class CimAcqImec : public CimAcq
{
private:
    Neuropix_basestation_api    IM;
    QVector<ElectrodePacket>    E;
    int                         nE;
    volatile bool               paused,
                                pauseAck,
                                zeroFill;

public:
    CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p );
    virtual ~CimAcqImec();

    virtual void run();
    virtual void update();

private:
    void setPause( bool pause )  {QMutexLocker ml( &runMtx );paused = pause;}
    bool isPaused() const        {QMutexLocker ml( &runMtx );return paused;}
    bool setPauseAck( bool ack )
        {
            QMutexLocker    ml( &runMtx );
            bool            wasAck = pauseAck;
            pauseAck = ack;
            return wasAck;
        }
    bool isPauseAck()            {QMutexLocker ml( &runMtx );return pauseAck;}

    bool fetchE();

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
    bool _resumeAcq();

    bool configure();
    bool startAcq();
    void close();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


