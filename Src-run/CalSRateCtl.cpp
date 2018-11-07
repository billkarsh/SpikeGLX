
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
    :   QObject( parent ), thd(0)
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

    calUI->outTE->clear();

    write( "These are the old rates and the new measurement results:" );
    write( "(A text message indicates an unsuccessful measurement)\n" );

    for( int is = 0, ns = vIM.size(); is < ns; ++is ) {

        const CalSRStream   &S = vIM[is];

        if( !S.err.isEmpty() ) {
            write( QString(
            "    IM%1  %2  :  %3" )
            .arg( S.ip )
            .arg( S.srate, 0, 'f', 6 )
            .arg( S.err ) );
        }
        else if( S.av == 0 ) {
            write( QString(
            "    IM%1  %2  :  canceled" )
            .arg( S.ip )
            .arg( S.srate, 0, 'f', 6 ) );
        }
        else {
            write( QString(
            "    IM%1  %2  :  %3 +/- %4" )
            .arg( S.ip )
            .arg( S.srate, 0, 'f', 6 )
            .arg( S.av, 0, 'f', 6 )
            .arg( S.se, 0, 'f', 6 ) );
        }
    }

    if( vNI.size() ) {

        CalSRStream &S = vNI[0];

        if( !S.err.isEmpty() ) {
            write( QString(
            "    NI  %1  :  %2" )
            .arg( S.srate, 0, 'f', 6 )
            .arg( S.err ) );
        }
        else if( S.av == 0 ) {
            write( QString(
            "    NI  %1  :  canceled" )
            .arg( S.srate, 0, 'f', 6 ) );
        }
        else {
            write( QString(
            "    NI  %1  :  %2 +/- %3" )
            .arg( S.srate, 0, 'f', 6 )
            .arg( S.av, 0, 'f', 6 )
            .arg( S.se, 0, 'f', 6 ) );
        }
    }

    write( "\nUnsuccessful measurements will not be applied." );

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
                    mainApp()->runDir(),
                    "BIN Files (*.ap.bin *.nidq.bin)" );

    if( f.isEmpty() )
        return;

    calUI->fileLE->setText( f );
}


void CalSRateCtl::go()
{
    QString f;

    baseName.clear();
    vIM.clear();
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

    baseName = DFName::chopType( f );

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

    thd = new CalSRThread( baseName, vIM, vNI );
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

    DAQ::Params p;

    p.loadSettings();

    for( int is = 0, ns = vIM.size(); is < ns; ++is ) {

        const CalSRStream   &S = vIM[is];

        if( S.av > 0 ) {

            if( isGChk )
                p.im.each[S.ip].srate = S.av;

            if( isFChk ) {

                QString     name =
                                QString("%1.imec%2.ap.meta")
                                .arg( baseName )
                                .arg( S.ip );
                KVParams    kvp;

                kvp.fromMetaFile( name );
                kvp["imSampRate"] = S.av;
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

            if( isGChk )
                p.ni.srate = S.av;

            if( isFChk ) {

                QString     name = baseName + ".nidq.meta";
                KVParams    kvp;

                kvp.fromMetaFile( name );
                kvp["niSampRate"] = S.av;
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
    else if( isGChk )
        mainApp()->cfgCtl()->setParams( p, true );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CalSRateCtl::write( const QString &s )
{
    QTextEdit   *te = calUI->outTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
}


bool CalSRateCtl::verifySelection( QString &f )
{
    QString err;
    bool    ok;

    f = calUI->fileLE->text().trimmed();

    ok = DFName::isValidInputFile( f, &err, "20170903" );

    if( !ok ) {
        write( "Selected file error:" );
        write( err );
    }

    return ok;
}


void CalSRateCtl::setJobsOne( QString &f )
{
    int ip, type = DFName::typeAndIP( ip, f, 0 );

    (type == 0 ? vIM : vNI).push_back( CalSRStream( ip ) );
}


void CalSRateCtl::setJobsAll( QString &f )
{
    KVParams    kvp;
    int         np;

    kvp.fromMetaFile( DFName::forceMetaSuffix( f ) );

    np = kvp["typeImEnabled"].toInt();

    for( int ip = 0; ip < np; ++ip )
        vIM.push_back( CalSRStream( ip ) );

    if( kvp["typeNiEnabled"].toInt() > 1 )
        vNI.push_back( CalSRStream( -1 ) );
}


