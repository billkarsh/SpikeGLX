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
    volatile bool               pauseAck;

public:
    CimAcqImec( IMReaderWorker *owner, const Params &p )
    :   CimAcq( owner, p ), pauseAck(false)  {}
    virtual ~CimAcqImec();

    virtual void run();
    virtual bool pause( bool pause, bool changed );

private:
    void setPauseAck( bool ack ) {QMutexLocker ml( &runMtx );pauseAck = ack;}
    bool isPauseAck()            {QMutexLocker ml( &runMtx );return pauseAck;}

    void SETLBL( const QString &s );
    void SETVAL( int val );
    void SETVALBLOCKING( int val );
    bool _open();
    bool _manualProbeSettings();
    bool _calibrateADC();
    bool _selectElectrodes();
    bool _setReferences();
    bool _setGains();
    bool _setHighPassFilter();
    bool _setGainCorrection();
    bool _setNeuralRecording();
    bool _setElectrodeMode();
    bool _setTriggerMode();
    bool _setStandbyAll();
    bool _setRecording();
    bool _pauseAcq();
    bool _resumeAcq( bool changed );

    void bist();
    bool configure();
    bool startAcq();
    void close();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


