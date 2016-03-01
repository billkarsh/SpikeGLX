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

public:
    CimAcqImec( IMReaderWorker *owner, const Params &p )
    :   CimAcq( owner, p )  {}
    virtual ~CimAcqImec();

    virtual void run();

private:
    bool _open();
    bool _selectElectrodesEach();
    bool _setRefsEach();
    bool _setRefsAll();
    bool _setGainEach();
    bool _setAPGainAll();
    bool _setLFGainAll();
    bool _setHighPassFilter();
    bool _setNeuralRecording();
    bool _setElectrodeMode();
    bool _setTriggerMode();
    bool _setStandbyAll();
    bool _setRecording();

    void bist();
    bool configure();
    bool startAcq();
    void close();
    void runError( QString err );
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


