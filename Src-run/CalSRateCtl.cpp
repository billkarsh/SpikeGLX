
#include "ui_CalSRateDlg.h"

#include "CalSRateCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DFName.h"
#include "KVParams.h"
#include "HelpButDialog.h"

#include <QFileDialog>
#include <QMessageBox>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

CalSRateCtl::CalSRateCtl( QObject *parent )
    :   QObject(parent), thd(0)
{
    dlg = new HelpButDialog( "CalSRate_Help" );

    calUI = new Ui::CalSRateDlg;
    calUI->setupUi( dlg );
    calUI->allRB->setChecked( true );
    calUI->cancelBut->setEnabled( false );
    ConnectUI( calUI->browseBut, SIGNAL(clicked()), this, SLOT(browse()) );
    ConnectUI( calUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( calUI->applyBut, SIGNAL(clicked()), this, SLOT(apply()) );

    dlg->exec();
}


CalSRateCtl::~CalSRateCtl()
{
    if( thd ) {

        QMetaObject::invokeMethod(
            thd->worker, "cancel",
            Qt::DirectConnection );

        delete thd;
        thd = 0;
    }

    if( calUI ) {
        delete calUI;
        calUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CalSRateCtl::percent( int pct )
{
    write( QString("done %1%").arg( pct ) );
}


void CalSRateCtl::finished()
{
    if( thd ) {
        delete thd;
        thd = 0;
    }

    QString msg;
    CalSRRun::fmtResults( msg, vIM, vOB, vNI );
    msg += "\nNote: Apply pertains to all numeric measurements.";

    calUI->outTE->clear();
    write( msg );

    calUI->specifyGB->setEnabled( true );
    calUI->cancelBut->setEnabled( false );
    calUI->applyGB->setEnabled( true );
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CalSRateCtl::browse()
{
    QString f = QFileDialog::getOpenFileName(
                    dlg,
                    "Select binary output file",
                    mainApp()->dataDir(),
                    "BIN Files (*.ap.bin *.obx.bin *.nidq.bin)" );

    if( f.isEmpty() )
        return;

    calUI->fileLE->setText( f );
}


void CalSRateCtl::go()
{
    QString f;

    vIM.clear();
    vOB.clear();
    vNI.clear();
    calUI->outTE->clear();

// --------------------
// Verify selected file
// --------------------

    if( !verifySelection( f ) )
        return;

// ----------------
// Set up job lists
// ----------------

    runTag = DFRunTag( f );

    if( calUI->oneRB->isChecked() )
        setJobsOne( f );
    else
        setJobsAll( f );

// -------
// Process
// -------

    calUI->specifyGB->setEnabled( false );
    calUI->cancelBut->setEnabled( true );
    calUI->applyGB->setEnabled( false );

    thd = new CalSRThread( runTag, vIM, vOB, vNI );
    ConnectUI( thd->worker, SIGNAL(percent(int)), this, SLOT(percent(int)) );
    ConnectUI( thd->worker, SIGNAL(finished()), this, SLOT(finished()) );
    Connect( calUI->cancelBut, SIGNAL(clicked()), thd->worker, SLOT(cancel()), Qt::DirectConnection );

    thd->startRun();
}


void CalSRateCtl::apply()
{
// -----------------
// Anything checked?
// -----------------

    bool    isGChk = calUI->globalChk->isChecked(),
            isFChk = calUI->fileChk->isChecked(),
            isRslt = false;

    if( !(isGChk || isFChk) ) {

        QMessageBox::information(
            dlg,
            "No Update Targets",
            "Use checkboxes to specify how results are applied." );
        return;
    }

// ----
// Imec
// ----

    DAQ::Params             p;
    ConfigCtl               *cfg    = mainApp()->cfgCtl();
    CimCfg::ImProbeTable    &prbTab = cfg->prbTab;

    p.loadSettings();
    prbTab.loadProbeSRates();
    prbTab.loadOneBoxSRates();

    for( int is = 0, ns = vIM.size(); is < ns; ++is ) {

        const CalSRStream   &S = vIM[is];

        if( S.av > 0 ) {

            QString     name = runTag.filename( 0, S.ip, "ap.meta" );
            KVParams    kvp;
            quint64     hssn;

            if( !kvp.fromMetaFile( name ) )
                continue;

            hssn = kvp["imDatHs_sn"].toULongLong();

            if( isGChk ) {

                // Update working srate
                //
                // Update the current PrbEach records that
                // correspond to this hssn.

                for( int ip = 0, np = prbTab.nSelProbes(); ip < np; ++ip ) {

                    if( prbTab.get_iProbe( ip ).hssn == hssn )
                        p.im.prbj[ip].srate = S.av;
                }

                // Update database
                prbTab.set_hssn_SRate( hssn, S.av );
            }

            if( isFChk ) {

                // AP

                kvp["imSampRate"] = S.av;

                kvp["fileTimeSecs"] =
                    kvp["fileSizeBytes"].toLongLong()
                    / (2 * kvp["nSavedChans"].toInt())
                    / S.av;

                kvp.toMetaFile( name );

                // LFP ?

                QString pn( "Probe3A" );
                if( kvp.contains( "imDatPrb_pn" ) )
                    pn = kvp["imDatPrb_pn"].toString();

                IMROTbl *R = IMROTbl::alloc( pn );

                if( R->nLF() ) {

                    name = runTag.filename( 1, S.ip, "lf.meta" );

                    if( kvp.fromMetaFile( name ) ) {

                        kvp["imSampRate"] = S.av / 12.0;

                        kvp["fileTimeSecs"] =
                            kvp["fileSizeBytes"].toLongLong()
                            / kvp["nSavedChans"].toInt()
                            * 6
                            / S.av;

                        kvp.toMetaFile( name );
                    }
                }

                delete R;
            }

            isRslt = true;
        }
    }

// ---
// Obx
// ---

    for( int is = 0, ns = vOB.size(); is < ns; ++is ) {

        const CalSRStream   &S = vOB[is];

        if( S.av > 0 ) {

            QString     name = runTag.filename( 2, S.ip, "meta" );
            KVParams    kvp;
            int         obsn;

            if( !kvp.fromMetaFile( name ) )
                continue;

            obsn = kvp["imDatBsc_sn"].toInt();

            if( isGChk ) {

                // Update working srate
                //
                // Update the current ObxEach records that
                // correspond to this obsn.

                for( int ip = 0, np = prbTab.nSelOneBox(); ip < np; ++ip ) {

                    if( prbTab.get_iOneBox( ip ).obsn == obsn )
                        p.im.mod_iSelOneBox( ip ).srate = S.av;
                }

                // Update database
                prbTab.set_obsn_SRate( obsn, S.av );
            }

            if( isFChk ) {

                kvp["obSampRate"] = S.av;

                kvp["fileTimeSecs"] =
                    kvp["fileSizeBytes"].toLongLong()
                    / (2 * kvp["nSavedChans"].toInt())
                    / S.av;

                kvp.toMetaFile( name );
            }

            isRslt = true;
        }
    }

// ----
// Nidq
// ----

    if( vNI.size() ) {

        const CalSRStream   &S = vNI[0];

        if( S.av > 0 ) {

            QString     name = runTag.filename( 3, -1, "meta" );
            KVParams    kvp;

            kvp.fromMetaFile( name );

            if( isGChk ) {
                p.ni.srate = S.av;
                p.ni.setSRate( kvp["niClockSource"].toString(), S.av );
                p.ni.saveSRateTable();
            }

            if( isFChk ) {

                kvp["niSampRate"] = S.av;

                kvp["fileTimeSecs"] =
                    kvp["fileSizeBytes"].toLongLong()
                    / (2 * kvp["nSavedChans"].toInt())
                    / S.av;

                kvp.toMetaFile( name );
            }

            isRslt = true;
        }
    }

// -------------
// Apply globals
// -------------

    if( !isRslt ) {

        QMessageBox::information(
            dlg,
            "No Valid Results",
            "Nothing done: No new results were calculated." );
    }
    else if( isGChk ) {
        cfg->setParams( p, true );
        prbTab.saveProbeSRates();
        prbTab.saveOneBoxSRates();
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CalSRateCtl::write( const QString &s )
{
    QTextEdit   *te = calUI->outTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine ); // H-scrollbar to zero
}


// Sync keys "syncSourcePeriod" and others added in version 20170903.
//
bool CalSRateCtl::verifySelection( QString &f )
{
    QString err;
    bool    ok;

    f = calUI->fileLE->text().trimmed();

    ok = DFName::isValidInputFile( err, f, {"syncSourcePeriod"} );

    if( !ok ) {
        write( "Selected file error:" );
        write( err );
    }

    return ok;
}


void CalSRateCtl::setJobsOne( QString &f )
{
    int ip, fType = DFName::typeAndIP( ip, f, 0 );

    switch( fType ) {
        case 0:  vIM.push_back( CalSRStream( jsIM, ip ) ); break;
//      case 1:  // dialog filter excludes
        case 2:  vOB.push_back( CalSRStream( jsOB, ip ) ); break;
        default: vNI.push_back( CalSRStream( jsNI, -1 ) );
    }
}


void CalSRateCtl::setJobsAll( QString &f )
{
    KVParams    kvp;

    kvp.fromMetaFile( DFName::forceMetaSuffix( f ) );

    KVParams::const_iterator    it = kvp.find( "typeImEnabled" );

    if( it != kvp.end() ) {

        for( int ip = 0, np = it.value().toInt(); ip < np; ++ip )
            vIM.push_back( CalSRStream( jsIM, ip ) );

        it = kvp.find( "typeObEnabled" );

        if( it != kvp.end() && it.value().toInt() > 0 ) {
            for( int ip = 0, np = it.value().toInt(); ip < np; ++ip )
                vOB.push_back( CalSRStream( jsOB, ip ) );
        }

        if( kvp["typeNiEnabled"].toInt() > 0 )
            vNI.push_back( CalSRStream( jsNI, -1 ) );
    }
    else {

        // Deprecated type encoding

        QString sTypes =  kvp["typeEnabled"].toString();

        if( sTypes.contains( "imec" ) )
            vIM.push_back( CalSRStream( jsIM, 0 ) );

        if( sTypes.contains( "nidq" ) )
            vNI.push_back( CalSRStream( jsNI, -1 ) );
    }
}


