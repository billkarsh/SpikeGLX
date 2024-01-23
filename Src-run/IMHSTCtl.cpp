
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMHSTCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "MainApp.h"
#include "Version.h"

#include <QFileDialog>
#include <QThread>

using namespace Neuropixels;


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static QString getNPErrorString()
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return buf;
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMHSTCtl::IMHSTCtl( QObject *parent ) : QObject(parent)
{
    dlg = new HelpButDialog( "HST_Help" );

    hstUI = new Ui::IMBISTDlg;
    hstUI->setupUi( dlg );
    ConnectUI( hstUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( hstUI->clearBut, SIGNAL(clicked()), this, SLOT(clear()) );
    ConnectUI( hstUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    hstUI->testCB->clear();
    hstUI->testCB->addItem( "Run All Tests" );
    hstUI->testCB->addItem( "Communication" );
    hstUI->testCB->addItem( "Supply Voltages" );
    hstUI->testCB->addItem( "Control Signals" );
    hstUI->testCB->addItem( "Master Clock" );
    hstUI->testCB->addItem( "PSB Data Bus" );
    hstUI->testCB->addItem( "Signal Generator" );

    write(
        "\nBefore running these tests...\n"
        "Connect the HST (tester dongle) to the desired headstage." );
    isHelloText = true;

    _closeSlots();

    dlg->setWindowTitle( "HST (Imec Headstage Diagnostics)" );
    dlg->exec();
}


IMHSTCtl::~IMHSTCtl()
{
    _closeSlots();

    if( hstUI ) {
        delete hstUI;
        hstUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMHSTCtl::go()
{
    if( isHelloText )
        clear();

    int itest = hstUI->testCB->currentIndex();

    if( !itest )
        test_runAll();
    else {
        switch( itest ) {
            case 1: test_communication(); break;
            case 2: test_supplyVoltages(); break;
            case 3: test_controlSignals(); break;
            case 4: test_masterClock(); break;
            case 5: test_PSBDataBus(); break;
            case 6: test_signalGenerator(); break;
        }
    }
}


void IMHSTCtl::clear()
{
    hstUI->outTE->clear();
    isHelloText = false;
}


void IMHSTCtl::save()
{
    QString fn = QFileDialog::getSaveFileName(
                    dlg,
                    "Save test results as text file",
                    mainApp()->dataDir(),
                    "Text files (*.txt)" );

    if( fn.length() ) {

        QFile   f( fn );

        if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

            QTextStream ts( &f );

            ts << hstUI->outTE->toPlainText();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMHSTCtl::okVersions()
{
    basestationID   bs;
    firmware_Info   info;
    QString         bsfw, bscfw;
    NP_ErrorCode    err;
    int             slot = hstUI->slotSB->value();

    np_getDeviceInfo( slot, &bs );

    if( bs.platformid != NPPlatform_PXI )
        return true;

    err = np_bs_getFirmwareInfo( slot, &info );

    if( err != SUCCESS ) {
        write( "Error checking firmware:" );
        write(
            QString("IMEC bs_getFirmwareInfo(slot %1) error %2:")
            .arg( slot ).arg( err ) );
        write( getNPErrorString() );
        return false;
    }

    bsfw = QString("%1.%2.%3")
            .arg( info.major ).arg( info.minor ).arg( info.build );

    err = np_bsc_getFirmwareInfo( slot, &info );

    if( err != SUCCESS ) {
        write( "Error checking firmware:" );
        write(
            QString("IMEC bsc_getFirmwareInfo(slot %1) error %2:")
            .arg( slot ).arg( err ) );
        write( getNPErrorString() );
        return false;
    }

    bscfw = QString("%1.%2.%3")
            .arg( info.major ).arg( info.minor ).arg( info.build );

    if( bsfw != VERS_IMEC_BS || bscfw != VERS_IMEC_BSC ) {
        write( "ERROR: Wrong IMEC Firmware Version(s) ---" );
        if( bsfw != VERS_IMEC_BS )
            write( QString("   - BS(slot %1) Has: %2 Requires: %3")
                    .arg( slot ).arg( bsfw ).arg( VERS_IMEC_BS ) );
        if( bscfw != VERS_IMEC_BSC )
            write( QString("   - BSC(slot %1) Has: %2 Requires: %3")
                    .arg( slot ).arg( bscfw ).arg( VERS_IMEC_BSC ) );
        write("(1) Select menu item 'Tools/Update Imec PXIe Firmware'.");
        write("(2) Read the help for that dialog (click '?' in title bar).");
        write("(3) Required files are in the download package 'Firmware' folder.");
        return false;
    }
}


void IMHSTCtl::write( const QString &s )
{
    QTextEdit   *te = hstUI->outTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine );
    guiBreathe();
}


bool IMHSTCtl::_openSlot()
{
    int slot = hstUI->slotSB->value();

    if( openSlots.end() == find( openSlots.begin(), openSlots.end(), slot ) )
        openSlots.push_back( slot );

    return true;
}


void IMHSTCtl::_closeSlots()
{
    foreach( int is, openSlots )
        np_closeBS( is );

    openSlots.clear();
}


void IMHSTCtl::_closeHST()
{
    write( "-----------------------------------" );
    np_closeBS( hstUI->slotSB->value() );
}


bool IMHSTCtl::stdStart( int itest, int secs )
{
    if( !okVersions() )
        return false;

    write( "-----------------------------------" );
    write( QString("Test %1").arg( hstUI->testCB->itemText( itest ) ) );

    bool    ok = _openSlot();

    if( ok ) {

        if( secs ) {
            write( QString("Starting test, allow ~%1 seconds...")
                    .arg( secs ) );
        }
        else
            write( "Starting test..." );
    }
    else
        _closeHST();

    return ok;
}


bool IMHSTCtl::stdTest( const QString &fun, NP_ErrorCode err )
{
    if( err == SUCCESS ) {
        write( QString("%1 result = SUCCESS").arg( fun ) );
        return true;
    }

    write( QString("%1 result = FAIL: '%2'")
        .arg( fun ).arg( getNPErrorString() ) );

    _closeHST();
    return false;
}


void IMHSTCtl::test_runAll()
{
    if( !okVersions() )
        return;

    test_communication();
    test_supplyVoltages();
    test_controlSignals();
    test_masterClock();
    test_PSBDataBus();
    test_signalGenerator();
}


void IMHSTCtl::test_communication()
{
    if( !stdStart( 1 ) )
        return;

    int             slot = hstUI->slotSB->value(),
                    port = hstUI->portSB->value();
    NP_ErrorCode    err;

    err = np_HSTestPCLK( slot, port );

    if( !stdTest( "HSTestPCLK", err ) )
        return;

    err = np_HSTestI2C( slot, port );

    if( !stdTest( "HSTestI2C", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_supplyVoltages()
{
    if( !stdStart( 2 ) )
        return;

    int             slot = hstUI->slotSB->value(),
                    port = hstUI->portSB->value();
    NP_ErrorCode    err;

    err = np_HSTestVDDA1V8( slot, port );

    if( !stdTest( "HSTestVDDDA1V8", err ) )
        return;

    err = np_HSTestVDDD1V8( slot, port );

    if( !stdTest( "HSTestVDDD1V8", err ) )
        return;

    err = np_HSTestVDDA1V2( slot, port );

    if( !stdTest( "HSTestVDDA1V2", err ) )
        return;

    err = np_HSTestVDDD1V2( slot, port );

    if( !stdTest( "HSTestVDDD1V2", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_controlSignals()
{
    if( !stdStart( 3 ) )
        return;

    int             slot = hstUI->slotSB->value(),
                    port = hstUI->portSB->value();
    NP_ErrorCode    err;

    err = np_HSTestNRST( slot, port );

    if( !stdTest( "HSTestNRST", err ) )
        return;

    err = np_HSTestREC_NRESET( slot, port );

    if( !stdTest( "HSTestREC_NRESET", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_masterClock()
{
    if( !stdStart( 4 ) )
        return;

    NP_ErrorCode    err;

    err = np_HSTestMCLK(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestMCLK", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_PSBDataBus()
{
    if( !stdStart( 5 ) )
        return;

    NP_ErrorCode    err;

    err = np_HSTestPSB(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestPSB", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_signalGenerator()
{
    if( !stdStart( 6 ) )
        return;

    NP_ErrorCode    err;

    err = np_HSTestOscillator(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestOscillator", err ) )
        return;

    _closeHST();
}

#endif  // HAVE_IMEC


