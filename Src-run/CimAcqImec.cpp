
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDir>


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

// User manual Sec 5.6 "Probe signal offset" says value [0.6 .. 0.7]
#define MAX10BIT    512
#define OFFSET      0.6F


void CimAcqImec::run()
{
// ---------
// Configure
// ---------

    setPauseAck( false );

    if( !configure() )
        return;

// -------
// Buffers
// -------

    const int
        tpntPerFetch    = 12,
        tpntPerBlock    = 48,
        apPerTpnt       = p.im.imCumTypCnt[CimCfg::imSumAP],
        lfPerTpnt       = p.im.imCumTypCnt[CimCfg::imSumNeural]
                            - p.im.imCumTypCnt[CimCfg::imSumAP],
        syPerTpnt       = 1,
        chnPerTpnt      = (apPerTpnt + lfPerTpnt + syPerTpnt);

    QVector<float>  lfLast( lfPerTpnt, 0.0F );
    vec_i16         i16Buf( tpntPerBlock * chnPerTpnt );
    ElectrodePacket E;

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

    double  lastCheckT  = getTime(),
            startT      = lastCheckT,
            peak_loopT  = 0,
            sumdT       = 0,
            dT;
    int     nTpnt       = 0,
            ndT         = 0;

    while( !isStopped() ) {

        if( isPaused() ) {
            setPauseAck( true );
            usleep( 0.01 * 1e6 );
            continue;
        }

        double  loopT = getTime();
        qint16  *dst;

        // -------------
        // Read a packet
        // -------------

        int err = IM.neuropix_readElectrodeData( E );

        // ------------------
        // Handle empty fetch
        // ------------------

        if( err == DATA_BUFFER_EMPTY ) {

            if( isPaused() )
                continue;

            // BK: Allow up to 5 seconds for (external) trigger.
            // BK: Tune with experience.

            if( loopT - startT >= 5.0 ) {
                runError( "DAQ IMReader getting no samples." );
                return;
            }

            goto next_fetch;
        }

        // -------------------
        // Handle packet error
        // -------------------

        if( err != READ_SUCCESS ) {

            if( isPaused() )
                continue;

            runError(
                QString("IMEC readElectrodeData error %1.").arg( err ) );
            return;
        }

        // ---------------
        // Emplace samples
        // ---------------

        dst = &i16Buf[nTpnt*chnPerTpnt];

        for( int it = 0; it < tpntPerFetch; ++it ) {

            // ap - as is
            for( int ap = 0; ap < apPerTpnt; ++ap )
                *dst++ = yscl*(E.apData[it][ap] - OFFSET);

            // lf - interpolated
            for( int lf = 0; lf < lfPerTpnt; ++lf ) {
                *dst++ = yscl*(lfLast[lf]
                            + it*(E.lfpData[lf]-lfLast[lf])/tpntPerFetch
                            - OFFSET);
            }

            // synch
            *dst++ = E.synchronization[it];
        }

        // update saved lf
        for( int lf = 0; lf < lfPerTpnt; ++lf )
            lfLast[lf] = E.lfpData[lf];

        // -------
        // Publish
        // -------

        nTpnt += tpntPerFetch;

        if( nTpnt >= tpntPerBlock ) {

            owner->imQ->enqueue( i16Buf, nTpnt, totalTPts );
            totalTPts += tpntPerBlock;
            nTpnt     = 0;
        }

        // ---------------
        // Rate statistics
        // ---------------

        dT = getTime() - loopT;

        sumdT += dT;
        ++ndT;

        if( dT > peak_loopT )
            peak_loopT = dT;

next_fetch:
        if( loopT - lastCheckT >= 5.0 && !isPaused() ) {

            float   qf = IM.neuropix_fifoFilling();

            if( qf >= 5.0F ) {  // 5.0F standard

                Warning() <<
                    QString("IMEC FIFOQFill% %1, loop ms <%2> peak %3")
                    .arg( qf, 0, 'f', 2 )
                    .arg( 1000*sumdT/ndT, 0, 'f', 2 )
                    .arg( 1000*peak_loopT, 0, 'f', 2 );

                if( qf >= 95.0F ) {
                    runError(
                        QString(
                        "IMEC Ethernet queue overflow; stopping run...") );
                    return;
                }
            }

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
/* pause ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::pause( bool pause, bool changed )
{
    if( pause ) {

        setPause( true );

        while( !isPauseAck() )
            usleep( 0.01 * 1e6 );

        return _pauseAcq();
    }
    else if( _resumeAcq( changed ) ) {

        setPause( false );
        setPauseAck( false );
    }

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
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    path = QString("%1/1%2%3")
            .arg( path )
            .arg( imVers.pSN )
            .arg( imVers.opt );

    if( !QDir( path ).exists() ) {
        runError( QString("Can't find path [%1].").arg( path ) );
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
    int             nC = T.e.size(),
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

#if 0
    const int   *r2c    =
        (T.opt == 4 ? IMROTbl::r2c276() : IMROTbl::r2c384());
    int         nRef    =
        (T.opt == 4 ? IMROTbl::imOpt4Refs : IMROTbl::imOpt3Refs);

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

#if 0
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
    int             nC = T.e.size(),
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


bool CimAcqImec::_setGains()
{
    SETLBL( "set gains" );

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.e.size(),
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
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    path = QString("%1/1%2%3")
            .arg( path )
            .arg( imVers.pSN )
            .arg( imVers.opt );

    if( !QDir( path ).exists() ) {
        runError( QString("Can't find path [%1].").arg( path ) );
        return false;
    }

    std::vector<unsigned short> G;
    int                         err;

// Read from csv to API
//
// Note: The read functions don't understand paths.

    QDir::setCurrent( path );

        err = IM.neuropix_readGainCalibrationFromCsv(
                "Gain correction.csv" );

    QDir::setCurrent( home );

    if( err != READCSV_SUCCESS ) {
        runError(
            QString("IMEC readGainCalibrationFromCsv error %1.")
            .arg( err ) );
        return false;
    }

// Read params from API

    err = IM.neuropix_getGainCorrectionCalibration( G );

    if( err != SUCCESS ) {
        runError( "IMEC readGainCalibrationFromCsv error." );
        return false;
    }

// Resize according to probe type

    switch( imVers.opt ) {
        case 1:
        case 2:
            G.resize( IMROTbl::imOpt1Elec );
        break;
        case 3:
            G.resize( IMROTbl::imOpt3Elec );
        break;
        case 4:
            G.resize( IMROTbl::imOpt4Elec );
        break;
    }

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


bool CimAcqImec::_resumeAcq( bool changed )
{
    if( changed ) {

        if( !_selectElectrodes() )
            return false;

        if( !_setReferences() )
            return false;

        if( !_setGains() )
            return false;
    }

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

    if( !_setLEDs() )
        return false;

    if( !_manualProbeSettings() )
        return false;

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


