
#include "SvyPrb.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "AIQ.h"
#include "DataFile.h"


/* ---------------------------------------------------------------- */
/* SvySBTT -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "s b t1 t2"
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

// Pattern: (s b t1 t2)(s b t1 t2)()()...
//
void SvyVSBTT::fromMeta( const DataFile *df )
{
    QVariant    qv = df->getParam( "imSvyMaxBnk" );

    maxbank = 0;

    if( qv != QVariant::Invalid )
        maxbank = qv.toInt();

    e.clear();

// Any banks?

    qv = df->getParam( "imIsSvyRun" );

    if( qv == QVariant::Invalid || !qv.toBool() ) {
        nmaps = 0;
        return;
    }

    nmaps = 1;

// SBTT

    qv = df->getParam( "~svySBTT" );

    if( qv == QVariant::Invalid )
        return;

    QStringList sl = qv.toString().split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Entries

    nmaps += n;

    e.reserve( n );

    for( int i = 0; i < n; ++i )
        e.push_back( SvySBTT::fromString( sl[i] ) );

    if( df->subtypeFromObj() == "imec.lf" ) {

        for( int i = 0; i < n; ++i ) {
            e[i].t1 /= 12;
            e[i].t2 /= 12;
        }
    }

    return;
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
        int             nB = E.roTbl->nSvyShank() * (E.svyMaxBnk + 1);

        E.sns.chanMapFile.clear();
        E.sns.uiSaveChanStr.clear();

        // Adopt user IMRO; set default bank
        cfg->validImROTbl( err, E, ip );
        E.roTbl->fillShankAndBank( 0, 0 );

        cfg->validImMaps( err, E, ip );

        E.sns.deriveSaveBits(
            err,
            p.jsip2stream( jsIM, ip ),
            p.stream_nChans( jsIM, ip ) );

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

        if( S >= E.roTbl->nSvyShank() )
            continue;

        if( ++B > E.svyMaxBnk ) {

            if( ++S >= E.roTbl->nSvyShank() ) {
                run->dfHaltiq( p.stream2iq( QString("imec%1").arg( ip ) ) );
                continue;
            }

            B = 0;
        }

        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );
        QString                     err;
        qint64                      t0 = run->dfGetFileStart( jsIM, ip ),
                                    t1 = run->getQ( jsIM, ip )->endCount() - t0,
                                    t2;

        run->grfHardPause( true );
        run->grfWaitPaused();

        E.roTbl->fillShankAndBank( S, B );
        E.roTbl->selectSites( P.slot, P.port, P.dock, true );
        E.sns.chanMapFile.clear();
        cfg->validImMaps( err, E, ip );
        run->grfUpdateProbe( ip, true, true );

        run->grfHardPause( false );

        // Record (S B t1 t2) transition times.
        // t2 = resume time + 1.0 sec margin

        t2 = run->getQ( jsIM, ip )->endCount() + E.srate - t0;

        vSBTT[ip] +=
            QString("(%1 %2 %3 %4)").arg( S ).arg( B ).arg( t1 ).arg( t2 );

        run->dfSetSBTT( ip, vSBTT[ip] );
    }

    return true;
}


// Clean up.
//
void SvyPrbRun::finish()
{
    MainApp *app = mainApp();
    app->cfgCtl()->setParams( oldParams, true );
    app->runSvyFinished();
}


