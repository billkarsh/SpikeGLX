
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"


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

    const float yscl = 32768.0F/0.6F;

    double	lastCheckT  = getTime(),
            startT      = lastCheckT,
            peak_loopT  = 0,
            sumdT       = 0,
            dT;
    int     nTpnt       = 0,
            ndT         = 0;

    while( !isStopped() ) {

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
            runError(
                QString("IMEC readElectrodeData error %1.").arg( err ) );
            return;
        }

        // ---------------
        // Emplace samples
        // ---------------

// BK: LFP interpolate not implemented until need it

        dst = &i16Buf[nTpnt*chnPerTpnt];

        for( int it = 0; it < tpntPerFetch; ++it ) {

            // ap
            for( int ap = 0; ap < apPerTpnt; ++ap )
                *dst++ = yscl*(E.apData[it][ap] - 0.6F);

            // lf
            for( int lf = 0; lf < lfPerTpnt; ++lf )
                *dst++ = yscl*(E.lfpData[lf] - 0.6F);

            // synch
            *dst++ = E.synchronization[it];
        }

        // -------
        // Publish
        // -------

        nTpnt += tpntPerFetch;

        if( nTpnt >= tpntPerBlock ) {

            owner->imQ->enqueue( i16Buf, nTpnt, totalTPts );
            totalTPts += tpntPerBlock;
            nTpnt     = 0;
        }

// BK: Retest how it works when xilinx off.
// BK: Flesh out configging for real.
// BK: Time to go real metadata??

        // ---------------
        // Rate statistics
        // ---------------

        dT = getTime() - loopT;

        sumdT += dT;
        ++ndT;

        if( dT > peak_loopT )
            peak_loopT = dT;

next_fetch:
        if( loopT - lastCheckT >= 5.0 ) {

            float   qf = IM.neuropix_fifoFilling();

            if( qf >= 2.0F ) {  // 2.0F standard

                Warning() <<
                    QString("IMEC FIFOQFill% %1, loop ms <%2> peak %3")
                    .arg( qf, 0, 'f', 2 )
                    .arg( 1000*sumdT/ndT, 0, 'f', 2 )
                    .arg( 1000*peak_loopT, 0, 'f', 2 );
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

bool CimAcqImec::_open()
{
    int err = IM.neuropix_open();

    if( err != OPEN_SUCCESS ) {
        runError(
            QString("IMEC open error %1.").arg( err ) );
        return false;
    }

    return true;
}


bool CimAcqImec::_hwrVers()
{
    VersionNumber   vn;
    int             err = IM.neuropix_getHardwareVersion( &vn );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC getHardwareVersion error %1.").arg( err ) );
        return false;
    }

    Log() << "IMEC hardware version " << vn.major << "." << vn.minor;
    return true;
}


bool CimAcqImec::_bsVers()
{
    uchar   bsVersMaj, bsVersMin;
    int     err = IM.neuropix_getBSVersion( bsVersMaj );

    if( err != CONFIG_SUCCESS ) {
        runError(
            QString("IMEC getBSVersion error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_getBSRevision( bsVersMin );

    if( err != CONFIG_SUCCESS ) {
        runError(
            QString("IMEC getBSRevision error %1.").arg( err ) );
        return false;
    }

    Log() << "IMEC basestation version " << bsVersMaj << "." << bsVersMin;
    return true;
}


bool CimAcqImec::_apiVers()
{
    VersionNumber   vn = IM.neuropix_getAPIVersion();

    Log() << "IMEC API version " << vn.major << "." << vn.minor;
    return true;
}


bool CimAcqImec::_probeID()
{
    AsicID  asicID;
    int     err = IM.neuropix_readId( asicID );

    if( err != EEPROM_SUCCESS ) {
        runError(
            QString("IMEC readId error %1.").arg( err ) );
        return false;
    }

    Log()
        << QString("IMEC probe: SN(%1) type(%2)")
            .arg( asicID.serialNumber, 0, 16 )
            .arg( asicID.probeType );
    return true;
}


//bool CimAcqImec::_selectElectrodesEach()
//{
//    int bank = 0;

//    for( int ic = 0; ic < 384; ++ic ) {

//        int err = IM.neuropix_selectElectrode( ic, bank );

//        if( err != SHANK_SUCCESS ) {
//            runError(
//                QString("IMEC selectElectrode(%1,%2) error %3.")
//                .arg( ic ).arg( bank ).arg( err ) );
//            return false;
//        }
//    }

//    Log() << "Set all channels to bank " << bank;
//    return true;
//}


bool CimAcqImec::_setRefsEach()
{
    int ref = 0;

    for( int ic = 0; ic < 384; ++ic ) {

        int err = IM.neuropix_setReference( ic, ref );

        if( err != BASECONFIG_SUCCESS ) {
            runError(
                QString("IMEC setReference(%1,%2) error %3.")
                .arg( ic ).arg( ref ).arg( err ) );
            return false;
        }
    }

    Log() << "Set all channels to reference " << ref;
    return true;
}


bool CimAcqImec::_setRefsAll()
{
    int ref = 0;
    int err = IM.neuropix_writeAllReferences( ref );

    if( err != BASECONFIG_SUCCESS ) {
        runError(
            QString("IMEC writeAllReferences(%1) error %2.")
            .arg( ref ).arg( err ) );
        return false;
    }

    Log() << "Set all channels to reference " << ref;
    return true;
}


bool CimAcqImec::_setGainEach()
{
// BK: Need to code a table of gain index to gain value
    int gain = 0;

    for( int ic = 0; ic < 384; ++ic ) {

        int err = IM.neuropix_setGain( ic, gain, gain );

        if( err != BASECONFIG_SUCCESS ) {
            runError(
                QString("IMEC setGain(%1,%2,%3) error %4.")
                .arg( ic ).arg( gain ).arg( gain ).arg( err ) );
            return false;
        }
    }

    Log()
        << QString("Set all channels to gain(%1,%2)")
            .arg( gain ).arg( gain );
    return true;
}


bool CimAcqImec::_setAPGainAll()
{
    int gain = 0;
    int err = IM.neuropix_writeAllAPGains( gain );

    if( err != BASECONFIG_SUCCESS ) {
        runError(
            QString("IMEC writeAllAPGains(%1) error %2.")
            .arg( gain ).arg( err ) );
        return false;
    }

    Log()
        << QString("Set all channels to AP gain(%1)")
            .arg( gain );
    return true;
}


bool CimAcqImec::_setLFGainAll()
{
    int gain = 0;
    int err = IM.neuropix_writeAllLFPGains( gain );

    if( err != BASECONFIG_SUCCESS ) {
        runError(
            QString("IMEC writeAllLFPGains(%1) error %2.")
            .arg( gain ).arg( err ) );
        return false;
    }

    Log()
        << QString("Set all channels to LFP gain(%1)")
            .arg( gain );
    return true;
}


bool CimAcqImec::_setHighPassFilter()
{
    int filterID = 0;
    int err = IM.neuropix_setFilter( filterID );

    if( err != BASECONFIG_SUCCESS ) {
        runError(
            QString("IMEC setFilter error %1.").arg( err ) );
        return false;
    }

    Log() << "Set filter index " << filterID;
    return true;
}


bool CimAcqImec::_setNeuralRecording()
{
    int err = IM.neuropix_mode( ASIC_RECORDING );

    if( err != DIGCTRL_SUCCESS ) {
        runError(
            QString("IMEC (readout)mode error %1.").arg( err ) );
        return false;
    }

    Log() << "IMEC set to recording mode";
    return true;
}


bool CimAcqImec::_setElectrodeMode()
{
    bool    electrodeMode = true;
    int     err = IM.neuropix_datamode( electrodeMode );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC datamode error %1.").arg( err ) );
        return false;
    }

    Log() << "IMEC set to electrode mode";
    return true;
}


bool CimAcqImec::_setTriggerMode()
{
    bool    softwareStart = true;
    int     err = IM.neuropix_triggerMode( softwareStart );

    if( err != CONFIG_SUCCESS ) {
        runError(
            QString("IMEC triggerMode error %1.").arg( err ) );
        return false;
    }

    Log()
        << "Set trigger/softwareStart mode "
        << (softwareStart ? "T" : "F");
    return true;
}


bool CimAcqImec::_setStandbyAll()
{
    int err = IM.neuropix_writeAllStandby( false );

    if( err != BASECONFIG_SUCCESS ) {
        runError(
            QString("IMEC writeAllStandby error %1.").arg( err ) );
        return false;
    }

    Log() << QString("Set all channels to amplified.");
    return true;
}


bool CimAcqImec::_setRecording()
{
    int err = IM.neuropix_nrst( false );

    if( err != DIGCTRL_SUCCESS ) {
        runError(
            QString("IMEC nrst( false ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError(
            QString("IMEC resetDatapath error %1.").arg( err ) );
        return false;
    }

    return true;
}


bool CimAcqImec::configure()
{
    if( !_open() )
        return false;

// BK: Called automatically?
//Log() << "config deser " << IM.neuropix_configureDeserializer();
//Log() << "config ser " << IM.neuropix_configureSerializer();

    if( !_hwrVers() )
        return false;

    if( !_bsVers() )
        return false;

    if( !_apiVers() )
        return false;

//    if( !_probeID() )
//        return false;

//    if( !_selectElectrodesEach() )
//        return false;

//    if( !_setRefsEach() )
//        return false;

    if( !_setRefsAll() )
        return false;

//    if( !_setGainEach() )
//        return false;

    if( !_setAPGainAll() )
        return false;

    if( !_setLFGainAll() )
        return false;

    if( !_setHighPassFilter() )
        return false;

    if( !_setNeuralRecording() )
        return false;

    if( !_setElectrodeMode() )
        return false;

    if( !_setTriggerMode() )
        return false;

//    if( !_setStandbyAll() )
//        return false;

    if( !_setRecording() )
        return false;

    return true;
}

/* ---------------------------------------------------------------- */
/* startAcq ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::startAcq()
{
// BK: Diagnostic test pattern
//    Log() << "te " << IM.neuropix_te( 1 );

// BK: Provisional start until neuropix_setNeuralStart working.
// BK: Following completes sequence {nrst(F),resetDatapath,nrst(T)}.

    int err = IM.neuropix_nrst( true );

    if( err != DIGCTRL_SUCCESS ) {
        runError(
            QString("IMEC nrst( true ) error %1.").arg( err ) );
        return false;
    }

// neuropix_setNeuralStart not yet implemented.
#if 0
    int err = IM.neuropix_setNeuralStart();

    if( err != CONFIG_SUCCESS ) {
        runError(
            QString("IMEC setNeuralStart error %1.").arg( err ) );
        return false;
    }
#endif

// BK: Perhaps use Debug() for status
    Log() << "IMEC acquisition started";
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


