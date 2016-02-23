#ifndef CNIACQDMX_H
#define CNIACQDMX_H

#ifdef HAVE_NIDAQmx

#include "CniAcq.h"
#include "NI/NIDAQmx.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Dmx NI-DAQ input
//
class CniAcqDmx : public CniAcq
{
private:
    TaskHandle  taskAI1, taskAI2,
                taskDI1, taskDI2,
                taskCTR;
    uInt32      maxSampPerChan,
                kMuxedSampPerChan;
    int         kmux, KAI1, KAI2,
                kmn1, kma1, kxa1, kxd1,
                kmn2, kma2, kxa2, kxd2;

public:
    CniAcqDmx( NIReaderWorker *owner, const Params &p )
    :   CniAcq( owner, p ),
        taskAI1(0), taskAI2(0),
        taskDI1(0), taskDI2(0),
        taskCTR(0)
        {setDO( false );}
    virtual ~CniAcqDmx();

    virtual void run();

private:
    void setDO( bool onoff );

    bool createAITasks(
        const QString   &aiChanStr1,
        const QString   &aiChanStr2,
        quint32         maxMuxedSampPerChan );

    bool createDITasks(
        const QString   &diChanStr1,
        const QString   &diChanStr2,
        quint32         maxMuxedSampPerChan );

    bool createCTRTask();

    bool configure();
    bool startTasks();
    void destroyTasks();
    void runError();
};

#endif  // HAVE_NIDAQmx

#endif  // CNIACQDMX_H


