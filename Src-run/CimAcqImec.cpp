
#ifdef HAVE_IMEC

#include "CimAcqImec.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "MetricsWindow.h"
#include "Run.h"
#include "Subset.h"
#include "Biquad.h"

#include <QThread>


// TPNTPERFETCH reflects the AP/LF sample rate ratio.
#define TPNTPERFETCH    12
#define OBX_N_ACQ       24
#define AVEE            5
#define MAXE            24

// Experiment switches
//#define TESTDUPSAMPS
//#define TSTAMPCHECKS
//#define LATENCYFILE
//#define PROFILE
//#define TUNE            0
//#define TUNEOBX         0


/* ---------------------------------------------------------------- */
/* ImSimLfDat ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define PFBUFSMP    (4 * MAXE * TPNTPERFETCH)

ImSimLfDat::~ImSimLfDat()
{
    if( f ) {
        f->close();
        delete f;
        f = 0;
    }
}


bool ImSimLfDat::init( QString &err, const QString &pfName )
{
// existence tests

    QFile   ftest;

    ftest.setFileName( pfName + ".lf.meta" );
    if( !ftest.exists() ) {
        Log() << QString("Missing probe file '%1'.").arg( pfName + ".lf.meta" );
        return true;
    }

    ftest.setFileName( pfName + ".lf.bin" );
    if( !ftest.exists() ) {
        Log() << QString("Missing probe file '%1'.").arg( pfName + ".lf.bin" );
        return true;
    }

// init

    KVParams    kvp;
    kvp.fromMetaFile( pfName + ".lf.meta" );

    QStringList sl;

    sl = kvp["acqApLfSy"].toString().split(
            QRegExp("^\\s+|\\s*,\\s*"),
            QString::SkipEmptyParts );
    for( int i = 0; i < 3; ++i )
        acq[i] = sl[i].toInt();

    nG      = kvp["nSavedChans"].toInt();
    nC      = acq[1] + acq[2];
    smpEOF  = kvp["fileSizeBytes"].toLongLong() / (nG*sizeof(qint16));

    ibuf_ic.resize( PFBUFSMP * nC );

    if( nC != nG ) {

        Subset::rngStr2Vec( ig2ic, kvp["snsSaveChanSubset"].toString() );
        ibuf_ig.resize( nG );

        // LF & SY indices consecutively follow AP indices,
        // so offset everything after AP back to zero.

        for( int ig = 0; ig < nG; ++ig )
            ig2ic[ig] -= acq[0];
    }

    f = new QFile( pfName + ".lf.bin" );

    if( !f->open( QIODevice::ReadOnly ) ) {
        err = QString("error opening file '%1'.").arg( pfName + ".lf.bin" );
        return false;
    }

    return true;
}


void ImSimLfDat::load1()
{
    if( !f )
        return;

// file rollover

    if( smpCur >= smpEOF ) {
        f->seek( 0 );
        smpCur = 0;
    }
    ++smpCur;

// load sample

    if( inbuf >= PFBUFSMP )
        inbuf = 0;

    if( nC == nG )
        f->read( (char*)&ibuf_ic[inbuf*nC], nC*sizeof(qint16) );
    else {
        uint    *map = &ig2ic[0];
        qint16  *src = &ibuf_ig[0],
                *dst;

        f->read( (char*)src, nG*sizeof(qint16) );

        dst = &ibuf_ic[inbuf*nC];
        memset( dst, 0, nC*sizeof(qint16) );
        for( int ig = 0; ig < nG; ++ig )
            dst[map[ig]] = *src++;
    }

    ++inbuf;
}


void ImSimLfDat::get_ie( struct electrodePacket* E, int ie, int nC )
{
    if( f )
        memcpy( &E->lfpData[0], &ibuf_ic[ie*nC], acq[1]*sizeof(qint16) );
    else
        memset( &E->lfpData[0], 0, nC*sizeof(qint16) );
}


void ImSimLfDat::retireN( int n )
{
    if( !f )
        return;

    if( inbuf > n )
        memcpy( &ibuf_ic[0], &ibuf_ic[n*nC], (inbuf-n)*nC*sizeof(qint16) );

    inbuf -= n;
}

/* ---------------------------------------------------------------- */
/* ImSimApDat ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimApDat::~ImSimApDat()
{
    if( f ) {
        f->close();
        delete f;
        f = 0;
    }
}


bool ImSimApDat::init( QString &err, const QString &pfName )
{
    KVParams    kvp;
    kvp.fromMetaFile( pfName + ".ap.meta" );

    QStringList sl;

    sl = kvp["acqApLfSy"].toString().split(
            QRegExp("^\\s+|\\s*,\\s*"),
            QString::SkipEmptyParts );
    for( int i = 0; i < 3; ++i )
        acq[i] = sl[i].toInt();

    {
        IMROTbl *R = IMROTbl::alloc( kvp["imDatPrb_pn"].toString() );
            fetchType = R->apiFetchType();
        delete R;
    }

    nG      = kvp["nSavedChans"].toInt();
    nC      = acq[0] + acq[2];
    smpEOF  = kvp["fileSizeBytes"].toLongLong() / (nG*sizeof(qint16));

    ibuf_ic.resize( PFBUFSMP * nC );

    if( nC != nG ) {

        Subset::rngStr2Vec( ig2ic, kvp["snsSaveChanSubset"].toString() );
        ibuf_ig.resize( nG );

        // If a gap (acq[1]) between AP and SY indices
        // then make SY indices consecutively follow AP

        if( acq[1] ) {
            for( int sy = acq[2]; sy > 0; --sy ) {
                if( ig2ic[nG-sy] > acq[0] )
                    ig2ic[nG-sy] = acq[0] + acq[2] - sy;
            }
        }
    }

    f = new QFile( pfName + ".ap.bin" );

    if( !f->open( QIODevice::ReadOnly ) ) {
        err = QString("error opening file '%1'.").arg( pfName + ".ap.bin" );
        return false;
    }

    return true;
}


bool ImSimApDat::load1()
{
// file rollover

    if( smpCur >= smpEOF ) {
        f->seek( 0 );
        smpCur = 0;
    }
    ++smpCur;
    ++tstamp;

// load sample

    if( inbuf >= PFBUFSMP )
        inbuf = 0;

    if( nC == nG )
        f->read( (char*)&ibuf_ic[inbuf*nC], nC*sizeof(qint16) );
    else {
        uint    *map = &ig2ic[0];
        qint16  *src = &ibuf_ig[0],
                *dst;

        f->read( (char*)src, nG*sizeof(qint16) );

        dst = &ibuf_ic[inbuf*nC];
        memset( dst, 0, nC*sizeof(qint16) );
        for( int ig = 0; ig < nG; ++ig )
            dst[map[ig]] = *src++;
    }

    ++inbuf;

    return tstamp % 12 == 1;
}


void ImSimApDat::fetchT0( struct electrodePacket* E, int* out, ImSimLfDat &LF )
{
    if( !inbuf ) {
        *out = 0;
        return;
    }

    int n = qMin( inbuf / TPNTPERFETCH, MAXE );

// deliver

    qint16  *src = &ibuf_ic[0];

    for( int ie = 0; ie < n; ++ie, ++E ) {

        for( int t = 0; t < TPNTPERFETCH; ++t, src += nC ) {

            memcpy( &E->apData[t][0], src, acq[0]*sizeof(qint16) );
            E->timestamp[t] = 3 * (tstamp - inbuf + ie*TPNTPERFETCH + t);
            E->Status[t]    = src[acq[0]];
        }

        LF.get_ie( E, ie, acq[0] );
    }

    *out = n;

// retire LF

    LF.retireN( n );

// retire AP

    n *= TPNTPERFETCH;

    if( inbuf > n )
        memcpy( &ibuf_ic[0], &ibuf_ic[n*nC], (inbuf-n)*nC*sizeof(qint16) );

    inbuf -= n;
}


void ImSimApDat::fetchT2(struct PacketInfo* H, int16_t* D, int is, int nAP, int smpMax, int* out )
{
    if( !inbuf ) {
        *out = 0;
        return;
    }

    int n = qMin( inbuf, qMin( smpMax, MAXE * TPNTPERFETCH ) );

// deliver

    qint16  *src = &ibuf_ic[0];

    for( int i = 0; i < n; ++i, src += nC, D += nAP, ++H ) {

        memcpy( D, src + is * nAP, nAP*sizeof(qint16) );
        H->Timestamp    = 3 * (tstamp - inbuf + i);
        H->Status       = src[acq[0] + is];
    }

    *out = n;

// retire if all shanks fetched

    if( nAP == acq[0] || is == 3 ) {

        if( inbuf > n )
            memcpy( &ibuf_ic[0], &ibuf_ic[n*nC], (inbuf-n)*nC*sizeof(qint16) );

        inbuf -= n;
    }
}

/* ---------------------------------------------------------------- */
/* ImSimDat ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimDat::~ImSimDat()
{
    if( bufMtx ) {
        delete bufMtx;
        bufMtx = 0;
    }
}


bool ImSimDat::init( QString &err, const QString &pfName )
{
    bufMtx = new QMutex;

    if( !AP.init( err, pfName ) )
        return false;

    if( AP.acq[1] )
        return LF.init( err, pfName );

    return true;
}


void ImSimDat::loadToN( qint64 N )
{
    while( AP.tstamp < N ) {
        bufMtx->lock();
            if( AP.load1() )
                LF.load1();
        bufMtx->unlock();
    }
}


void ImSimDat::fifo( int *packets, int *empty ) const
{
    bufMtx->lock();
        *packets = AP.inbuf;
    bufMtx->unlock();

    *empty = PFBUFSMP - *packets;

    if( AP.fetchType == 0 ) {
        *packets /= TPNTPERFETCH;
        *empty = PFBUFSMP / TPNTPERFETCH - *packets;
    }
}


void ImSimDat::fetchT0( struct electrodePacket* E, int* out )
{
    QMutexLocker    ml( bufMtx );
    AP.fetchT0( E, out, LF );
}


void ImSimDat::fetchT2(struct PacketInfo* H, int16_t* D, int is, int nAP, int smpMax, int* out )
{
    QMutexLocker    ml( bufMtx );
    AP.fetchT2( H, D, is, nAP, smpMax, out );
}

/* ---------------------------------------------------------------- */
/* ImSimPrbWorker ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Loop period is 1.0 packet (TPNTPERFETCH).
//
void ImSimPrbWorker::run()
{
    double      T0              = getTime();
    const int   rate            = 3e4;
    const int   loopPeriod_us   = TPNTPERFETCH * 1e6 / rate;

    while( !isStopped() ) {

        double  loopT = getTime();

        for( int i = 0, n = simDat.size(); i < n; ++i )
            simDat[i].loadToN( (getTime() - T0) * rate );

        // Fetch no more often than every loopPeriod_us

        loopT = 1e6*(getTime() - loopT);    // microsec

        if( loopT < loopPeriod_us )
            QThread::usleep( loopPeriod_us - loopT );
        else
            QThread::usleep( loopPeriod_us );
    }

    emit finished();
}

/* ---------------------------------------------------------------- */
/* ImSimThread ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImSimPrbThread::ImSimPrbThread( std::vector<ImSimDat> &simDat )
{
    thread  = new QThread;
    worker  = new ImSimPrbWorker( simDat );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

    thread->start();
}


ImSimPrbThread::~ImSimPrbThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() ) {

        worker->stop();
        thread->wait();
    }

    delete thread;
}

/* ---------------------------------------------------------------- */
/* ImAcqShared ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqShared::ImAcqShared()
    :   awake(0), asleep(0), ip_CAR(-1), stop(false)
{
// Experiment to histogram successive timestamp differences.
#ifdef TSTAMPCHECKS
    tStampBins.assign( 34, 0 );
    tStampEvtByPrb.assign( 32, 0 );
#endif
}


// Experiment to histogram successive timestamp differences.
//
ImAcqShared::~ImAcqShared()
{
#ifdef TSTAMPCHECKS
    Log()<<"------ Intrafetch timestamp diffs ------";
    for( int i = 0; i < 34; ++i )
        Log()<<QString("bin %1  N %2").arg( i ).arg( tStampBins[i] );
    Log()<<"---- Timestamp incidents each probe ----";
    for( int i = 0; i < 32; ++i )
        Log()<<QString("probe %1  N %2").arg( i ).arg( tStampEvtByPrb[i] );
    Log()<<"----------------------------------------";
#endif
}


// Experiment to histogram successive timestamp differences.
// One can collect just difs within packets, or just between
// packets, or both.
//
void ImAcqShared::tStampHist_EPack( const electrodePacket* E, int ip, int ie, int it )
{
#ifdef TSTAMPCHECKS
    qint64  dif = -999999;
    if( it > 0 )        // intra-packet
        dif = E[ie].timestamp[it] - E[ie].timestamp[it-1];
    else if( ie > 0 )   // inter-packet
        dif = E[ie].timestamp[0] - E[ie-1].timestamp[11];

// @@@ FIX Experiment to report the zero delta value.
#if 1
    if( dif == 0 ) {
        if( !tStampEvtByPrb[ip] ) {
            Warning()<<
            QString("ZERO TS probe %1, value %2")
            .arg( ip ).arg( E[0].timestamp[0] );
        }
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 0
    if( dif == 0 ) {
        Log()<<QString("ZERO TSTAMP DIF: stamp %1")
        .arg( E[ie].timestamp[it] );
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 0
    if( dif > 31 ) {
        Log()<<QString("BIGDIF: ip %1 dif %2 stamp %3")
        .arg( ip ).arg( dif ).arg( E[ie].timestamp[it] );
    }
#endif

    if( dif != -999999 ) {

        if( dif < 0 )
            ++tStampBins[32];
        else if( dif > 31 )
            ++tStampBins[33];
        else
            ++tStampBins[dif];

        if( dif < 3 || dif > 4 )
            ++tStampEvtByPrb[ip];
    }
#else
    Q_UNUSED( E )
    Q_UNUSED( ip )
    Q_UNUSED( ie )
    Q_UNUSED( it )
#endif
}


void ImAcqShared::tStampHist_PInfo( const PacketInfo* H, int ip, int it )
{
#ifdef TSTAMPCHECKS
    qint64  dif = -999999;
    if( it > 0 )        // intra-packet
        dif = H[it].Timestamp - H[it-1].Timestamp;

// @@@ FIX Experiment to report the zero delta value.
#if 1
    if( dif == 0 ) {
        if( !tStampEvtByPrb[ip] ) {
            Warning()<<
            QString("ZERO TS probe %1, value %2")
            .arg( ip ).arg( H[1].Timestamp );
        }
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 0
    if( dif == 0 ) {
        Log()<<QString("ZERO TSTAMP DIF: stamp %1")
        .arg( H[it-1].Timestamp );
    }
#endif

// @@@ FIX Report disabled: too many instances.
#if 1
    if( dif > 31 ) {
        Log()<<QString("BIGDIF: ip %1 dif %2 stamp %3")
        .arg( ip ).arg( dif ).arg( H[it].Timestamp );
    }
#endif

    if( dif != -999999 ) {

        if( dif < 0 )
            ++tStampBins[32];
        else if( dif > 31 )
            ++tStampBins[33];
        else
            ++tStampBins[dif];

        if( dif < 3 || dif > 4 )
            ++tStampEvtByPrb[ip];
    }
#else
    Q_UNUSED( H )
    Q_UNUSED( ip )
    Q_UNUSED( it )
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqQFlt ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ImAcqQFlt::ImAcqQFlt( const DAQ::Params &p, AIQ *Qf, int ip )
    :   tTrip(0), Qf(Qf)
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    hipass = 0;
    lopass = 0;

    if( p.im.prbAll.qf_loCutStr != "0" ) {
        hipass = new Biquad( bq_type_highpass,
                        p.im.prbAll.qf_loCutStr.toDouble() / E.srate );
    }

    if( p.im.prbAll.qf_hiCutStr != "INF" ) {
        lopass = new Biquad( bq_type_lowpass,
                        p.im.prbAll.qf_hiCutStr.toDouble() / E.srate );
    }

    maxInt  = E.roTbl->maxInt();
    nC      = p.stream_nChans( jsIM, ip );
    nAP     = E.imCumTypCnt[CimCfg::imSumAP];

    car.setAuto( E.roTbl );
    car.setChans( nC, nAP, 1 );
    car.setSU( &E.sns.shankMap );
}


ImAcqQFlt::~ImAcqQFlt()
{
    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }
}


void ImAcqQFlt::mapChanged( const DAQ::Params &p, int ip )
{
    carMtx.lock();
        car.setSU( &p.im.prbj[ip].sns.shankMap );
    carMtx.unlock();
}


void ImAcqQFlt::trip()
{
    tTrip = getTime();
}


bool ImAcqQFlt::testTrip()
{
    if( tTrip && (getTime() - tTrip) < 4.0 )
        return true;

    tTrip = 0;
    return false;
}


void ImAcqQFlt::enqueue( qint16 *D, int ntpts )
{
    if( testTrip() || !Qf->qf_isClient() )
        enqueueZero( ntpts );
    else {
        if( hipass )
            hipass->applyBlockwiseMem( D, maxInt, ntpts, nC, 0, nAP );

        if( lopass )
            lopass->applyBlockwiseMem( D, maxInt, ntpts, nC, 0, nAP );

        carMtx.lock();
            car.gbl_dmx_tbl_auto( D, ntpts );
        carMtx.unlock();

        Qf->enqueue( D, ntpts );
    }
}


void ImAcqQFlt::enqueueZero( int ntpts ) const
{
    Qf->enqueueZeroIM( ntpts, 0, 0 );
}

/* ---------------------------------------------------------------- */
/* ImAcqStream ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqStream::ImAcqStream(
    const CimCfg::ImProbeTable  &T,
    const DAQ::Params           &p,
    AIQ                         *Q,
    int                         js,
    int                         ip )
    :   tLastErrFlagsReport(0), tLastFifoReport(0), peakDT(0),
        sumTot(0), totPts(0ULL), Q(Q), QFlt(0),
        errCOUNT{0,0,0,0}, errSERDES{0,0,0,0}, errLOCK{0,0,0,0},
        errPOP{0,0,0,0}, errSYNC{0,0,0,0}, errMISS{0,0,0,0},
        tStampLastFetch(0), fifoAve(0), fifoN(0),
        sumN(0), js(js), ip(ip), statusLastFetch(0), simType(false)
#ifdef PAUSEWHOLESLOT
        , zeroFill(false)
#endif
{
// @@@ FIX Experiment to report large fetch cycle times.
    tLastFetch      = 0;

    vtStampMiss.resize( MAXE * TPNTPERFETCH );
    vstatusMiss.resize( MAXE * TPNTPERFETCH );

#ifdef PROFILE
    sumLag  = 0;
    sumGet  = 0;
    sumScl  = 0;
    sumEnq  = 0;
#endif

// fetchType controls:
// - Reading electrodePackets vs pseudo-packets.
// - LF-band data to process?
// - Value shifting.
// - Fifo read method.
// - Yield pace in units of packets.

    if( js == jsIM ) {

        const CimCfg::PrbEach   &E      = p.im.prbj[ip];
        const int               *cum    = E.imCumTypCnt;

        nAP = cum[CimCfg::imTypeAP];
        nLF = cum[CimCfg::imTypeLF] - nAP;
        nCH = cum[CimCfg::imSumAll];

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
        slot = P.slot;
        port = P.port;
        dock = P.dock;

        fetchType   = E.roTbl->apiFetchType();
        simType     = T.simprb.isSimProbe( slot, port, dock );
    }
    else {

        const CimCfg::ObxEach   &E      = p.im.get_iStrOneBox( ip );
        const int               *cum    = E.obCumTypCnt;

        Subset::rngStr2Vec( vXA, E.uiXAStr );

        nXA = cum[CimCfg::obTypeXA];
        nXD = cum[CimCfg::obTypeXD] - nXA;
        nCH = cum[CimCfg::obSumAll];

        const CimCfg::ImProbeDat    &P = T.get_iOneBox( p.im.obx_istr2isel( ip ) );
        slot = P.slot;
        port = P.port;
        dock = P.dock;

        fetchType = 9;
    }
}


ImAcqStream::~ImAcqStream()
{
    sendErrMetrics();

    if( QFlt ) {
        delete QFlt;
        QFlt = 0;
    }
}


QString ImAcqStream::metricsName( int shank ) const
{
    QString stream;

    if( shank < 0 || fetchType != 4 ) {
        switch( js ) {
            case jsNI: stream = "  nidq"; break;
            case jsOB: stream = QString(" obx%1").arg( ip, 2, 10, QChar('0') ); break;
            case jsIM: stream = QString("imec%1").arg( ip, 2, 10, QChar('0') ); break;
        }
    }
    else
        stream = QString("quad%1-%2").arg( ip, 2, 10, QChar('0') ).arg( shank );

    return stream;
}


// Policy: Send only if nonzero.
//
void ImAcqStream::sendErrMetrics() const
{
    int sMax = (fetchType == 4 ? 4 : 1);

    for( int is = 0; is < sMax; ++is ) {

        if( errCOUNT[is] + errSERDES[is] + errLOCK[is] +
            errPOP[is] + errSYNC[is] + errMISS[is] ) {

            QMetaObject::invokeMethod(
                mainApp()->metrics(),
                "errUpdateFlags",
                Qt::QueuedConnection,
                Q_ARG(QString, metricsName( fetchType == 4 ? is : -1 )),
                Q_ARG(quint32, errCOUNT[is]),
                Q_ARG(quint32, errSERDES[is]),
                Q_ARG(quint32, errLOCK[is]),
                Q_ARG(quint32, errPOP[is]),
                Q_ARG(quint32, errSYNC[is]),
                Q_ARG(quint32, errMISS[is]) );

            if( uniformDev() < 0.10 ) {
                QMetaObject::invokeMethod(
                    mainApp(),
                    "window_RunMetrics",
                    Qt::QueuedConnection );
            }
        }
    }
}


void ImAcqStream::fileMissReport() const
{
    if( !mtStampMiss.size() )
        return;

    MainApp     *app    = mainApp();
    QString     fname   =
        QString("%1/%2.missed_samples.%3%4.txt")
        .arg( app->dataDir( 0 ) )
        .arg( app->cfgCtl()->acceptedParams.sns.runName )
        .arg( js == jsOB ? "obx" : "imec" )
        .arg( ip );
    QFile       f( fname );
    QTextStream ts( &f );
    f.open( QIODevice::WriteOnly | QIODevice::Text );

    QMap<quint64,int>::const_iterator
        it  = mtStampMiss.begin(),
        end = mtStampMiss.end();

    for( ; it != end; ++it )
        ts << QString("%1,%2\n").arg( it.key() ).arg( it.value() );
}


#define STATBRIDGE( A, B )  \
    ((1<<11) | ((A|B) & ~(1<<6)) | (A&B) & (1<<6))

void ImAcqStream::checkErrs_EPack( electrodePacket* E, int nE )
{
// -----------------------
// Assess timestamp deltas
// -----------------------

// Cross-fetch

    quint32 tsfirst = E[0].timestamp[0];
    int     nmiss   = 0;
    quint16 stfirst = E[0].Status[0];

    vtStampMiss[0] = 0;

    if( !tStampLastFetch || tStampLastFetch >= tsfirst )
        ;   // init or rollover
    else if( tsfirst - tStampLastFetch > 4 ) {
        vtStampMiss[0] = 3 * (tsfirst - tStampLastFetch) / 10;
        vstatusMiss[0] = STATBRIDGE( statusLastFetch, stfirst );
        nmiss         += vtStampMiss[0];
#ifdef TSTAMPCHECKS
        Log() <<
        QString("~~ CROSS-FETCH GAP IM %1  val %2")
        .arg( ip ).arg( tsfirst - tStampLastFetch );
#endif
    }

    tStampLastFetch = E[nE-1].timestamp[TPNTPERFETCH-1];
    statusLastFetch = E[nE-1].Status[TPNTPERFETCH-1];

// Intra-fetch

    for( int it = 1, nT = nE * TPNTPERFETCH; it < nT; ++it ) {

        int     ie = it / TPNTPERFETCH,
                is = it - ie * TPNTPERFETCH;
        quint32 tsnext = E[ie].timestamp[is];
        quint16 stnext = E[ie].Status[is];

        vtStampMiss[it] = 0;

        if( tsfirst >= tsnext )
            ;   // init or rollover
        else if( tsnext - tsfirst > 4 ) {
            vtStampMiss[it] = 3 * (tsnext - tsfirst) / 10;
            vstatusMiss[it] = STATBRIDGE( stfirst, stnext );
            nmiss          += vtStampMiss[it];
        }

        tsfirst = tsnext;
        stfirst = stnext;
    }

    if( nmiss )
        errMISS[0] += nmiss;

// ------------------
// Assess error flags
// ------------------

    for( int ie = 0; ie < nE; ++ie ) {

        const quint16   *errs = E[ie].Status;

        for( int i = 0; i < TPNTPERFETCH; ++i ) {

            int err = errs[i];

            if( err & ELECTRODEPACKET_STATUS_ERR_COUNT )    ++errCOUNT[0];
            if( err & ELECTRODEPACKET_STATUS_ERR_SERDES )   ++errSERDES[0];
            if( err & ELECTRODEPACKET_STATUS_ERR_LOCK )     ++errLOCK[0];
            if( err & ELECTRODEPACKET_STATUS_ERR_POP )      ++errPOP[0];
            if( err & ELECTRODEPACKET_STATUS_ERR_SYNC )     ++errSYNC[0];
        }
    }

    double  tFlags = getTime();

    if( tFlags - tLastErrFlagsReport >= 2.0 ) {

        sendErrMetrics();
        tLastErrFlagsReport = tFlags;
    }
}


void ImAcqStream::checkErrs_PInfo( PacketInfo* H, int nT, int shank )
{
// -----------------------
// Assess timestamp deltas
// -----------------------

    if( !shank ) {

        // Cross-fetch

        quint32 tsfirst = H[0].Timestamp;
        int     nmiss   = 0;

        vtStampMiss[0] = 0;

        if( !tStampLastFetch || tStampLastFetch >= tsfirst )
            ;   // init or rollover
        else if( tsfirst - tStampLastFetch > 4 ) {
            vtStampMiss[0] = 3 * (tsfirst - tStampLastFetch) / 10;
            vstatusMiss[0] = STATBRIDGE( statusLastFetch, H[0].Status );
            nmiss         += vtStampMiss[0];
#ifdef TSTAMPCHECKS
            Log() <<
            QString("~~ CROSS-FETCH GAP %1 %2  val %3")
            .arg(js==jsIM?"IM":"OB")
            .arg( ip ).arg( tsfirst - tStampLastFetch );
#endif
        }

        tStampLastFetch = H[nT-1].Timestamp;
        statusLastFetch = H[nT-1].Status;

        // Intra-fetch

        for( int it = 1; it < nT; ++it ) {

            quint32 tsnext = H[it].Timestamp;

            vtStampMiss[it] = 0;

            if( tsfirst >= tsnext )
                ;   // init or rollover
            else if( tsnext - tsfirst > 4 ) {
                vtStampMiss[it] = 3 * (tsnext - tsfirst) / 10;
                vstatusMiss[it] = STATBRIDGE( H[it-1].Status, H[it].Status );
                nmiss          += vtStampMiss[it];
            }

            tsfirst = tsnext;
        }

        if( nmiss )
            errMISS[0] += nmiss;
    }
    else {
        for( int it = 0; it < nT; ++it ) {
            if( vtStampMiss[it] )
                errMISS[shank] = errMISS[0];
        }
    }

// ------------------
// Assess error flags
// ------------------

    for( int it = 0; it < nT; ++it ) {

        const quint16   err = H[it].Status;

        if( err & ELECTRODEPACKET_STATUS_ERR_COUNT )    ++errCOUNT[shank];
        if( err & ELECTRODEPACKET_STATUS_ERR_SERDES )   ++errSERDES[shank];
        if( err & ELECTRODEPACKET_STATUS_ERR_LOCK )     ++errLOCK[shank];
        if( err & ELECTRODEPACKET_STATUS_ERR_POP )      ++errPOP[shank];
        if( err & ELECTRODEPACKET_STATUS_ERR_SYNC )     ++errSYNC[shank];
    }

    if( fetchType != 4 || shank == 3 ) {

        double  tFlags = getTime();

        if( tFlags - tLastErrFlagsReport >= 2.0 ) {

            sendErrMetrics();
            tLastErrFlagsReport = tFlags;
        }
    }
}


bool ImAcqStream::checkFifo( int *packets, CimAcqImec *acq )
{
    double  tFifo = getTime();

    fifoAve += acq->fifoPct( packets, *this );
    ++fifoN;

    if( tFifo - tLastFifoReport >= 5.0 ) {

        if( fifoN > 0 ) {
            fifoAve /= fifoN;
            sqb.ave();
        }

        if( QFlt && (fifoAve > 1 || sqb.is_increased()) )
            QFlt->trip();

        QString stream = metricsName();

        QMetaObject::invokeMethod(
            mainApp()->metrics(),
            "prfUpdateFifo",
            Qt::QueuedConnection,
            Q_ARG(QString, stream),
            Q_ARG(int, fifoAve) );

        if( fifoAve >= 5 ) {    // 5% standard

            Warning() <<
                QString("IMEC FIFO queue %1 fill% %2")
                .arg( stream )
                .arg( fifoAve, 2, 10, QChar('0') );

            if( fifoAve >= 95 ) {
                acq->runError(
                    QString("IMEC FIFO queue %1 overflow; stopping run.")
                    .arg( stream ) );
                return false;
            }
        }

        if( sqb.is_changed() ) {

            Warning() <<
            QString("IMEC Quad-base %1 shank disparity %2 samples {%3 %4 %5 %6}.")
            .arg( stream )
            .arg( sqb.delta_is )
            .arg( sqb.each[0] )
            .arg( sqb.each[1] )
            .arg( sqb.each[2] )
            .arg( sqb.each[3] );

            if( sqb.delta_is >= 10 ) {
                acq->runError(
                QString(
                "IMEC Quad-base %1 shanks desynchronized; stopping run.\n"
                "Try disabling Filtered IM Streams (IM Setup Tab).")
                .arg( stream ) );
                return false;
            }
        }

        fifoAve         = 0;
        fifoN           = 0;
        tLastFifoReport = tFifo;
        sqb.reset();
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* ImAcqWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqWorker::ImAcqWorker(
    CimAcqImec                  *acq,
    ImAcqShared                 &shr,
    std::vector<ImAcqStream>    &streams )
    :   QObject(0), tLastYieldReport(getTime()), yieldSum(0),
        acq(acq), shr(shr), streams(streams)
{
}


void ImAcqWorker::run()
{
// Size buffers
// ------------
// - lfLast[][]: each probe must retain the prev LF for all channels.
// - i16Buf[]:   max sized over stream nCH; reused each iID.
// - D[]:        max sized over {fetchType, MAXE}; reused each iID.
//
    std::vector<std::vector<qint16> >   lfLast;
    vec_i16                             i16Buf;

    const int   nID     = streams.size();
    int         nCHMax  = 0,
                T2Chans = OBX_N_ACQ,
                nT0     = 0,
                nT2     = 0,    // NP 2.0 or OneBox
                nSY     = 1;    // for NP2020

    lfLast.resize( nID );

    for( int iID = 0; iID < nID; ++iID ) {

        ImAcqStream &S = streams[iID];

        // init stream QFlt

        if( S.js == jsIM && acq->owner->imQf.size() )
            S.QFlt = new ImAcqQFlt( acq->p, acq->owner->imQf[S.ip], S.ip );

        // stream chans (i16Buf)

        if( S.nCH > nCHMax )
            nCHMax = S.nCH;

        // acq voltage chans (D)

        switch( S.fetchType ) {
            case 0: ++nT0; lfLast[iID].assign( S.nLF, 0 ); break;
            case 4: nSY = 4;
            case 2: ++nT2; T2Chans = qMax( T2Chans, S.nAP ); break;
            case 9: ++nT2; T2Chans = qMax( T2Chans, OBX_N_ACQ ); break;
        }
    }

    i16Buf.resize( MAXE * TPNTPERFETCH * nCHMax );

    if( nT0 )
        D.resize( MAXE * sizeof(electrodePacket) / sizeof(qint32) );
    else
        D.resize( MAXE * TPNTPERFETCH * T2Chans * sizeof(qint16) / sizeof(qint32) );

    if( nT2 )
        H.resize( MAXE * TPNTPERFETCH * nSY );

    if( !shr.wait() )
        goto exit;

// -----------------------
// Fetch : Scale : Enqueue
// -----------------------

    lastCheckT = shr.startT;

    while( !acq->isStopped() && !shr.stopping() ) {

        loopT = getTime();

        // -------------
        // Do my streams
        // -------------

        for( int iID = 0; iID < nID; ++iID ) {

            ImAcqStream &S = streams[iID];

            if( !S.totPts ) {
                S.Q->setTZero( loopT );
                if( S.QFlt )
                    S.QFlt->Qf->setTZero( loopT );
            }

            if( S.QFlt && S.ip == shr.checkCAR() ) {
                shr.updateCAR( -1 );
                S.QFlt->mapChanged( acq->p, S.ip );
            }

            double  dtTot = getTime();
            bool    ok;

            switch( S.fetchType ) {
                case 0: ok = doProbe_T0( &lfLast[iID][0], i16Buf, S ); break;
                case 2:
                case 4: ok = doProbe_T2( i16Buf, S ); break;
                case 9: ok = do_obx( i16Buf, S ); break;
            }

            if( !ok )
                goto exit;

            dtTot = getTime() - dtTot;

            if( dtTot > S.peakDT )
                S.peakDT = dtTot;

            S.sumTot += dtTot;
            ++S.sumN;
        }

        // -----
        // Yield
        // -----

        if( !workerYield() )
            goto exit;

        // ---------------
        // Rate statistics
        // ---------------

        if( loopT - lastCheckT >= 5.0 ) {

            for( int iID = 0; iID < nID; ++iID ) {

                ImAcqStream &S = streams[iID];

#ifdef PROFILE
                profile( S );
#endif
                S.peakDT    = 0;
                S.sumTot    = 0;
                S.sumN      = 0;
            }

            lastCheckT  = getTime();
        }
    }

exit:
    for( int iID = 0; iID < nID; ++iID )
        streams[iID].fileMissReport();

    emit finished();
}


bool ImAcqWorker::doProbe_T0( qint16 *lfLast, vec_i16 &dst1D, ImAcqStream &S )
{
#ifdef PROFILE
    double  prbT0 = getTime();
#endif

    electrodePacket*    E   = reinterpret_cast<electrodePacket*>(&D[0]);
    qint16*             dst = &dst1D[0];
    int                 nQ  = 0,
                        nE;

// -----
// Fetch
// -----

    if( !acq->fetchE_T0( nE, E, S ) )
        return false;

    if( !nE ) {

// @@@ FIX Adjust sample waiting for trigger type

        // BK: Allow up to 5 seconds for (external) trigger.
        // BK: Tune with experience.

        if( !S.totPts && loopT - shr.startT >= 5.0 ) {
            acq->runError(
                QString("IMEC probe %1 getting no samples.").arg( S.ip ) );
            return false;
        }

        return true;
    }

#ifdef PROFILE
    S.sumGet += getTime() - prbT0;
#endif

// -----
// Scale
// -----

#ifdef PROFILE
    double  dtScl = getTime();
#endif

    for( int ie = 0; ie < nE; ++ie ) {

        const qint16    *srcLF  = 0;
        const quint16   *srcSY  = 0;
        electrodePacket *pE     = &E[ie];

        srcLF = pE->lfpData;
        srcSY = pE->Status;

        for( int it = 0; it < TPNTPERFETCH; ++it ) {

            // ------
            // Misses
            // ------

            int z = S.vtStampMiss[ie * TPNTPERFETCH + it];

            if( z ) {

                if( nQ ) {
                    S.Q->enqueue( &dst1D[0], nQ );
                    if( S.QFlt )
                        S.QFlt->enqueue( &dst1D[0], nQ );
                    S.totPts += nQ;
                    dst = &dst1D[0];
                    nQ  = 0;
                }

                if( S.mtStampMiss.size() < 30000 )
                    S.mtStampMiss[S.totPts] = z;

                S.Q->enqueueZeroIM( z, 1,
                    S.vstatusMiss[ie * TPNTPERFETCH + it] );
                if( S.QFlt )
                    S.QFlt->enqueueZero( z );
                S.totPts += z;
            }

            // ----------
            // ap - as is
            // ----------

            memcpy( dst, E[ie].apData[it], S.nAP * sizeof(qint16) );
            dst += S.nAP;

            // -----------------
            // lf - interpolated
            // -----------------

            float slope = float(it)/TPNTPERFETCH;

            for( int ic = 0, nc = S.nLF; ic < nc; ++ic )
                *dst++ = lfLast[ic] + slope*(srcLF[ic]-lfLast[ic]);

            // ----
            // sync
            // ----

            *dst++ = srcSY[it];
            ++nQ;

            // ------------
            // tStamp check
            // ------------

            shr.tStampHist_EPack( E, S.ip, ie, it );

        }   // it

        // ---------------
        // update saved lf
        // ---------------

        memcpy( &lfLast[0], &srcLF[0], S.nLF*sizeof(qint16) );
    }   // ie

#ifdef PROFILE
    S.sumScl += getTime() - dtScl;
#endif

// -------
// Enqueue
// -------

    S.tPreEnq = getTime();

#ifdef PAUSEWHOLESLOT
    if( S.zeroFill ) {
        S.Q->enqueueZero( S.tPostEnq, S.tPreEnq );
        S.zeroFill = false;
    }
#endif

    S.Q->enqueue( &dst1D[0], nQ );
    if( S.QFlt )
        S.QFlt->enqueue( &dst1D[0], nQ );
    S.totPts += nQ;

    S.tPostEnq = getTime();

#ifdef PROFILE
    S.sumLag += mainApp()->getRun()->getStreamTime() -
                (S.Q->tZero() + S.totPts / S.Q->sRate());
    S.sumEnq += S.tPostEnq - S.tPreEnq;
#endif

    return true;
}


bool ImAcqWorker::doProbe_T2( vec_i16 &dst1D, ImAcqStream &S )
{
#ifdef PROFILE
    double  prbT0 = getTime();
#endif

    qint16  *src = reinterpret_cast<qint16*>(&D[0]),
            *dst = &dst1D[0];
    int     vNT[4], // each sub fetch
            nQ   = 0,
            nT;     // total timepoints

// -----
// Fetch
// -----

    if( S.fetchType == 2 ) {
        if( !acq->fetchD_T2( nT, &H[0], src, S, S.nAP, MAXE * TPNTPERFETCH ) )
            return false;
    }
    else {
        nT = 0;

        // lowest pending
        int fmin = MAXE * TPNTPERFETCH, fifo, empty;
        if( !S.simType ) {
            for( int is = 0; is < 4; ++is ) {
                np_getPacketFifoStatus( S.slot, S.port, S.dock,
                    streamsource_t(is), &fifo, &empty );
                fmin = qMin( fmin, fifo );
            }
        }
        else {
            acq->simDat[acq->ip2simdat[S.ip]].fifo( &fifo, &empty );
            fmin = qMin( fmin, fifo );
        }

        // fetch each NP2020 shank
        if( fmin ) {
            for( int is = 0; is < 4; ++is ) {
                if( !acq->fetchD_T2( vNT[is], &H[nT], src + nT*384,
                            S, 384, fmin, streamsource_t(is) ) ) {

                    return false;
                }
                nT += vNT[is];
            }
            nT = vNT[0];
        }
    }

    if( !nT ) {

// @@@ FIX Adjust sample waiting for trigger type

        // BK: Allow up to 5 seconds for (external) trigger.
        // BK: Tune with experience.

        if( !S.totPts && loopT - shr.startT >= 5.0 ) {
            acq->runError(
                QString("IMEC probe %1 getting no samples.").arg( S.ip ) );
            return false;
        }

        return true;
    }

#ifdef PROFILE
    S.sumGet += getTime() - prbT0;
#endif

//------------------------------------------------------------------
// Experiment to check duplicate data values returned in fetch.
// Report which indices match every ~1 sec.
#ifdef TESTDUPSAMPS
{
    if( S.ip == 1 && nT > 1 ) {
        std::vector<int> v;
        int ndup = 0;
        for( int i = 0; i < nT - 1; ++i ) {
            for( int j = i+1; j < nT; ++j ) {
                for( int ch = 0; ch < 384; ++ch ) {
                    if( src[384*j + ch] != src[384*i + ch] )
                       goto next_j;
                }
                ++ndup;
                v.push_back( i );
                v.push_back( j );
//                break;
next_j:;
            }
        }
        if( ndup > 0 ) {
            static double duplast = 0;
            double t = getTime();
            if( t - duplast >= 1 ) {
                Log() << "PRB Dups " << ndup << " nT " << nT;
                QString s;
                for( int i = 0; i < ndup; ++i )
                    s += QString(" %1/%2").arg( v[2*i] ).arg( v[2*i+1] );
                Log() << "I/J:" << s;
                duplast = t;
            }
        }
    }
}
#endif
//------------------------------------------------------------------

// -----
// Scale
// -----

#ifdef PROFILE
    double  dtScl = getTime();
#endif

    for( int it = 0; it < nT; ++it ) {

        // ------
        // Misses
        // ------

        int z = S.vtStampMiss[it];

        if( z ) {

            if( nQ ) {
                S.Q->enqueue( &dst1D[0], nQ );
                if( S.QFlt )
                    S.QFlt->enqueue( &dst1D[0], nQ );
                S.totPts += nQ;
                dst = &dst1D[0];
                nQ  = 0;
            }

            if( S.mtStampMiss.size() < 30000 )
                S.mtStampMiss[S.totPts] = z;

            S.Q->enqueueZeroIM( z, (S.fetchType == 2 ? 1 : 4),
                S.vstatusMiss[it] );
            if( S.QFlt )
                S.QFlt->enqueueZero( z );
            S.totPts += z;
        }

        // ----------
        // ap - as is
        // ----------

// Option to invert sign; not needed in 3.0
#if 1
        if( S.fetchType == 2 ) {
            memcpy( dst, src, S.nAP * sizeof(qint16) );
            src += S.nAP;
        }
        else {
            int toff = 0;
            for( int is = 0; is < 4; ++is ) {
                memcpy( dst + is * 384, src + (toff + it) * 384,
                    384 * sizeof(qint16) );
                toff += vNT[is];
            }
        }
#else
        for( int k = 0; k < S.nAP; ++k )
            dst[k] = -src[k];
#endif

        dst += S.nAP;

        // ----
        // sync
        // ----

        if( S.fetchType == 2 )
            *dst++ = H[it].Status;
        else {
            int toff = 0;
            for( int is = 0; is < 4; ++is ) {
                *dst++ = H[toff + it].Status;
                toff  += vNT[is];
            }
        }

        ++nQ;

        // ------------
        // tStamp check
        // ------------

        shr.tStampHist_PInfo( &H[0], S.ip, it );

    }   // it

#ifdef PROFILE
    S.sumScl += getTime() - dtScl;
#endif

// -------
// Enqueue
// -------

    S.tPreEnq = getTime();

#ifdef PAUSEWHOLESLOT
    if( S.zeroFill ) {
        S.Q->enqueueZero( S.tPostEnq, S.tPreEnq );
        S.zeroFill = false;
    }
#endif

    S.Q->enqueue( &dst1D[0], nQ );
    if( S.QFlt )
        S.QFlt->enqueue( &dst1D[0], nQ );
    S.totPts += nQ;

    S.tPostEnq = getTime();

#ifdef PROFILE
    S.sumLag += mainApp()->getRun()->getStreamTime() -
                (S.Q->tZero() + S.totPts / S.Q->sRate());
    S.sumEnq += S.tPostEnq - S.tPreEnq;
#endif

    return true;
}


bool ImAcqWorker::do_obx( vec_i16 &dst1D, ImAcqStream &S )
{
#ifdef PROFILE
    double  prbT0 = getTime();
#endif

    qint16  *src = reinterpret_cast<qint16*>(&D[0]),
            *dst = &dst1D[0];
    int     nQ   = 0,
            nT;

// -----
// Fetch
// -----

    if( !acq->fetch_obx( nT, &H[0], src, S ) )
        return false;

    if( !nT ) {

// @@@ FIX Adjust sample waiting for trigger type

        // BK: Allow up to 5 seconds for (external) trigger.
        // BK: Tune with experience.

        if( !S.totPts && loopT - shr.startT >= 5.0 ) {
            acq->runError(
                QString("IMEC OneBox %1 getting no samples.").arg( S.ip ) );
            return false;
        }

        return true;
    }

#ifdef PROFILE
    S.sumGet += getTime() - prbT0;
#endif

//------------------------------------------------------------------
// Experiment to check duplicate data values returned in fetch.
// Report which indices match every ~1 sec.
#ifdef TESTDUPSAMPS
{
    if( S.ip == 0 && nT > 1 ) {
        std::vector<int> v;
        int ndup = 0;
        for( int i = 0; i < nT - 1; ++i ) {
            for( int j = i+1; j < nT; ++j ) {
                for( int ch = 0; ch < OBX_N_ACQ; ++ch ) {
                    if( src[OBX_N_ACQ*j + ch] != src[OBX_N_ACQ*i + ch] )
                       goto next_j;
                }
                ++ndup;
                v.push_back( i );
                v.push_back( j );
//                break;
next_j:;
            }
        }
        if( ndup > 0 ) {
            static double duplast = 0;
            double t = getTime();
            if( t - duplast >= 1 ) {
                Log() << "OBX Dups " << ndup << " nT " << nT;
                QString s;
                for( int i = 0; i < ndup; ++i )
                    s += QString(" %1/%2").arg( v[2*i] ).arg( v[2*i+1] );
                Log() << "I/J:" << s;
                duplast = t;
            }
        }
    }
}
#endif
//------------------------------------------------------------------

// -----
// Scale
// -----

#ifdef PROFILE
    double  dtScl = getTime();
#endif

    for( int it = 0; it < nT; ++it ) {

        // ------
        // Misses
        // ------

        int z = S.vtStampMiss[it];

        if( z ) {

            if( nQ ) {
                S.Q->enqueue( &dst1D[0], nQ );
                S.totPts += nQ;
                dst = &dst1D[0];
                nQ  = 0;
            }

            if( S.mtStampMiss.size() < 30000 )
                S.mtStampMiss[S.totPts] = z;

            S.Q->enqueueZeroIM( z, 1, S.vstatusMiss[it] );
            S.totPts += z;
        }

        // ---------
        // XA subset
        // ---------

        for( int ic = 0; ic < S.nXA; ++ic )
            *dst++ = src[S.vXA[ic]];

        // ---------
        // XD packed
        // ---------

        if( S.nXD ) {

            int XD = 0;

            for( int iline = 0; iline < imOBX_NCHN; ++iline )
                XD += src[imOBX_NCHN + iline] << iline;

            *dst++ = XD;
        }

        src += OBX_N_ACQ;

        // ----
        // sync
        // ----

        *dst++ = H[it].Status;
        ++nQ;

        // ------------
        // tStamp check
        // ------------

        shr.tStampHist_PInfo( &H[0], S.ip, it );

    }   // it

#ifdef PROFILE
    S.sumScl += getTime() - dtScl;
#endif

// -------
// Enqueue
// -------

    S.tPreEnq = getTime();

#ifdef PAUSEWHOLESLOT
    if( S.zeroFill ) {
        S.Q->enqueueZero( S.tPostEnq, S.tPreEnq );
        S.zeroFill = false;
    }
#endif

    S.Q->enqueue( &dst1D[0], nQ );
    S.totPts += nQ;

    S.tPostEnq = getTime();

#ifdef PROFILE
    S.sumLag += mainApp()->getRun()->getStreamTime() -
                (S.Q->tZero() + S.totPts / S.Q->sRate());
    S.sumEnq += S.tPostEnq - S.tPreEnq;
#endif

    return true;
}


bool ImAcqWorker::workerYield()
{
// Get maximum outstanding packets for this worker thread

    int     maxQPkts    = 0,
            nID         = streams.size();
    bool    hichn       = false;

    for( int iID = 0; iID < nID; ++iID ) {

        ImAcqStream &S = streams[iID];
        int         packets;

        if( !S.checkFifo( &packets, acq ) )
            return false;

        if( S.fetchType != 0 ) {
            // Round to TPNTPERFETCH
            int pkt = packets;
            packets /= TPNTPERFETCH;
            if( pkt - packets * TPNTPERFETCH >= TPNTPERFETCH/2 )
                ++packets;

            if( S.nAP > 384 ) {
                packets *= S.nAP / 384;
                hichn    = true;
            }
        }

        if( packets > maxQPkts )
            maxQPkts = packets;
    }

// Yield time if fewer than the average fetched packet count.
// For high channel count probes, sleep less often.

    double  t = getTime();

    if( !acq->p.im.prbAll.lowLatency && maxQPkts < AVEE ) {

        double  udev    = uniformDev();
        bool    sleep   = false;

        if( acq->p.im.prbAll.qf_on ) {
            if( hichn )
                sleep = udev <= 0.05;
            else
                sleep = udev <= 0.40;
        }
        else if( hichn )
            sleep = udev <= 0.40;
        else
            sleep = true;

        if( sleep ) {
            QThread::usleep( 250 );
            yieldSum += getTime() - t;
        }
    }

    if( t - tLastYieldReport >= 5.0 ) {

        int awakePct = qMax( 0, int(100.0*(1.0 - yieldSum/5.0)) );

        for( int iID = 0; iID < nID; ++iID ) {

            QMetaObject::invokeMethod(
                mainApp()->metrics(),
                "prfUpdateAwake",
                Qt::QueuedConnection,
                Q_ARG(QString, streams[iID].metricsName()),
                Q_ARG(int, awakePct) );
        }

        yieldSum            = 0;
        tLastYieldReport    = t;
    }

    return true;
}


// sumN is the number of loop executions in the 5 sec check
// interval. The minimum value is 5*srate/(MAXE*TPNTPERFETCH).
//
// sumTot/sumN is the average loop time to process the samples.
// The maximum value is MAXE*TPNTPERFETCH/srate.
//
// Get measures the time spent fetching the data.
// Scl measures the time spent scaling the data.
// Enq measures the time spent enquing data to the stream.
//
// Required values header is written at run start.
//
void ImAcqWorker::profile( ImAcqStream &S )
{
#ifndef PROFILE
    Q_UNUSED( S )
#else
    Log() <<
        QString(
        "%1 %2 loop ms <%3> lag<%4> get<%5> scl<%6> enq<%7> n(%8) %(%9)")
        .arg( S.js == jsIM ? "imec" : " obx" )
        .arg( S.ip, 2, 10, QChar('0') )
        .arg( 1000*S.sumTot/S.sumN, 0, 'f', 3 )
        .arg( 1000*S.sumLag/S.sumN, 0, 'f', 3 )
        .arg( 1000*S.sumGet/S.sumN, 0, 'f', 3 )
        .arg( 1000*S.sumScl/S.sumN, 0, 'f', 3 )
        .arg( 1000*S.sumEnq/S.sumN, 0, 'f', 3 )
        .arg( S.sumN )
        .arg( acq->fifoPct( 0, S ), 2, 10, QChar('0') );

    S.sumLag    = 0;
    S.sumGet    = 0;
    S.sumScl    = 0;
    S.sumEnq    = 0;
#endif
}

/* ---------------------------------------------------------------- */
/* ImAcqThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ImAcqThread::ImAcqThread(
    CimAcqImec                  *acq,
    ImAcqShared                 &shr,
    std::vector<ImAcqStream>    &streams )
{
    thread  = new QThread;
    worker  = new ImAcqWorker( acq, shr, streams );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Boost priority for worker handling single high channel count probe.

//    if( streams.size() == 1 && streams[0].nAP > 384 )
//        thread->setServiceLevel( QThread::QualityOfService::High );

    thread->start();

// Boost priority for worker handling single high channel count probe.

    if( streams.size() == 1 && streams[0].nAP > 384 )
        thread->setPriority( QThread::HighestPriority );
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

CimAcqImec::CimAcqImec( IMReaderWorker *owner, const DAQ::Params &p )
    :   CimAcq(owner, p), T(mainApp()->cfgCtl()->prbTab), simThd(0)
#ifdef PAUSEWHOLESLOT
        , pausStreamsRequired(0), pausSlot(-1)
#endif
{
}

/* ---------------------------------------------------------------- */
/* ~CimAcqImec ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CimAcqImec::~CimAcqImec()
{
    for( int iThd = 0, nThd = cfgThd.size(); iThd < nThd; ++iThd ) {
        ImCfgThread *T = cfgThd[iThd];
        if( T ) {
            if( T->thread->isRunning() )
                T->thread->wait( 10000/nThd );
            delete T;
        }
    }

    acqShr.kill();

    if( simThd ) {
        delete simThd;
        simThd = 0;
    }

    for( int iThd = 0, nThd = acqThd.size(); iThd < nThd; ++iThd ) {
        ImAcqThread *T = acqThd[iThd];
        if( T ) {
            if( T->thread->isRunning() )
                T->thread->wait( 10000/nThd );
            delete T;
        }
    }

    QThread::msleep( 2000 );

// Disable sync, close hardware

    for( int is = 0, ns = T.nSelSlots(); is < ns; ++is ) {

        int slot = T.getEnumSlot( is );

        if( !T.simprb.isSimSlot( slot ) ) {
            if( T.isSlotPXIType( slot ) ) {
                np_switchmatrix_clear( slot, SM_Output_SMA );
                np_switchmatrix_clear( slot, SM_Output_PXISYNC );
            }
            else
                _aux_doneObxSlot( slot );
            np_closeBS( slot );
        }
    }
}

/* ---------------------------------------------------------------- */
/* CimAcqImec::run ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimAcqImec::run()
{
// ---------
// Configure
// ---------

// Hardware

    if( !configure() )
        return;

// Create worker threads

    createAcqWorkerThreads();

// Wait for threads to reach ready (sleep) state

    acqShr.runMtx.lock();
    {
        int nThd = acqThd.size();

        while( acqShr.asleep < nThd ) {
            acqShr.runMtx.unlock();
                QThread::usleep( 10 );
            acqShr.runMtx.lock();
        }
    }
    acqShr.runMtx.unlock();

// -----
// Start
// -----

    atomicSleepWhenReady();

    if( isStopped() || !startAcq() )
        return;

// ---
// Run
// ---

#ifdef PROFILE
    // Table header, see profile discussion
    {
        double  srate = (p.im.get_nProbes() ?
                            p.im.prbj[0].srate :
                            p.im.get_iSelOneBox( 0 ).srate);

        Log() <<
            QString("Require loop ms < [[ %1 ]] n > [[ %2 ]] MAXE %3")
            .arg( 1000*MAXE*TPNTPERFETCH/srate, 0, 'f', 3 )
            .arg( qRound( 5*srate/(MAXE*TPNTPERFETCH) ) )
            .arg( MAXE );
    }
#endif

    acqShr.startT = getTime();

// Wake all workers

    if( simDat.size() )
        simThd = new ImSimPrbThread( simDat );

    acqShr.condWake.wakeAll();

// Sleep main thread until external stop command

    atomicSleepWhenReady();

// --------
// Clean up
// --------
}

/* ---------------------------------------------------------------- */
/* update --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef PAUSEWHOLESLOT
// Updating settings affects all ports on that slot.
//
void CimAcqImec::update( int ip )
{
    const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
    NP_ErrorCode                err;

    pauseSlot( P.slot );

    while( !pauseAllAck() )
        QThread::usleep( 100 );

// ----------------------
// Stop streams this slot
// ----------------------

    err = np_arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1)%2")
            .arg( P.slot ).arg( makeErrorString( err ) ) );
        return;
    }

// --------------------------
// Update settings this probe
// --------------------------

    if( !_1t_selectElectrodes( P ) )
        return;

    if( !_1t_selectReferences( P ) )
        return;

    if( !_1t_selectGains( P ) )
        return;

    if( !_1t_selectAPFilters( P ) )
        return;

    if( !_1t_setStandby( P ) )
        return;

    if( !_1t_writeProbe( P ) )
        return;

// -------------------------------
// Set slot to software triggering
// -------------------------------

    err = np_switchmatrix_clear( P.slot, SM_Output_AcquisitionTrigger );

    if( err != SUCCESS  ) {
        runError(
            QString("IMEC switchmatrix_clear(slot %1)%2")
            .arg( P.slot ).arg( makeErrorString( err ) ) );
        return;
    }

    err = np_switchmatrix_set( P.slot, SM_Output_AcquisitionTrigger, SM_Input_SWTrigger1, true );

    if( err != SUCCESS  ) {
        runError(
            QString("IMEC switchmatrix_set(slot %1)%2")
            .arg( P.slot ).arg( makeErrorString( err ) ) );
        return;
    }

// ------------
// Arm the slot
// ------------

    err = np_arm( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC arm(slot %1)%2")
            .arg( P.slot ).arg( makeErrorString( err ) ) );
        return;
    }

// ----------------
// Restart the slot
// ----------------

    err = np_setSWTrigger( P.slot );

    if( err != SUCCESS ) {
        runError(
            QString("IMEC setSWTrigger(slot %1)%2")
            .arg( P.slot ).arg( makeErrorString( err ) ) );
        return;
    }

// -----------------
// Reset pause flags
// -----------------

    pauseSlot( -1 );
}
#else
void CimAcqImec::update( int ip )
{
    const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );

// --------------------------
// Update settings this probe
// --------------------------

    if( p.im.prbAll.qf_on )
        acqShr.updateCAR( ip );

    if( T.simprb.isSimProbe( P.slot, P.port, P.dock ) )
        return;

    if( !_1t_selectElectrodes( P ) )
        return;

    if( !_1t_selectReferences( P ) )
        return;

    if( !_1t_selectGains( P ) )
        return;

    if( !_1t_selectAPFilters( P ) )
        return;

    if( !_1t_setStandby( P ) )
        return;

    _1t_writeProbe( P );

}
#endif

/* ---------------------------------------------------------------- */
/* Opto ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return space-sep string with 14 factors, or,
// return error message that starts with "O" or "I".
//
QString CimAcqImec::opto_getAttens( int ip, int color )
{
    double                      atten;
    QString                     msg;
    const CimCfg::ImProbeDat    &P      = T.get_iProbe( ip );
    int                         nSite   = p.im.prbj[ip].roTbl->nOptoSites();
    NP_ErrorCode                err;

    if( !nSite ) {
        msg =
        QString("OPTOGETATTENS: Probe (ip %1) is not an optical probe.").arg( ip );
    }
    else {

        for( int site = 0; site < nSite; ++site ) {

            err = np_getEmissionSiteAttenuation(
                    P.slot, P.port, P.dock, wavelength_t(color), site, &atten );

            if( err != SUCCESS ) {
                msg = QString("IMEC getEmissionSiteAttenuation(slot %1, port %2)%3")
                        .arg( P.slot ).arg( P.port ).arg( makeErrorString( err ) );
                runError( msg );
                break;
            }

            msg += QString("%1 ").arg( atten, 0, 'f', 9 );
        }
    }

    return msg;
}


QString CimAcqImec::opto_emit( int ip, int color, int site )
{
    QString                     msg;
    const CimCfg::ImProbeDat    &P      = T.get_iProbe( ip );
    int                         nSite   = p.im.prbj[ip].roTbl->nOptoSites();
    NP_ErrorCode                err;

    if( !nSite ) {
        msg =
        QString("OPTOEMIT: Probe (ip %1) is not an optical probe.").arg( ip );
    }
    else if( site >= 0 ) {
        err = np_setEmissionSite( P.slot, P.port, P.dock, wavelength_t(color), site );

        if( err != SUCCESS ) {
            msg = QString("IMEC setEmissionSite(slot %1, port %2)%3")
                    .arg( P.slot ).arg( P.port ).arg( makeErrorString( err ) );
            runError( msg );
        }
    }
    else {
        err = np_disableEmissionPath( P.slot, P.port, P.dock, wavelength_t(color) );

        if( err != SUCCESS ) {
            msg = QString("IMEC disableEmissionPath(slot %1, port %2)%3")
                    .arg( P.slot ).arg( P.port ).arg( makeErrorString( err ) );
            runError( msg );
        }
    }

    return msg;
}

/* ---------------------------------------------------------------- */
/* Pause controls ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef PAUSEWHOLESLOT

void CimAcqImec::pauseSlot( int slot )
{
    QMutexLocker    ml( &runMtx );

    pausSlot            = slot;
    pausStreamsRequired = (slot >= 0 ? T.nQualStreamsThisSlot( slot ) : 0);
    pausStreamsReported.clear();
}


bool CimAcqImec::pauseAck( int port, int dock )
{
    QMutexLocker    ml( &runMtx );
    int             key = 10 * port + dock;
    bool            wasAck = pausStreamsReported.contains( key );

    pausStreamsReported.insert( key );
    return wasAck;
}


bool CimAcqImec::pauseAllAck() const
{
    QMutexLocker    ml( &runMtx );

    return pausStreamsReported.count() >= pausStreamsRequired;
}

#endif

/* ---------------------------------------------------------------- */
/* fetchE --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::fetchE_T0( int &nE, electrodePacket* E, ImAcqStream &S )
{
    nE = 0;

#ifdef PAUSEWHOLESLOT
// --------------------------------
// Hardware pause acknowledged here
// --------------------------------

    if( pausedSlot() == S.slot ) {

ackPause:
        if( !pauseAck( S.port, S.dock ) )
            S.zeroFill = true;

        return true;
    }
#endif

// --------------------
// Else fetch real data
// --------------------

// @@@ FIX Experiment to report large fetch cycle times.
#ifdef TSTAMPCHECKS
    double tFetch = getTime();
    if( S.tLastFetch ) {
        if( tFetch - S.tLastFetch > 0.010 ) {
            Log() <<
                QString("       IM %1  dt %2  Q% %3")
                .arg( S.ip ).arg( int(1000*(tFetch - S.tLastFetch)) )
                .arg( fifoPct( 0, S ) );
        }
    }
    S.tLastFetch = tFetch;
#endif

    int             out;
    NP_ErrorCode    err = SUCCESS;

    if( !S.simType )
        err = np_readElectrodeData( S.slot, S.port, S.dock, E, &out, MAXE );
    else
        simDat[ip2simdat[S.ip]].fetchT0( E, &out );

// @@@ FIX Experiment to report fetched packet count vs time.
#ifdef LATENCYFILE
if( S.ip == 0 ) {
    static double q0 = getTime();
    static QFile f;
    static QTextStream ts( &f );
    double qq = getTime() - q0;
    if( qq >= 5.0 && qq < 6.0 ) {
        if( !f.isOpen() ) {
            f.setFileName( "pace_T0.txt" );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
        }
        ts<<QString("%1\t%2\n").arg( qq ).arg( out );
        if( qq >= 6.0 )
            f.close();
    }
}
#endif

    if( err != SUCCESS ) {

#ifdef PAUSEWHOLESLOT
        if( pausedSlot() == S.slot )
            goto ackPause;
#endif

        runError(
            QString(
            "IMEC readElectrodeData(slot %1, port %2, dock %3)%4")
            .arg( S.slot ).arg( S.port ).arg( S.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    nE = out;

// -----
// Shift
// -----

#if 0
#define EP_AP   (sizeof(E[0].apData)/sizeof(qint16))

    for( int ie = 0; ie < nE; ++ie ) {

        qint16  *v;

        v = &E[ie].apData[0][0];
        for( int ic = 0, nc = EP_AP; ic < nc; ++ic, ++v )
            *v >>= 4;

        v = &E[ie].lfpData[0];
        for( int ic = 0, nc = S.nLF; ic < nc; ++ic, ++v )
            *v >>= 4;
    }
#endif

// ----
// Tune
// ----

#ifdef TUNE
    // Tune AVEE and MAXE on designated probe
    if( TUNE == S.ip ) {
        static std::vector<uint> pkthist( 1 + MAXE, 0 ); // 0 + [1..MAXE]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- ave %1  max %2")
                .arg( AVEE ).arg( MAXE );
            for( int i = 0; i <= MAXE; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.assign( 1 + MAXE, 0 );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[out];
    }
#endif

// -----------------
// Check error flags
// -----------------

    if( nE && !S.simType )
        S.checkErrs_EPack( E, nE );

    return true;
}


bool CimAcqImec::fetchD_T2(
    int             &nT,
    PacketInfo*     H,
    qint16*         D,
    ImAcqStream     &S,
    int             nC,
    int             smpMax,
    streamsource_t  shank )
{
    nT = 0;

#ifdef PAUSEWHOLESLOT
// --------------------------------
// Hardware pause acknowledged here
// --------------------------------

    if( pausedSlot() == S.slot ) {

ackPause:
        if( !pauseAck( S.port, S.dock ) )
            S.zeroFill = true;

        return true;
    }
#endif

// --------------------
// Else fetch real data
// --------------------

// @@@ FIX Experiment to report large fetch cycle times.
#ifdef TSTAMPCHECKS
    double tFetch = getTime();
    if( S.tLastFetch ) {
        if( tFetch - S.tLastFetch > 0.010 ) {
            Log() <<
                QString("       IM %1  dt %2  Q% %3")
                .arg( S.ip ).arg( int(1000*(tFetch - S.tLastFetch)) )
                .arg( fifoPct( 0, S ) );
        }
    }
    S.tLastFetch = tFetch;
#endif

    int             out;
    NP_ErrorCode    err = SUCCESS;

    if( !S.simType ) {
        err = np_readPackets(
                S.slot, S.port, S.dock, shank,
                H, D, nC, smpMax, &out );
    }
    else
        simDat[ip2simdat[S.ip]].fetchT2( H, D, shank, nC, smpMax, &out );

// @@@ FIX Experiment to report fetched packet count vs time.
#ifdef LATENCYFILE
if( S.ip == 0 && shank == SourceAP ) {
    static double q0 = getTime();
    static QFile f;
    static QTextStream ts( &f );
    double qq = getTime() - q0;
    if( qq >= 5.0 && qq < 6.0 ) {
        if( !f.isOpen() ) {
            f.setFileName( "pace_T2.txt" );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
        }
        ts<<QString("%1\t%2\n").arg( qq ).arg( out );
        if( qq >= 6.0 )
            f.close();
    }
}
#endif

    if( err != SUCCESS ) {

#ifdef PAUSEWHOLESLOT
        if( pausedSlot() == S.slot )
            goto ackPause;
#endif

        runError(
            QString(
            "IMEC readPackets(slot %1, port %2, dock %3)%4")
            .arg( S.slot ).arg( S.port ).arg( S.dock )
            .arg( makeErrorString( err ) ) );
        return false;
    }

    nT = out;

// ----
// Tune
// ----

#ifdef TUNE
    // Tune AVEE and MAXE on designated probe
    if( TUNE == S.ip && shank == SourceAP ) {
        static std::vector<uint> pkthist( 1 + MAXE*TPNTPERFETCH, 0 ); // 0 + [1..MAX]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- ave %1  max %2")
                .arg( AVEE*TPNTPERFETCH ).arg( MAXE*TPNTPERFETCH );
            for( int i = 0; i <= MAXE*TPNTPERFETCH; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.assign( 1 + MAXE*TPNTPERFETCH, 0 );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[out];
    }
#endif

// -----------------
// Check error flags
// -----------------

    if( nT && !S.simType )
        S.checkErrs_PInfo( H, nT, shank );

    return true;
}


bool CimAcqImec::fetch_obx( int &nT, PacketInfo* H, qint16* D, ImAcqStream &S )
{
    nT = 0;

#ifdef PAUSEWHOLESLOT
// --------------------------------
// Hardware pause acknowledged here
// --------------------------------

    if( pausedSlot() == S.slot ) {

ackPause:
        if( !pauseAck( S.port, S.dock ) )
            S.zeroFill = true;

        return true;
    }
#endif

// --------------------
// Else fetch real data
// --------------------

// @@@ FIX Experiment to report large fetch cycle times.
#ifdef TSTAMPCHECKS
    double tFetch = getTime();
    if( S.tLastFetch ) {
        if( tFetch - S.tLastFetch > 0.010 ) {
            Log() <<
                QString("       OB %1  dt %2  Q% %3")
                .arg( S.ip ).arg( int(1000*(tFetch - S.tLastFetch)) )
                .arg( fifoPct( 0, S ) );
        }
    }
    S.tLastFetch = tFetch;
#endif

    int             out;
    NP_ErrorCode    err = SUCCESS;

    err = np_ADC_readPackets(
            S.slot, H, D, OBX_N_ACQ, MAXE * TPNTPERFETCH, &out );

// @@@ FIX Experiment to report fetched packet count vs time.
#ifdef LATENCYFILE
if( S.ip == 0 ) {
    static double q0 = getTime();
    static QFile f;
    static QTextStream ts( &f );
    double qq = getTime() - q0;
    if( qq >= 5.0 && qq < 6.0 ) {
        if( !f.isOpen() ) {
            f.setFileName( "pace_obx.txt" );
            f.open( QIODevice::WriteOnly | QIODevice::Text );
        }
        ts<<QString("%1\t%2\n").arg( qq ).arg( out );
        if( qq >= 6.0 )
            f.close();
    }
}
#endif

    if( err != SUCCESS ) {

#ifdef PAUSEWHOLESLOT
        if( pausedSlot() == S.slot )
            goto ackPause;
#endif

        runError(
            QString(
            "IMEC ADC_readPackets(slot %1)%2")
            .arg( S.slot ).arg( makeErrorString( err ) ) );
        return false;
    }

    nT = out;

// ----
// Tune
// ----

#ifdef TUNEOBX
    // Tune AVEE and MAXE on designated probe
    if( TUNEOBX == S.ip ) {
        static std::vector<uint> pkthist( 1 + MAXE*TPNTPERFETCH, 0 ); // 0 + [1..MAX]
        static double tlastpkreport = getTime();
        double tpk = getTime() - tlastpkreport;
        if( tpk >= 5.0 ) {
            Log()<<QString("---------------------- ave %1  max %2")
                .arg( AVEE*TPNTPERFETCH ).arg( MAXE*TPNTPERFETCH );
            for( int i = 0; i <= MAXE*TPNTPERFETCH; ++i ) {
                uint x = pkthist[i];
                if( x )
                    Log()<<QString("%1\t%2").arg( i ).arg( x );
            }
            pkthist.assign( 1 + MAXE*TPNTPERFETCH, 0 );
            tlastpkreport = getTime();
        }
        else
            ++pkthist[out];
    }
#endif

// -----------------
// Check error flags
// -----------------

    if( nT )
        S.checkErrs_PInfo( H, nT, 0 );

    return true;
}

/* ---------------------------------------------------------------- */
/* fifoPct -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int CimAcqImec::fifoPct( int *packets, ImAcqStream &S ) const
{
    int pct = 0;

#ifdef PAUSEWHOLESLOT
    if( pausedSlot() != S.slot ) {
#else
    if( 1 ) {
#endif

        int             nused, nempty;
        NP_ErrorCode    err;

        if( !packets )
            packets = &nused;

        if( !S.simType ) {

            switch( S.fetchType ) {
                case 0:
                    err = np_getElectrodeDataFifoState(
                            S.slot, S.port, S.dock, packets, &nempty );
                    if( err != SUCCESS ) {
                        Warning() <<
                            QString("IMEC getElectrodeDataFifoState(slot %1, port %2, dock %3)%4")
                            .arg( S.slot ).arg( S.port ).arg( S.dock )
                            .arg( makeErrorString( err ) );
                    }
                    break;
                case 2:
                    err = np_getPacketFifoStatus(
                            S.slot, S.port, S.dock, SourceAP, packets, &nempty );
                    if( err != SUCCESS ) {
                        Warning() <<
                            QString("IMEC getPacketFifoStatus(slot %1, port %2, dock %3)%4")
                            .arg( S.slot ).arg( S.port ).arg( S.dock )
                            .arg( makeErrorString( err ) );
                    }
                    break;
                case 4:
                    {
                    int fmin = 10000000,
                        fmax = 0;
                    for( int is = 0; is < 4; ++is ) {
                        err = np_getPacketFifoStatus(
                            S.slot, S.port, S.dock,
                            streamsource_t(is), packets, &nempty );
                        if( err != SUCCESS ) {
                            Warning() <<
                                QString("IMEC getPacketFifoStatus(slot %1, port %2, dock %3)%4")
                                .arg( S.slot ).arg( S.port ).arg( S.dock )
                                .arg( makeErrorString( err ) );
                        }
                        S.sqb.each[is] += *packets;
                        fmin = qMin( fmin, *packets );
                        fmax = qMax( fmax, *packets );
                    }
                    *packets        = fmax;
                    S.sqb.delta_is += fmax - fmin;
                    ++S.sqb.nave;
                    }
                    break;
                case 9:
                    err = np_ADC_getPacketFifoStatus( S.slot, packets, &nempty );
                    if( err != SUCCESS ) {
                        Warning() <<
                            QString("IMEC ADC_getPacketFifoStatus(slot %1)%2")
                            .arg( S.slot ).arg( makeErrorString( err ) );
                    }
                    break;
            }
        }
        else
            simDat[ip2simdat[S.ip]].fifo( packets, &nempty );

        pct = (100 * *packets) / (*packets + nempty);
    }

    return pct;
}

/* ---------------------------------------------------------------- */
/* configure ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define STOPCHECK   if( isStopped() ) return false;


void CimAcqImec::STEPAUX()
{
    QMetaObject::invokeMethod(
        mainApp(), "rsAuxStep",
        Qt::QueuedConnection );
}


void CimAcqImec::STEPPROBE()
{
    QMetaObject::invokeMethod(
        mainApp(), "rsProbeStep",
        Qt::QueuedConnection );
}


void CimAcqImec::STEPSTART()
{
    QMetaObject::invokeMethod(
        mainApp(), "rsStartStep",
        Qt::QueuedConnection );
}


bool CimAcqImec::configure()
{
    if( !_aux_config() )
        return false;

//@OBX 02/01/2023 Jan answers in email that probe-configuration API
//@OBX is NOT currently threadsafe, though that is future ambition.

    if( !_1t_configProbes( T ) )
        return false;

//    if( !_mt_configProbes( T ) )
//        return false;

    return _st_config();
}

/* ---------------------------------------------------------------- */
/* createAcqWorkerThreads ----------------------------------------- */
/* ---------------------------------------------------------------- */

// High channel count probes get own worker
//
void CimAcqImec::createAcqWorkerThreads()
{
// @@@ FIX Tune streams per thread here and in triggers
    const int                   nStrPerThd = 3;
    std::vector<ImAcqStream>    streams;

// Probes, PXI & OneBox

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

        if( p.stream_nChans( jsIM, ip ) > 384 ) {

            std::vector<ImAcqStream>    solo;
            solo.push_back( ImAcqStream( T, p, owner->imQ[ip], jsIM, ip ) );
            acqThd.push_back( new ImAcqThread( this, acqShr, solo ) );
            continue;
        }

        streams.push_back( ImAcqStream( T, p, owner->imQ[ip], jsIM, ip ) );

        if( streams.size() >= nStrPerThd ) {
            acqThd.push_back( new ImAcqThread( this, acqShr, streams ) );
            streams.clear();
        }
    }

// OneBox ADC streams

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip ) {

        streams.push_back( ImAcqStream( T, p, owner->obQ[ip], jsOB, ip ) );

        if( streams.size() >= nStrPerThd ) {
            acqThd.push_back( new ImAcqThread( this, acqShr, streams ) );
            streams.clear();
        }
    }

// Push last stream if needed

    if( streams.size() )
        acqThd.push_back( new ImAcqThread( this, acqShr, streams ) );
}

/* ---------------------------------------------------------------- */
/* Start ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool CimAcqImec::startAcq()
{
    STOPCHECK;

    if( p.stream_nIM() + p.stream_nOB() == 0 )
        goto done;

    if( !p.im.prbAll.trgSource ) {

        if( !_st_softStart() )
            return false;

        Log() << "IMEC Acquisition started";
    }
    else
        Log() << "IMEC Waiting for external trigger";

done:
    STEPSTART();
    return true;
}

/* ---------------------------------------------------------------- */
/* Error  --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CimAcqImec::runError( QString err )
{
    Error() << err;
    emit owner->daqError( err );
}


QString CimAcqImec::makeErrorString( NP_ErrorCode err ) const
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
}

#endif  // HAVE_IMEC


