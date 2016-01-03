
#ifdef HAVE_NIDAQmx

#include "CniAODmx.h"
#include "AOCtl.h"
#include "Util.h"

#include <QSet>
#include <limits.h>

using namespace DAQ;

/* ---------------------------------------------------------------- */
/* Macros --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define DAQmxErrChk(functionCall)                           \
    do {                                                    \
    if( DAQmxFailed(dmxErrNum = (functionCall)) )           \
        {dmxFnName = STR(functionCall); goto Error_Out;}    \
    } while( 0 )

#define DAQmxErrChkNoJump(functionCall)                     \
    (DAQmxFailed(dmxErrNum = (functionCall)) &&             \
    (dmxFnName = STR(functionCall)))

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static QVector<char>    dmxErrMsg;
static const char       *dmxFnName;
static int32            dmxErrNum;

/* ---------------------------------------------------------------- */
/* clearDmxErrors ------------------------------------------------- */
/* ---------------------------------------------------------------- */

static void clearDmxErrors()
{
    dmxErrMsg.clear();
    dmxFnName   = "";
    dmxErrNum   = 0;
}

/* ---------------------------------------------------------------- */
/* lastDAQErrMsg -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Capture latest dmxErrNum as a descriptive C-string.
// Call as soon as possible after offending operation.
//
static void lastDAQErrMsg()
{
    const int msgbytes = 2048;
    dmxErrMsg.resize( msgbytes );
    dmxErrMsg[0] = 0;
    DAQmxGetExtendedErrorInfo( &dmxErrMsg[0], msgbytes );
}

/* ---------------------------------------------------------------- */
/* destroyTask ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

static void destroyTask( TaskHandle &taskHandle )
{
    if( taskHandle ) {
        DAQmxStopTask( taskHandle );
        DAQmxClearTask( taskHandle );
        taskHandle = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Derived -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CniAODmx::Derived::clear()
{
    aiIndices.clear();
    niChanStr.clear();
    kmux = 1;
}


// Derive the data actually needed within putScans().
// This must be called any time AO parameters change.
//
// - niChanStr is NIDAQ string of form "dev6/ao4,...".
//
// - aiIndices are the indices of the AI-channels to
// copy from the AI data stream to the AO stream.
//
// - kmux is the number replicants of the data to place
// into the AO stream if the AO clock source is running
// kmux times faster than the actual sample rate.
//
bool CniAODmx::Derived::usr2drv( AOCtl *aoC )
{
// --------
// Defaults
// --------

    clear();

// -----------------------
// niChanStr and aiIndices
// -----------------------

    const Params            &p      = aoC->p;
    const AOCtl::User       &usr    = aoC->usr;
    const QMap<uint,uint>   &o2i    = aoC->o2iMap;

    if( !o2i.size() )
        return false;

    int nAOChnOnDevice = CniCfg::aoDevChanCount[usr.devStr];

    if( nAOChnOnDevice ) {

        QSet<int>   uniqueAO;
        QString     basename = QString("%1/ao").arg( usr.devStr );

        aiIndices.reserve( nAOChnOnDevice );

        for( int ko = 0; ko < nAOChnOnDevice; ++ko ) {

            if( o2i.contains( ko ) && !uniqueAO.contains( ko ) ) {

                if( niChanStr.size() )
                    niChanStr += ", ";

                niChanStr += QString("%1%2").arg( basename ).arg( ko );

                aiIndices.push_back( o2i[ko] );
                uniqueAO.insert( ko );
            }
        }
    }

// ------
// volume
// ------

    volume = usr.volume;

// ----
// kmux
// ----

    if( p.ni.isMuxingMode() )
        kmux = p.ni.muxFactor;

    return aiIndices.size() != 0;
}


qint16 CniAODmx::Derived::vol( qint16 val )
{
    return qBound( SHRT_MIN, int(val * volume), SHRT_MAX );
}

/* ---------------------------------------------------------------- */
/* CniAODmx ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

CniAODmx::CniAODmx( AOCtl *owner, const DAQ::Params &p )
    :   CniAO( owner, p ),
        hipass(bq_type_highpass, 0.1 / p.ni.srate),
        lopass(bq_type_lowpass, 9000 / p.ni.srate),
        taskHandle(0)
{
}


CniAODmx::~CniAODmx()
{
    destroyTask( taskHandle );
}


bool CniAODmx::doAutoStart()
{
    QMutexLocker    ml( &owner->aoMtx );

    if( owner->usr.autoStart ) {

        QString err;

        if( owner->valid( err ) )
            return true;
        else
            Error() << "AO autostart failed because [" << err << "].";
    }

    return false;
}


bool CniAODmx::readyForScans()
{
    QMutexLocker    ml( &owner->aoMtx );
    return taskHandle != 0;
}


// Note: The actual clock signal should be the same for AO & AI.
// For multiplexed modes that clock will be running (muxFactor)
// faster than the true sample rate p.ai.srate. Therefore, in those
// modes extra samples must be replicated (see extractAOChans).
//
bool CniAODmx::devStart()
{
    QMutexLocker    ml( &owner->aoMtx );

    if( !drv.usr2drv( owner ) )
        return false;

    clearDmxErrors();

    const AOCtl::User   &usr = owner->usr;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskHandle ) )
     || DAQmxErrChkNoJump( DAQmxCreateAOVoltageChan(
                            taskHandle,
                            STR2CHR( drv.niChanStr ),
                            "",
                            usr.range.rmin,
                            usr.range.rmax,
                            DAQmx_Val_Volts,
                            NULL ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            taskHandle,
                            (usr.isClockInternal() ?
                                "Ctr0InternalOutput" :
                                STR2CHR( usr.aoClockStr )),
                            1,  // Not applicable for output
                            DAQmx_Val_Rising,
                            DAQmx_Val_ContSamps,
                            0 ) )
     || DAQmxErrChkNoJump( DAQmxCfgOutputBuffer(
                            taskHandle,
                            2.0
                                * owner->bufSecs()
                                * p.ni.srate * drv.kmux ) ) ) {

        if( DAQmxFailed( dmxErrNum ) )
            lastDAQErrMsg();

        destroyTask( taskHandle );

        if( DAQmxFailed( dmxErrNum ) ) {

            QString e = QString("DAQmx Error: Fun=<%1> Err=<%2> [%3].")
                        .arg( dmxFnName )
                        .arg( dmxErrNum )
                        .arg( &dmxErrMsg[0] );

            Error() << e;
        }

        return false;
    }

// We don't have a good way to set the following attributes
// with real error checking enabled. Just set them and hope.

    DAQmxSetWriteRegenMode(
        taskHandle,
        DAQmx_Val_DoNotAllowRegen );

    DAQmxSetImplicitUnderflowBehavior(
        taskHandle,
        DAQmx_Val_PauseUntilDataAvailable );

    taskRunT    = getTime();
    errCnt      = 0;

    return true;
}


void CniAODmx::devStop()
{
    QMutexLocker    ml( &owner->aoMtx );
    destroyTask( taskHandle );
}


void CniAODmx::putScans( vec_i16 &aiData )
{
    QMutexLocker    ml( &owner->aoMtx );

    if( !taskHandle )
        return;

// A periodic restart bombs latency back to the minimum

    const double    latencyResetSecs = 20.0;

    if( getTime() - taskRunT > latencyResetSecs )
        owner->restart();

    vec_i16 aoData;
    int32   nWrite = extractAOChans( aoData, aiData ),
            nWritten;

    if( DAQmxFailed( DAQmxWriteBinaryI16(
                        taskHandle,
                        nWrite,
                        true,   // autostart
                        2.0,    // timeout seconds
                        DAQmx_Val_GroupByScanNumber,
                        &aoData[0],
                        &nWritten,
                        NULL ) ) ) {

//        lastDAQErrMsg();
//        Error() << "AO Error [" << &dmxErrMsg[0] << "]";

        // Principal failure mode is that we were late fetching a block
        // and feeding more samples. That's not a show stopper. StopTask
        // resets the error state and the next Write call restarts it.
        // If persistent, though, we restart everything.

        if( ++errCnt < 5 )
            DAQmxStopTask( taskHandle );
        else {
            Debug() << "AOTask restart";
            owner->restart();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// 1) Apply bandpass [0.1 .. 9K]Hz filter if selected.
//
// 2) From the full aiData packet, select only channels used
// for AO, and replicate them kmux times to account for higher
// muxing clock rate. Packing is DAQmx_Val_GroupByScanNumber.
//
// 3) Return number of samples = ntpts * kmux.
//
int CniAODmx::extractAOChans( vec_i16 &aoData, vec_i16 &aiData )
{
    int nao     = drv.aiIndices.size(),
        n16     = p.ni.niCumTypCnt[CniCfg::niSumAll],
        ntpts   = int(aiData.size() / n16);

// ------
// Filter
// ------

    if( owner->usr.filter ) {

        int nNeural = p.ni.niCumTypCnt[CniCfg::niSumNeural];

        for( int i = 0; i < nao; ++i ) {

            int iai = drv.aiIndices[i];

            if( iai < nNeural ) {
                hipass.apply1BlockwiseMem1( &aiData[0], ntpts, n16, iai );
                lopass.apply1BlockwiseMem1( &aiData[0], ntpts, n16, iai );
            }
        }
    }

// ----------------
// Extract / expand
// ----------------

    aoData.resize( ntpts * drv.kmux * nao );

    // for each output channel
    for( int iao = 0; iao < nao; ++iao ) {

        const qint16    *src    = &aiData[drv.aiIndices[iao]];
        qint16          *dst    = &aoData[iao];

        // for each timepoint
        for( int t = 0; t < ntpts; ++t, src += n16 ) {

            qint16  v1 = *src, v2;

            *dst = drv.vol( v1 );
            dst += nao;

            if( t >= ntpts - 1 || (v2 = src[n16]) == v1 ) {
                // flat line
                for( int i = 1; i < drv.kmux; ++i ) {
                    *dst = drv.vol( v1 );
                    dst += nao;
                }
            }
            else {
                // sloped line
                float	slope = float(v2 - v1) / drv.kmux;
                for( int i = 1; i < drv.kmux; ++i ) {
                    *dst = drv.vol( qint16(v1 + i * slope) );
                    dst += nao;
                }
            }
        }
    }

    return ntpts * drv.kmux;
}

#endif  // HAVE_NIDAQmx


