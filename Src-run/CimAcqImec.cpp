
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDir>
#include <QThread>


// User manual Sec 5.7 "Probe signal offset" says value [0.6 .. 0.7].
// TPNTPERFETCH reflects the AP/LF sample rate ratio.
// OVERFETCH ensures we fetch a little more than loopSecs generates.
#define MAX10BIT        512
#define OFFSET          0.6F
#define TPNTPERFETCH    12
#define OVERFETCH       1.20
//#define PROFILE


/* ---------------------------------------------------------------- */
/* ImAcqShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqShared::ImAcqShared( const DAQ::Params &p, double loopSecs )
    :   p(p), totPts(0ULL),
        maxE(qRound(OVERFETCH * loopSecs * p.im.all.srate / TPNTPERFETCH)),
        awake(0), asleep(0), nE(0), stop(false)
{
    E.resize( maxE );

    for( int ip = 0; ip < p.im.nProbes; ++ip ) {

        const int   *cum = p.im.each[ip].imCumTypCnt;

        apPerTpnt.push_back( cum[CimCfg::imTypeAP] );
        lfPerTpnt.push_back( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] );
        syPerTpnt.push_back( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );
        chnPerTpnt.push_back( apPerTpnt[ip] + lfPerTpnt[ip] + syPerTpnt[ip] );
    }

#ifdef PROFILE
    sumScl.fill( 0.0, p.im.nProbes );
    sumEnq.fill( 0.0, p.im.nProbes );
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct t_sh12   { const quint16 sy[12]; };
struct t_384    { const float   lf[384]; };
struct t_12x384 { const float   ap[12][384]; };


void ImAcqWorker::run()
{
// Minus here ala User manual 5.7 "Probe signal inversion"
    const float yscl = -MAX10BIT/0.6F;
    const int   nID  = vID.size();

    std::vector<std::vector<float> >    lfLast;
    std::vector<std::vector<qint16> >   i16Buf;

    lfLast.resize( nID );
    i16Buf.resize( nID );

    for( int iID = 0; iID < nID; ++iID ) {

        int pID = vID[iID];

        lfLast[iID].assign( shr.lfPerTpnt[pID], 0.0F );
        i16Buf[iID].resize( TPNTPERFETCH * shr.maxE * shr.chnPerTpnt[pID] );
    }

    for(;;) {

        if( !shr.wake() )
            break;

        // -----------
        // E -> i16Buf
        // -----------

        for( int ie = 0; ie < shr.nE; ++ie ) {

            for( int iID = 0; iID < nID; ++iID ) {

#ifdef PROFILE
                double  dtScl = getTime();
#endif

                const int       pID     = vID[iID];
                const t_12x384  *srcAP  = &((t_12x384*)&shr.E[ie].apData)[0];
                const float     *srcLF  = ((t_384*)&shr.E[ie].lfpData)[0].lf;
                const quint16   *srcSY  = ((t_sh12*)&shr.E[ie].synchronization)[0].sy;

                float   *pLfLast    = &lfLast[iID][0];
                qint16  *dst        = &i16Buf[iID][TPNTPERFETCH * ie * shr.chnPerTpnt[pID]];

                for( int it = 0; it < TPNTPERFETCH; ++it ) {

                    // ----------
                    // ap - as is
                    // ----------

                    const float *AP = srcAP->ap[it];

                    for( int ap = 0, nap = shr.apPerTpnt[pID]; ap < nap; ++ap )
                        *dst++ = yscl*(AP[ap] - OFFSET);

                    // -----------------
                    // lf - interpolated
                    // -----------------

                    float slope = float(it)/TPNTPERFETCH;

                    for( int lf = 0, nlf = shr.lfPerTpnt[pID]; lf < nlf; ++lf ) {
                        *dst++ = yscl*(pLfLast[lf]
                                    + slope*(srcLF[lf]-pLfLast[lf])
                                    - OFFSET);
                    }

                    // -----
                    // synch
                    // -----

                    *dst++ = srcSY[it];
                }

                // ---------------
                // update saved lf
                // ---------------

                memcpy( pLfLast, srcLF, shr.lfPerTpnt[pID]*sizeof(float) );

#ifdef PROFILE
                shr.sumScl[pID] += getTime() - dtScl;
#endif
            } // probes scaling
        } // E packets

        // -------
        // Publish
        // -------

        for( int iID = 0; iID < nID; ++iID ) {

#ifdef PROFILE
            double  dtEnq = getTime();
#endif

            const int pID = vID[iID];

            imQ[pID]->enqueue(
                i16Buf[iID], shr.tStamp,
                shr.totPts, TPNTPERFETCH * shr.nE );

#ifdef PROFILE
            shr.sumEnq[pID] += getTime() - dtEnq;
#endif
        } // probes publish
    } // top - forever

    emit finished();
}

/* ---------------------------------------------------------------- */
/* ImAcqThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqThread::ImAcqThread(
    ImAcqShared     &shr,
    QVector<AIQ*>   &imQ,
    QVector<int>    &vID )
{
    thread  = new QThread;
    worker  = new ImAcqWorker( shr, imQ, vID );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImAcqThread::~ImAcqThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}

/* ---------------------------------------------------------------- */
/* ~CimAcqImec ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::~CimAcqImec()
{
    close();

// Kill all threads

    shr.kill();

    for( int iThd = 0; iThd < nThd; ++iThd )
        delete imT[iThd];
}

/* ---------------------------------------------------------------- */
/* CimAcqImec::run ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimAcqImec::run()
{
// ---------
// Configure
// ---------

    setPauseAck( false );

// Hardware

    if( !configure() )
        return;

// Create worker threads

    const int   nPrbPerThd = 3;

    for( int ip0 = 0; ip0 < p.im.nProbes; ip0 += nPrbPerThd ) {

        QVector<int>    vID;

        for( int id = 0; id < nPrbPerThd; ++id ) {

            if( ip0 + id < p.im.nProbes )
                vID.push_back( ip0 + id );
            else
                break;
        }

        imT.push_back( new ImAcqThread( shr, owner->imQ, vID ) );
        ++nThd;
    }

// Wait for threads to reach ready (sleep) state

    shr.runMtx.lock();
        while( shr.asleep < nThd ) {
            shr.runMtx.unlock();
                usleep( 10 );
            shr.runMtx.lock();
        }
    shr.runMtx.unlock();

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( !startAcq() )
        return;

// -----
// Fetch
// -----

#ifdef PROFILE
    double  sumGet = 0, sumThd = 0, dtThd;

    // Table header, see profile discussion below

    Log() <<
        QString("Required loop ms < [[ %1 ]] n > [[ %2 ]]")
        .arg( 1000*TPNTPERFETCH*shr.maxE/p.im.all.srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.all.srate/(TPNTPERFETCH*shr.maxE) ) );
#endif

    double  lastCheckT  = getTime(),
            startT      = lastCheckT,
            peak_loopT  = 0,
            sumdT       = 0,
            dT;
    int     ndT         = 0;

    while( !isStopped() ) {

        if( isPaused() ) {
            setPauseAck( true );
            usleep( 1e6 * 0.01 );
            continue;
        }

        double  loopT = getTime();

        // -----
        // Fetch
        // -----

        if( !fetchE( loopT ) )
            return;

        // ------------------
        // Handle empty fetch
        // ------------------

        if( !shr.nE ) {

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

#ifdef PROFILE
        sumGet += getTime() - loopT;
#endif

        /* ---------------- */
        /* Process new data */
        /* ---------------- */

#ifdef PROFILE
        dtThd = getTime();
#endif

        // Chunk params

        shr.awake   = 0;
        shr.asleep  = 0;

        // Wake all threads

        shr.condWake.wakeAll();

        // Wait all threads started, and all done

        shr.runMtx.lock();
            while( shr.awake  < nThd
                || shr.asleep < nThd ) {

                shr.runMtx.unlock();
                    usleep( 1e6*loopSecs/8 );
                shr.runMtx.lock();
            }
        shr.runMtx.unlock();

        // Update counts

        shr.totPts += TPNTPERFETCH * shr.nE;
        shr.nE      = 0;

#ifdef PROFILE
        sumThd += getTime() - dtThd;
#endif

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
// sumdT/ndT is the actual average time to process 12*maxE samples.
// The required maximum time is 1000*12*30/30000 = [[ 12.0 ms ]].
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
// Thd measures the time spent waking and waiting for worker threads.
//
// nDT is the number of actual loop executions in the 5 sec
// check interval. The required minimum value to keep up with
// 30000 samples, 12*30 at a time, is 5*30000/(12*30) = [[ 417 ]].
//
// Required values header is written above at run start.

            double  sumScl = 0,
                    sumEnq = 0;

            for( int ip = 0; ip < p.im.nProbes; ++ip ) {
                sumScl += shr.sumScl[ip];
                sumEnq += shr.sumEnq[ip];
            }

            Log() <<
                QString(
                "loop ms <%1> get<%2> scl<%3> enq<%4>"
                " thd<%5> n(%6) \%(%7)")
                .arg( 1000*sumdT/ndT, 0, 'f', 3 )
                .arg( 1000*sumGet/ndT, 0, 'f', 3 )
                .arg( 1000*sumScl/ndT, 0, 'f', 3 )
                .arg( 1000*sumEnq/ndT, 0, 'f', 3 )
                .arg( 1000*sumThd/ndT, 0, 'f', 3 )
                .arg( ndT )
                .arg( IM.neuropix_fifoFilling(), 0, 'f', 3 );

            sumGet = 0;
            sumThd = 0;
            shr.sumScl.fill( 0.0, p.im.nProbes );
            shr.sumEnq.fill( 0.0, p.im.nProbes );
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
/* pause ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::pause( bool pause, bool changed )
{
    if( pause ) {

        setPause( true );

        while( !isPauseAck() )
            usleep( 1e6 * 0.01 );

        return _pauseAcq();
    }
    else if( _resumeAcq( changed ) ) {

        setPause( false );
        setPauseAck( false );
    }

    return true;
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
bool CimAcqImec::fetchE( double loopT )
{
    shr.nE = 0;

    if( isPaused() )
        return true;

    do {

        int err = IM.neuropix_readElectrodeData( shr.E[shr.nE] );

        if( !shr.nE )
            shr.tStamp = loopT;

        if( err == DATA_BUFFER_EMPTY )
            return true;

        if( err != READ_SUCCESS ) {

            if( isPaused() )
                return true;

            runError(
                QString("IMEC readElectrodeData error %1.").arg( err ) );
            return false;
        }

    } while( ++shr.nE < shr.maxE );

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
        runError( QString("IMEC: Open error %1.").arg( err ) );
        return false;
    }

    SETVAL( 40 );
    return true;
}


bool CimAcqImec::_setLEDs()
{
    int err = IM.neuropix_ledOff( p.im.noLEDs );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC: LED off error %1.").arg( err ) );
        return false;
    }

    SETVAL( 50 );
    return true;
}


bool CimAcqImec::_manualProbeSettings( int ip )
{
    const CimCfg::ImProbeDat    &P = mainApp()->cfgCtl()->prbTab.probes[ip];

// MS: force not yet implemented per probe for 3B
    if( P.force ) {

        AsicID  A;
        A.serialNumber  = P.sn;
        A.probeType     = P.type - 1;

        int err = IM.neuropix_writeId( A );

        if( err != EEPROM_SUCCESS ) {
            runError( QString("IMEC: Probe %1 writeId error %2.")
            .arg( ip )
            .arg( err ) );
            return false;
        }

        Log() <<
            QString("IMEC: Manually set probe %1: SN=%2, type=%3")
            .arg( ip )
            .arg( A.serialNumber )
            .arg( (int)A.probeType + 1 );
    }

    SETVAL( 60 );
    return true;
}


bool CimAcqImec::_calibrateADC_fromFiles( int ip )
{
    SETLBL( QString("calibrate probe %1 ADC").arg( ip )  );

    const CimCfg::ImProbeDat    &P = mainApp()->cfgCtl()->prbTab.probes[ip];

#if 0
// MS: skipADC not yet implemented per probe for 3B
    if( P.skipADC ) {
        SETVAL( 100 );
        Log() <<
            QString("IMEC: Probe %1 ADC calibration -- SKIPPED BY USER --")
            .arg( ip );
        return true;
    }
#endif

    QString home    = appPath(),
            path    = QString("%1/ImecProbeData").arg( home );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    path = QString("%1/1%2%3")
            .arg( path )
            .arg( P.sn )
            .arg( P.type );

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
            QString("IMEC: Probe %1 readComparatorCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err1 ) );
        return false;
    }

    if( err2 != READCSV_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readADCOffsetCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err2 ) );
        return false;
    }

    if( err3 != READCSV_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readADCSlopeCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err3 ) );
        return false;
    }

// Read parameters from API

    err1 = IM.neuropix_getADCCompCalibration( C );

    if( err1 != SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 getADCCompCalibration error %2.")
            .arg( ip )
            .arg( err1 ) );
        return false;
    }

    err1 = IM.neuropix_getADCPairCommonCalibration( P );

    if( err1 != SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 getADCPairCommonCalibration error %2.")
            .arg( ip )
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
                    QString("IMEC: Probe %1 ADCCalibration error %2.")
                    .arg( ip )
                    .arg( err1 ) );
                return false;
            }
        }
    }

    SETVAL( 100 );
    Log() << QString("IMEC: Probe %1 ADC calibrated").arg( ip );
    return true;
}


// MS: calADC from EEPROM obsolete?
bool CimAcqImec::_calibrateADC_fromEEPROM( int ip )
{
    SETLBL( QString("calibrate probe %1 ADC").arg( ip ) );

    int err = IM.neuropix_applyAdcCalibrationFromEeprom();

    if( err != SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 applyAdcCalibrationFromEeprom error.")
            .arg( ip ) );
        return false;
    }

    SETVAL( 100 );
    Log() << QString("IMEC: Probe %1 ADC calibrated").arg( ip );
    return true;
}


bool CimAcqImec::_selectElectrodes()
{
    if( p.im.roTbl.type < 3 )
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
                QString("IMEC: SelectElectrode(%1,%2) error %3.")
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
    const int   *r2c = IMROTbl::typeTo_r2c( T.type );
    int         nRef = IMROTbl::typeToNRef( T.type );

    for( int ir = 1; ir < nRef; ++ir ) {

        if( !fRef[ir] ) {

            int ic  = r2c[ir];

            err = IM.neuropix_selectElectrode( ic, 0xFF, false );

            if( err != SHANK_SUCCESS ) {
                runError(
                    QString("IMEC: SelectElectrode(%1,%2) error %3.")
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
            QString("IMEC: SetExtRef(%1) error %2.")
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
                QString("IMEC: SetReference(%1,%2) error %3.")
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
                QString("IMEC: SetStandby(%1) error %2.")
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
                QString("IMEC: SetGain(%1,%2,%3) error %4.")
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
        runError( QString("IMEC: SetFilter error %1.").arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log()
        << "IMEC: Set highpass filter "
        << p.im.idxToFlt( p.im.hpFltIdx )
        << "Hz";
    return true;
}


bool CimAcqImec::_correctGain_fromFiles( int ip )
{
    if( !p.im.doGainCor )
        return true;

    SETLBL( QString("correct probe %1 gains").arg( ip ) );

    QString home    = appPath(),
            path    = QString("%1/ImecProbeData").arg( home );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    const CimCfg::ImProbeDat    &P = mainApp()->cfgCtl()->prbTab.probes[ip];

    path = QString("%1/1%2%3")
            .arg( path )
            .arg( P.sn )
            .arg( P.type );

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
            QString("IMEC: Probe %1 readGainCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err ) );
        return false;
    }

// Read params from API

    err = IM.neuropix_getGainCorrectionCalibration( G );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readGainCalibrationFromCsv error.")
            .arg( ip ) );
        return false;
    }

// Resize according to probe type

    G.resize( IMROTbl::typeToNElec( P.type ) );

// Write to basestation FPGA

    err = IM.neuropix_gainCorrection( G );

    if( err != CONFIG_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 gainCorrection error %2.")
            .arg( ip )
            .arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log() << QString("IMEC: Applied probe %1 gain corrections").arg( ip );
    return true;
}


// MS: correct gain from EEPROM obsolete?
bool CimAcqImec::_correctGain_fromEEPROM( int ip )
{
    if( !p.im.doGainCor )
        return true;

    SETLBL(
    QString("correct probe %1 gains...3 to 5 min...can't be aborted...")
    .arg( ip ) );

    int err = IM.neuropix_applyGainCalibrationFromEeprom();

    if( err != SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 applyGainCalibrationFromEeprom error.")
            .arg( ip ) );
        return false;
    }

    SETVAL( 100 );
    Log() << QString("IMEC: Applied probe %1 gain corrections").arg( ip );
    return true;
}


bool CimAcqImec::_setNeuralRecording()
{
    SETLBL( "set readout modes" );

    int err = IM.neuropix_mode( ASIC_RECORDING );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC: Recording mode error %1.").arg( err ) );
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
        runError( "IMEC: Datamode error." );
        return false;
    }

    SETVAL( 70 );
    Log() << "IMEC: Set electrode mode";
    return true;
}


bool CimAcqImec::_setTriggerMode()
{
    int     err = IM.neuropix_triggerMode( p.im.all.softStart );

    if( err != CONFIG_SUCCESS ) {
        runError( QString("IMEC: TriggerMode error %1.").arg( err ) );
        return false;
    }

    SETVAL( 100 );
    Log()
        << "IMEC: Trigger source: "
        << (p.im.all.softStart ? "software" : "hardware");
    return true;
}


bool CimAcqImec::_setStandbyAll()
{
    int err = IM.neuropix_writeAllStandby( false );

    if( err != BASECONFIG_SUCCESS ) {
        runError( QString("IMEC: WriteAllStandby error %1.").arg( err ) );
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
        runError( QString("IMEC: NRST( false ) error %1.").arg( err ) );
        return false;
    }

    SETVAL( 80 );

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC: Reset datapath error." );
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
        runError( QString("IMEC: NRST( false ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_prnrst( false );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC: PRNRST( false ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC: Reset datapath error." );
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

        if( !_setStandby() )
            return false;

        if( !_setGains() )
            return false;
    }

    int err = IM.neuropix_prnrst( true );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC: PRNRST( true ) error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_resetDatapath();

    if( err != SUCCESS ) {
        runError( "IMEC: Reset datapath error." );
        return false;
    }

    err = IM.neuropix_nrst( true );

    if( err != DIGCTRL_SUCCESS ) {
        runError( QString("IMEC: NRST( true ) error %1.").arg( err ) );
        return false;
    }

    return true;
}


bool CimAcqImec::configure()
{
// MS: For now, config only one probe
//    int nProbes = p.im.nProbes;
    int nProbes = 1;

    STOPCHECK;

    if( !_open() )
        return false;

    STOPCHECK;

    if( !_setLEDs() )
        return false;

    STOPCHECK;

    for( int ip = 0; ip < nProbes; ++ip ) {

        if( !_manualProbeSettings( ip ) )
            return false;

        STOPCHECK;
    }

    for( int ip = 0; ip < nProbes; ++ip ) {

        if( !_calibrateADC_fromFiles( ip ) )
            return false;

        STOPCHECK;
    }

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

    for( int ip = 0; ip < nProbes; ++ip ) {

        if( !_correctGain_fromFiles( ip ) )
            return false;

        STOPCHECK;
    }

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

    if( p.im.all.softStart ) {

// BK: Diagnostic test pattern
//        Log() << "te " << IM.neuropix_te( 1 );

        // Following completes sequence {nrst(F),resetDatapath,nrst(T)}.

        int err = IM.neuropix_nrst( true );

        if( err != DIGCTRL_SUCCESS ) {
            runError(
                QString("IMEC: NRST( true ) error %1.").arg( err ) );
            return false;
        }

        // - Reset SDRAM buffer
        // - Restart acquisition
        // - Set EXT_START pin

        err = IM.neuropix_setNeuralStart();

        if( err != CONFIG_SUCCESS ) {
            runError(
                QString("IMEC: SetNeuralStart error %1.").arg( err ) );
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


