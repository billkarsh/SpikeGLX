
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDir>
#include <QThread>


// T0FUDGE used to sync IM and NI stream tZero values.
// TPNTPERFETCH reflects the AP/LF sample rate ratio.
// OVERFETCH ensures we fetch a little more than loopSecs generates.
// READMAX is a temporary running mode until readElectrodeData fixed.
#define T0FUDGE         0.0
#define TPNTPERFETCH    12
#define OVERFETCH       1.20
//#define PROFILE
//#define READMAX


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

struct t_12x384 { const qint16  ap[12][384]; };
struct t_384    { const qint16  lf[384]; };
struct t_sh12   { const quint16 sy[12]; };


void ImAcqWorker::run()
{
    const int   nID = vID.size();

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
                const qint16    *srcLF  = ((t_384*)&shr.E[ie].lfpData)[0].lf;
                const quint16   *srcSY  = ((t_sh12*)&shr.E[ie].aux)[0].sy;

                float   *pLfLast    = &lfLast[iID][0];
                qint16  *dst        = &i16Buf[iID][TPNTPERFETCH * ie * shr.chnPerTpnt[pID]];

                for( int it = 0; it < TPNTPERFETCH; ++it ) {

                    // ----------
                    // ap - as is
                    // ----------

                    const qint16    *AP = srcAP->ap[it];
                    int             nap = shr.apPerTpnt[pID];

                    memcpy( dst, AP, nap*sizeof(qint16) );
                    dst += nap;

                    // -----------------
                    // lf - interpolated
                    // -----------------

                    float slope = float(it)/TPNTPERFETCH;

                    for( int lf = 0, nlf = shr.lfPerTpnt[pID]; lf < nlf; ++lf )
                        *dst++ = pLfLast[lf] + slope*(srcLF[lf]-pLfLast[lf]);

                    // ----
                    // sync
                    // ----

                    *dst++ = srcSY[it];
                }

                // ---------------
                // update saved lf
                // ---------------

                for( int lf = 0, nlf = shr.lfPerTpnt[pID]; lf < nlf; ++lf )
                    pLfLast[lf] = srcLF[lf];

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
                        i16Buf[iID], shr.totPts,
                        TPNTPERFETCH * shr.nE );

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
#ifdef READMAX
// 0.024 OK for Bill  laptop
// 0.055 OK for Win10 laptop
        loopSecs(0.055), shr( p, loopSecs ),
#else
        loopSecs(0.020), shr( p, loopSecs ),
#endif
        nThd(0), paused(false), pauseAck(false)
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
        QString("Require loop ms < [[ %1 ]] n > [[ %2 ]]")
        .arg( 1000*TPNTPERFETCH*shr.maxE/p.im.all.srate, 0, 'f', 3 )
        .arg( qRound( 5*p.im.all.srate/(TPNTPERFETCH*shr.maxE) ) );
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

        if( !fetchE( loopT ) )
            return;

        // ------------------
        // Handle empty fetch
        // ------------------

        if( !shr.nE ) {

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

        if( !shr.totPts ) {
            for( int ip = 0; ip < p.im.nProbes; ++ip )
                owner->imQ[ip]->setTZero( loopT + T0FUDGE );
        }

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

        // -----
        // Yield
        // -----

next_fetch:
        dT = getTime() - loopT;

#ifndef READMAX
        if( dT < loopSecs )
            usleep( 1e6*(loopSecs - dT) );
#endif

        // ---------------
        // Rate statistics
        // ---------------

        sumdT += dT;
        ++ndT;

        if( dT > peak_loopT )
            peak_loopT = dT;

        if( loopT - lastCheckT >= 5.0 && !isPaused() ) {

            int qf = fifoPct();

            if( qf >= 5 ) { // 5% standard

                Warning() <<
                    QString("IMEC FIFOQFill% %1, loop ms <%2> peak %3")
                    .arg( qf, 2, 10, QChar('0') )
                    .arg( 1000*sumdT/ndT, 0, 'f', 3 )
                    .arg( 1000*peak_loopT, 0, 'f', 3 );

                if( qf >= 95 ) {
                    runError(
                        QString(
                        "IMEC Ethernet queue overflow; stopping run.") );
                    return;
                }
            }

#ifdef PROFILE
// sumdT/ndT is the actual average loop time to process the samples.
// The maximum value is TPNTPERFETCH*maxE/srate.
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
// Thd measures the time spent waking and waiting for worker threads.
//
// nDT is the number of actual loop executions in the 5 sec check
// interval. The minimum value is 5*srate/(TPNTPERFETCH*maxE).
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
                " thd<%5> n(%6) %(%7)")
                .arg( 1000*sumdT/ndT, 0, 'f', 3 )
                .arg( 1000*sumGet/ndT, 0, 'f', 3 )
                .arg( 1000*sumScl/ndT, 0, 'f', 3 )
                .arg( 1000*sumEnq/ndT, 0, 'f', 3 )
                .arg( 1000*sumThd/ndT, 0, 'f', 3 )
                .arg( ndT )
                .arg( fifoPct(), 2, 10, QChar('0') );

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
/* update --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::update( int ip )
{
    setPause( true );

    while( !isPauseAck() )
        usleep( 1e6*loopSecs/8 );

    if( _pauseAcq() )
        _resumeAcq( ip );

    setPause( false );
    setPauseAck( false );
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
#ifdef READMAX
bool CimAcqImec::fetchE( double loopT )
{
    shr.nE = 0; // fetched packet count

// ----------------------------------
// Fill with zeros if hardware paused
// ----------------------------------

// MS: For now, just probe zero

   if( isPaused() ) {

zeroFill:
        setPauseAck( true );
        usleep( 1e6*loopSecs/8 );

        double  t0          = owner->imQ[0]->tZero();
        quint64 targetCt    = (loopT+loopSecs - t0) * p.im.all.srate;

        if( targetCt > shr.totPts ) {

            shr.nE =
                qMin( int((targetCt - shr.totPts)/TPNTPERFETCH), shr.maxE );

            if( shr.nE > 0 )
                memset( &shr.E[0], 0, shr.nE*sizeof(ElectrodePacket) );
        }

        return true;
    }

// --------------------
// Else fetch real data
// --------------------

// MS: For now, just fetch from probe zero

    const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[0]];

// Read exactly shr.maxE and hope it's real data

    int err = IM.readElectrodeData( P.slot, P.port, &shr.E[0], shr.maxE );

    if( err != SUCCESS ) {

        if( isPaused() )
            goto zeroFill;

        runError(
            QString("IMEC readElectrodeData(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    shr.nE = shr.maxE;

    return true;
}
#else
bool CimAcqImec::fetchE( double loopT )
{
    shr.nE = 0; // fetched packet count

// ----------------------------------
// Fill with zeros if hardware paused
// ----------------------------------

// MS: For now, just probe zero

    if( isPaused() ) {

zeroFill:
        setPauseAck( true );
        usleep( 1e6*loopSecs/8 );

        double  t0          = owner->imQ[0]->tZero();
        quint64 targetCt    = (loopT+loopSecs - t0) * p.im.all.srate;

        if( targetCt > shr.totPts ) {

            shr.nE =
                qMin( int((targetCt - shr.totPts)/TPNTPERFETCH), shr.maxE );

            if( shr.nE > 0 )
                memset( &shr.E[0], 0, shr.nE*sizeof(ElectrodePacket) );
        }

        return true;
    }

// --------------------
// Else fetch real data
// --------------------

// MS: For now, just fetch from probe zero

    const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[0]];

    uint    out;
    int     err = IM.readElectrodeData(
                    P.slot, P.port, &shr.E[0], out, shr.maxE );

    if( err != SUCCESS ) {

        if( isPaused() )
            goto zeroFill;

        runError(
            QString("IMEC readElectrodeData(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    shr.nE = out;

#if 0
// MS: Tune loopSecs and OVERFETCH

    Log()
        << out << " "
        << shr.maxE << " "
        << qRound( 100*(float(shr.maxE) - out)/shr.maxE );
#endif

    return true;
}
#endif

/* ---------------------------------------------------------------- */
/* fifoPct -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int CimAcqImec::fifoPct()
{
// MS: Filling approximated by probe zero, for efficiency
    quint8  pct;
    const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[0]];

    IM.fifoFilling( P.slot, P.port, 0, pct );
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

    int err = IM.openBS( STR2CHR( addr ) );

    if( err != SUCCESS ) {
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
    SETLBL( QString("open probe %1").arg( P.ip ), true );

    int err = IM.openProbe( P.slot, P.port );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC openProbe(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    err = IM.init( P.slot, P.port );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC init(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    SETVAL( 27 );
    return true;
}


bool CimAcqImec::_calibrateADC( const CimCfg::ImProbeDat &P )
{
    if( p.im.each[P.ip].skipCal ) {
        Log() << QString("IMEC Skipping probe %1 ADC calibration").arg( P.ip );
        return true;
    }

    SETLBL( QString("calibrate probe %1 ADC").arg( P.ip )  );

    QString path = QString("%1/ImecProbeData").arg( appPath() );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_ADCCalibration.csv").arg( path ).arg( P.sn ) ;

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file [%1].").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    int err = IM.setADCCalibration( P.slot, P.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setADCCalibration(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    SETVAL( 53 );
    Log() << QString("IMEC Probe %1 ADC calibrated").arg( P.ip );
    return true;
}


bool CimAcqImec::_calibrateGain( const CimCfg::ImProbeDat &P )
{
    if( p.im.each[P.ip].skipCal ) {
        Log() << QString("IMEC Skipping probe %1 gain calibration").arg( P.ip );
        return true;
    }

    SETLBL( QString("calibrate probe %1 gains").arg( P.ip ) );

    QString path = QString("%1/ImecProbeData").arg( appPath() );

    if( !QDir().mkpath( path ) ) {
        runError( QString("Failed to create folder [%1].").arg( path ) );
        return false;
    }

    path = QString("%1/%2/%2_gainCalValues.csv").arg( path ).arg( P.sn ) ;

    if( !QFile( path ).exists() ) {
        runError( QString("Can't find file [%1].").arg( path ) );
        return false;
    }

    path.replace( "/", "\\" );

    int err = IM.setGainCalibration( P.slot, P.port, STR2CHR( path ) );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setGainCalibration(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    SETVAL( 56 );
    Log() << QString("IMEC Probe %1 gains calibrated").arg( P.ip );
    return true;
}


// Synthetic data generation for testing:
// 0 = normal
// 1 = each chan is ADC id num
// 2 = each chan is chan id num
// 3 = each chan linear ramping
//
bool CimAcqImec::_dataGenerator( const CimCfg::ImProbeDat &P )
{
    int err = IM.selectDataSource( P.slot, P.port, 3 );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC selectDataSource(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    Log() << QString("IMEC Probe %1 generating synthetic data").arg( P.ip );
    return true;
}


bool CimAcqImec::_setLEDs( const CimCfg::ImProbeDat &P )
{
     SETLBL( QString("set probe %1 LED").arg( P.ip ) );

    int err = IM.setHSLed( P.slot, P.port, p.im.each[P.ip].LEDEnable );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setHSLed(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    SETVAL( 58 );
    Log() << QString("IMEC Probe %1 LED set").arg( P.ip );
    return true;
}


bool CimAcqImec::_selectElectrodes( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("select probe %1 electrodes").arg( P.ip ) );

    const IMROTbl   &T = p.im.each[P.ip].roTbl;
    int             nC = T.nChan(),
                    err;

// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        if( T.chIsRef( ic ) )
            continue;

        err = IM.selectElectrode( P.slot, P.port, ic, T.e[ic].bank );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC selectElectrode(slot %1, port %2) error %3.")
                .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }


    SETVAL( 59 );
    Log() << QString("IMEC Probe %1 electrodes selected").arg( P.ip );
    return true;
}


bool CimAcqImec::_setReferences( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 references").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan(),
                    err;

// ------------------------------------
// Connect all according to table refid
// ------------------------------------

// refid    (ref,bnk)   who
// -----    ---------   ---
//   0        (0,0)     ext
//   1        (1,0)     tip
//   2        (2,0)     192
//   3        (2,1)     576
//   4        (2,2)     960
//
    for( int ic = 0; ic < nC; ++ic ) {

        int rid = R.e[ic].refid,
            ref = (rid < 2 ? rid : 2),
            bnk = (rid > 1 ? rid - 2 : 0);


        err = IM.setReference( P.slot, P.port, ic,
                ChannelReference(ref), bnk );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setReference(slot %1, port %2) error %3.")
                .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }

    SETVAL( 60 );
    Log() << QString("IMEC Probe %1 references set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setGains( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 gains").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan(),
                    err;

// --------------------------------
// Set all according to table gains
// --------------------------------

    for( int ic = 0; ic < nC; ++ic ) {

        const IMRODesc  &E = R.e[ic];

        err = IM.setGain( P.slot, P.port, ic,
                    IMROTbl::gainToIdx( E.apgn ),
                    IMROTbl::gainToIdx( E.lfgn ) );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setGain(slot %1, port %2) error %3.")
                .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }

    SETVAL( 61 );
    Log() << QString("IMEC Probe %1 gains set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setHighPassFilter( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 filters").arg( P.ip ) );

    const IMROTbl   &R = p.im.each[P.ip].roTbl;
    int             nC = R.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        int err = IM.setAPCornerFrequency( P.slot, P.port, ic, R.e[ic].apflt );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setAPCornerFrequency(slot %1, port %2) error %3.")
                .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }

    SETVAL( 62 );
    Log() << QString("IMEC Probe %1 filters set").arg( P.ip );
    return true;
}


bool CimAcqImec::_setStandby( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("set probe %1 standby").arg( P.ip ) );

// --------------------------------------------------
// Turn ALL channels on or off according to stdbyBits
// --------------------------------------------------

    int nC = p.im.each[P.ip].roTbl.nChan();

    for( int ic = 0; ic < nC; ++ic ) {

        int err = IM.setStdb( P.slot, P.port, ic,
                    p.im.each[P.ip].stdbyBits.testBit( ic ) );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setStandby(slot %1, port %2) error %3.")
                .arg( P.slot ).arg( P.port ).arg( err ) );
            return false;
        }
    }

    SETVAL( 63 );
    Log() << QString("IMEC Probe %1 standby chans set").arg( P.ip );
    return true;
}


bool CimAcqImec::_writeProbe( const CimCfg::ImProbeDat &P )
{
    SETLBL( QString("writing probe %1...").arg( P.ip ) );

    int err = IM.writeProbeConfiguration( P.slot, P.port, true );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC writeProbeConfig(slot %1, port %2) error %3.")
            .arg( P.slot ).arg( P.port ).arg( err ) );
        return false;
    }

    SETVAL( 100 );
    return true;
}


bool CimAcqImec::_setTrigger( bool software )
{
    SETLBL( "set triggering", true );

    for( int is = 0, ns = T.slot.size(); is < ns; ++is ) {

        int err = IM.setTriggerSource( is,
                    (software ? 0 : p.im.all.trgSource) );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerSource(slot %1) error %2.")
                .arg( is ).arg( err ) );
            return false;
        }

        SETVAL( (is+1)*33/ns );

        err = IM.setTriggerEdge( is, p.im.all.trgRising );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setTriggerEdge(slot %1) error %2.")
                .arg( is ).arg( err ) );
            return false;
        }
    }

    SETVAL( 66 );
    if( !software ) {
        Log()
            << "IMEC Trigger source: "
            << (p.im.all.trgSource ? "hardware" : "software");
    }
    return true;
}


bool CimAcqImec::_setArm()
{
    SETLBL( "arm system" );

    if( !_arm() )
        return false;

    SETVAL( 100 );
    Log() << "IMEC Armed";
    return true;
}


bool CimAcqImec::_arm()
{
    for( int is = 0, ns = T.slot.size(); is < ns; ++is ) {

        int err = IM.arm( T.slot[is] );

        if( err != SUCCESS ) {
            runError(
                QString("IMEC arm(slot %1) error %2.")
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

        if( err != SUCCESS ) {
            runError(
                QString("IMEC setSWTrigger(slot %1) error %2.")
                .arg( T.slot[is] ).arg( err ) );
            return false;
        }
    }

    if( T.comIdx > 0 ) {

        for( int is = 0, ns = T.slot.size(); is < ns; ++is ) {

            int err = IM.startInfiniteStream(  T.slot[is] );

            if( err != SUCCESS ) {
                runError(
                    QString("IMEC startInfiniteStream error %1.")
                    .arg( err ) );
                return false;
            }
        }
    }

    return true;
}


bool CimAcqImec::_pauseAcq()
{
    for( int is = 0, ns = T.slot.size(); is < ns; ++is )
        IM.stopInfiniteStream( T.slot[is] );

    return _arm();
}


bool CimAcqImec::_resumeAcq( int ip )
{
    if( ip >= 0 ) {

        const CimCfg::ImProbeDat    &P = T.probes[T.id2dat[ip]];

        if( !_selectElectrodes( P ) )
            return false;

        if( !_setReferences( P ) )
            return false;

        if( !_setGains( P ) )
            return false;

        if( !_setStandby( P ) )
            return false;

        if( !_writeProbe( P ) )
            return false;
    }

    if( !_setTrigger( true ) )
        return false;

    if( !_arm() )
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

        if( !_calibrateADC( P ) )
            return false;

        STOPCHECK;

        if( !_calibrateGain( P ) )
            return false;

//        STOPCHECK;

//        if( !_dataGenerator( P ) )
//            return false;

        STOPCHECK;

        if( !_setLEDs( P ) )
            return false;

        STOPCHECK;

        if( !_selectElectrodes( P ) )
            return false;

        STOPCHECK;

        if( !_setReferences( P ) )
            return false;

        STOPCHECK;

        if( !_setGains( P ) )
            return false;

        STOPCHECK;

        if( !_setHighPassFilter( P ) )
            return false;

        STOPCHECK;

        if( !_setStandby( P ) )
            return false;

        STOPCHECK;

        if( !_writeProbe( P ) )
            return false;

        STOPCHECK;
    }

    if( !_setTrigger() )
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

    if( !p.im.all.trgSource ) {

        if( !_softStart() )
            return false;

        Log() << "IMEC Acquisition started";
    }
    else
        Log() << "IMEC Waiting for external trigger";

    return true;
}

/* ---------------------------------------------------------------- */
/* close ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::close()
{
    if( T.comIdx > 0 ) {

        for( int is = 0, ns = T.slot.size(); is < ns; ++is )
            IM.stopInfiniteStream( T.slot[is] );
    }

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


