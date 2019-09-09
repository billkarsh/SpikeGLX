
#include "ui_IMROEditor_T21.h"

#include "IMROTbl_T21.h"
#include "IMROEditor_T21.h"
#include "Util.h"
#include "CimCfg.h"

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMROEditor_T21::IMROEditor_T21( QObject *parent )
    :   QObject( parent ), R0(0), R(0), type(21),
        running(false)
{
    loadSettings();

    edDlg = new QDialog;

    edDlg->setWindowFlags( edDlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    edUI = new Ui::IMROEditor_T21;
    edUI->setupUi( edDlg );
    ConnectUI( edUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( edUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    edUI->prbLbl->setText( QString::number( type ) );
}


IMROEditor_T21::~IMROEditor_T21()
{
    saveSettings();

    if( edUI ) {
        delete edUI;
        edUI = 0;
    }

    if( edDlg ) {
        delete edDlg;
        edDlg = 0;
    }

    if( R0 ) {
        delete R0;
        R0 = 0;
    }

    if( R ) {
        delete R;
        R = 0;
    }
}


// Return true if changed.
//
bool IMROEditor_T21::Edit( QString &outFile, const QString &file, int selectRow )
{
    Q_UNUSED( selectRow )

    inFile = file;

    if( inFile.contains( "*" ) )
        inFile.clear();

    if( inFile.isEmpty() )
        defaultBut();
    else
        loadFile( inFile );

// Run the dialog

    int ret = edDlg->exec();

    outFile = R0File;

    return ret == QDialog::Accepted;
}


void IMROEditor_T21::defaultBut()
{
    createR();
    R->fillDefault();

    copyR2R0();
    R0File.clear();

    edUI->statusLbl->setText( "Default table set" );
}


void IMROEditor_T21::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    edDlg,
                    "Load IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)" );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void IMROEditor_T21::okBut()
{
    edDlg->accept();
}


void IMROEditor_T21::cancelBut()
{
    R0File = inFile;
    edDlg->reject();
}


void IMROEditor_T21::createR()
{
    if( R )
        delete R;

    R = new IMROTbl_T21;
}


void IMROEditor_T21::copyR2R0()
{
    if( !R0 )
        R0 = new IMROTbl_T21;

    R0->copyFrom( R );
}


// Called only from ctor.
//
void IMROEditor_T21::loadSettings()
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void IMROEditor_T21::saveSettings() const
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    settings.setValue( "lastDlgDir", lastDir );
}


void IMROEditor_T21::loadFile( const QString &file )
{
    createR();

    QString msg;
    bool    ok = R->loadFile( msg, file );

    edUI->statusLbl->setText( msg );

    if( ok ) {

        if( R->type == type ) {

            copyR2R0();
            R0File = file;
        }
        else {
            edUI->statusLbl->setText(
                QString("Can't use type %1 file for this probe.")
                .arg( R->type ) );
        }
    }
}


