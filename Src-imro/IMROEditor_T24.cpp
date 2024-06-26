
#include "ui_IMROEditor_T24.h"

#include "IMROTbl_T24.h"
#include "IMROEditor_T24.h"
#include "Util.h"

#include <QFileDialog>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMROEditor_T24::IMROEditor_T24( QObject *parent, const QString &pn )
    :   QObject(parent), Rini(0), Rcur(0), pn(pn), running(false)
{
    Rref = new IMROTbl_T24( pn );

    loadSettings();

    edDlg = new QDialog;

    edDlg->setWindowFlags( edDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    edUI = new Ui::IMROEditor_T24;
    edUI->setupUi( edDlg );
    ConnectUI( edUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( edUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    edUI->prbLbl->setText( pn );
}


IMROEditor_T24::~IMROEditor_T24()
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

    if( Rini ) {
        delete Rini;
        Rini = 0;
    }

    if( Rref ) {
        delete Rref;
        Rref = 0;
    }

    if( Rcur ) {
        delete Rcur;
        Rcur = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if changed.
//
bool IMROEditor_T24::Edit( QString &outFile, const QString &file, int selectRow )
{
    Q_UNUSED( selectRow )

    iniFile = file;

    if( iniFile.contains( "*" ) )
        iniFile.clear();

    if( iniFile.isEmpty() )
        defaultBut();
    else
        loadFile( iniFile );

    if( !Rini )
        Rini = new IMROTbl_T24( pn );

    Rini->copyFrom( Rref );

// Run the dialog

    int ret = edDlg->exec();

    outFile = refFile;

    return ret == (QDialog::Accepted && Rcur != Rini);
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROEditor_T24::defaultBut()
{
    createRcur();
    Rcur->fillDefault();

    copyRcur2ref();
    refFile.clear();

    edUI->statusLbl->setText( "Default table set" );
}


void IMROEditor_T24::loadBut()
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


void IMROEditor_T24::okBut()
{
    edDlg->accept();
}


void IMROEditor_T24::cancelBut()
{
    refFile = iniFile;
    edDlg->reject();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROEditor_T24::createRcur()
{
    if( Rcur )
        delete Rcur;

    Rcur = new IMROTbl_T24( pn );
}


void IMROEditor_T24::copyRcur2ref()
{
    if( !Rref )
        Rref = new IMROTbl_T24( pn );

    Rref->copyFrom( Rcur );
}


// Called only from ctor.
//
void IMROEditor_T24::loadSettings()
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void IMROEditor_T24::saveSettings() const
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    settings.setValue( "lastDlgDir", lastDir );
}


void IMROEditor_T24::loadFile( const QString &file )
{
    createRcur();

    QString msg;
    bool    ok = Rcur->loadFile( msg, file );

    edUI->statusLbl->setText( msg );

    if( ok ) {
        copyRcur2ref();
        refFile = file;
    }
    else {
        defaultBut();
        edUI->statusLbl->setText(
            QString("Setting defaults: %1.").arg( msg ) );
    }
}


