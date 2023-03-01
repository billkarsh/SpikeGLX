
#include "CalSRate.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMAP.h"
#include "DataFileNI.h"
#include "DataFileOB.h"

#include <QMessageBox>
#include <QProgressDialog>
#include <QDesktopWidget>

#include <math.h>


//#define EDGEFILES




/* ---------------------------------------------------------------- */
/* CalSRWorker ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CalSRWorker::run()
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


void CalSRWorker::reportTenth( int tenth )
{
    int pct = qMin( 100.0, 100.0 * (pctCum + tenth) / pctMax );

    if( pct > pctRpt ) {

        pctRpt = pct;
        emit percent( pct );
    }
}


void CalSRWorker::calcRateIM( CalSRStream &S )
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


void CalSRWorker::calcRateOB( CalSRStream &S )
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


void CalSRWorker::calcRateNI( CalSRStream &S )
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


void CalSRWorker::scanDigital(
    CalSRStream     &S,
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
    qint64  nRem    = df->sampCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1,
            tenth   = 0;
    bool    isHi    = false;

    int iword   = df->fileChans().indexOf( dword ),
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

        ntpts = df->readSamps( data, xpos, nthis, QBitArray() );

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


void CalSRWorker::scanAnalog(
    CalSRStream     &S,
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
    qint64  nRem    = df->sampCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1,
            tenth   = 0;
    bool    isHi    = false;

    int iword   = df->fileChans().indexOf( syncChan ),
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

        ntpts = df->readSamps( data, xpos, nthis, QBitArray() );

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
/* CalSRThread ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

CalSRThread::CalSRThread(
    DFRunTag                    &runTag,
    std::vector<CalSRStream>    &vIM,
    std::vector<CalSRStream>    &vOB,
    std::vector<CalSRStream>    &vNI )
{
    thread  = new QThread;
    worker  = new CalSRWorker( runTag, vIM, vOB, vNI );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// Thread manually started via startRun().
//    thread->start();
}


CalSRThread::~CalSRThread()
{
// worker object auto-deleted asynchronously
// thread object manually deleted synchronously (so we can call wait())

    if( thread->isRunning() )
        thread->wait();

    delete thread;
}


void CalSRThread::startRun()
{
    thread->start();
}

/* ---------------------------------------------------------------- */
/* CalSRRun ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Assert run parameters for generating calibration data files.
//
void CalSRRun::initRun()
{
    MainApp     *app = mainApp();
    ConfigCtl   *cfg = app->cfgCtl();
    DAQ::Params p;
    QDateTime   tCreate( QDateTime::currentDateTime() );

// ---------------------
// Set custom run params
// ---------------------

    p = oldParams = cfg->acceptedParams;

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

        CimCfg::PrbEach     &E = p.im.prbj[ip];
        int                 nC = p.stream_nChans( jsIM, ip );

        E.sns.saveBits.clear();
        E.sns.saveBits.resize( nC );
        E.sns.saveBits.setBit( nC - 1 );
        E.sns.uiSaveChanStr = QString::number( nC - 1 );
    }

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip ) {

        CimCfg::ObxEach     &E = p.im.obxj[ip];
        int                 nC = p.stream_nChans( jsOB, ip );

        E.sns.saveBits.clear();
        E.sns.saveBits.resize( nC );
        E.sns.saveBits.setBit( nC - 1 );
        E.sns.uiSaveChanStr = QString::number( nC - 1 );
    }

    if( p.stream_nNI() ) {

        int word;

        p.ni.sns.saveBits.clear();
        p.ni.sns.saveBits.resize( p.stream_nChans( jsNI, 0 ) );

        if( p.sync.niChanType == 0 )
            word = p.ni.niCumTypCnt[CniCfg::niSumAnalog] + p.sync.niChan/16;
        else
            word = p.sync.niChan;

        p.ni.sns.saveBits.setBit( word );
        p.ni.sns.uiSaveChanStr = QString::number( word );
    }

    p.mode.mGate        = DAQ::eGateImmed;
    p.mode.mTrig        = DAQ::eTrigImmed;
    p.mode.manOvShowBut = false;
    p.mode.manOvInitOff = false;

    p.sns.notes     = "Calibrating sample rates";
    p.sns.runName   =
        QString("CalSRate_%1")
        .arg( dateTime2Str( tCreate, Qt::ISODate ).replace( ":", "." ) );
    p.sns.fldPerPrb = false;

    cfg->setParams( p, false );
}


void CalSRRun::initTimer()
{
    runTZero = getTime();
}


int CalSRRun::elapsedMS()
{
    return 1000*(getTime() - runTZero);
}


// - Analyze data files.
//
void CalSRRun::finish()
{
    MainApp             *app = mainApp();
    ConfigCtl           *cfg = app->cfgCtl();
    const DAQ::Params   &p   = cfg->acceptedParams;

    runTag = DFRunTag( app->dataDir(), p.sns.runName );

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip )
        vIM.push_back( CalSRStream( ip ) );

    for( int ip = 0, np = p.stream_nOB(); ip < np; ++ip )
        vOB.push_back( CalSRStream( ip ) );

    if( p.stream_nNI() )
        vNI.push_back( CalSRStream( -1 ) );

    createPrgDlg();

    thd = new CalSRThread( runTag, vIM, vOB, vNI );
    ConnectUI( thd->worker, SIGNAL(finished()), this, SLOT(finish_cleanup()) );
    ConnectUI( thd->worker, SIGNAL(percent(int)), prgDlg, SLOT(setValue(int)) );

    thd->startRun();
}


// - Ask if user accepts new values...
// - Write new values into user parameters.
// - Restore user parameters.
//
void CalSRRun::finish_cleanup()
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

    QString msg =
        "These are the old rates and the new measurement results:\n"
        "(A text message indicates an unsuccessful measurement)\n\n";

// ----
// Imec
// ----

    if( (ns = vIM.size()) ) {

        for( int is = 0; is < ns; ++is ) {

            const CalSRStream   &S    = vIM[is];
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

            const CalSRStream   &S    = vOB[is];
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

        CalSRStream &S = vNI[0];

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

            const CalSRStream   &S = vIM[is];

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

            const CalSRStream   &S = vOB[is];

            if( S.av > 0 ) {
                oldParams.im.obxj[S.ip].srate = S.av;
                cfg->prbTab.set_iOneBox_SRate( S.ip, S.av );
            }
        }

        cfg->prbTab.saveOneBoxSRates();

        // ----
        // Nidq
        // ----

        if( vNI.size() && vNI[0].av > 0 ) {
            oldParams.ni.srate = vNI[0].av;
            oldParams.ni.setSRate( oldParams.ni.clockSource, vNI[0].av );
            oldParams.ni.saveSRateTable();
        }
    }

    cfg->setParams( oldParams, true );
    app->runCalFinished();
}


void CalSRRun::createPrgDlg()
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


