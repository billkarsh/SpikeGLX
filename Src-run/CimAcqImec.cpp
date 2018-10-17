
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDir>
#include <QThread>
#include <math.h>


// User manual Sec 5.7 "Probe signal offset" says value [0.6 .. 0.7]
// T0FUDGE used to sync IM and NI stream tZero values.
// TPNTPERFETCH reflects the AP/LF sample rate ratio.
#define MAX10BIT        512
#define OFFSET          0.6F
#define T0FUDGE         0.0
#define TPNTPERFETCH    12
#define MAXE            16
#define LOOPSECS        0.004
#define PROFILE
//#define TUNE


/* ---------------------------------------------------------------- */
/* CimAcqImec ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq( owner, p ), nE(0),
        paused(false), pauseAck(false), zeroFill(false)
{
    E.resize( MAXE );
}

/* ---------------------------------------------------------------- */
/* ~CimAcqImec ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::~CimAcqImec()
{
    close();
}

/* ---------------------------------------------------------------- */
/* run ------------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimAcqImec::run()
{
// ---------
// Configure
// ---------

    zeroFill = false;
    setPauseAck( false );

// Hardware

    if( !configure() )
        return;

// -------
// Buffers
// -------

    const int
        apPerTpnt   = p.im.imCumTypCnt[CimCfg::imSumAP],
        lfPerTpnt   = p.im.imCumTypCnt[CimCfg::imSumNeural]
                        - p.im.imCumTypCnt[CimCfg::imSumAP],
        syPerTpnt   = 1,
        chnPerTpnt  = (apPerTpnt + lfPerTpnt + syPerTpnt);

    QVector<float>  lfLast( lfPerTpnt, 0.0F );
    vec_i16         i16Buf( MAXE * TPNTPERFETCH * chnPerTpnt );
    float           *pLfLast = &lfLast[0];

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( !startAcq() )
        return;

// -----
// Fetch
// -----

// Minus here ala User manual 5.7 "Probe signal inversion"

    const float yscl = -MAX10BIT/0.6F;

    double  tPreEnq = 0, tPostEnq = 0;

#ifdef PROFILE
    double  dtScl, sumGet = 0, sumScl = 0, sumEnq = 0;

    // Table header, see profile discussion below

    Log() <<
        QString("Required loop ms < [[ %1 ]] n > [[ %2 ]] MAXE %3")
        .arg( 1000*MAXE*TPNTPERFETCH/p.im.srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.srate/(MAXE*TPNTPERFETCH) ) )
        .arg( MAXE );
#endif

    double  startT      = getTime(),
            lastCheckT  = startT,
            peak_loopT  = 0,
            sumdT       = 0,
            dT;
    int     ndT         = 0;

    while( !isStopped() ) {

        double  loopT = getTime();

        // -----
        // Fetch
        // -----

        if( !fetchE() )
            return;

#ifdef TUNE
        // Tune LOOPSECS and MAXE
        static QVector<uint> pkthist( 1 + MAXE, 0 );  // 0 + [1..MAXE]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- nom %1  max %2")
                .arg( LOOPSECS*p.im.srate/TPNTPERFETCH )
                .arg( MAXE );
            for( int i = 0; i <= MAXE; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.fill( 0, 1 + MAXE );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[nE];
#endif

        // ------------------
        // Handle empty fetch
        // ------------------

        if( !nE ) {

            // BK: Allow up to 5 seconds for (external) trigger.
            // BK: Tune with experience.

            if( !totPts && loopT - startT >= 5.0 ) {
                runError( "IMReader getting no samples." );
                return;
            }

            goto next_fetch;
        }

#ifdef PROFILE
        sumGet += getTime() - loopT;
#endif

        // -----------
        // E -> i16Buf
        // -----------

#ifdef PROFILE
        dtScl = getTime();
#endif

        for( int ie = 0; ie < nE; ++ie ) {

            const ElectrodePacket   &src = E[ie];

            qint16  *dst = &i16Buf[TPNTPERFETCH*ie*chnPerTpnt];

            for( int it = 0; it < TPNTPERFETCH; ++it ) {

                // ----------
                // ap - as is
                // ----------

                const float *AP = src.apData[it];

                for( int ap = 0; ap < apPerTpnt; ++ap )
                    *dst++ = yscl*(AP[ap] - OFFSET);

                // -----------------
                // lf - interpolated
                // -----------------

                float slope = float(it)/TPNTPERFETCH;

                for( int lf = 0; lf < lfPerTpnt; ++lf ) {
                    *dst++ = yscl*(pLfLast[lf]
                                + slope*(src.lfpData[lf]-pLfLast[lf])
                                - OFFSET);
                }

                // ----
                // sync
                // ----

                *dst++ = src.synchronization[it];
            }

            // ---------------
            // update saved lf
            // ---------------

            memcpy( pLfLast, &src.lfpData[0], lfPerTpnt*sizeof(float) );
        }

#ifdef PROFILE
        sumScl += getTime() - dtScl;
#endif

        // -------
        // Publish
        // -------

        if( !totPts )
            owner->imQ->setTZero( loopT + T0FUDGE );

        tPreEnq = getTime();

        if( zeroFill ) {
            owner->imQ->enqueueZero( tPostEnq, tPreEnq );
            zeroFill = false;
        }

        owner->imQ->enqueue( &i16Buf[0], TPNTPERFETCH * nE );
        tPostEnq = getTime();

        totPts += TPNTPERFETCH * nE;
        nE      = 0;

#ifdef PROFILE
        sumEnq += tPostEnq - tPreEnq;
#endif

        // -----
        // Yield
        // -----

next_fetch:
        dT = getTime() - loopT;

        if( dT < LOOPSECS && isPaused() )
            QThread::usleep( qMin( 1e6 * 0.5*(LOOPSECS - dT), 1000.0 ) );

        // ---------------
        // Rate statistics
        // ---------------

        sumdT += dT;
        ++ndT;

        if( dT > peak_loopT )
            peak_loopT = dT;

        if( loopT - lastCheckT >= 5.0 && !isPaused() ) {

            float   qf = IM.neuropix_fifoFilling();

            if( qf >= 5.0F ) {  // 5.0F standard

                Warning() <<
                    QString("IMEC FIFOQFill% %1, loop ms <%2> peak %3")
                    .arg( qf, 0, 'f', 2 )
                    .arg( 1000*sumdT/ndT, 0, 'f', 3 )
                    .arg( 1000*peak_loopT, 0, 'f', 3 );

                if( qf >= 95.0F ) {
                    runError(
                        QString(
                        "IMEC Ethernet queue overflow; stopping run.") );
                    return;
                }
            }

#ifdef PROFILE
// sumdT/ndT is the actual average time to process 12 samples.
// The required maximum time is 1000*12/30000 = [[ 0.400 ms ]].
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
//
// nDT is the number of actual loop executions in the 5 sec
// check interval. The required minimum value to keep up with
// 30000 samples, 12 at a time, is 5*30000/12 = [[ 12500 ]].
//
// Required values header is written above at run start.

            Log() <<
                QString("loop ms <%1> get<%2> scl<%3> enq<%4>"
                " n(%5) %(%6)")
                .arg( 1000*sumdT/ndT, 0, 'f', 3 )
                .arg( 1000*sumGet/ndT, 0, 'f', 3 )
                .arg( 1000*sumScl/ndT, 0, 'f', 3 )
                .arg( 1000*sumEnq/ndT, 0, 'f', 3 )
                .arg( ndT )
                .arg( IM.neuropix_fifoFilling(), 0, 'f', 3 );

            sumGet = 0;
            sumScl = 0;
            sumEnq = 0;
#endif

            peak_loopT  = 0;
            sumdT       = 0;
            ndT         = 0;
            lastCheckT  = getTime();
        }
    }

// ----
// Exit
// ----

    close();
}

/* ---------------------------------------------------------------- */
/* update --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::update()
{
    setPause( true );

    while( !isPauseAck() )
        QThread::usleep( 1e6*LOOPSECS/8 );

    if( _pauseAcq() && _resumeAcq() ) {

        setPause( false );
        setPauseAck( false );
    }
}

/* ---------------------------------------------------------------- */
/* fetchE --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Continue to fetch E packets until either:
//  - E-vector full
//  - No data returned
//  - Error
//
// Return true if no error.
//
bool CimAcqImec::fetchE()
{
    nE = 0;

// --------------------------------
// Hardware pause acknowledged here
// --------------------------------

    if( isPaused() ) {

ackPause:
        if( !setPauseAck( true ) )
            zeroFill = true;

        return true;
    }

// --------------------
// Else fetch real data
// --------------------

    double  tGet;

    do {
        tGet = getTime();

        int err = IM.neuropix_readElectrodeData( E[nE] );

        tGet = getTime() - tGet;

        if( err == DATA_BUFFER_EMPTY )
            return true;

        if( err != READ_SUCCESS ) {

            if( isPaused() )
                goto ackPause;

            runError(
                QString("IMEC readElectrodeData error %1.").arg( err ) );
            return false;
        }

    } while( ++nE < MAXE && tGet < 1e-4 );

    return true;
}

/* ---------------------------------------------------------------- */
/* configure ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define STOPCHECK   if( isStopped() ) return false;


void CimAcqImec::SETLBL( const QString &s )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetLabel",
        Qt::QueuedConnection,
        Q_ARG(QString, s) );
}


void CimAcqImec::SETVAL( int val )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetValue",
        Qt::QueuedConnection,
        Q_ARG(int, val) );
}


void CimAcqImec::SETVALBLOCKING( int val )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetValue",
        Qt::BlockingQueuedConnection,
        Q_ARG(int, val) );
}


bool CimAcqImec::_open()
{
    SETLBL( "open session" );

    int err = IM.neuropix_open();

    if( err != OPEN_SUCCESS ) {
        runError( QString("IMEC open error %1.").arg( err ) );
        return false;
    }

    SETVAL( 40 );
    return true;
}


bool CimAcqImec::_setLEDs()
{
    int err = IM.neuropix_ledOff( p.im.noLEDs );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC ledOff error %1.").arg( err ) );
        return false;
    }

    SETVAL( 50 );
    return true;
}


bool CimAcqImec::_manualProbeSettings()
{
    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    if( imVers.force ) {

        AsicID  A;
        A.serialNumber  = imVers.pSN.toUInt();
        A.probeType     = imVers.opt - 1;

        int err = IM.neuropix_writeId( A );

        if( err != EEPROM_SUCCESS ) {
            runError( QString("IMEC writeId error %1.").arg( err ) );
            return false;
        }

        Log()
            << "IMEC: Manually set probe: SN=" << A.serialNumber
            << ", opt=" << (int)A.probeType + 1;
    }

    SETVAL( 60 );
    return true;
}


bool CimAcqImec::_calibrateADC_fromFiles()
{
    SETLBL( "calibrate ADC" );

    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    if( imVers.skipADC ) {
        SETVAL( 100 );
        Log() << "IMEC: ADC calibration -- SKIPPED BY USER --";
        return true;
    }

    QString home    = appPath(),
            path    = QString("%1/ImecProbeData").arg( home );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    path = QString("%1/%2").arg( path ).arg( imVers.pLB );

    if( !QDir( path ).exists() ) {
        runError( QString("Can't find path '%1'.").arg( path ) );
        return false;
    }

    std::vector<adcComp>        C;
    std::vector<adcPairCommon>  P;
    int                         err1, err2, err3;

// Read from csv to API
//
// Note: The read functions don't understand paths.

    QDir::setCurrent( path );

        err1 = IM.neuropix_readComparatorCalibrationFromCsv(
                "Comparator calibration.csv" );

        err2 = IM.neuropix_readADCOffsetCalibrationFromCsv(
                "Offset calibration.csv" );

        err3 = IM.neuropix_readADCSlopeCalibrationFromCsv(
                "Slope calibration.csv" );

    QDir::setCurrent( home );

    if( err1 != READCSV_SUCCESS ) {
        runError(
            QString("IMEC readComparatorCalibrationFromCsv error %1.")
            .arg( err1 ) );
        return false;
    }

    if( err2 != READCSV_SUCCESS ) {
        runError(
            QString("IMEC readADCOffsetCalibrationFromCsv error %1.")
            .arg( err2 ) );
        return false;
    }

    if( err3 != READCSV_SUCCESS ) {
        runError(
            QString("IMEC readADCSlopeCalibrationFromCsv error %1.")
            .arg( err3 ) );
        return false;
    }

// Read parameters from API

    err1 = IM.neuropix_getADCCompCalibration( C );

    if( err1 != SUCCESS ) {
        runError(
            QString("IMEC getADCCompCalibration error %1.")
            .arg( err1 ) );
        return false;
    }

    err1 = IM.neuropix_getADCPairCommonCalibration( P );

    if( err1 != SUCCESS ) {
        runError(
            QString("IMEC getADCPairCommonCalibration error %1.")
            .arg( err1 ) );
        return false;
    }

// Write parameters to probe

    for( int i = 0; i < 15; i += 2 ) {

        for( int k = 0; k < 2; ++k ) {

            int ipair   = i + k,
                i2      = i + ipair;

            err1 = IM.neuropix_ADCCalibration( ipair,
                    C[i2].compP,     C[i2].compN,
                    C[i2 + 2].compP, C[i2 + 2].compN,
                    P[ipair].slope,  P[ipair].fine,
                    P[ipair].coarse, P[ipair].cfix );

            if( err1 != BASECONFIG_SUCCESS ) {
                runError(
                    QString("IMEC ADCCalibration error %1.").arg( err1 ) );
                return false;
            }
        }
    }

    SETVAL( 100 );
    Log() << "IMEC: ADC calibrated";
    return true;
}


bool CimAcqImec::_calibrateADC_fromEEPROM()
{
    SETLBL( "calibrate ADC" );

    int err = IM.neuropix_applyAdcCalibrationFromEeprom();

    if( err != SUCCESS ) {
        runError( "IMEC applyAdcCalibrationFromEeprom error." );
        return false;
    }

    SETVAL( 100 );
    Log() << "IMEC: ADC calibrated";
    return true;
}


bool CimAcqImec::_selectElectrodes()
{
    if( p.im.roTbl.opt < 3 )
        return true;

    SETLBL( "select electrodes" );

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.nChan(),
                    err;

// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        err = IM.neuropix_selectElectrode( ic, T.e[ic].bank, false );

        if( err != SHANK_SUCCESS ) {
            runError(
                QString("IMEC selectElectrode(%1,%2) error %3.")
                .arg( ic ).arg( T.e[ic].bank ).arg( err ) );
            return false;
        }
    }

    SETVAL( 50 );

// ----------------------------------------
// Compile usage stats for the ref channels
// ----------------------------------------

    QVector<int>    fRef( IMROTbl::imOpt3Refs, 0 );

    for( int ic = 0; ic < nC; ++ic )
        ++fRef[T.e[ic].refid];

// -------------------------------
// Disconnect unused internal refs
// -------------------------------

#if 1
    const int   *r2c = IMROTbl::optTo_r2c( T.opt );
    int         nRef = IMROTbl::optToNRef( T.opt );

    for( int ir = 1; ir < nRef; ++ir ) {

        if( !fRef[ir] ) {

            int ic  = r2c[ir];

            err = IM.neuropix_selectElectrode( ic, 0xFF, false );

            if( err != SHANK_SUCCESS ) {
                runError(
                    QString("IMEC selectElectrode(%1,%2) error %3.")
                    .arg( ic ).arg( 0xFF ).arg( err ) );
                return false;
            }
        }
    }
#endif

    SETVAL( 80 );

// ------------------------
// Dis/connect external ref
// ------------------------

    // This call also downloads to ASIC

#if 1
    err = IM.neuropix_setExtRef( fRef[0] > 0, true );
#else
    // always connect
    err = IM.neuropix_setExtRef( true, true );
#endif

    if( err != SHANK_SUCCESS ) {
        runError(
            QString("IMEC setExtRef(%1) error %2.")
            .arg( fRef[0] > 0 ).arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log() << "IMEC: Electrodes selected";
    return true;
}


// Download to ASIC done by _setGains.
//
bool CimAcqImec::_setReferences()
{
    SETLBL( "set references" );

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.nChan(),
                    err;

// ------------------------------------
// Connect all according to table refid
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        err = IM.neuropix_setReference( ic, T.e[ic].refid, false );

        if( err != BASECONFIG_SUCCESS ) {
            runError(
                QString("IMEC setReference(%1,%2) error %3.")
                .arg( ic ).arg( T.e[ic].refid ).arg( err ) );
            return false;
        }
    }

    SETVAL( 100 );
    Log() << "IMEC: References set";
    return true;
}


// Download to ASIC done by _setGains.
//
bool CimAcqImec::_setStandby()
{
    if( p.im.roTbl.opt != 3 )
        return true;

    SETLBL( "set standby" );

// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = p.im.roTbl.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        int err = IM.neuropix_setStdb(
                    ic, p.im.stdbyBits.testBit( ic ), false );

        if( err != BASECONFIG_SUCCESS ) {
            runError(
                QString("IMEC setStandby(%1) error %2.")
                .arg( ic ).arg( err ) );
            return false;
        }
    }

    SETVAL( 100 );
    Log() << "IMEC: Standby channels set";
    return true;
}


bool CimAcqImec::_setGains()
{
    SETLBL( "set gains" );

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.nChan(),
                    err;

// --------------------------------
// Set all according to table gains
// --------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        const IMRODesc  &E = T.e[ic];

        err = IM.neuropix_setGain(
                ic,
                IMROTbl::gainToIdx( E.apgn ),
                IMROTbl::gainToIdx( E.lfgn ),
                false );

        if( err != BASECONFIG_SUCCESS ) {
            runError(
                QString("IMEC setGain(%1,%2,%3) error %4.")
                .arg( ic ).arg( E.apgn ).arg( E.lfgn ).arg( err ) );
            return false;
        }
    }

    SETVAL( 80 );

// -------------------
// Download selections
// -------------------

    IM.neuropix_setGain(
        0,
        IMROTbl::gainToIdx( T.e[0].apgn ),
        IMROTbl::gainToIdx( T.e[0].lfgn ),
        true );

    SETVAL( 100 );
    Log() << "IMEC: Gains set";
    return true;
}


bool CimAcqImec::_setHighPassFilter()
{
    SETLBL( "set filters" );

    int err = IM.neuropix_setFilter( p.im.hpFltIdx );

    if( err != BASECONFIG_SUCCESS ) {
        runError( QString("IMEC setFilter error %1.").arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log()
        << "IMEC: Set highpass filter "
        << p.im.idxToFlt( p.im.hpFltIdx )
        << "Hz";
    return true;
}


bool CimAcqImec::_correctGain_fromFiles()
{
    if( !p.im.doGainCor )
        return true;

    SETLBL( "correct gains" );

    QString home    = appPath(),
            path    = QString("%1/ImecProbeData").arg( home );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder '%1'.").arg( path ) );
        return false;
    }

    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    path = QString("%1/%2").arg( path ).arg( imVers.pLB );

    if( !QDir( path ).exists() ) {
        runError( QString("Can't find path '%1'.").arg( path ) );
        return false;
    }

    std::vector<unsigned short> G;
    QFile                       F( "Gain correction.csv" );
    int                         err;

// Read from csv to API
//
// Note: The read functions don't understand paths.

    QDir::setCurrent( path );

        if( !F.exists() )
            F.setFileName( "results_gainCalibration.csv" );

        err = IM.neuropix_readGainCalibrationFromCsv(
                F.fileName().toStdString() );

    QDir::setCurrent( home );

    if( err != READCSV_SUCCESS ) {
        runError(
            QString("IMEC readGainCalibrationFromCsv error %1 file '%2'.")
            .arg( err )
            .arg( F.fileName() ) );
        return false;
    }

// Read params from API

    err = IM.neuropix_getGainCorrectionCalibration( G );

    if( err != SUCCESS ) {
        runError( "IMEC readGainCalibrationFromCsv error." );
        return false;
    }

// Resize according to probe type

    G.resize( IMROTbl::optToNElec( imVers.opt ) );

// Write to basestation FPGA

    err = IM.neuropix_gainCorrection( G );

    if( err != CONFIG_SUCCESS ) {
        runError( QString("IMEC gainCorrection error %1.").arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log() << "IMEC: Applied gain corrections";
    return true;
}


bool CimAcqImec::_correctGain_fromEEPROM()
{
    if( !p.im.doGainCor )
        return true;

    SETLBL( "correct gains...3 to 5 min...can't be aborted..." );

    int err = IM.neuropix_applyGainCalibrationFromEeprom();

    if( err != SUCCESS ) {
        runError( "IMEC applyGainCalibrationFromEeprom error." );
        return false;
    }

    SETVAL( 100 );
    Log() << "IMEC: Applied gain corrections";
    return true;
}


bool CimAcqImec::_setNeuralRecording()
{
    SETLBL( "set readout modes" );

    int err = IM.neuropix_mode( ASIC_RECORDING );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC (readout)mode error %1.").arg( err ) );
        return false;
    }

    SETVAL( 30 );
    Log() << "IMEC: Set recording mode";
    return true;
}


bool CimAcqImec::_setElectrodeMode()
{
    bool    electrodeMode = true;
    int     err = IM.neuropix_datamode( electrodeMode );

    if( err != SUCCESS ) {
        runError( "IMEC datamode error." );
        return false;
    }

    SETVAL( 70 );
    Log() << "IMEC: Set electrode mode";
    return true;
}


bool CimAcqImec::_setTriggerMode()
{
    int     err = IM.neuropix_triggerMode( p.im.softStart );

    if( err != CONFIG_SUCCESS ) {
        runError( QString("IMEC triggerMode error %1.").arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log()
        << "IMEC: Trigger source: "
        << (p.im.softStart ? "software" : "hardware");
    return true;
}


bool CimAcqImec::_setStandbyAll()
{
    int err = IM.neuropix_writeAllStandby( false );

    if( err != BASECONFIG_SUCCESS ) {
        runError( QString("IMEC writeAllStandby error %1.").arg( err ) );
        return false;
    }

    Log() << QString("IMEC: Set all channels to amplified.");
    return true;
}


bool CimAcqImec::_setRecording()
{
    SETLBL( "arm system" );

    int err = IM.neuropix_nrst( false );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC nrst( false ) error %1.").arg( err ) );
        return false;
    }

    SETVAL( 80 );

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC resetDatapath error." );
        return false;
    }

    SETVAL( 100 );
    Log() << "IMEC: Armed";
    return true;
}


bool CimAcqImec::_pauseAcq()
{
    int err = IM.neuropix_nrst( false );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC nrst( false ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_prnrst( false );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC prnrst( false ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC resetDatapath error." );
        return false;
    }

    return true;
}


bool CimAcqImec::_resumeAcq()
{
    if( !_selectElectrodes() )
        return false;

    if( !_setReferences() )
        return false;

    if( !_setStandby() )
        return false;

    if( !_setGains() )
        return false;

    int err = IM.neuropix_prnrst( true );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC prnrst( true ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC resetDatapath error." );
        return false;
    }

    err = IM.neuropix_nrst( true );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC nrst( true ) error %1.").arg( err ) );
        return false;
    }

    return true;
}


bool CimAcqImec::configure()
{
    STOPCHECK;

    if( !_open() )
        return false;

    STOPCHECK;

    if( !_setLEDs() )
        return false;

    STOPCHECK;

    if( !_manualProbeSettings() )
        return false;

    STOPCHECK;

    if( mainApp()->cfgCtl()->imVers.force ) {

        if( !_calibrateADC_fromFiles() )
            return false;
    }
    else if( !_calibrateADC_fromEEPROM() )
        return false;

    STOPCHECK;

    if( !_selectElectrodes() )
        return false;

    STOPCHECK;

    if( !_setReferences() )
        return false;

    STOPCHECK;

    if( !_setStandby() )
        return false;

    STOPCHECK;

    if( !_setGains() )
        return false;

    STOPCHECK;

    if( !_setHighPassFilter() )
        return false;

    STOPCHECK;

    if( mainApp()->cfgCtl()->imVers.force ) {

        if( !_correctGain_fromFiles() )
            return false;
    }
    else if( !_correctGain_fromEEPROM() )
        return false;

    STOPCHECK;

    if( !_setNeuralRecording() )
        return false;

    if( !_setElectrodeMode() )
        return false;

    if( !_setTriggerMode() )
        return false;

    if( !_setRecording() )
        return false;

// Flush all progress messages
    SETVALBLOCKING( 100 );

    return true;
}

/* ---------------------------------------------------------------- */
/* startAcq ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::startAcq()
{
    STOPCHECK;

    if( p.im.softStart ) {

// BK: Diagnostic test pattern
//        Log() << "te " << IM.neuropix_te( 1 );

        // Following completes sequence {nrst(F),resetDatapath,nrst(T)}.

        int err = IM.neuropix_nrst( true );

        if( err != DIGCTRL_SUCCESS ) {
            runError(
                QString("IMEC nrst( true ) error %1.").arg( err ) );
            return false;
        }

        // - Reset SDRAM buffer
        // - Restart acquisition
        // - Set EXT_START pin

        err = IM.neuropix_setNeuralStart();

        if( err != CONFIG_SUCCESS ) {
            runError(
                QString("IMEC setNeuralStart error %1.").arg( err ) );
            return false;
        }

        Log() << "IMEC: Acquisition started";
    }
    else
        Log() << "IMEC: Waiting for external trigger";

    return true;
}

/* ---------------------------------------------------------------- */
/* close ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::close()
{
    IM.neuropix_close();
}

/* ---------------------------------------------------------------- */
/* runError ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::runError( QString err )
{
    close();

    if( !err.isEmpty() ) {

        Error() << err;
        emit owner->daqError( err );
    }
}

#endif  // HAVE_IMEC


