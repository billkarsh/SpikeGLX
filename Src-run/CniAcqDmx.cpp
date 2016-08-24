
#ifdef HAVE_NIDAQmx

#include "CniAcqDmx.h"
#include "Util.h"
#include "Subset.h"

//#define PERFMON
#if defined(PERFMON)
#include <windows.h>
#include <psapi.h>
#endif


#define DAQ_TIMEOUT_SEC     2.5

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
/* aiChanString --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Compose NIDAQ channel string of form "dev6/ai4, ...".
// Return channel count.
//
static int aiChanString(
    QString             &str,
    const QString       &dev,
    const QVector<uint> &chnVec )
{
    str.clear();

    int nc = chnVec.size();

    if( nc ) {
        QString basename = QString("%1/ai").arg( dev );
        str = QString("%1%2").arg( basename ).arg( chnVec[0] );
        for( int ic = 1; ic < nc; ++ic )
            str += QString(", %1%2").arg( basename ).arg( chnVec[ic] );
    }

    return nc;
}

/* ---------------------------------------------------------------- */
/* diChanString --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Compose NIDAQ channel string of form "dev6/line4,...".
// Return channel (a.k.a. line or bit) count.
//
static int diChanString(
    QString             &str,
    const QString       &dev,
    const QVector<uint> &chnVec )
{
    str.clear();

    int nc = chnVec.size();

    if( nc ) {
        QString basename = QString("%1/line").arg( dev );
        str = QString("%1%2").arg( basename ).arg( chnVec[0] );
        for( int ic = 1; ic < nc; ++ic )
            str += QString(", %1%2").arg( basename ).arg( chnVec[ic] );
    }

    return nc;
}

/* ---------------------------------------------------------------- */
/* demuxMerge ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// - Merge data from 2 devices.
// - Group by whole timepoints.
// - Subgroup (mn0 | mn1 |...| ma0 | ma1 |...| xa | xd).
// - Average oversampled xa chans.
// - Downsample oversampled xd and pack bytes into low-order bits.
//
static void demuxMerge(
    vec_i16                     &merged,
    const vec_i16               &rawAI1,
    const vec_i16               &rawAI2,
    const std::vector<uInt8>    &rawDI1,
    const std::vector<uInt8>    &rawDI2,
    int                         nwhole,
    int                         kmux,
    int                         kmn1,
    int                         kma1,
    int                         kxa1,
    int                         kxd1,
    int                         kmn2,
    int                         kma2,
    int                         kxa2,
    int                         kxd2 )
{
    qint16          *dst    = &merged[0];
    const qint16    *sA1    = (rawAI1.size() ? &rawAI1[0] : 0),
                    *sA2    = (rawAI2.size() ? &rawAI2[0] : 0);
    const uInt8     *sD1    = (kxd1 ? &rawDI1[0] : 0),
                    *sD2    = (kxd2 ? &rawDI2[0] : 0);

// ----------
// Not muxing
// ----------

    if( kmux == 1 ) {

        for( int w = 0; w < nwhole; ++w ) {

            // Copy XA

            if( kxa1 ) {
                memcpy( dst, sA1, kxa1*sizeof(qint16) );
                dst += kxa1;
                sA1 += kxa1;
            }

            if( kxa2 ) {
                memcpy( dst, sA2, kxa2*sizeof(qint16) );
                dst += kxa2;
                sA2 += kxa2;
            }

            // Copy XD

            int     ibit    = -1;
            quint16 W       = 0;
            bool    hasBits = false;

            for( int i = 0; i < kxd1; ++i ) {

                if( ++ibit > 15 ) {
                    *dst++  = W;
                    ibit    = 0;
                    W       = 0;
                    hasBits = false;
                }

                W |= quint16(*sD1++) << ibit;
                hasBits = true;
            }

            for( int i = 0; i < kxd2; ++i ) {

                if( ++ibit > 15 ) {
                    *dst++  = W;
                    ibit    = 0;
                    W       = 0;
                    hasBits = false;
                }

                W |= quint16(*sD2++) << ibit;
                hasBits = true;
            }

            if( hasBits )
                *dst++ = W;
        }

        return;
    }

// ------
// Muxing
// ------

// In each timepoint the muxed channels form a matrix. As acquired,
// each column is a muxer (so, ncol = kmn1 + kmn2 + kma1 + kma2).
// There are kmux rows. We will transpose this matrix so that all
// the samples from a given muxer are together. In each timepoint
// the xa values are oversampled by kmux, so we will average them.

    int     ncol    = kmn1 + kmn2 + kma1 + kma2,
            nrow    = kmux,
            ntmp    = nrow * ncol;
    vec_i16 vtmp( ntmp );

    for( int w = 0; w < nwhole; ++w ) {

        std::vector<long>   sumxa1( kxa1, 0 ),
                            sumxa2( kxa2, 0 );
        qint16              *tmp = &vtmp[0];

        for( int s = 0; s < kmux; ++s ) {

            // Fill MN, MA matrix

            if( kmn1 ) {
                memcpy( tmp, sA1, kmn1*sizeof(qint16) );
                tmp += kmn1;
                sA1 += kmn1;
            }

            if( kmn2 ) {
                memcpy( tmp, sA2, kmn2*sizeof(qint16) );
                tmp += kmn2;
                sA2 += kmn2;
            }

            if( kma1 ) {
                memcpy( tmp, sA1, kma1*sizeof(qint16) );
                tmp += kma1;
                sA1 += kma1;
            }

            if( kma2 ) {
                memcpy( tmp, sA2, kma2*sizeof(qint16) );
                tmp += kma2;
                sA2 += kma2;
            }

            // Sum XA

            for( int x = 0; x < kxa1; ++x )
                sumxa1[x] += *sA1++;

            for( int x = 0; x < kxa2; ++x )
                sumxa2[x] += *sA2++;
        }

        // Transpose and store MN, MA:
        // Original element address is [ncol*Y + X].
        // Swap roles X <-> Y and row <-> col.

        for( int iacq = 0; iacq < ntmp; ++iacq ) {

            int Y = iacq / ncol,
                X = iacq - ncol * Y;

            dst[nrow*X + Y] = vtmp[iacq];
        }

        dst += ntmp;

        // Copy XA averages

        for( int x = 0; x < kxa1; ++x )
            *dst++ = sumxa1[x] / kmux;

        for( int x = 0; x < kxa2; ++x )
            *dst++ = sumxa2[x] / kmux;

        // Copy XD

        int     ibit    = -1;
        quint16 W       = 0;
        bool    hasBits = false;

        for( int i = 0; i < kxd1; ++i ) {

            if( ++ibit > 15 ) {
                *dst++  = W;
                ibit    = 0;
                W       = 0;
                hasBits = false;
            }

            W |= quint16(*sD1++) << ibit;
            hasBits = true;
        }

        for( int i = 0; i < kxd2; ++i ) {

            if( ++ibit > 15 ) {
                *dst++  = W;
                ibit    = 0;
                W       = 0;
                hasBits = false;
            }

            W |= quint16(*sD2++) << ibit;
            hasBits = true;
        }

        if( hasBits )
            *dst++ = W;

        // Eat extra XD

        if( kxd1 )
            sD1 += (kmux - 1) * kxd1;

        if( kxd2 )
            sD2 += (kmux - 1) * kxd2;
    }
}

/* ---------------------------------------------------------------- */
/* ~CniAcqDmx ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

CniAcqDmx::~CniAcqDmx()
{
    setDO( false );
    destroyTasks();
}

/* ---------------------------------------------------------------- */
/* run ------------------------------------------------------------ */
/* ---------------------------------------------------------------- */

/*  DAQ Strategy
    ------------
    (1) Task Configuration

    DAQmx will be configured for triggered+buffered analog input
    using DAQmxCfgSampClkTiming API. The interesting parameters:

    - source + activeEdge:
    An external train of clock pulses is applied here and we'll set
    the rising edges to command acquisition of one sample from each
    listed ai channel. This triggering clock signal is typically
    generated by the muxing micro-controller. The identical signal
    must be applied to each participating NI device for proper
    synchronization. This signal determines the acquisition rate.
    The 'rate' function parameter is ONLY used for buffer sizing
    (see below).

    - sampleMode:
    Set DAQmx_Val_ContSamps for continuous buffered acq on every
    rising edge.

    - rate + sampsPerChanToAcquire:
    DAQmx uses the rate slot (our high estimate of actual rate)
    to make sure that the buffer size we actually specify in the
    sampsPerChanToAcquire slot is at least as large as they think
    is prudent. We will set rate=1, a tiny but legal value, just
    to emphasize that WE are taking full responsibility for sizing;
    and we will be more conservative than NI. Note that the buffer
    size is specified in units of samplesPerChan. We must guess a
    worst-case latency: (by how many seconds might the sample
    fetching thread lag?) The correct size is then:

        maxSampPerChan      = latency-secs * samples/sec.
        maxMuxedSampPerChan = kmux * maxSampPerChan.

    In practice we find interruptions lasting a second or more.

    (2) Sample Fetching
    Here the workhorse will be the DAQmxReadBinaryI16 API. This
    read operation offers two usage modes (both needed):

    - Get a specified number of integral samples.
    - Get all available integral samples.

    We'll be calling this function quasi-periodically to retrieve
    samples but because of potential latency we can not use a fixed
    request size. Rather, we will always ask for everything on dev1
    to prevent buffer overflow. Data fidelity depends on good time
    stamps (really, sample index) so if we are also using dev2 then
    we will use the actual sample count from dev1 to get that many
    samples from dev2. At first one might suspect that dev2 could
    overflow because dev2 fetches might be smaller than the actual
    dev2 count. However, dev1 & dev2 share the same clock so the
    dev1 count is a reliable proxy for dev2. Any supposed extras
    in dev2 are actually also in dev1 and we will sweep them out
    on the next cycle.

    The read function is nice about delivering back whole 'samples'
    but remember that the number of data points in a NIDAQ sample
    is effectively characterized as:

        Nmuxer = # physical NIDAQ lines.

    This does not account for muxing. Rather, we must maintain
    whole timepoints, each having:

        Nchan = Nmuxer * channels/muxer.

    The read function knows nothing of muxing so may well deliver data
    for partial timepoints. It will fall to us to reassemble timepoints
    manually. I'll manage that in a simple way, sizing a fetch buffer to
    hold maxMuxedSampPerChan, plus 1 extra timepoint. On each read I'll
    track any fractional timepoint tail. On the next read we will slide
    that fraction forward and append new fetched data to that.

    Digital Input
    -------------
    We read the data with DAQmxReadDigitalLines which gets one byte
    per channel per sample. This will allow simple repacking of data
    as 16bit words with one bit per channel per sample. We will assign
    the first acquired line to bit 0, the next line to bit 1, and so on
    regardless of line index number, though order is preserved.

    Another reason we choose this instead of DAQmxReadDigitalU16 is
    the latter says it can be used for ports having up to 16 lines.
    We believe the former can be used across port boundaries without
    regard for line/port counts.
*/

void CniAcqDmx::run()
{
// ---------
// Configure
// ---------

    if( !configure() )
        return;

// -------
// Buffers
// -------

// IMPORTANT
// ---------
// Any of the raw buffers may get zero allocated size if no channels
// of that type were selected...so never access &rawXXX[0] without
// testing array size.

    vec_i16 rawAI1( (kMuxedSampPerChan + kmux)*KAI1 ),
            rawAI2( (kMuxedSampPerChan + kmux)*KAI2 ),
            merged( maxSampPerChan*(
                        kmux*(kmn1+kma1+kmn2+kma2)
                        + kxa1+kxa2
                        + (kxd1+kxd2 + 15)/16
                        ) );
    std::vector<uInt8>  rawDI1( (kMuxedSampPerChan + kmux)*kxd1 ),
                        rawDI2( (kMuxedSampPerChan + kmux)*kxd2 );

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( !startTasks() ) {
        runError();
        return;
    }

    if( p.ni.syncEnable )
        setDO( true );

// -----
// Fetch
// -----

    const int loopPeriod_us = 1000 * daqAIFetchPeriodMillis();

    double  peak_loopT  = 0;
    int32   nFetched;
    int     peak_nWhole = 0,
            nWhole      = 0,
            rem         = 0,
            remFront    = true,
            nTries      = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        nWhole = 0;

        // Slide partial timepoint forward.
        //
        // The purpose of remFront is to prevent sliding again
        // next time, just in case we got no samples this time.
        // In reality, if we actually had any rem (meaning acq
        // is in progress) and we got no samples, then we have
        // much bigger problems than that.

        if( rem && !remFront ) {

            if( KAI1 ) {
                memcpy(
                    &rawAI1[0],
                    &rawAI1[(nFetched-rem)*KAI1],
                    rem*KAI1*sizeof(qint16) );
            }

            if( kxd1 ) {
                memcpy(
                    &rawDI1[0],
                    &rawDI1[(nFetched-rem)*kxd1],
                    rem*kxd1*sizeof(uInt8) );
            }

            if( KAI2 ) {
                memcpy(
                    &rawAI2[0],
                    &rawAI2[(nFetched-rem)*KAI2],
                    rem*KAI2*sizeof(qint16) );
            }

            if( kxd2 ) {
                memcpy(
                    &rawDI2[0],
                    &rawDI2[(nFetched-rem)*kxd2],
                    rem*kxd2*sizeof(uInt8) );
            }

            remFront = true;
        }

        // Fetch ALL dev1 samples, appending them to rem.
        // The first used channel type on dev1 sets nFetched,
        // and that specifies the fetch count for other reads.

        nFetched = 0;

        if( KAI1 ) {

            DAQmxErrChk( DAQmxReadBinaryI16(
                            taskAI1,
                            DAQmx_Val_Auto,
                            DAQ_TIMEOUT_SEC,
                            DAQmx_Val_GroupByScanNumber,
                            &rawAI1[rem*KAI1],
                            (kMuxedSampPerChan+kmux-rem)*KAI1,
                            &nFetched,
                            NULL ) );

            if( !nFetched )
                goto next_fetch;
        }

        if( kxd1 ) {

            DAQmxErrChk( DAQmxReadDigitalLines(
                            taskDI1,
                            (nFetched ? nFetched : DAQmx_Val_Auto),
                            DAQ_TIMEOUT_SEC,
                            DAQmx_Val_GroupByScanNumber,
                            &rawDI1[rem*kxd1],
                            (kMuxedSampPerChan+kmux-rem)*kxd1,
                            &nFetched,
                            NULL,
                            NULL ) );

            if( !nFetched )
                goto next_fetch;
        }

        // Fetch nFetched dev2 samples
        // Append to rem

        if( p.ni.isDualDevMode ) {

            int32   nFetched2;

            if( KAI2 ) {

                DAQmxErrChk( DAQmxReadBinaryI16(
                                taskAI2,
                                nFetched,
                                DAQ_TIMEOUT_SEC,
                                DAQmx_Val_GroupByScanNumber,
                                &rawAI2[rem*KAI2],
                                (kMuxedSampPerChan+kmux-rem)*KAI2,
                                &nFetched2,
                                NULL ) );

                if( nFetched2 != nFetched )
                    Warning() << "Detected dev2-dev1 analog phase shift.";
            }

            if( kxd2 ) {

                DAQmxErrChk( DAQmxReadDigitalLines(
                                taskDI2,
                                nFetched,
                                DAQ_TIMEOUT_SEC,
                                DAQmx_Val_GroupByScanNumber,
                                &rawDI2[rem*kxd2],
                                (kMuxedSampPerChan+kmux-rem)*kxd2,
                                &nFetched2,
                                NULL,
                                NULL ) );

                if( nFetched2 != nFetched )
                    Warning() << "Detected dev2-dev1 digital phase shift.";
            }
        }

        // ---------------
        // Update counters
        // ---------------

        nFetched += rem;
        nWhole    = nFetched / kmux;

        if( nWhole ) {
            rem         = nFetched - kmux * nWhole;
            remFront    = false;
        }
        else {
            rem         = nFetched;
            remFront    = true;
        }

        // ---------
        // MEM usage
        // ---------

#if defined(PERFMON)
{
    static double   lastMonT = 0;
    if( loopT - lastMonT > 0.1 ) {
        PROCESS_MEMORY_COUNTERS info;
        GetProcessMemoryInfo( GetCurrentProcess(), &info, sizeof(info) );
        Log()
            << "nWhole= " << nWhole
            << " curMB= " << info.WorkingSetSize / (1024*1024)
            << " peakMB= " << info.PeakWorkingSetSize / (1024*1024);
        lastMonT = loopT;
    }
}
#endif

        // ---------------
        // Process samples
        // ---------------

        if( nWhole ) {

            if( nWhole > peak_nWhole )
                peak_nWhole = nWhole;

            // ---------------
            // Demux and merge
            // ---------------

            demuxMerge(
                merged,
                rawAI1, rawAI2,
                rawDI1, rawDI2,
                nWhole, kmux,
                kmn1, kma1, kxa1, kxd1,
                kmn2, kma2, kxa2, kxd2 );

            // -------
            // Publish
            // -------

            owner->niQ->enqueue( merged, nWhole, totalTPts );
            totalTPts += nWhole;
        }

        // ------------------
        // Handle empty fetch
        // ------------------

        // Allow retries in case of empty fetches. With USB
        // devices empty fetches happen routinely even at
        // high sample rates, and may have to do with packet
        // boundaries. Also, very low sample rates can cause
        // gaps. We choose 1100 retries to accommodate 0.5Hz
        // sample rate and higher. That many loop iterations
        // is still only about 1.1 seconds.

next_fetch:
        if( !nWhole ) {

            if( ++nTries > 1100 ) {
                Error() << "DAQ NIReader getting no samples.";
                goto Error_Out;
            }
        }
        else
            nTries = 0;

        // ---------------
        // Loop moderation
        // ---------------

        // Here we moderate the loop speed to make fetches more
        // or less the same size (loopPeriod_us per iteration).
        // We also generate some diagnostic timing stats. Note
        // that we measure things in microseconds for precision
        // but report in milliseconds for comprehension.

        loopT = 1e6*(getTime() - loopT);    // microsec

        if( loopT > peak_loopT )
            peak_loopT = loopT;

        if( loopT < loopPeriod_us )
            usleep( loopPeriod_us - loopT );

        // ---------------
        // Rate statistics
        // ---------------

#if 0
        QString stats =
            QString(
            "DAQ rate S/max/millis %1/%2/%3"
            "   peak S/millis %4/%5")
            .arg( nWhole, 6 )
            .arg( maxSampPerChan, 6 )
            .arg( loopT/1000, 10, 'f', 2 );
            .arg( peak_nWhole, 6 )
            .arg( peak_loopT/1000, 10, 'f', 2 );
        Log() << stats;
#endif
    }

// ----
// Exit
// ----

Error_Out:
    runError();

    setDO( false );
}

/* ---------------------------------------------------------------- */
/* setDO ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CniAcqDmx::setDO( bool onoff )
{
    QString err = CniCfg::setDO( p.ni.syncLine, onoff );

    if( !err.isEmpty() )
        emit owner->daqError( err );
}

/* ---------------------------------------------------------------- */
/* createAITasks -------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CniAcqDmx::createAITasks(
    const QString   &aiChanStr1,
    const QString   &aiChanStr2,
    quint32         maxMuxedSampPerChan )
{
    taskAI1 = 0,
    taskAI2 = 0;

    if( aiChanStr1.isEmpty() )
        goto device2;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskAI1 ) )
     || DAQmxErrChkNoJump( DAQmxCreateAIVoltageChan(
                            taskAI1,
                            STR2CHR( aiChanStr1 ),
                            "",
                            p.ni.termCfg,
                            p.ni.range.rmin,
                            p.ni.range.rmax,
                            DAQmx_Val_Volts,
                            NULL ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            taskAI1,
                            (p.ni.isClock1Internal() ?
                                "Ctr0InternalOutput" :
                                STR2CHR( p.ni.clockStr1 )),
                            1,  // smallest legal value
                            DAQmx_Val_Rising,
                            DAQmx_Val_ContSamps,
                            maxMuxedSampPerChan ) )
     || DAQmxErrChkNoJump( DAQmxTaskControl(
                            taskAI1,
                            DAQmx_Val_Task_Commit ) ) ) {

        return false;
    }

// BK: Note this property for future work on glitch recovery.
// BK: Also note this: DAQmx_Val_OverwriteUnreadSamps
//    DAQmxSetSampClkOverrunBehavior( taskAI1, DAQmx_Val_IgnoreOverruns );

device2:

    if( aiChanStr2.isEmpty() )
        return true;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskAI2 ) )
     || DAQmxErrChkNoJump( DAQmxCreateAIVoltageChan(
                            taskAI2,
                            STR2CHR( aiChanStr2 ),
                            "",
                            p.ni.termCfg,
                            p.ni.range.rmin,
                            p.ni.range.rmax,
                            DAQmx_Val_Volts,
                            NULL ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            taskAI2,
                            STR2CHR( p.ni.clockStr2 ),
                            1,  // smallest legal value
                            DAQmx_Val_Rising,
                            DAQmx_Val_ContSamps,
                            maxMuxedSampPerChan ) )
     || DAQmxErrChkNoJump( DAQmxTaskControl(
                            taskAI2,
                            DAQmx_Val_Task_Commit ) ) ) {

        return false;
    }

// BK: Note this property for future work on glitch recovery.
//    DAQmxSetSampClkOverrunBehavior( taskAI2, DAQmx_Val_IgnoreOverruns );

    return true;
}

/* ---------------------------------------------------------------- */
/* createDITasks -------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CniAcqDmx::createDITasks(
    const QString   &diChanStr1,
    const QString   &diChanStr2,
    quint32         maxMuxedSampPerChan )
{
    taskDI1 = 0,
    taskDI2 = 0;

    if( diChanStr1.isEmpty() )
        goto device2;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskDI1 ) )
     || DAQmxErrChkNoJump( DAQmxCreateDIChan(
                            taskDI1,
                            STR2CHR( diChanStr1 ),
                            "",
                            DAQmx_Val_ChanForAllLines ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            taskDI1,
                            (p.ni.isClock1Internal() ?
                                "Ctr0InternalOutput" :
                                STR2CHR( p.ni.clockStr1 )),
                            1,  // smallest legal value
                            DAQmx_Val_Rising,
                            DAQmx_Val_ContSamps,
                            maxMuxedSampPerChan ) )
     || DAQmxErrChkNoJump( DAQmxTaskControl(
                            taskDI1,
                            DAQmx_Val_Task_Commit ) ) ) {

        return false;
    }

// BK: Note this property for future work on glitch recovery.
//    DAQmxSetSampClkOverrunBehavior( taskDI1, DAQmx_Val_IgnoreOverruns );

device2:

    if( diChanStr2.isEmpty() )
        return true;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskDI2 ) )
     || DAQmxErrChkNoJump( DAQmxCreateDIChan(
                            taskDI2,
                            STR2CHR( diChanStr2 ),
                            "",
                            DAQmx_Val_ChanForAllLines ) )
     || DAQmxErrChkNoJump( DAQmxCfgSampClkTiming(
                            taskDI2,
                            STR2CHR( p.ni.clockStr2 ),
                            1,  // smallest legal value
                            DAQmx_Val_Rising,
                            DAQmx_Val_ContSamps,
                            maxMuxedSampPerChan ) )
     || DAQmxErrChkNoJump( DAQmxTaskControl(
                            taskDI2,
                            DAQmx_Val_Task_Commit ) ) ) {

        return false;
    }

// BK: Note this property for future work on glitch recovery.
//    DAQmxSetSampClkOverrunBehavior( taskDI2, DAQmx_Val_IgnoreOverruns );

    return true;
}

/* ---------------------------------------------------------------- */
/* createCTRTask -------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CniAcqDmx::createCTRTask()
{
    taskCTR = 0;

    if( !p.ni.isClock1Internal() )
        return true;

    if( DAQmxErrChkNoJump( DAQmxCreateTask( "", &taskCTR ) )
     || DAQmxErrChkNoJump( DAQmxCreateCOPulseChanFreq(
                            taskCTR,
                            STR2CHR( QString("%1/ctr0").arg( p.ni.dev1 ) ),
                            "",
                            DAQmx_Val_Hz,
                            DAQmx_Val_Low,
                            0.0,
                            p.ni.srate,
                            0.5 ) )
     || DAQmxErrChkNoJump( DAQmxCfgImplicitTiming(
                            taskCTR,
                            DAQmx_Val_ContSamps,
                            1 ) )   // not used
     || DAQmxErrChkNoJump( DAQmxTaskControl(
                            taskCTR,
                            DAQmx_Val_Task_Commit ) ) ) {

        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* configure ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool CniAcqDmx::configure()
{
    const double    lateSecs = 2.5;  // worst expected latency

    QString aiChanStr1, aiChanStr2,
            diChanStr1, diChanStr2;

    clearDmxErrors();

    kmux = (p.ni.isMuxingMode() ? p.ni.muxFactor : 1);

    maxSampPerChan      = uInt32(lateSecs * p.ni.srate);
    kMuxedSampPerChan   = kmux * maxSampPerChan;

// ----------------------------------------
// Channel types, counts and NI-DAQ strings
// ----------------------------------------

    // temporary use of vc
    {
        QVector<uint>   vc;

        Subset::rngStr2Vec( vc, p.ni.uiMNStr1 );
        kmn1 = vc.size();

        Subset::rngStr2Vec( vc, p.ni.uiMAStr1 );
        kma1 = vc.size();

        Subset::rngStr2Vec( vc, p.ni.uiXAStr1 );
        kxa1 = vc.size();

        Subset::rngStr2Vec( vc,
            QString("%1,%2,%3")
                .arg( p.ni.uiMNStr1 )
                .arg( p.ni.uiMAStr1 )
                .arg( p.ni.uiXAStr1 ) );

        KAI1 = aiChanString( aiChanStr1, p.ni.dev1, vc );

        Subset::rngStr2Vec( vc, p.ni.uiXDStr1 );
        kxd1 = diChanString( diChanStr1, p.ni.dev1, vc );

        Subset::rngStr2Vec( vc, p.ni.uiMNStr2() );
        kmn2 = vc.size();

        Subset::rngStr2Vec( vc, p.ni.uiMAStr2() );
        kma2 = vc.size();

        Subset::rngStr2Vec( vc, p.ni.uiXAStr2() );
        kxa2 = vc.size();

        Subset::rngStr2Vec( vc,
            QString("%1,%2,%3")
                .arg( p.ni.uiMNStr2() )
                .arg( p.ni.uiMAStr2() )
                .arg( p.ni.uiXAStr2() ) );

        KAI2 = aiChanString( aiChanStr2, p.ni.dev2, vc );

        Subset::rngStr2Vec( vc, p.ni.uiXDStr2() );
        kxd2 = diChanString( diChanStr2, p.ni.dev2, vc );
    }

// ----------
// Task setup
// ----------

    if( p.ni.syncEnable ) {
        setDO( false );
        msleep( 1000 );
    }

    if( !createCTRTask() ) {
        runError();
        return false;
    }

    if( !createAITasks( aiChanStr1, aiChanStr2, kMuxedSampPerChan ) ) {
        runError();
        return false;
    }

    if( !createDITasks( diChanStr1, diChanStr2, kMuxedSampPerChan ) ) {
        runError();
        return false;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* startTasks ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Start slaves then masters.
//
bool CniAcqDmx::startTasks()
{
    if( p.ni.isDualDevMode ) {

        if( taskDI2 && DAQmxErrChkNoJump( DAQmxStartTask( taskDI2 ) ) )
            return false;

        if( taskAI2 && DAQmxErrChkNoJump( DAQmxStartTask( taskAI2 ) ) )
            return false;
    }

    if( taskDI1 && DAQmxErrChkNoJump( DAQmxStartTask( taskDI1 ) ) )
        return false;

    if( taskAI1 && DAQmxErrChkNoJump( DAQmxStartTask( taskAI1 ) ) )
        return false;

    if( taskCTR && DAQmxErrChkNoJump( DAQmxStartTask( taskCTR ) ) )
        return false;

    return true;
}

/* ---------------------------------------------------------------- */
/* destroyTasks --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CniAcqDmx::destroyTasks()
{
    destroyTask( taskCTR );
    destroyTask( taskAI1 );
    destroyTask( taskDI1 );
    destroyTask( taskAI2 );
    destroyTask( taskDI2 );
}

/* ---------------------------------------------------------------- */
/* runError ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CniAcqDmx::runError()
{
    if( DAQmxFailed( dmxErrNum ) )
        lastDAQErrMsg();

    destroyTasks();

    if( DAQmxFailed( dmxErrNum ) ) {

        QString e = QString("DAQmx Error:\nFun=<%1>\n").arg( dmxFnName );
        e += QString("ErrNum=<%1>\n").arg( dmxErrNum );
        e += QString("ErrMsg='%1'.").arg( &dmxErrMsg[0] );

        Error() << e;

        emit owner->daqError( e );
    }
    else if( !isStopped() )
        emit owner->daqError( "No NI-DAQ samples fetched." );
}

#endif  // HAVE_NIDAQmx


