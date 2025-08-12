
#ifdef HAVE_IMEC

#include "ui_IMFirmDlg.h"

#include "IMEC/NeuropixAPI.h"

#include "IMFirmCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "Version.h"

#include <QButtonGroup>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

using namespace Neuropixels;


/* ---------------------------------------------------------------- */
/* Statics ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

static IMFirmCtl    *ME;


static QString makeErrorString( NP_ErrorCode err )
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMFirmCtl::IMFirmCtl( QObject *parent ) : QObject(parent)
{
    dlg = new HelpButDialog( "Firmware_Help" );

    dlg->setWindowFlags( dlg->windowFlags() & ~Qt::WindowCloseButtonHint );

    firmUI = new Ui::IMFirmDlg;
    firmUI->setupUi( dlg );

    QButtonGroup    *bg;

    bg = new QButtonGroup( this );
    bg->addButton( firmUI->bsIntRadio );
    bg->addButton( firmUI->bsExtRadio );
    firmUI->bsIntRadio->setChecked( true );
    bsRadClicked();

    bg = new QButtonGroup( this );
    bg->addButton( firmUI->bscIntRadio );
    bg->addButton( firmUI->bscExtRadio );
    firmUI->bscIntRadio->setChecked( true );
    bscRadClicked();

    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 0 );

    ConnectUI( firmUI->bsIntRadio, SIGNAL(clicked()), this, SLOT(bsRadClicked()) );
    ConnectUI( firmUI->bsExtRadio, SIGNAL(clicked()), this, SLOT(bsRadClicked()) );
    ConnectUI( firmUI->bscIntRadio, SIGNAL(clicked()), this, SLOT(bscRadClicked()) );
    ConnectUI( firmUI->bscExtRadio, SIGNAL(clicked()), this, SLOT(bscRadClicked()) );

    ConnectUI( firmUI->bsBrowse, SIGNAL(clicked()), this, SLOT(bsBrowse()) );
    ConnectUI( firmUI->bscBrowse, SIGNAL(clicked()), this, SLOT(bscBrowse()) );
    ConnectUI( firmUI->updateBut, SIGNAL(clicked()), this, SLOT(update()) );

    dlg->exec();
}


IMFirmCtl::~IMFirmCtl()
{
    if( firmUI ) {
        delete firmUI;
        firmUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMFirmCtl::bsRadClicked()
{
    bool enab = firmUI->bsExtRadio->isChecked();
    firmUI->bsfileLE->setEnabled( enab );
    firmUI->bsBrowse->setEnabled( enab );
}


void IMFirmCtl::bscRadClicked()
{
    bool enab = firmUI->bscExtRadio->isChecked();
    firmUI->bscfileLE->setEnabled( enab );
    firmUI->bscBrowse->setEnabled( enab );
}


void IMFirmCtl::bsBrowse()
{
    QString fn = QFileDialog::getOpenFileName(
                    dlg,
                    "Load BS FPGA file",
                    appPath(),
                    QString("Bin files (*.bin)") );

    if( fn.length() )
        firmUI->bsfileLE->setText( fn );
}


void IMFirmCtl::bscBrowse()
{
    QString fn = QFileDialog::getOpenFileName(
                    dlg,
                    "Load QBSC FPGA file",
                    appPath(),
                    QString("Bin files (*.bin)") );

    if( fn.length() )
        firmUI->bscfileLE->setText( fn );
}


void IMFirmCtl::update()
{
    static bool inUpdate = false;

    if( inUpdate )
        return;

    inUpdate = true;
    firmUI->updateBut->setEnabled( false );
    firmUI->buttonBox->setEnabled( false );

    ME = this;

    QString         sbs, sbsc;
    NP_ErrorCode    err;

// ----
// Slot
// ----

    int slot = 1 + firmUI->slotCB->currentIndex();

    if( slot < 2 ) {

        firmUI->PBar->setMaximum( 1 );
        firmUI->PBar->setValue( 0 );

        QMessageBox::information( dlg,
            "No Slot Selected",
            "Select a base station module and then 'Update'." );
        goto exit;
    }

// -----------------
// Size all the work
// -----------------

    bsBytes     = 0;
    bscBytes    = 0;
    barOffset   = 0;

// -------
// BS prep
// -------

    if( firmUI->bsGrp->isChecked() ) {

        if( firmUI->bsExtRadio->isChecked() ) {

            sbs = firmUI->bsfileLE->text();

            if( sbs.contains( "(" ) ) {
                QMessageBox::critical( dlg,
                    "BS Path Not Set",
                    "Use Browse button to select a BS file." );
                goto exit;
            }

            QFileInfo   fi( sbs );

            if( !fi.fileName().contains( "BS_" ) ) {
                QMessageBox::critical( dlg,
                    "Not BS File",
                    "File name should contain 'BS_ and _FPGA_'." );
                goto exit;
            }

            bsBytes = fi.size();
        }
        else {
            sbs     = "x";
            bsBytes = VERS_IMEC_BS_BYTES;
        }
    }

// --------
// BSC prep
// --------

    if( firmUI->bscGrp->isChecked() ) {

        if( firmUI->bsExtRadio->isChecked() ) {

            sbsc = firmUI->bscfileLE->text();

            if( sbsc.contains( "(" ) ) {
                QMessageBox::critical( dlg,
                    "BSC Path Not Set",
                    "Use Browse button to select a BCS file." );
                goto exit;
            }

            QFileInfo   fi( sbsc );

            if( !fi.fileName().contains( "QBSC_" ) ) {
                QMessageBox::critical( dlg,
                    "Not BSC File",
                    "File name should contain 'QBSC_ and _FPGA_'." );
                goto exit;
            }

            bscBytes = fi.size();
        }
        else {
            sbsc        = "x";
            bscBytes    = VERS_IMEC_BSC_BYTES;
        }
    }

    if( sbs.isEmpty() && sbsc.isEmpty() ) {

        firmUI->PBar->setMaximum( 1 );
        firmUI->PBar->setValue( 0 );

        QMessageBox::information( dlg,
            "No Source Data Selected",
            "Select BS and/or BSC sources and then 'Update'." );
        goto exit;
    }

    firmUI->PBar->setMaximum( bsBytes + bscBytes );

// ---------
// BS update
// ---------

    if( !sbs.isEmpty() ) {

        firmUI->statusLE->setText( "Updating BS..." );

        if( firmUI->bsExtRadio->isChecked() ) {

            sbs.replace( "/", "\\" );
            err = np_bs_updateFirmware( slot, STR2CHR( sbs ), callback );
            if( err != SUCCESS ) {
                Error() <<
                    QString("IMEC bs_updateFirmware(slot %1)%2")
                    .arg( slot ).arg( makeErrorString( err ) );
                firmUI->statusLE->setText( "Error updating BS" );
                goto close;
            }
        }
        else {
            err = np_bs_resetFirmware( slot, callback );
            if( err != SUCCESS ) {
                Error() <<
                    QString("IMEC bs_resetFirmware(slot %1)%2")
                    .arg( slot ).arg( makeErrorString( err ) );
                firmUI->statusLE->setText( "Error updating BS" );
                goto close;
            }
        }

        firmUI->statusLE->setText( "BS update OK" );

        barOffset = bsBytes;
    }

// ----------
// BSC update
// ----------

    if( !sbsc.isEmpty() ) {

        firmUI->statusLE->setText( "Updating BSC..." );

        if( firmUI->bscExtRadio->isChecked() ) {

            sbsc.replace( "/", "\\" );
            err = np_bsc_updateFirmware( slot, STR2CHR( sbsc ), callback );
            if( err != SUCCESS ) {
                Error() <<
                    QString("IMEC bsc_updateFirmware(slot %1)%2")
                    .arg( slot ).arg( makeErrorString( err ) );
                firmUI->statusLE->setText( "Error updating BSC" );
                goto close;
            }
        }
        else {
            err = np_bsc_resetFirmware( slot, callback );
            if( err != SUCCESS ) {
                Error() <<
                    QString("IMEC bsc_resetFirmware(slot %1)%2")
                    .arg( slot ).arg( makeErrorString( err ) );
                firmUI->statusLE->setText( "Error updating BSC" );
                goto close;
            }
        }

        firmUI->statusLE->setText( "BSC update OK" );
    }

// ------
// Report
// ------

    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 1 );

    QMessageBox::information( dlg,
        "Completed",
        QString("Updating is done. %1")
        .arg( !sbs.isEmpty() ?
            "You must power cycle chassis and restart SpikeGLX." :
            "You must quit and restart SpikeGLX." ) );

// -----
// Close
// -----

close:
    np_closeBS( slot );

exit:
    firmUI->updateBut->setEnabled( true );
    firmUI->buttonBox->setEnabled( true );
    inUpdate = false;
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int IMFirmCtl::callback( size_t bytes )
{
    QMetaObject::invokeMethod(
        ME->firmUI->PBar,
        "setValue",
        Qt::QueuedConnection,
        Q_ARG(int, ME->barOffset + bytes) );

    QThread::usleep( 1500 );
    guiBreathe();
    return 1;
}

#endif  // HAVE_IMEC


