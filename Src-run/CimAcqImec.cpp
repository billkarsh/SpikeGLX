
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"


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
// -----------
// Diagnostics
// -----------

#if 0
    bist();
    return;
#endif

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
/* bist ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Run built-in self tests.
//
void CimAcqImec::bist()
{
    close();

    if( !_open() )
        return;

    int err;

    Q_UNUSED( err )

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    Log() << "bist4 " << IM.neuropix_test4();
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    unsigned char   brd_ver, fpga_ver;
    err = IM.neuropix_test5( brd_ver, fpga_ver, 10 );
    Log() << "bist5 " << err << " brd " << brd_ver << " fpga " << fpga_ver;
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    unsigned char   prbs_err;
    Log() << "bist6 strt " << IM.neuropix_startTest6();
    msleep( 5000 );
    err = IM.neuropix_stopTest6( prbs_err );
    Log() << "bist6 stop " << err << " prbs " << prbs_err;
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    Log() << "bist7 strt " << IM.neuropix_startTest7();
    msleep( 5000 );
    err = IM.neuropix_stopTest7();
    Log()
        << "bist7 stop " << err
        << " errs " << IM.neuropix_test7GetErrorCounter()
        << " pats " << IM.neuropix_test7GetTotalChecked()
        << " mask " << IM.neuropix_test7GetErrorMask();
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    for( int ab = 0; ab < 2; ++ab ) {
        bool te = !ab;
        for( int line = 0; line < 4; ++line ) {
            Log()
                << "bist8 strt " << IM.neuropix_startTest8( te, line )
                << " te " << (te?"A":"B")
                << " line " << line;
            msleep( 1000 );
            Log() << "bist8 stop " << IM.neuropix_stopTest8();

            bool spi_adc_err, spi_ctr_err, spi_syn_err;

            err = IM.neuropix_test8GetErrorCounter(
                    te, spi_adc_err, spi_ctr_err, spi_syn_err );

            Log()
                << "bist8 te " << (te?"A":"B")
                << " adc_err " << spi_adc_err
                << " ctr_err " << spi_ctr_err
                << " syn_err " << spi_syn_err;
        }
    }
#endif
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#if 0
    if( _setNeuralRecording() ) {
        for( int ab = 0; ab < 2; ++ab ) {
            bool te = !ab;
            Log() << "nrst false " << IM.neuropix_nrst( false );
            Log() << "resetPath " << IM.neuropix_resetDatapath();
            Log()
                << "bist9 strt " << IM.neuropix_startTest9( te )
                << " te " << (te?"A":"B") << " ******";
            Log() << "nrst true " << IM.neuropix_nrst( true );

            for( int trial = 0; trial < 10; ++ trial ) {

                msleep( 1000 );

                unsigned int    syn_errors, syn_total,
                                ctr_errors, ctr_total,
                                te_spi0_errors, te_spi1_errors,
                                te_spi2_errors, te_spi3_errors,
                                te_spi_total;

                IM.neuropix_test9GetResults(
                        syn_errors, syn_total,
                        ctr_errors, ctr_total,
                        te_spi0_errors, te_spi1_errors,
                        te_spi2_errors, te_spi3_errors,
                        te_spi_total );

                Log() << "bist9 try " << trial;
                Log() << "syn err " << syn_errors << " total " << syn_total;
                Log() << "ctr err " << ctr_errors << " total " << ctr_total;
                Log()
                    << "te errr " << te_spi0_errors
                    << " " << te_spi1_errors
                    << " " << te_spi2_errors
                    << " " << te_spi3_errors
                    << " tot " << te_spi_total;
            }

            Log() << "bist9 stop " << IM.neuropix_stopTest9();
        }
    }
#endif

    close();
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

    SETVAL( 100 );
    return true;
}


bool CimAcqImec::_selectElectrodes()
{
    SETLBL( "select electrodes" );

    if( p.im.roTbl.opt >= 3 ) {

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

        if( err != SHANK_SUCCESS ) {
            runError(
                QString("IMEC setExtRef(%1) error %2.")
                .arg( fRef[0] > 0 ).arg( err ) );
            return false;
        }
#endif
IM.neuropix_selectElectrode( 0, T.e[0].bank, true );
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


bool CimAcqImec::_setGainCorrection()
{
    if( !p.im.doGainCor )
        return true;

    SETLBL( "correct gains...takes 5 min...can't be aborted..." );

    int err = IM.neuropix_readGainCorrection();

    if( err != EEPROM_SUCCESS ) {
        runError( QString("IMEC readGainCorrection error %1.").arg( err ) );
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
        runError( QString("IMEC datamode error %1.").arg( err ) );
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
        runError( QString("IMEC resetDatapath error %1.").arg( err ) );
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
        runError( QString("IMEC resetDatapath error %1.").arg( err ) );
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
        runError( QString("IMEC resetDatapath error %1.").arg( err ) );
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

    if( !_manualProbeSettings() )
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

    if( !_setGainCorrection() )
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

#if 1
// BK: Provisional start until neuropix_setNeuralStart working.
// BK: Following completes sequence {nrst(F),resetDatapath,nrst(T)}.

        int err0 = IM.neuropix_nrst( true );

        if( err0 != DIGCTRL_SUCCESS ) {
            runError(
                QString("IMEC nrst( true ) error %1.").arg( err0 ) );
            return false;
        }
#endif

// BK: neuropix_setNeuralStart implemented 3.4 ?? Not yet
#if 0
        int err = IM.neuropix_setNeuralStart();

        if( err != CONFIG_SUCCESS ) {
            runError(
                QString("IMEC setNeuralStart error %1.").arg( err ) );
            return false;
        }
#endif

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


