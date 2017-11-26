
#include "CalSRate.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMAP.h"
#include "DataFileNI.h"

#include <QDateTime>
#include <QMessageBox>

#include <math.h>


//#define EDGEFILES




// Process file set similarly to finish().
// Offer to update metadata and/or daq.ini.
//
void CalSRate::fromArbFile( const QString &file )
{
    QRegExp re("\\.imec\\.ap\\.|\\.imec\\.lf\\.|\\.nidq\\.");
    re.setCaseSensitivity( Qt::CaseInsensitive );

// ------------------
// Force to imec name
// ------------------

    {
        double  av = 0, se = 0;
        QString name    = QString(file).replace( re, ".imec.ap." ),
                result  = "<disabled>",
                err;

        CalcRateIM( err, av, se, name );

        if( !err.isEmpty() )
            result = err;
        else if( av > 0 ) {
            result =
                QString("%1 +/- %2")
                .arg( av, 0, 'f', 6 )
                .arg( se, 0, 'f', 6 );
        }

        Log() << "IM: " << result;
    }

// ------------------
// Force to nidq name
// ------------------

    {
        double  av = 0, se = 0;
        QString name    = QString(file).replace( re, ".nidq." ),
                result  = "<disabled>",
                err;

        CalcRateNI( err, av, se, name );

        if( !err.isEmpty() )
            result = err;
        else if( av > 0 ) {
            result =
                QString("%1 +/- %2")
                .arg( av, 0, 'f', 6 )
                .arg( se, 0, 'f', 6 );
        }

        Log() << "NI: " << result;
    }
}


// Assert run parameters for generating calibration data files.
//
void CalSRate::initCalRun()
{
    MainApp     *app = mainApp();
    ConfigCtl   *cfg = app->cfgCtl();
    DAQ::Params p;
    QDateTime   tCreate( QDateTime::currentDateTime() );

// ---------------------
// Set custom run params
// ---------------------

    p = oldParams = cfg->acceptedParams;

    if( p.im.enabled ) {

        p.sns.imChans.saveBits.clear();
        p.sns.imChans.saveBits.resize( p.im.imCumTypCnt[CimCfg::imSumAll] );
        p.sns.imChans.saveBits.setBit( p.im.imCumTypCnt[CimCfg::imSumAll] - 1 );
    }

    if( p.ni.enabled ) {

        p.sns.niChans.saveBits.clear();
        p.sns.niChans.saveBits.resize( p.ni.niCumTypCnt[CniCfg::niSumAll] );

        if( p.sync.niChanType == 0 ) {

            int word = p.ni.niCumTypCnt[CniCfg::niSumAnalog]
                        + p.sync.niChan/16;

            p.sns.niChans.uiSaveChanStr = QString::number( word );
            p.sns.niChans.saveBits.setBit( word );
        }
        else {
            p.sns.niChans.uiSaveChanStr = QString::number( p.sync.niChan );
            p.sns.niChans.saveBits.setBit( p.sync.niChan );
        }
    }

    p.mode.mGate        = DAQ::eGateImmed;
    p.mode.mTrig        = DAQ::eTrigImmed;
    p.mode.manOvShowBut = false;
    p.mode.manOvInitOff = false;

    p.sns.notes     = "Calibrating sample rates";
    p.sns.runName   =
        QString("CalSRate_%1")
        .arg( tCreate.toString( Qt::ISODate ).replace( ":", "." ) );

    cfg->setParams( p, false );
}


void CalSRate::initCalRunTimer()
{
    calRunTZero = getTime();
}


int CalSRate::calRunElapsedMS()
{
    return 1000*(getTime() - calRunTZero);
}


// - Analyze data files.
// - Ask if user accepts new values...
// - Write new values into user parameters.
// - Restore user parameters.
//
void CalSRate::finishCalRun()
{
    double  imAV = 0, imSE = 0,
            niAV = 0, niSE = 0;
    MainApp             *app = mainApp();
    ConfigCtl           *cfg = app->cfgCtl();
    const DAQ::Params   &p   = cfg->acceptedParams;

    QString imResult = "<disabled>",
            niResult = imResult;

    if( p.im.enabled )
        doCalRunFile( imAV, imSE, imResult, p.sns.runName, "imec.ap" );

    if( p.ni.enabled )
        doCalRunFile( niAV, niSE, niResult, p.sns.runName, "nidq" );

    int yesNo = QMessageBox::question(
        0,
        "Verify New Sample Rates",
        QString(
        "These are the old rates and the new measurement results:\n"
        "(A text message indicates an unsuccessful measurement)\n\n"
        "    IM  %1  :  %2\n"
        "    NI  %3  :  %4\n\n"
        "Unsuccessful measurements will not be applied.\n"
        "Do you wish to apply the successful measurements?")
        .arg( p.im.srate, 0, 'f', 6 )
        .arg( imResult )
        .arg( p.ni.srate, 0, 'f', 6 )
        .arg( niResult ),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo == QMessageBox::Yes ) {

        if( imAV > 0 )
            oldParams.im.srate = imAV;

        if( niAV > 0 )
            oldParams.ni.srate = niAV;
    }

    cfg->setParams( oldParams, true );
    app->calFinished();
}


void CalSRate::doCalRunFile(
    double          &av,
    double          &se,
    QString         &result,
    const QString   &runName,
    const QString   &stream )
{
    QString file =
                QString("%1/%2_g0_t0.%3.bin")
                .arg( mainApp()->runDir() )
                .arg( runName )
                .arg( stream ),
            err;

    if( stream == "nidq" )
        CalcRateNI( err, av, se, file );
    else
        CalcRateIM( err, av, se, file );

    if( !err.isEmpty() )
        result = err;
    else if( av > 0 ) {
        result =
            QString("%1 +/- %2")
            .arg( av, 0, 'f', 6 )
            .arg( se, 0, 'f', 6 );
    }
}


void CalSRate::CalcRateIM(
    QString         &err,
    double          &av,
    double          &se,
    const QString   &file )
{
// ---------
// Open file
// ---------

    DataFileIMAP    *df = new DataFileIMAP;

    if( !df->openForRead( file ) )
        return;

// ---------------------------
// Extract sync metadata items
// ---------------------------

    QVariant    qv;
    double      syncPer;
    int         syncChan;

    qv = df->getParam( "syncSourcePeriod" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncSourcePeriod'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncPer = qv.toDouble();

#if 0
    qv = df->getParam( "syncImThresh" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncImThresh'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncThresh = qv.toDouble();
#endif

#if 0
    qv = df->getParam( "syncImChanType" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncImChanType'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncType = qv.toInt();
#endif

    qv = df->getParam( "syncImChan" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncImChan'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncChan = qv.toInt();

// ----
// Scan
// ----

    scanDigital(
        err, av, se, df,
        syncPer, syncChan,
        df->cumTypCnt()[CimCfg::imSumNeural] );

// -----
// Close
// -----

close:
    df->closeAndFinalize();
    delete df;
}


void CalSRate::CalcRateNI(
    QString         &err,
    double          &av,
    double          &se,
    const QString   &file )
{
// ---------
// Open file
// ---------

    DataFileNI  *df = new DataFileNI;

    if( !df->openForRead( file ) )
        return;

// ---------------------------
// Extract sync metadata items
// ---------------------------

    QVariant    qv;
    double      syncPer,
                syncThresh;
    int         syncType,
                syncChan;

    qv = df->getParam( "syncSourcePeriod" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncSourcePeriod'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncPer = qv.toDouble();

    qv = df->getParam( "syncNiThresh" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncNiThresh'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncThresh = qv.toDouble();

    qv = df->getParam( "syncNiChanType" );
    if( qv == QVariant::Invalid ) {
        err =
        QString("%1 missing meta item 'syncNiChanType'")
        .arg( df->streamFromObj() );
        goto close;
    }
    else
        syncType = qv.toInt();

    qv = df->getParam( "syncNiChan" );
    if( qv == QVariant::Invalid ) {
        err =
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
            err, av, se, df,
            syncPer, syncChan,
            df->cumTypCnt()[CniCfg::niSumAnalog] + syncChan/16 );
    }
    else
        scanAnalog( err, av, se, df, syncPer, syncThresh, syncChan );

// -----
// Close
// -----

close:
    df->closeAndFinalize();
    delete df;
}


void CalSRate::scanDigital(
    QString         &err,
    double          &av,
    double          &se,
    DataFile        *df,
    double          syncPer,
    int             syncChan,
    int             dword )
{
#ifdef EDGEFILES
QFile f( QString("%1/%2_edges.txt")
        .arg( mainApp()->runDir() )
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

    QVector<Bin>    vB;
    int             nb = 0;

    double  srate   = df->samplingRateHz();
    qint64  nRem    = df->scanCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1;
    bool    isHi;

    int iword   = df->channelIDs().indexOf( dword ),
        mask    = 1 << (syncChan % 16);

    if( iword < 0 ) {
        err =
        QString("%1 sync word (chan %2) not included in saved channels")
        .arg( df->streamFromObj() )
        .arg( dword );
        return;
    }

// --------------------------
// Collect and bin the counts
// --------------------------

    for(;;) {

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
                vB.remove( ib );
                --nb;
            }
        }
    }
    else
        err = QString("%1 no edges found").arg( df->fileLblFromObj() );

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
            se = sqrt( var / N ) / per;

        av = s1 / (N * per);
    }
    else {
        err = QString("%1 too many outliers")
                .arg( df->fileLblFromObj() );
    }

#ifdef EDGEFILES
Log() << df->fileLblFromObj() << " win N " << nthEdge << " " << N;
#endif
}


void CalSRate::scanAnalog(
    QString         &err,
    double          &av,
    double          &se,
    DataFile        *df,
    double          syncPer,
    double          syncThresh,
    int             syncChan )
{
#ifdef EDGEFILES
QFile f( QString("%1/%2_edges.txt")
        .arg( mainApp()->runDir() )
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

    QVector<Bin>    vB;
    int             nb = 0;

    double  srate   = df->samplingRateHz();
    qint64  nRem    = df->scanCount(),
            xpos    = 0,
            lastX   = 0;
    int     nC      = df->numChans(),
            nthEdge = (quint64(nRem / (srate * syncPer)) - 1) / statN,
            iEdge   = nthEdge - 1;
    bool    isHi;

    int iword   = df->channelIDs().indexOf( syncChan ),
        T       = syncThresh / df->vRange().rmax * 32768;

    if( iword < 0 ) {
        err =
        QString("%1 sync chan [%2] not included in saved channels")
        .arg( df->streamFromObj() )
        .arg( syncChan );
        return;
    }

// --------------------------
// Collect and bin the counts
// --------------------------

    for(;;) {

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
                vB.remove( ib );
                --nb;
            }
        }
    }
    else
        err = QString("%1 no edges found").arg( df->fileLblFromObj() );

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
            se = sqrt( var / N ) / per;

        av = s1 / (N * per);
    }
    else {
        err = QString("%1 too many outliers")
                .arg( df->fileLblFromObj() );
    }

#ifdef EDGEFILES
Log() << df->fileLblFromObj() << " win N " << nthEdge << " " << N;
#endif
}


