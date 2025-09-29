
#ifdef HAVE_IMEC

#include "ui_IMFirmDlg.h"

#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;

#include "IMFirmCtl.h"
#include "IMROTbl.h"
#include "Util.h"

#include <QDirIterator>
#include <QFileDialog>
#include <QMessageBox>
#include <QThread>

#define BYFILE


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
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
    dlg = new QDialog();

    dlg->setWindowFlags( dlg->windowFlags() & ~Qt::WindowCloseButtonHint );

    firmUI = new Ui::IMFirmDlg;
    firmUI->setupUi( dlg );

    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 0 );

    QPalette palette;
    palette = firmUI->PBar->palette();
    palette.setBrush( QPalette::Highlight, Qt::darkGray );
    firmUI->PBar->setPalette( palette );

    ConnectUI( firmUI->slotCB, SIGNAL(currentIndexChanged(int)), this, SLOT(slotChanged()) );
    ConnectUI( firmUI->updateBut, SIGNAL(clicked()), this, SLOT(update()) );
    ConnectUI( firmUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    slotChanged();
    dlg->resize( dlg->minimumSizeHint() );
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

void IMFirmCtl::slotChanged()
{
    int slot = 1 + firmUI->slotCB->currentIndex();

    if( slot >= 2 )
        if( verGet( slot ) )
            verEval();
        else {
            write( "Basestation error..." );
            write( "" );
            write( "Check message in status box below." );
            write( "Check extended info in Log/Console window." );
        }
    else {
        verInit( "" );
        write( "Select slot above." );
    }
}


void IMFirmCtl::update()
{
    static bool inUpdate = false;

    if( inUpdate )
        return;

    inUpdate = true;
    firmUI->updateBut->setEnabled( false );
    firmUI->buttonBox->setEnabled( false );
    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 0 );

    ME = this;

    QString         sbs, sbsc;
    NP_ErrorCode    err;

// ----
// Slot
// ----

    int slot = 1 + firmUI->slotCB->currentIndex();

    if( slot < 2 ) {

        QMessageBox::information( dlg,
            "No Slot Selected",
            "Select a base station module." );
        goto exit;
    }
    else if( tech == t_tech_sim ) {

        QMessageBox::information( dlg,
            "Unidentified Module",
            "No module found at this slot." );
        goto exit;
    }
    else if( !jobBits ) {

        QMessageBox::information( dlg,
            "No Action Needed",
            "Your module is already up to date." );
        goto exit;
    }

    firmUI->statusLbl->setText( "" );

// -------
// By file
// -------

#ifdef BYFILE
    if( !paths( sbs, sbsc ) )
        goto exit;
#endif

// -----------------
// Size all the work
// -----------------

    bsBytes     = 0;
    bscBytes    = 0;
    barOffset   = 0;

// -------
// BS prep
// -------

    if( jobBits & 1 ) {
#ifdef BYFILE
        bsBytes = QFileInfo( sbs ).size();
#else
        size_t bytes;
        err = np_bs_getFirmwareSize( slot, &bytes );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC np_bs_getFirmwareSize(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BS" );
            goto exit;
        }
        bsBytes = bytes;
#endif
    }

// --------
// BSC prep
// --------

    if( jobBits & 2 ) {
#ifdef BYFILE
        bscBytes = QFileInfo( sbsc ).size();
#else
        size_t bytes;
        err = np_bsc_getFirmwareSize( slot, &bytes );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC np_bsc_getFirmwareSize(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BSC" );
            goto exit;
        }
        bscBytes = bytes;
#endif
    }

    firmUI->PBar->setMaximum( bsBytes + bscBytes );

// ---------
// BS update
// ---------

    if( jobBits & 1 ) {

        firmUI->statusLbl->setText( "Updating BS..." );

#ifdef BYFILE
        sbs.replace( "/", "\\" );
        err = np_bs_updateFirmware( slot, STR2CHR( sbs ), callback );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC bs_updateFirmware(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BS" );
            goto close;
        }
#else
        err = np_bs_resetFirmware( slot, callback );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC bs_resetFirmware(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BS" );
            goto close;
        }
#endif

        firmUI->statusLbl->setText( "BS update OK" );
        barOffset = bsBytes;
    }

// ----------
// BSC update
// ----------

    if( jobBits & 2 ) {

        firmUI->statusLbl->setText( "Updating BSC..." );

#ifdef BYFILE
        sbsc.replace( "/", "\\" );
        err = np_bsc_updateFirmware( slot, STR2CHR( sbsc ), callback );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC bsc_updateFirmware(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BSC" );
            goto close;
        }
#else
        err = np_bsc_resetFirmware( slot, callback );
        if( err != SUCCESS ) {
            Error() <<
                QString("IMEC bsc_resetFirmware(slot %1)%2")
                .arg( slot ).arg( makeErrorString( err ) );
            beep( "Error updating BSC" );
            goto close;
        }
#endif

        firmUI->statusLbl->setText( "BSC update OK" );
    }

// ------
// Report
// ------

    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 1 );

    QMessageBox::information( dlg,
        "Completed",
        QString("Update is done.\n%1")
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


void IMFirmCtl::helpBut()
{
    showHelp( "Firmware_Help" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMFirmCtl::beep( const QString &msg )
{
    firmUI->statusLbl->setText( msg );

    if( !msg.isEmpty() )
        ::Beep( 440, 200 );
}


void IMFirmCtl::write( const QString &s )
{
    QTextEdit   *te = firmUI->actionTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine ); // H-scrollbar to zero
}


void IMFirmCtl::verInit( const QString &s )
{
    tech    = t_tech_sim;
    jobBits = 0;
    firmUI->techLE->setText( s );
    firmUI->bscurLE->setText( s );
    firmUI->bsreqLE->setText( s );
    firmUI->bsccurLE->setText( s );
    firmUI->bscreqLE->setText( s );
    firmUI->actionTE->clear();
    firmUI->PBar->setMaximum( 1 );
    firmUI->PBar->setValue( 0 );
    firmUI->statusLbl->setText( "" );
}


bool IMFirmCtl::verGet( int slot )
{
    firmware_Info   info;
    HardwareID      hID;
    QString         bs, bsc;
    NP_ErrorCode    err;

    verInit( "???" );

    err = np_getBSCHardwareID( slot, &hID );
    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC getBSCHardwareID(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) );
        beep( "Error identifying BSC" );
        return false;
    }
    tech = IMROTbl::bscpnToTech( hID.ProductNumber );
    firmUI->techLE->setText( IMROTbl::strTech( tech ) );

    QString bsreq,
            bscreq;
    IMROTbl::bscReqVers( bsreq, bscreq, tech );
    firmUI->bsreqLE->setText( bsreq );
    firmUI->bscreqLE->setText( bscreq );

    err = np_bs_getFirmwareInfo( slot, &info );
    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC bs_getFirmwareInfo(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) );
        beep( "Error reading BS version" );
        return false;
    }
    bs = QString("%1.%2.%3")
            .arg( info.major ).arg( info.minor ).arg( info.build );
    firmUI->bscurLE->setText( bs );

    err = np_bsc_getFirmwareInfo( slot, &info );
    if( err != SUCCESS ) {
        Error() <<
            QString("IMEC bsc_getFirmwareInfo(slot %1)%2")
            .arg( slot ).arg( makeErrorString( err ) );
        beep( "Error reading BSC version" );
        return false;
    }
    bsc = QString("%1.%2.%3")
            .arg( info.major ).arg( info.minor ).arg( info.build );
    firmUI->bsccurLE->setText( bsc );

    return true;
}


void IMFirmCtl::verEval()
{
    if( firmUI->bscurLE->text() != firmUI->bsreqLE->text() )
        jobBits += 1;
    if( firmUI->bsccurLE->text() != firmUI->bscreqLE->text() )
        jobBits += 2;

    switch( jobBits ) {
        case 0:
            write( "None: BS and BSC are up to date." ); return;
        case 1:
            write( "BS needs to be updated..." ); break;
        case 2:
            write( "BSC needs to be updated..." ); break;
        case 3:
            write( "BS and BSC both need to be updated..." ); break;
    }

    write( "" );
    write( "(1) Click 'Update'." );

#ifdef BYFILE
    write( "(2) Select folder 'SGLX_Firmware' from your download." );
    write( "(3) The update(s) are performed automatically." );
    write( "----  Check status box and Log window for any messages." );
    write( "(4) Follow post-update instructions." );
#else
    write( "(2) The update(s) are performed automatically." );
    write( "----  Check status box and Log window for any messages." );
    write( "(3) Follow post-update instructions." );
#endif
}


bool IMFirmCtl::paths( QString &bs, QString &bsc )
{
    QDir    targetdir( appPath() );
    targetdir.cdUp();

    QString path =
        QFileDialog::getExistingDirectory(
            dlg,
            "Select SGLX_Firmware Directory",
            targetdir.absolutePath(),
            QFileDialog::DontResolveSymlinks
            | QFileDialog::ShowDirsOnly );

    if( path.isEmpty() ) {
        firmUI->statusLbl->setText( "Cancelled" );
        return false;
    }

    if( QDir( path ).dirName() != "SGLX_Firmware" ) {
        beep( "Select 'SGLX_Firmware' in your SpikeGLX download" );
        return false;
    }

    if( !path.endsWith( "/" )  )
        path += "/";

    switch( tech ) {
        case t_tech_opto: path += "NP_PXI_OPTO_Firmware/"; break;
        case t_tech_nxt:  path += "NP_PXI_NXT_Firmware/"; break;
        default:          path += "NP_PXI_STD_Firmware/"; break;
    }

// Verify BS, BSC both exist

    QString _bs,
            _bsc,
            bsBld  = "_B" + verToBuild( firmUI->bsreqLE->text() ) + ".",
            bscBld = "_B" + verToBuild( firmUI->bscreqLE->text() ) + ".";

    QDirIterator    it( path );

    while( it.hasNext() ) {

        if( !_bs.isEmpty() && !_bsc.isEmpty() )
            break;

        it.next();

        QFileInfo   fi      = it.fileInfo();
        QString     entry   = fi.fileName();

        if( !fi.isDir() ) {
            if( entry.contains( "BS_" ) && entry.contains( bsBld ) )
                _bs = entry;
            else if( entry.contains( "QBSC_" ) && entry.contains( bscBld ) )
                _bsc = entry;
        }
    }

    if( _bs.isEmpty() || _bsc.isEmpty() ) {
        beep( "Selected folder has wrong version" );
        return false;
    }

    if( jobBits & 1 )
        bs = path + _bs;

    if( jobBits & 2 )
        bsc = path + _bsc;

    return true;
}


// Return "c" from "a.b.c"
//
QString IMFirmCtl::verToBuild( const QString &vers )
{
    int ic = vers.lastIndexOf( '.' ) + 1;
    return vers.last( vers.size() - ic );
}


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


