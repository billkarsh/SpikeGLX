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
    TaskHandle  taskAI1, taskAI2,
                taskDI1, taskDI2,
                taskCTR;

public:
    CimAcqImec( IMReaderWorker *owner, const Params &p )
    :   CimAcq( owner, p ),
        taskAI1(0), taskAI2(0),
        taskDI1(0), taskDI2(0),
        taskCTR(0)
        {setDO( false );}
    virtual ~CimAcqImec();

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

    bool startTasks();
    void destroyTasks();

    void runError();
};

#endif  // HAVE_IMEC

#endif  // CIMACQIMEC_H


