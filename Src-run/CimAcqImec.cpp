
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
/* CimAcqImec ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// MS: loopSecs for ThinkPad T450 (2 core)
// MS: [[ Core i7-5600U @ 2.6Ghz, 8GB, Win7Pro-64bit ]]
// MS: 1 probe 0.005 with both audio and shankview
// MS: 4 probe 0.005 with both audio and shankview
// MS: 5 probe 0.010 if just audio or shankview
// MS: 6 probe 0.050 if no audio or shankview
//
CimAcqImec::CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
:   CimAcq( owner, p ),
    T(mainApp()->cfgCtl()->prbTab),
    loopSecs(0.005), shr( p, loopSecs ),
    nThd(0), pauseAck(false)
{
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

            float   qf = fifoPct();

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
                .arg( fifoPct(), 0, 'f', 3 );

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

bool CimAcqImec::pause( bool pause, int ipChanged )
{
    if( pause ) {

        setPause( true );

        while( !isPauseAck() )
            usleep( 1e6 * 0.01 );

        return _pauseAcq();
    }
    else if( _resumeAcq( ipChanged ) ) {

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
// MS: For now, just fetch from probe zero

    const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[0]];

    shr.nE = 0; // fetched packet count

    if( isPaused() )
        return true;

    do {

        int err = IM.readElectrodeData( P.slot, P.port, shr.E[shr.nE] );

        if( !shr.nE )
            shr.tStamp = loopT;

        if( err == DATA_BUFFER_EMPTY )
            return true;

        if( err != XXX_SUCCESS ) {

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
/* fifoPct -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

float CimAcqImec::fifoPct()
{
// MS: Filling approximated by probe zero, for efficiency
    double  pct;
    const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[0]];

    IM.fifoFilling( P.slot, P.port, pct );
    return pct;
}

/* ---------------------------------------------------------------- */
/* configure ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define STOPCHECK   if( isStopped() ) return false;


void CimAcqImec::SETLBL( const QString &s, bool zero )
{
    QMetaObject::invokeMethod(
        mainApp(), "runInitSetLabel",
        Qt::QueuedConnection,
        Q_ARG(QString, s),
        Q_ARG(bool, zero) );
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


bool CimAcqImec::_open( const CimCfg::ImProbeTable &T )
{
    SETLBL( "open session", true );

    if( T.comIdx == 0 ) {
        runError( "PXI interface not yet supported." );
        return false;
    }

    QString addr = QString("10.2.0.%1").arg( T.comIdx - 1 );

    err = IM.openBS( addr );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC openBS( %1 ) error %2.")
            .arg( addr ).arg( err ) );
        return false;
    }

    SETVAL( 100 );
    return true;
}


bool CimAcqImec::_openProbe( const CimCfg::ImProbeDat &P )
{
    SETLBL( "open probe", true );
double  t0 = getTime();

    int err = IM.openProbe( P.slot, P.port );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: openProbe(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    err = IM.init( P.slot, P.port );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: init(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

Log()<<"dt _openProbe "<<getTime()-t0;
    SETVAL( 5 );
    return true;
}


bool CimAcqImec::_calibrateADC( const CimCfg::ImProbeDat &P, int ip )
{
#if 0
// MS: skipADC not yet implemented per probe for 3B
    if( P.skipADC ) {
        Log() <<
            QString("IMEC: Probe %1 ADC calibration -- SKIPPED BY USER --")
            .arg( ip );
        return true;
    }
#endif

    SETLBL( QString("calibrate probe %1 ADC").arg( ip )  );
double  t0 = getTime();

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

    if( err1 != XXX_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readComparatorCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err1 ) );
        return false;
    }

    if( err2 != XXX_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readADCOffsetCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err2 ) );
        return false;
    }

    if( err3 != XXX_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 readADCSlopeCalibrationFromCsv error %2.")
            .arg( ip )
            .arg( err3 ) );
        return false;
    }

// Read parameters from API

    err1 = IM.neuropix_getADCCompCalibration( C );

    if( err1 != XXX_SUCCESS ) {
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

            if( err1 != XXX_SUCCESS ) {
                runError(
                    QString("IMEC: Probe %1 ADCCalibration error %2.")
                    .arg( ip )
                    .arg( err1 ) );
                return false;
            }
        }
    }

Log()<<"dt _calibrateADC "<<getTime()-t0;
    SETVAL( 10 );
    Log() << QString("IMEC: Probe %1 ADC calibrated").arg( ip );
    return true;
}


bool CimAcqImec::_calibrateGain( const CimCfg::ImProbeDat &P, int ip )
{
    if( !p.im.doGainCor )
        return true;

    SETLBL( QString("calibrate probe %1 gains").arg( ip ) );
double  t0 = getTime();

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

    std::vector<unsigned short> G;
    int                         err;

// Read from csv to API
//
// Note: The read functions don't understand paths.

    QDir::setCurrent( path );

        err = IM.neuropix_readGainCalibrationFromCsv(
                "Gain correction.csv" );

    QDir::setCurrent( home );

    if( err != XXX_SUCCESS ) {
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

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: Probe %1 gainCorrection error %2.")
            .arg( ip )
            .arg( err ) );
        return false;
    }

Log()<<"dt _calibrateGain "<<getTime()-t0;
    SETVAL( 20 );
    Log() << QString("IMEC: Applied probe %1 gain corrections").arg( ip );
    return true;
}


bool CimAcqImec::_setLEDs( const CimCfg::ImProbeDat &P, int ip )
{
     SETLBL( "set LEDs" );
double  t0 = getTime();

   int err = IM.setHSLed( P.slot, P.port, p.im.each[ip].LEDEnable );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: setHSLed(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

Log()<<"dt _setLEDs "<<getTime()-t0;
    SETVAL( 30 );
    return true;
}


bool CimAcqImec::_manualProbeSettings( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "override sn" );
double  t0 = getTime();

// MS: force not yet implemented per probe for 3B
    if( P.force ) {

        AsicID  A;
        A.serialNumber  = P.sn;
        A.probeType     = P.type - 1;

        int err = IM.neuropix_writeId( A );

        if( err != XXX_SUCCESS ) {
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

Log()<<"dt _manualProbeSettings "<<getTime()-t0;
    SETVAL( 40 );
    return true;
}


bool CimAcqImec::_selectElectrodes( const CimCfg::ImProbeDat &P, int ip )
{
    if( p.im.roTbl.type < 3 )
        return true;

    SETLBL( "select electrodes" );
double  t0 = getTime();

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.nChan(),
                    err;

// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        err = IM.neuropix_selectElectrode( ic, T.e[ic].bank, false );

        if( err != XXX_SUCCESS ) {
            runError(
                QString("IMEC: SelectElectrode(%1,%2) error %3.")
                .arg( ic ).arg( T.e[ic].bank ).arg( err ) );
            return false;
        }
    }

    SETVAL( 42 );

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

            if( err != XXX_SUCCESS ) {
                runError(
                    QString("IMEC: SelectElectrode(%1,%2) error %3.")
                    .arg( ic ).arg( 0xFF ).arg( err ) );
                return false;
            }
        }
    }
#endif

    SETVAL( 45 );

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

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: SetExtRef(%1) error %2.")
            .arg( fRef[0] > 0 ).arg( err ) );
        return false;
    }

Log()<<"dt _selectElectrodes "<<getTime()-t0;
    SETVAL( 50 );
    Log() << "IMEC: Electrodes selected";
    return true;
}


bool CimAcqImec::_setReferences( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "set references" );
double  t0 = getTime();

    const IMROTbl   &T = p.im.roTbl;
    int             nC = T.nChan(),
                    err;

// ------------------------------------
// Connect all according to table refid
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        err = IM.neuropix_setReference( ic, T.e[ic].refid, false );

        if( err != XXX_SUCCESS ) {
            runError(
                QString("IMEC: SetReference(%1,%2) error %3.")
                .arg( ic ).arg( T.e[ic].refid ).arg( err ) );
            return false;
        }
    }

Log()<<"dt _setReferences "<<getTime()-t0;
    SETVAL( 60 );
    Log() << "IMEC: References set";
    return true;
}


bool CimAcqImec::_setGains( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "set gains" );
double  t0 = getTime();

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

        if( err != XXX_SUCCESS ) {
            runError(
                QString("IMEC: SetGain(%1,%2,%3) error %4.")
                .arg( ic ).arg( E.apgn ).arg( E.lfgn ).arg( err ) );
            return false;
        }
    }

    SETVAL( 65 );

// -------------------
// Download selections
// -------------------

    IM.neuropix_setGain(
        0,
        IMROTbl::gainToIdx( T.e[0].apgn ),
        IMROTbl::gainToIdx( T.e[0].lfgn ),
        true );

Log()<<"dt _setGains "<<getTime()-t0;
    SETVAL( 70 );
    Log() << "IMEC: Gains set";
    return true;
}


bool CimAcqImec::_setHighPassFilter( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "set filters" );
double  t0 = getTime();

    int err = IM.neuropix_setFilter( p.im.hpFltIdx );

    if( err != XXX_SUCCESS ) {
        runError( QString("IMEC: SetFilter error %1.").arg( err ) );
        return false;
    }

Log()<<"dt _setHighPassFilter "<<getTime()-t0;
    SETVAL( 80 );
    Log()
        << "IMEC: Set highpass filter "
        << p.im.idxToFlt( p.im.hpFltIdx )
        << "Hz";
    return true;
}


bool CimAcqImec::_setStandby( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "set standby" );
double  t0 = getTime();

// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = p.im.roTbl.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        int err = IM.setStdb( P.slot, P.port, ic,
                    p.im.each[ip].stdbyBits.testBit( ic ) );

        if( err != XXX_SUCCESS ) {
            runError(
                QString("IMEC: SetStandby(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }

Log()<<"dt _setStandby "<<getTime()-t0;
    SETVAL( 90 );
    Log() << "IMEC: Standby channels set";
    return true;
}


bool CimAcqImec::_writeProbe( const CimCfg::ImProbeDat &P, int ip )
{
    SETLBL( "writing...", true );
double  t0 = getTime();

    int err = IM.writeProbeConfiguration( P.slot, P.port, true );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: writeProbeConfig(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

Log()<<"dt _writeProbe "<<getTime()-t0;
    SETVAL( 100 );
    return true;
}


bool CimAcqImec::_setTriggerMode()
{
    SETLBL( "set triggering", true );

    int err = IM.setTriggerSource( slot, source );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: setTriggerSource(slot %1) error %2.")
            .arg( slot ).arg( err ) );
        return false;
    }

    SETVAL( 33 );

    err = IM.setTriggerEdge( slot, rising );

    if( err != XXX_SUCCESS ) {
        runError(
            QString("IMEC: setTriggerEdge(slot %1) error %2.")
            .arg( slot ).arg( err ) );
        return false;
    }

    SETVAL( 66 );
    Log()
        << "IMEC: Trigger source: "
        << (p.im.all.softStart ? "software" : "hardware");
    return true;
}


bool CimAcqImec::_setArm()
{
    SETLBL( "arm system" );

    if( !_arm() )
        return false;

    SETVAL( 100 );
    Log() << "IMEC: Armed";
    return true;
}


bool CimAcqImec::_arm()
{
    for( int is = 0, ns = T.slot.size(); is < ns; ++is ) {

        int err = IM.arm( T.slot[is] );

        if( err != XXX_SUCCESS ) {
            runError( QString("IMEC: arm(slot %1) error %2."))
                .arg( T.slot[is] ).arg( err ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::_softStart()
{
    for( int is = 0, ns = T.slot.size(); is < ns; ++is ) {

        int err = IM.setSWTrigger( T.slot[is] );

        if( err != XXX_SUCCESS ) {
            runError(
                QString("IMEC: setSWTrigger(slot %1) error %2.")
                .arg( T.slot[is] ).arg( err ) );
            return false;
        }
    }

    return true;
}


bool CimAcqImec::_pauseAcq()
{
    return arm()
}


bool CimAcqImec::_resumeAcq( int ipChanged )
{
    if( ipChanged >= 0 ) {

        const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[ipChanged]];

        if( !_selectElectrodes( P, ip ) )
            return false;

        if( !_setReferences( P, ip ) )
            return false;

        if( !_setGains( P, ip ) )
            return false;

        if( !_setStandby( P, ip ) )
            return false;

        if( !_writeProbe( P, ip ) )
            return false;
    }

    if( !arm() )
        return false;

    return _softStart();
}


bool CimAcqImec::configure()
{
    STOPCHECK;

    if( !_open( T ) )
        return false;

    STOPCHECK;

    for( int ip = 0; ip < p.im.nProbes; ++ip ) {

        const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[ip]];

        if( !_openProbe( P ) )
            return false;

        STOPCHECK;

        if( !_calibrateADC( P, ip ) )
            return false;

        STOPCHECK;

        if( !_calibrateGain( P, ip ) )
            return false;

        STOPCHECK;

        if( !_setLEDs( P, ip ) )
            return false;

        STOPCHECK;

        if( !_manualProbeSettings( P, ip ) )
            return false;

        STOPCHECK;

        if( !_selectElectrodes( P, ip ) )
            return false;

        STOPCHECK;

        if( !_setReferences( P, ip ) )
            return false;

        STOPCHECK;

        if( !_setGains( P, ip ) )
            return false;

        STOPCHECK;

        if( !_setHighPassFilter( P, ip ) )
            return false;

        STOPCHECK;

        if( !_setStandby( P, ip ) )
            return false;

        STOPCHECK;

        if( !_writeProbe( P, ip ) )
            return false;

        STOPCHECK;
    }

    if( !_setTriggerMode() )
        return false;

    if( !_setArm() )
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

        if( !_softStart() )
            return false;

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
    for( int is = 0, ns = T.slot.size(); is < ns; ++is )
        IM.close( T.slot[is], -1 );
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


