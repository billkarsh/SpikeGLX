
#include "SvyPrb.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "AIQ.h"
#include "DataFile.h"

#include <QRegularExpression>


/* ---------------------------------------------------------------- */
/* SvySBTT -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "s b t1 t2"
//
SvySBTT SvySBTT::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegularExpression("\\s+"),
                                Qt::SkipEmptyParts );

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
    maxbank = 0;
    nmaps   = 0;
    e.clear();

// Any banks?

    QVariant    qv = df->getParam( "imSvyMaxBnk" );
    if( qv.isValid() )
        maxbank = qv.toInt();

    qv = df->getParam( "imIsSvyRun" );
    if( !qv.isValid() || !qv.toBool() )
        return;

// SBTT

    qv = df->getParam( "~svySBTT" );
    if( !qv.isValid() )
        return;

    QStringList sl = qv.toString().split(
                        QRegularExpression("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        Qt::SkipEmptyParts );
    int         n  = sl.size();

 // See if first elem has t1 == 0 (new data)...
 // If not, create it (old data)

    SvySBTT E0 = SvySBTT::fromString( sl[0] );

    if( E0.t1 == 0 ) {
        // NEW: all elements are present
        e.reserve( nmaps = n );
        // elem-0
        e.push_back( E0 );
        // elem-1...
        for( int i = 1; i < n; ++i )
            e.push_back( SvySBTT::fromString( sl[i] ) );
    }
    else {
        // OLD: prepend elem-0
        e.reserve( nmaps = n + 1 );
        // elem-0
        qint64 t2 = 2 * 30000;
        qv = df->getParam( "imSvySettleSec" );
        if( qv.isValid() )
            t2 = qv.toLongLong() * 30000;
        e.push_back( SvySBTT( 0, 0, 0, t2 ) );
        // elem-1
        e.push_back( E0 );
        // elem-2...
        for( int i = 1; i < n; ++i )
            e.push_back( SvySBTT::fromString( sl[i] ) );
    }

    if( df->subtypeFromObj() == "imec.lf" ) {

        for( int i = 0; i < n; ++i ) {
            e[i].t1 /= 12;
            e[i].t2 /= 12;
        }
    }
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

        vSBTT[ip] =
        QString("(0 0 0 %4)")
        .arg( p.im.prbAll.svySettleSec * E.srate );

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


int SvyPrbRun::msPerBnk( bool first )
{
    return 1000 *
            (oldParams.im.prbAll.svySecPerBnk +
            (first ? oldParams.im.prbAll.svySettleSec : 0));
}


bool SvyPrbRun::nextBank()
{
    ConfigCtl   *cfg    = mainApp()->cfgCtl();
    Run         *run    = mainApp()->getRun();
    DAQ::Params &p      = cfg->acceptedParams;

// After run starts, write the run-start (first) transition,
// in case that's the only one

    if( !irunbank ) {
        for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip )
            run->dfSetSBTT( ip, vSBTT[ip] );
    }

// All probes done?

    if( ++irunbank >= nrunbank )
        return false;

// Advance each probe

    const CimCfg::ImProbeTable  &T = cfg->prbTab;

    for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {

        CimCfg::PrbEach &E = p.im.prbj[ip];
        int             &S = vCurShnk[ip],
                        &B = vCurBank[ip];

        if( S >= E.roTbl->nSvyShank() )
            continue;   // already done

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
        E.roTbl->selectSites( P.slot, P.port, P.dock, true, true );
        E.sns.chanMapFile.clear();
        cfg->validImMaps( err, E, ip );
        run->grfUpdateProbe( ip, true, true );

        run->grfHardPause( false );

        // Record (S B t1 t2) transition times.
        // t2 = resume time + 1.5 sec margin

        t2 = run->getQ( jsIM, ip )->endCount() + qint64(1.5 * E.srate) - t0;

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


