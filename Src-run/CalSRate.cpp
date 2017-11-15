
#include "CalSRate.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMAP.h"
#include "DataFileNI.h"

#include <QDateTime>
#include <QMessageBox>

#include <math.h>




// - Assert run parameters for generating calibration data files.
// - Set timer to collect needed sample count.
//
CalSRate::CalSRate()
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

        CimCfg::AttrEach    &E = p.im.each[0];

        E.sns.saveBits.clear();
        E.sns.saveBits.resize( E.imCumTypCnt[CimCfg::imSumAll] );
        E.sns.saveBits.setBit( E.imCumTypCnt[CimCfg::imSumAll] - 1 );
    }

    if( p.ni.enabled ) {

        p.ni.sns.saveBits.clear();
        p.ni.sns.saveBits.resize( p.ni.niCumTypCnt[CniCfg::niSumAll] );

        if( p.sync.niChanType == 0 ) {

            int word = p.ni.niCumTypCnt[CniCfg::niSumAnalog]
                        + p.sync.niChan/16;

            p.ni.sns.uiSaveChanStr = QString::number( word );
            p.ni.sns.saveBits.setBit( word );
        }
        else {
            p.ni.sns.uiSaveChanStr = QString::number( p.sync.niChan );
            p.ni.sns.saveBits.setBit( p.sync.niChan );
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


// - Analyze data files.
// - Ask if user accepts new values...
// - Write new values into user parameters.
// - Restore user parameters.
//
void CalSRate::finish()
{
    double  imAV = 0, imSE = 0,
            niAV = 0, niSE = 0;
    MainApp             *app = mainApp();
    ConfigCtl           *cfg = app->cfgCtl();
    const DAQ::Params   &p   = cfg->acceptedParams;

    QString imResult = "<disabled or no edges found>",
            niResult = imResult;

    if( p.im.enabled ) {

        CalcRateIM( imAV, imSE, p );

        if( imAV > 0 ) {

            imResult =
            QString("%1 +/- %2")
            .arg( imAV, 0, 'f', 4 ).arg( imSE, 0, 'f', 4 );
        }
    }

    if( p.ni.enabled ) {

        CalcRateNI( niAV, niSE, p );

        if( niAV > 0 ) {

            niResult =
            QString("%1 +/- %2")
            .arg( niAV, 0, 'f', 4 ).arg( niSE, 0, 'f', 4 );
        }
    }

    int yesNo = QMessageBox::question(
        0,
        "Verify New Sample Rates",
        QString(
        "These are the old rates and new measurement results:\n\n"
        "    IM  %1  :  %2\n"
        "    NI  %3  :  %4\n\n"
        "Do you accept the successful measurements?")
        .arg( p.im.all.srate, 0, 'f', 4 )
        .arg( imResult )
        .arg( p.ni.srate, 0, 'f', 4 )
        .arg( niResult ),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    if( yesNo == QMessageBox::Yes ) {

        if( imAV > 0 )
            oldParams.im.all.srate = imAV;

        if( niAV > 0 )
            oldParams.ni.srate = niAV;
    }

    cfg->setParams( oldParams, true );
    app->calFinished();
}


void CalSRate::CalcRateIM( double &av, double &se, const DAQ::Params &p )
{
    const int nthEdge = 10.0 / 1.0;

    DataFile    *df = new DataFileIMAP( 0 );
    df->openForRead(
        QString("%1/%2_g0_t0.%3.bin")
        .arg( mainApp()->runDir() )
        .arg( p.sns.runName )
        .arg( df->fileLblFromObj() ));

    double  srate   = df->samplingRateHz(),
            lastV   = 0,
            s1      = 0,
            s2      = 0;
    qint64  nRem    = df->scanCount(),
            xpos    = 0;
    int     iEdge   = nthEdge - 1,
            N       = 0;
    bool    isHi;

// Scan digital channel

    int mask = 1 << p.sync.imChan;

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
            isHi = (data[0] & mask) > 0;

        // Scan block for edges

        for( int i = 0; i < ntpts; ++i ) {

            if( isHi ) {

                if( (data[i] & mask) < 1 )
                    isHi = false;
            }
            else {

                if( (data[i] & mask) > 0 ) {

                    if( ++iEdge >= nthEdge ) {

                        if( lastV > 0 ) {

                            double v = xpos + i - lastV;

                            s1 += v;
                            s2 += v*v;
                            ++N;
                        }

                        lastV = xpos + i;
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

// Stats

    if( N > 0 ) {

        double  var = (s2 - s1*s1/N) / (N - 1);
        double  per = nthEdge * p.sync.sourcePeriod;

        if( var > 0.0 )
            se = sqrt( var / N ) / per;

        av = s1 / (N * per);
    }

    df->closeAndFinalize();
    delete df;
}


void CalSRate::CalcRateNI( double &av, double &se, const DAQ::Params &p )
{
    const int nthEdge = 10.0 / 1.0;

    DataFile    *df = new DataFileNI;
    df->openForRead(
        QString("%1/%2_g0_t0.%3.bin")
        .arg( mainApp()->runDir() )
        .arg( p.sns.runName )
        .arg( df->fileLblFromObj() ));

    double  srate   = df->samplingRateHz(),
            lastV   = 0,
            s1      = 0,
            s2      = 0;
    qint64  nRem    = df->scanCount(),
            xpos    = 0;
    int     iEdge   = nthEdge - 1,
            N       = 0;
    bool    isHi;

    if( p.sync.niChanType == 0 ) {

        // Scan digital channel

        int mask = 1 << p.sync.niChan;

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
                isHi = (data[0] & mask) > 0;

            // Scan block for edges

            for( int i = 0; i < ntpts; ++i ) {

                if( isHi ) {

                    if( (data[i] & mask) < 1 )
                        isHi = false;
                }
                else {

                    if( (data[i] & mask) > 0 ) {

                        if( ++iEdge >= nthEdge ) {

                            if( lastV > 0 ) {

                                double v = xpos + i - lastV;

                                s1 += v;
                                s2 += v*v;
                                ++N;
                            }

                            lastV = xpos + i;
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
    }
    else {

        // Scan analog channel

        int T = p.sync.niThresh/df->vRange().rmax * 32768;

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
                isHi = data[0] > T;

            // Scan block for edges

            for( int i = 0; i < ntpts; ++i ) {

                if( isHi ) {

                    if( data[i] <= T )
                        isHi = false;
                }
                else {

                    if( data[i] > T ) {

                        if( ++iEdge >= nthEdge ) {

                            if( lastV > 0 ) {

                                double v = xpos + i - lastV;

                                s1 += v;
                                s2 += v*v;
                                ++N;
                            }

                            lastV = xpos + i;
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
    }

// Stats

    if( N > 0 ) {

        double  var = (s2 - s1*s1/N) / (N - 1);
        double  per = nthEdge * p.sync.sourcePeriod;

        if( var > 0.0 )
            se = sqrt( var / N ) / per;

        av = s1 / (N * per);
    }

    df->closeAndFinalize();
    delete df;
}


