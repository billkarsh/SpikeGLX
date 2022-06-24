
#include "SvyPrb.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "AIQ.h"
#include "DataFileIMAP.h"
#include "DataFileNI.h"
#include "DataFileOB.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QDesktopWidget>

#include <math.h>


//#define EDGEFILES




/* ---------------------------------------------------------------- */
/* SvySBTT -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "s c t1 t2"
//
SvySBTT SvySBTT::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return SvySBTT(
            sl.at( 0 ).toInt(),
            sl.at( 1 ).toInt(),
            sl.at( 2 ).toLongLong(),
            sl.at( 3 ).toLongLong() );
}

/* ---------------------------------------------------------------- */
/* SvyVSBTT ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: (s c t1 t2)(s c t1 t2)()()...
//
bool SvyVSBTT::fromMeta( const DataFile *df )
{
    e.clear();

// Any banks?

    QVariant    qv = df->getParam( "imSvyMaxBnk" );

    if( qv == QVariant::Invalid || qv.toInt() < 0 ) {
        nmaps = 0;
        return false;
    }

    nmaps = 1;

// SBTT

    qv = df->getParam( "~svySBTT" );

    if( qv == QVariant::Invalid )
        return true;

    QStringList sl = qv.toString().split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Entries

    nmaps += n;

    e.reserve( n );

    for( int i = 0; i < n; ++i )
        e.push_back( SvySBTT::fromString( sl[i] ) );

    return true;
}

/* ---------------------------------------------------------------- */
/* SvyPrbWorker --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SvyPrbWorker::run()
{
    int nIM = vIM.size(),
        nOB = vOB.size(),
        nNI = vNI.size();

    pctCum = 0;
    pctMax = 10*(nIM + nOB + nNI);
    pctRpt = 0;

    for( int is = 0; is < nIM; ++is ) {

        calcRateIM( vIM[is] );

        if( isCanceled() )
            goto exit;

        reportTenth( 10 );
        pctCum += 10;
    }

    if( isCanceled() )
        goto exit;

    for( int is = 0; is < nOB; ++is ) {

        calcRateOB( vOB[is] );

        if( isCanceled() )
            goto exit;

        reportTenth( 10 );
        pctCum += 10;
    }

    if( isCanceled() )
        goto exit;

    for( int is = 0; is < nNI; ++is ) {

        calcRateNI( vNI[is] );

        if( isCanceled() )
            goto exit;

        reportTenth( 10 );
        pctCum += 10;
    }

exit:
    reportTenth( pctMax );
    emit finished();
}


void SvyPrbWorker::reportTenth( int tenth )
{
    int pct = qMin( 100.0, 100.0 * (pctCum + tenth) / pctMax );

    if( pct > pctRpt ) {

        pctRpt = pct;
        emit percent( pct );
    }
}


void SvyPrbWorker::calcRateIM( SvyPrbStream &S )
{
    QVariant    qv;
    double      syncPer;
    int         syncBit;

// ---------
// Open file
// ---------

    DataFileIMAP    *df = new DataFileIMAP( S.ip );

    if( !df->openForRead( runTag.filename( 0, S.ip, "ap.bin" ), S.err ) )
        goto close;

    S.srate = df->getParam( "imSampRate" ).toDouble();

// ---------------------------
// Extract sync metadata items
// ---------------------------

    qv = df->getParam( "syncSourcePeriod" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncSourcePeriod'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncPer = qv.toDouble();

    syncBit = 6;    // Sync signal always at bit 6 of AUX word

// ----
// Scan
// ----

    scanDigital(
        S, df, syncPer, syncBit,
        df->cumTypCnt()[CimCfg::imSumNeural] );

// -----
// Close
// -----

close:
    delete df;
}


void SvyPrbWorker::calcRateOB( SvyPrbStream &S )
{
    QVariant    qv;
    double      syncPer;
    int         syncBit;

// ---------
// Open file
// ---------

    DataFileOB  *df = new DataFileOB( S.ip );

    if( !df->openForRead( runTag.filename( 2, S.ip, "bin" ), S.err ) )
        goto close;

    S.srate = df->getParam( "obSampRate" ).toDouble();

// ---------------------------
// Extract sync metadata items
// ---------------------------

    qv = df->getParam( "syncSourcePeriod" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncSourcePeriod'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncPer = qv.toDouble();

    syncBit = 6;    // Sync signal always at bit 6 of AUX word

// ----
// Scan
// ----

    scanDigital(
        S, df, syncPer, syncBit,
        df->cumTypCnt()[CimCfg::obSumData] );

// -----
// Close
// -----

close:
    delete df;
}


void SvyPrbWorker::calcRateNI( SvyPrbStream &S )
{
    QVariant    qv;
    double      syncPer,
                syncThresh;
    int         syncType,
                syncChan;

// ---------
// Open file
// ---------

    DataFileNI  *df = new DataFileNI;

    if( !df->openForRead( runTag.filename( 3, -1, "bin" ), S.err ) )
        goto close;

    S.srate = df->getParam( "niSampRate" ).toDouble();

// ---------------------------
// Extract sync metadata items
// ---------------------------

    qv = df->getParam( "syncSourcePeriod" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncSourcePeriod'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncPer = qv.toDouble();

    qv = df->getParam( "syncNiThresh" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncNiThresh'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncThresh = qv.toDouble();

    qv = df->getParam( "syncNiChanType" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncNiChanType'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncType = qv.toInt();

    qv = df->getParam( "syncNiChan" );
    if( qv == QVariant::Invalid ) {
        S.err =
        QString("%1 missing meta item 'syncNiChan'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncChan = qv.toInt();

// ----
// Scan
// ----

    if( syncType == 0 ) {
        scanDigital(
            S, df, syncPer, syncChan,
            df->cumTypCnt()[CniCfg::niSumAnalog] + syncChan/16 );
    }
    else
        scanAnalog( S, df, syncPer, syncThresh, syncChan );

// -----
// Close
// -----

close:
    delete df;
}


void SvyPrbWorker::scanDigital(
    SvyPrbStream    &S,
    DataFile        *df,
    double          syncPer,
    int             syncBit,
    int             dword )
{
#ifdef EDGEFILES
QFile f( QString("%1/%2_edges.txt")
        .arg( mainApp()->dataDir() )
        .arg( df->fileLblFromObj() ) );
f.open( QIODevice::WriteOnly | QIODevice::Text );
QTextStream ts( &f );
#endif

// We set statN to the statistical sample size we want for averaging.
// Given statN and the pulser period, we next see how wide a counting
// window (nthEdge = n pulser periods) we can use given the time span
// of the file. Wider counting windows are better because the count
// error tends to be order(1) for a good clock no matter how wide the
// window. Wider windows give better standard error estimators.

    const int statN = 10;

    std::vector<Bin>    vB;
    int                 nb = 0;

    double  srate   = df->samplingRateHz();
    qint64  nRem    = df->scanCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1,
            tenth   = 0;
    bool    isHi    = false;

    int iword   = df->channelIDs().indexOf( dword ),
        mask    = 1 << (syncBit % 16);

    if( iword < 0 ) {
        S.err =
        QString("%1 sync word (chan %2) not included in saved channels")
        .arg( df->streamFromObj() )
        .arg( dword );
        return;
    }

// --------------------------
// Collect and bin the counts
// --------------------------

    for(;;) {

        if( isCanceled() ) {
            S.err = "canceled";
            return;
        }

        vec_i16 data;
        qint64  ntpts,
                chunk = srate,
                nthis = qMin( chunk, nRem );

        ntpts = df->readScans( data, xpos, nthis, QBitArray() );

        if( ntpts <= 0 )
            break;

        // Init high/low flag

        if( !xpos )
            isHi = (data[iword] & mask) > 0;

        // Scan block for edges

        for( int i = 0; i < ntpts; ++i ) {

            if( isHi ) {

                if( (data[i*nC + iword] & mask) < 1 )
                    isHi = false;
            }
            else {

                if( (data[i*nC + iword] & mask) > 0 ) {

                    if( ++iEdge >= nthEdge ) {

                        if( lastX > 0 ) {

                            qint64  c = xpos + i - lastX;

#ifdef EDGEFILES
ts << c << "\n";
#endif

                            for( int ib = 0; ib < nb; ++ib ) {

                                if( vB[ib].isIn( c ) )
                                    goto binned;
                            }

                            vB.push_back( Bin( c ) );
                            ++nb;
                        }

binned:
                        lastX = xpos + i;
                        iEdge = 0;
                        reportTenth( ++tenth );
                    }

                    isHi = true;
                }
            }
        }

        // Advance for next block

        xpos    += ntpts;
        nRem    -= ntpts;
    }

// ---------------
// Remove outliers
// ---------------

    if( nb ) {

        for( int ib = nb - 1; ib >= 0; --ib ) {

            if( vB[ib].n <= 1 ) {
                vB.erase( vB.begin() + ib );
                --nb;
            }
        }
    }
    else
        S.err = QString("%1 no edges found").arg( df->fileLblFromObj() );

// -----------------
// Collect stat sums
// -----------------

    double  s1 = 0,
            s2 = 0;
    int     N  = 0;

    for( int ib = 0; ib < nb; ++ib ) {

        const Bin   &b = vB[ib];

        for( int ic = 0; ic < b.n; ++ic ) {

            double  c = b.C[ic];

            s1 += c;
            s2 += c*c;
        }

        N += b.n;
    }

// ----------------------------
// Stats (tolerate one outlier)
// ----------------------------

    if( N >= statN - 1 ) {

        double  var = (s2 - s1*s1/N) / (N - 1);
        double  per = nthEdge * syncPer;

        if( var > 0.0 )
            S.se = sqrt( var / N ) / per;

        S.av = s1 / (N * per);
    }
    else {
        S.err = QString("%1 too many outliers")
                .arg( df->fileLblFromObj() );
    }

#ifdef EDGEFILES
Log() << df->fileLblFromObj() << " win N " << nthEdge << " " << N;
#endif
}


void SvyPrbWorker::scanAnalog(
    SvyPrbStream    &S,
    DataFile        *df,
    double          syncPer,
    double          syncThresh,
    int             syncChan )
{
#ifdef EDGEFILES
QFile f( QString("%1/%2_edges.txt")
        .arg( mainApp()->dataDir() )
        .arg( df->fileLblFromObj() ) );
f.open( QIODevice::WriteOnly | QIODevice::Text );
QTextStream ts( &f );
#endif

// We set statN to the statistical sample size we want for averaging.
// Given statN and the pulser period, we next see how wide a counting
// window (nthEdge = n pulser periods) we can use given the time span
// of the file. Wider counting windows are better because the count
// error tends to be order(1) for a good clock no matter how wide the
// window. Wider windows give better standard error estimators.

    const int statN = 10;

    std::vector<Bin>    vB;
    int                 nb = 0;

    double  srate   = df->samplingRateHz();
    qint64  nRem    = df->scanCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1,
            tenth   = 0;
    bool    isHi    = false;

    int iword   = df->channelIDs().indexOf( syncChan ),
        T       = syncThresh / df->vRange().rmax * 32768;

    if( iword < 0 ) {
        S.err =
        QString("%1 sync chan [%2] not included in saved channels")
        .arg( df->streamFromObj() )
        .arg( syncChan );
        return;
    }

// --------------------------
// Collect and bin the counts
// --------------------------

    for(;;) {

        if( isCanceled() ) {
            S.err = "canceled";
            return;
        }

        vec_i16 data;
        qint64  ntpts,
                chunk = srate,
                nthis = qMin( chunk, nRem );

        ntpts = df->readScans( data, xpos, nthis, QBitArray() );

        if( ntpts <= 0 )
            break;

        // Init high/low flag

        if( !xpos )
            isHi = data[iword] > T;

        // Scan block for edges

        for( int i = 0; i < ntpts; ++i ) {

            if( isHi ) {

                if( data[i*nC + iword] <= T )
                    isHi = false;
            }
            else {

                if( data[i*nC + iword] > T ) {

                    if( ++iEdge >= nthEdge ) {

                        if( lastX > 0 ) {

                            qint64  c = xpos + i - lastX;

#ifdef EDGEFILES
ts << c << "\n";
#endif

                            for( int ib = 0; ib < nb; ++ib ) {

                                if( vB[ib].isIn( c ) )
                                    goto binned;
                            }

                            vB.push_back( Bin( c ) );
                            ++nb;
                        }

binned:
                        lastX = xpos + i;
                        iEdge = 0;
                        reportTenth( ++tenth );
                    }

                    isHi = true;
                }
            }
        }

        // Advance for next block

        xpos    += ntpts;
        nRem    -= ntpts;
    }

// ---------------
// Remove outliers
// ---------------

    if( nb ) {

        for( int ib = nb - 1; ib >= 0; --ib ) {

            if( vB[ib].n <= 1 ) {
                vB.erase( vB.begin() + ib );
                --nb;
            }
        }
    }
    else
        S.err = QString("%1 no edges found").arg( df->fileLblFromObj() );

// -----------------
// Collect stat sums
// -----------------

    double  s1 = 0,
            s2 = 0;
    int     N  = 0;

    for( int ib = 0; ib < nb; ++ib ) {

        const Bin   &b = vB[ib];

        for( int ic = 0; ic < b.n; ++ic ) {

            double  c = b.C[ic];

            s1 += c;
            s2 += c*c;
        }

        N += b.n;
    }

// ----------------------------
// Stats (tolerate one outlier)
// ----------------------------

    if( N >= statN - 1 ) {

        double  var = (s2 - s1*s1/N) / (N - 1);
        double  per = nthEdge * syncPer;

        if( var > 0.0 )
            S.se = sqrt( var / N ) / per;

        S.av = s1 / (N * per);
    }
    else {
        S.err = QString("%1 too many outliers")
                .arg( df->fileLblFromObj() );
    }

#ifdef EDGEFILES
Log() << df->fileLblFromObj() << " win N " << nthEdge << " " << N;
#endif
}

/* ---------------------------------------------------------------- */
/* SvyPrbThread --------------------------------------------------- */
/* ---------------------------------------------------------------- */

SvyPrbThread::SvyPrbThread(
    DFRunTag                    &runTag,
    std::vector<SvyPrbStream>   &vIM,
    std::vector<SvyPrbStream>   &vOB,
    std::vector<SvyPrbStream>   &vNI )
{
    thread  = new QThread;
    worker  = new SvyPrbWorker( runTag, vIM, vOB, vNI );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Thread manually started via startRun().
//    thread->start();
}


SvyPrbThread::~SvyPrbThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}


void SvyPrbThread::startRun()
{
    thread->start();
}

/* ---------------------------------------------------------------- */
/* SvyPrbRun ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Assert run parameters for surveying all probe sites.
//
void SvyPrbRun::initRun()
{
    MainApp     *app = mainApp();
    ConfigCtl   *cfg = app->cfgCtl();
    DAQ::Params p;
    QDateTime   tCreate( QDateTime::currentDateTime() );
    int         np;

// ---------------------
// Set custom run params
// ---------------------

    p  = oldParams = cfg->acceptedParams;
    np = p.stream_nIM();

    vCurShnk.assign( np, 0 );
    vCurBank.assign( np, 0 );
    vSBTT.resize( np );

    for( int ip = 0; ip < np; ++ip ) {

        CimCfg::PrbEach &E = p.im.prbj[ip];
        QString         err;
        int             nB = E.roTbl->nShank() * (E.svyMaxBnk + 1);

        E.imroFile.clear();
        E.sns.shankMapFile.clear();
        E.sns.chanMapFile.clear();
        cfg->validImROTbl( err, E, ip );
        cfg->validImShankMap( err, E, ip );
        cfg->validImChanMap( err, E, ip );

        // AP + SY
        E.sns.uiSaveChanStr =
            QString("0:%1,%2")
                .arg( E.roTbl->nAP() - 1 )
                .arg( E.roTbl->nAP() + E.roTbl->nLF() );

        E.sns.deriveSaveBits(
            err,
            p.jsip2stream( 2, ip ),
            p.stream_nChans( 2, ip ) );

        if( nB > nrunbank )
            nrunbank = nB;
    }

    p.mode.mGate        = DAQ::eGateImmed;
    p.mode.mTrig        = DAQ::eTrigImmed;
    p.mode.manOvShowBut = false;
    p.mode.manOvInitOff = false;

    p.sns.notes     = "Survey probe banks";
    p.sns.runName   =
        QString("SvyPrb_%1")
        .arg( dateTime2Str( tCreate, Qt::ISODate ).replace( ":", "." ) );
    p.sns.fldPerPrb = false;

    cfg->setParams( p, false );
}


int SvyPrbRun::msPerBnk()
{
    return 1000 * oldParams.im.prbAll.svySecPerBnk;
}


bool SvyPrbRun::nextBank()
{
    if( ++irunbank >= nrunbank )
        return false;

    ConfigCtl                   *cfg    = mainApp()->cfgCtl();
    Run                         *run    = mainApp()->getRun();
    DAQ::Params                 &p      = cfg->acceptedParams;
    const CimCfg::ImProbeTable  &T      = cfg->prbTab;

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

        CimCfg::PrbEach &E = p.im.prbj[ip];
        int             &S = vCurShnk[ip],
                        &B = vCurBank[ip];

        if( S >= E.roTbl->nShank() )
            continue;

        if( ++B > E.svyMaxBnk ) {

            if( ++S >= E.roTbl->nShank() ) {
                run->dfHaltiq( p.stream2iq( QString("imec%1").arg( ip ) ) );
                continue;
            }

            B = 0;
        }

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
        QString                     err;
        qint64                      t0 = run->dfGetFileStart( 2, ip ),
                                    t1 = run->getQ( 2, ip )->endCount() - t0,
                                    t2;

        run->grfHardPause( true );
        run->grfWaitPaused();

        E.roTbl->fillShankAndBank( S, B );
        E.roTbl->selectSites( P.slot, P.port, P.dock, true );
        E.sns.shankMapFile.clear();
        E.sns.chanMapFile.clear();
        cfg->validImShankMap( err, E, ip );
        cfg->validImChanMap( err, E, ip );
        run->grfHardPause( false );

        run->grfUpdateProbe( ip, true, true );

        // Record (S B t1 t2) transition times.
        // t2 = resume time + 0.5 sec margin

        t2 = run->getQ( 2, ip )->endCount() + E.srate / 2 - t0;

        vSBTT[ip] +=
            QString("(%1 %2 %3 %4)").arg( S ).arg( B ).arg( t1 ).arg( t2 );

        run->dfSetSBTT( ip, vSBTT[ip] );
    }

    return true;
}


// - Analyze data files.
//
void SvyPrbRun::finish()
{
return;
    MainApp             *app = mainApp();
    ConfigCtl           *cfg = app->cfgCtl();
    const DAQ::Params   &p   = cfg->acceptedParams;

    runTag = DFRunTag( app->dataDir(), p.sns.runName );

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip )
        vIM.push_back( SvyPrbStream( ip ) );

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip )
        vOB.push_back( SvyPrbStream( ip ) );

    if( p.stream_nNI() )
        vNI.push_back( SvyPrbStream( -1 ) );

    createPrgDlg();

    thd = new SvyPrbThread( runTag, vIM, vOB, vNI );
    ConnectUI( thd->worker, SIGNAL(finished()), this, SLOT(finish_cleanup()) );
    ConnectUI( thd->worker, SIGNAL(percent(int)), prgDlg, SLOT(setValue(int)) );

    thd->startRun();
}


// - Present results to user...
// - Write new values into user parameters.
// - Restore user parameters.
//
void SvyPrbRun::finish_cleanup()
{
    if( thd ) {
        delete thd;
        thd = 0;
    }

    if( prgDlg ) {
        prgDlg->hide();
        prgDlg->deleteLater();
        prgDlg = 0;
    }

    MainApp             *app = mainApp();
    ConfigCtl           *cfg = app->cfgCtl();
    const DAQ::Params   &p   = cfg->acceptedParams;
    int                 ns;

#if 0
    QString msg =
        "These are the old rates and the new measurement results:\n"
        "(A text message indicates an unsuccessful measurement)\n\n";

// ----
// Imec
// ----

    if( (ns = vIM.size()) ) {

        for( int is = 0; is < ns; ++is ) {

            const SvyPrbStream  &S    = vIM[is];
            double              srate = p.im.prbj[S.ip].srate;

            if( !S.err.isEmpty() ) {
                msg += QString(
                "    IM%1  %2  :  %3\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 )
                .arg( S.err );
            }
            else if( S.av == 0 ) {
                msg += QString(
                "    IM%1  %2  :  canceled\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 );
            }
            else {
                msg += QString(
                "    IM%1  %2  :  %3 +/- %4\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 )
                .arg( S.av, 0, 'f', 6 )
                .arg( S.se, 0, 'f', 6 );
            }
        }
    }
    else
        msg += "    IM-all  :  <disabled>\n";

// ---
// Obx
// ---

    if( (ns = vOB.size()) ) {

        for( int is = 0; is < ns; ++is ) {

            const SvyPrbStream  &S    = vOB[is];
            double              srate = p.im.obxj[S.ip].srate;

            if( !S.err.isEmpty() ) {
                msg += QString(
                "    OB%1  %2  :  %3\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 )
                .arg( S.err );
            }
            else if( S.av == 0 ) {
                msg += QString(
                "    OB%1  %2  :  canceled\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 );
            }
            else {
                msg += QString(
                "    OB%1  %2  :  %3 +/- %4\n" )
                .arg( S.ip )
                .arg( srate, 0, 'f', 6 )
                .arg( S.av, 0, 'f', 6 )
                .arg( S.se, 0, 'f', 6 );
            }
        }
    }
    else
        msg += "    OB-all  :  <disabled>\n";

// ----
// Nidq
// ----

    if( vNI.size() ) {

        SvyPrbStream    &S = vNI[0];

        if( !S.err.isEmpty() ) {
            msg += QString(
            "    NI  %1  :  %2\n" )
            .arg( p.ni.srate, 0, 'f', 6 )
            .arg( S.err );
        }
        else if( S.av == 0 ) {
            msg += QString(
            "    NI  %1  :  canceled\n" )
            .arg( p.ni.srate, 0, 'f', 6 );
        }
        else {
            msg += QString(
            "    NI  %1  :  %2 +/- %3\n" )
            .arg( p.ni.srate, 0, 'f', 6 )
            .arg( S.av, 0, 'f', 6 )
            .arg( S.se, 0, 'f', 6 );
        }
    }
    else {
        msg += QString(
        "    NI  %1  :  <disabled>\n" )
        .arg( p.ni.srate, 0, 'f', 6 );
    }

    msg +=
        "\nUnsuccessful measurements will not be applied.\n"
        "Do you wish to apply the successful measurements?";

    int yesNo = QMessageBox::question(
        0,
        "Verify New Sample Rates",
        msg,
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo == QMessageBox::Yes ) {

        // ----
        // Imec
        // ----

        for( int is = 0, ns = vIM.size(); is < ns; ++is ) {

            const SvyPrbStream  &S = vIM[is];

            if( S.av > 0 ) {
                oldParams.im.prbj[S.ip].srate = S.av;
                cfg->prbTab.set_iProbe_SRate( S.ip, S.av );
            }
        }

        cfg->prbTab.saveProbeSRates();

        // ---
        // Obx
        // ---

        for( int is = 0, ns = vOB.size(); is < ns; ++is ) {

            const SvyPrbStream  &S = vOB[is];

            if( S.av > 0 ) {
                oldParams.im.obxj[S.ip].srate = S.av;
                cfg->prbTab.set_iOnebox_SRate( S.ip, S.av );
            }
        }

        cfg->prbTab.saveOneboxSRates();

        // ----
        // Nidq
        // ----

        if( vNI.size() && vNI[0].av > 0 ) {
            oldParams.ni.srate = vNI[0].av;
            oldParams.ni.setSRate( oldParams.ni.clockSource, vNI[0].av );
            oldParams.ni.saveSRateTable();
        }
    }
#endif

    cfg->setParams( oldParams, true );
    app->runSvyFinished();
}


void SvyPrbRun::createPrgDlg()
{
    prgDlg = new QProgressDialog();

    Qt::WindowFlags F = prgDlg->windowFlags();
    F |= Qt::WindowStaysOnTopHint;
    F &= ~(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);
    prgDlg->setWindowFlags( F );

    prgDlg->setWindowTitle( "Sample Rate Calibration" );
    prgDlg->setWindowModality( Qt::ApplicationModal );
    prgDlg->setAutoReset( false );
    prgDlg->setCancelButtonText( QString() );

    QSize   dlg = prgDlg->sizeHint();
    QRect   DT  = QApplication::desktop()->screenGeometry();

    prgDlg->setMinimumWidth( 1.25 * dlg.width() );
    dlg = prgDlg->sizeHint();

    prgDlg->move(
        (DT.width()  - dlg.width()) / 2,
        (DT.height() - dlg.height())/ 2 );

    prgDlg->show();
    guiBreathe();
}


