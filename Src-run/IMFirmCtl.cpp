
#ifdef HAVE_IMEC

#include "ui_IMFirmDlg.h"

#include "IMEC/NeuropixAPI.h"

#include "IMFirmCtl.h"
#include "HelpButDialog.h"
#include "Util.h"

#include <QFileDialog>
#include <QMessageBox>




/* ---------------------------------------------------------------- */
/* Statics ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

static IMFirmCtl    *ME;

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMFirmCtl::IMFirmCtl( QObject *parent ) : QObject( parent )
{
    dlg = new HelpButDialog( "Firmware_Help" );

    firmUI = new Ui::IMFirmDlg;
    firmUI->setupUi( dlg );
    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 0 );
    ConnectUI( firmUI->detectBut, SIGNAL(clicked()), this, SLOT(detect()) );
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

void IMFirmCtl::detect()
{
    firmUI->bsLE->clear();
    firmUI->bscLE->clear();

// -------------------
// Check selected slot
// -------------------

    int         slot = firmUI->slotSB->value();
    uint32_t    occ;

    if( SUCCESS != scanPXI( &occ ) || !(occ & (1 << slot)) ) {

        QMessageBox::information( dlg,
            "Invalid Slot",
            "No BS module detected at this slot.\n"
            "Wrong slot, or firmware is corrupt." );
    }

// -------
// Connect
// -------

    NP_ErrorCode    err = openBS( slot );

    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC openBS( %1 ) error %2 '%3'.")
            .arg( slot )
            .arg( err ).arg( np_GetErrorMessage( err ) );
        return;
    }

// --
// BS
// --

    quint16 build;
    quint8  maj8, min8;

    err = getBSBootVersion( slot, &maj8, &min8, &build );

    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC getBSBootVersion(slot %1) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) );
        goto close;
    }

    firmUI->bsLE->setText(
        QString("%1.%2.%3").arg( maj8 ).arg( min8 ).arg( build ) );

// ---
// BSC
// ---

    err = getBSCBootVersion( slot, &maj8, &min8, &build );

    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC getBSCBootVersion(slot %1) error %2 '%3'.")
            .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) );
        goto close;
    }

    firmUI->bscLE->setText(
        QString("%1.%2.%3").arg( maj8 ).arg( min8 ).arg( build ) );

// -----
// Close
// -----

close:
    closeBS( slot );
}


void IMFirmCtl::bsBrowse()
{
    QString fn = QFileDialog::getOpenFileName(
                    dlg,
                    "Load NP2_BS_FPGA file",
                    appPath(),
                    QString("Bin files (*.bin)") );

    if( fn.length() )
        firmUI->bsfileLE->setText( fn );
}


void IMFirmCtl::bscBrowse()
{
    QString fn = QFileDialog::getOpenFileName(
                    dlg,
                    "Load NP2_QBSC_FPGA file",
                    appPath(),
                    QString("Bin files (*.bin)") );

    if( fn.length() )
        firmUI->bscfileLE->setText( fn );
}


void IMFirmCtl::update()
{
    ME = this;

    QString sbs, sbsc;
    size_t  bytes = 0;

// -----------------
// Size all the work
// -----------------

    if( firmUI->bsGrp->isChecked() ) {

        sbs = firmUI->bsfileLE->text();

        if( sbs.contains( "(" ) ) {
            QMessageBox::critical( dlg,
                "BS Path Not Set",
                "Use Browse button to select a BS file." );
            return;
        }

        if( !sbs.contains( "NP2_BS_FPGA" ) ) {
            QMessageBox::critical( dlg,
                "Not BS File",
                "File name should contain 'NP2_BS_FPGA'." );
            return;
        }

        QFileInfo   fi( sbs );
        bytes += fi.size();
    }

    if( firmUI->bscGrp->isChecked() ) {

        sbsc = firmUI->bscfileLE->text();

        if( sbsc.contains( "(" ) ) {
            QMessageBox::critical( dlg,
                "BSC Path Not Set",
                "Use Browse button to select a BCS file." );
            return;
        }

        if( !sbsc.contains( "NP2_QBSC_FPGA" ) ) {
            QMessageBox::critical( dlg,
                "Not BSC File",
                "File name should contain 'NP2_QBSC_FPGA'." );
            return;
        }

        QFileInfo   fi( sbsc );
        bytes += fi.size();
    }

    if( sbs.isEmpty() && sbsc.isEmpty() ) {

        firmUI->PBar->setMaximum( 1 );
        firmUI->PBar->setValue( 0 );

        QMessageBox::information( dlg,
            "No Files Selected",
            "Select a BS or BSC file (or both) and then 'Update'." );
        return;
    }

    firmUI->PBar->setMaximum( bytes );

// ----
// Slot
// ----

    int             slot = firmUI->slotSB->value();
    NP_ErrorCode    err;

// --
// BS
// --

    if( !sbs.isEmpty() ) {

        sbs.replace( "/", "\\" );

        firmUI->statusLE->setText( "Updating BS..." );

        err = bs_update( slot, STR2CHR( sbs ), callback );

        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC bs_update(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) );
            firmUI->statusLE->setText( "Error updating BS" );
            goto close;
        }

        firmUI->statusLE->setText( "BS update OK" );
    }

// ---
// BSC
// ---

    if( !sbsc.isEmpty() ) {

        sbsc.replace( "/", "\\" );

        firmUI->statusLE->setText( "Updating BSC..." );

        err = qbsc_update( slot, STR2CHR( sbsc ), callback );

        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC qbsc_update(slot %1) error %2 '%3'.")
                .arg( slot ).arg( err ).arg( np_GetErrorMessage( err ) );
            firmUI->statusLE->setText( "Error updating BSC" );
            goto close;
        }

        firmUI->statusLE->setText( "BSC update OK" );
    }

    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 1 );

    QMessageBox::information( dlg,
        "Completed",
        QString("Updating is done.%1")
        .arg( !sbs.isEmpty() ?
            "You must restart hardware and PC." :
            "You must quit and restart SpikeGLX." ) );

// -----
// Close
// -----

close:
    closeBS( slot );
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
        Q_ARG(int, bytes) );

    guiBreathe();
    return 1;
}

#endif  // HAVE_IMEC


