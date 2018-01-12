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
    vec_i16             merged,
                        rawAI1,    rawAI2;
    std::vector<uInt8>  rawDI1,    rawDI2;
    int                 shift1[8], shift2[8];
    TaskHandle          taskAI1,   taskAI2,
                        taskDI1,   taskDI2,
                        taskIntCTR, taskSyncPls;
    uInt32              maxSampPerChan,
                        kMuxedSampPerChan;
    int                 kmux, KAI1, KAI2,
                        kmn1, kma1, kxa1, kxd1,
                        kmn2, kma2, kxa2, kxd2;

public:
    CniAcqDmx( NIReaderWorker *owner, const DAQ::Params &p )
    :   CniAcq( owner, p ),
        taskAI1(0), taskAI2(0),
        taskDI1(0), taskDI2(0),
        taskIntCTR(0), taskSyncPls(0)
        {setDO( false );}
    virtual ~CniAcqDmx();

    virtual void run();

private:
    bool createAITasks(
        const QString   &aiChanStr1,
        const QString   &aiChanStr2,
        quint32         maxMuxedSampPerChan );

    bool createDITasks(
        const QString   &diChanStr1,
        const QString   &diChanStr2,
        quint32         maxMuxedSampPerChan );

    bool createInternalCTRTask();
    bool createSyncPulserTask();

    bool configure();
    bool startTasks();
    void destroyTasks();
    void setDO( bool onoff );
    void slideRemForward( int rem, int nFetched );
    bool fetch( int32 &nFetched, int rem );
    void demuxMerge( int nwhole );
    void runError( const QString &err = "" );
};

#endif  // HAVE_NIDAQmx

#endif  // CNIACQDMX_H


