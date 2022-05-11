#ifndef CNIACQDMX_H
#define CNIACQDMX_H

#ifdef HAVE_NIDAQmx

#include "CniAcq.h"
#include "NI/NIDAQmx.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class AIBufBase
{
protected:
    uInt32  maxSamps;
    int     KAI;
public:
    AIBufBase( uInt32 maxSamps, int KAI )
    :   maxSamps(maxSamps), KAI(KAI)    {}
    virtual ~AIBufBase()                {}
    virtual void resize() = 0;
    virtual qint16 *i16ptr() = 0;
    virtual bool fetch( TaskHandle T, int32 &nFetched, int rem ) = 0;
    void slideRemForward( int rem, int nFetched );
};

// Unscaled data fetched as i16.
//
class AIBufRaw : public AIBufBase
{
private:
    vec_i16 rawAI;
public:
    AIBufRaw( uInt32 maxSamps, int KAI )
    :   AIBufBase( maxSamps, KAI )  {}
    virtual void resize()       {rawAI.resize( maxSamps*KAI );}
    virtual qint16 *i16ptr()    {return (rawAI.size() ? &rawAI[0] : 0);}
    virtual bool fetch( TaskHandle T, int32 &nFetched, int rem16 );
};

// Scaled data fetched as float64 to a float64 boundary.
// Fetched data are immediately converted to i16, abutting any i16 rem.
// Hence, result of fetching is packed i16 data.
// SlideRemForward moves remaining i16 to front.
//
class AIBufScl : public AIBufBase
{
private:
    double                  v2i;
    std::vector<float64>    sclAI;
public:
    AIBufScl( double v2i, uInt32 maxSamps, int KAI )
    :   AIBufBase( maxSamps, KAI ), v2i(v2i)    {}
    virtual void resize()       {sclAI.resize( maxSamps*KAI );}
    virtual qint16 *i16ptr()    {return (sclAI.size() ? (qint16*)&sclAI[0] : 0);}
    virtual bool fetch( TaskHandle T, int32 &nFetched, int rem16 );
};

// Dmx NI-DAQ input
//
class CniAcqDmx : public CniAcq
{
private:
    vec_i16             merged;
    AIBufBase           *AI1,       *AI2;
    std::vector<uInt32> rawDI1,     rawDI2;
    TaskHandle          taskAI1,    taskAI2,
                        taskDI1,    taskDI2,
                        taskIntCTR, taskSyncPls;
    QString             diClkTerm;
    uInt32              maxMuxedSampPerChan;
    int                 kmux, KAI1, KAI2,
                        kmn1, kma1, kxa1, kxd1,
                        kmn2, kma2, kxa2, kxd2;

public:
    CniAcqDmx( NIReaderWorker *owner, const DAQ::Params &p )
    :   CniAcq( owner, p ),
        AI1(0),     AI2(0),
        taskAI1(0), taskAI2(0),
        taskDI1(0), taskDI2(0),
        taskIntCTR(0), taskSyncPls(0)
        {setDO( false );}
    virtual ~CniAcqDmx();

    virtual void run();

private:
    bool createAITasks(
        const QString   &aiChanStr1,
        const QString   &aiChanStr2 );

    bool createDITasks(
        const QString   &diChanStr1,
        const QString   &diChanStr2 );

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


