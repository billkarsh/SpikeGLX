#ifndef CNIINDMX_H
#define CNIINDMX_H

#ifdef HAVE_NIDAQmx

#include "CniIn.h"
#include "NI/NIDAQmx.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Dmx NI-DAQ input
//
class CniInDmx : public CniIn
{
private:
    TaskHandle  taskAI1, taskAI2,
                taskDI1, taskDI2,
                taskCTR;

public:
    CniInDmx( NIReaderWorker *owner, const Params &p )
    :   CniIn( owner, p ),
        taskAI1(0), taskAI2(0),
        taskDI1(0), taskDI2(0),
        taskCTR(0)
        {setDO( false );}
    virtual ~CniInDmx();

    void run();

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

    bool startTasks();
    void destroyTasks();

    void runError();
};

#endif  // HAVE_NIDAQmx

#endif  // CNIINDMX_H


