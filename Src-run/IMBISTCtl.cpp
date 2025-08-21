
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMBISTCtl.h"
#include "IMROTbl.h"
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

    QString s( buf );

    if( s.startsWith( "NOTSUP" ) )
        s = "Selected test is inapplicable for this probe type";

    return s;
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMBISTCtl::IMBISTCtl( QObject *parent ) : QObject(parent)
{
    dlg = new HelpButDialog( "BIST_Help" );

    bistUI = new Ui::IMBISTDlg;
    bistUI->setupUi( dlg );
    ConnectUI( bistUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( bistUI->clearBut, SIGNAL(clicked()), this, SLOT(clear()) );
    ConnectUI( bistUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    _closeSlots();

    dlg->exec();
}


IMBISTCtl::~IMBISTCtl()
{
    _closeSlots();

    if( bistUI ) {
        delete bistUI;
        bistUI = 0;
    }

    if( dlg ) {
        delete dlg;
        dlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMBISTCtl::go()
{
    int itest = bistUI->testCB->currentIndex();

    if( !itest ) {

        if( !okVersions() || !probeType() )
            return;

        if( !test_bistBS() )
            return;

        test_bistHB();
        test_bistPRBS();
        test_bistI2CMM();
        test_bistEEPROM();
        test_bistSR();
        test_bistPSB();
        test_bistSignal();
        test_bistNoise();
    }
    else {
        switch( itest ) {
            case 1: test_bistBS(); break;
            case 2: test_bistHB(); break;
            case 3: test_bistPRBS(); break;
            case 4: test_bistI2CMM(); break;
            case 5: test_bistEEPROM(); break;
            case 6: test_bistSR(); break;
            case 7: test_bistPSB(); break;
            case 8: test_bistSignal(); break;
            case 9: test_bistNoise(); break;
        }
    }
}


void IMBISTCtl::clear()
{
    bistUI->outTE->clear();
}


void IMBISTCtl::save()
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

            ts << bistUI->outTE->toPlainText();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMBISTCtl::write( const QString &s )
{
    QTextEdit   *te = bistUI->outTE;

    te->append( s );
    te->moveCursor( QTextCursor::End );
    te->moveCursor( QTextCursor::StartOfLine ); // H-scrollbar to zero
    guiBreathe();
}


void IMBISTCtl::writeMapMsg( int slot )
{
    if( slot >= 20 ) {
        write(
            "\n"
            "Click 'Detect' in the 'Configure Acquisition' dialog\n"
            "before running the BISTs. This will assign slots to\n"
            "your OneBoxes." );
    }
}


bool IMBISTCtl::okVersions()
{
    basestationID   bs;
    firmware_Info   info;
    QString         bsfw, bscfw;
    NP_ErrorCode    err;
    int             slot = bistUI->slotSB->value();

    np_scanBS();

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

    return true;
}


bool IMBISTCtl::_openSlot()
{
    int slot = bistUI->slotSB->value();

    if( openSlots.end() == find( openSlots.begin(), openSlots.end(), slot ) )
        openSlots.push_back( slot );

    return true;
}


void IMBISTCtl::_closeSlots()
{
    for( int is : openSlots )
        np_closeBS( is );

    openSlots.clear();
}


bool IMBISTCtl::_openProbe()
{
    type        = -1;
    testEEPROM  = true;

    int             slot = bistUI->slotSB->value(),
                    port = bistUI->portSB->value(),
                    dock = bistUI->dockSB->value();
    NP_ErrorCode    err  = np_openProbe( slot, port, dock );

    if( err != SUCCESS && err != ALREADY_OPEN ) {
        write(
            QString("IMEC openProbe(slot %1, port %2, dock %3) error %4 '%5'.")
            .arg( slot ).arg( port ).arg( dock )
            .arg( err ).arg( getNPErrorString() ) );
        return false;
    }

    write( "Probe: open" );
    return true;
}


void IMBISTCtl::_closeProbe()
{
    write( "-----------------------------------" );
    np_closeBS( bistUI->slotSB->value() );
}


bool IMBISTCtl::probeType()
{
    int             slot = bistUI->slotSB->value(),
                    port = bistUI->portSB->value(),
                    dock = bistUI->dockSB->value();
    NP_ErrorCode    err;
    HardwareID      hID;

// ----
// HSPN
// ----

    err = np_getHeadstageHardwareID( slot, port, &hID );

    if( err != SUCCESS ) {
        write(
            QString("IMEC getHeadstageHardwareID(slot %1, port %2) error %3 '%4'.")
            .arg( slot ).arg( port )
            .arg( err ).arg( getNPErrorString() ) );

        if( err == NO_SLOT )
            writeMapMsg( slot );

        return false;
    }

// -------------------------------
// Test for NHP 128-channel analog
// -------------------------------

    QString prod(hID.ProductNumber);

    if( prod == "NPNH_HS_30" || prod == "NPNH_HS_31" ) {
        type = 1200;
        return true;
    }

// ---------------------------
// Test for Quad port (2 or 4)
// ---------------------------

    else if( prod.contains( "ext" ) ) {
        write("For Quad-probes (NP2020) only test ports (1 or 3).");
        return false;
    }

// --
// PN
// --

    err = np_getProbeHardwareID( slot, port, dock, &hID );

    if( err != SUCCESS ) {
        write(
            QString("IMEC getProbeHardwareID(slot %1, port %2, dock %3) error %4 '%5'.")
            .arg( slot ).arg( port ).arg( dock )
            .arg( err ).arg( getNPErrorString() ) );
        return false;
    }

    pn = hID.ProductNumber;

    if( !IMROTbl::pnToType( type, pn ) ) {
        write(
            QString("SpikeGLX probeType(slot %1, port %2, dock %3)"
            " error 'Probe part number %4 unsupported'.")
            .arg( slot ).arg( port ).arg( dock ).arg( pn ) );
        write("Try updating to a newer SpikeGLX/API version.");
        return false;
    }

    return true;
}


bool IMBISTCtl::EEPROMCheck()
{
    if( type != 21 && type != 24 ) {

        if( type == 1200 )
            testEEPROM = false;

        return true;
    }

// -----
// HS20?
// -----

    int             slot = bistUI->slotSB->value(),
                    port = bistUI->portSB->value();
    NP_ErrorCode    err;
    HardwareID      hID;

// ----
// HSSN
// ----

    err = np_getHeadstageHardwareID( slot, port, &hID );

    if( err != SUCCESS ) {
        write(
            QString("IMEC getHeadstageHardwareID(slot %1, port %2) error %3 '%4'.")
            .arg( slot ).arg( port )
            .arg( err ).arg( getNPErrorString() ) );
        return false;
    }

// ----
// HSHW
// ----

// --------------------------
// HS20 (tests for no EEPROM)
// --------------------------

    QString smaj(hID.version_Major), smin(hID.version_Minor);
    bool    noEEPROM =
                (smaj == "" || smaj == "0" || smaj == "1") &&
                (smin == "" || smin == "0");

    if( !hID.SerialNumber && noEEPROM )
        testEEPROM = false;

    return true;
}


bool IMBISTCtl::stdStart( int itest, int secs )
{
    if( !okVersions() )
        return false;

    write( "-----------------------------------" );
    write( QString("Test %1").arg( bistUI->testCB->itemText( itest ) ) );

    bool    ok = _openSlot();

    if( ok && itest != 1 ) {

        ok = probeType() && EEPROMCheck() && _openProbe();

        if( ok ) {

            if( secs ) {
                write( QString("Starting test, allow ~%1 seconds...")
                        .arg( secs ) );
            }
            else
                write( "Starting test..." );
        }
        else
            _closeProbe();
    }

    return ok;
}


void IMBISTCtl::stdFinish( NP_ErrorCode err )
{
    if( err == SUCCESS )
        write( "result = 0 'SUCCESS'" );
    else {
        write( QString("result = %1 '%2'")
            .arg( err ).arg( getNPErrorString() ) );
    }

    _closeProbe();
}


bool IMBISTCtl::test_bistBS()
{
    if( !stdStart( 1 ) )
        return false;

    int             slot = bistUI->slotSB->value();
    NP_ErrorCode    err;

    err = np_bistBS( slot );

    stdFinish( err );

    if( err == NO_SLOT ) {
        writeMapMsg( slot );
        write( "" );
        return false;
    }

    return true;
}


void IMBISTCtl::test_bistHB()
{
    if( !stdStart( 2, 5 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistHB(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistPRBS()
{
    if( !stdStart( 3, 10 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistStartPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    if( err != SUCCESS ) {

        write( QString("Error %1 starting test: '%2'")
                .arg( err ).arg( getNPErrorString() ) );
        _closeProbe();
        return;
    }

    QThread::msleep( 10000 );

    int prbs_err;

    err = np_bistStopPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            &prbs_err );

    if( err != SUCCESS ) {

        write( QString("Error %1 stopping test: '%2'")
                .arg( err ).arg( getNPErrorString() ) );
    }

    write( QString("Test result: serDes error count = %1")
            .arg( prbs_err ) );

    if( !prbs_err )
        write( "result = 0 'SUCCESS'" );
    else
        write( "result = 'FAILED'" );

    _closeProbe();
}


void IMBISTCtl::test_bistI2CMM()
{
    if( !stdStart( 4 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistI2CMM(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistEEPROM()
{
    if( !stdStart( 5 ) )
        return;

    NP_ErrorCode    err = SUCCESS;

    if( testEEPROM ) {
        err = np_bistEEPROM(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );
    }

    stdFinish( err );
}


void IMBISTCtl::test_bistSR()
{
    if( !stdStart( 6 ) )
        return;

    NP_ErrorCode    err = SUCCESS;

    IMROTbl *R      = IMROTbl::alloc( pn );
    int     nShnk   = R->nShank();
    bool    testSR  = (R->nBanks() > 1);
    uint8_t mask    = 0;
    delete R;

    if( testSR ) {
        err = np_bistSR(
                bistUI->slotSB->value(),
                bistUI->portSB->value(),
                bistUI->dockSB->value(), &mask );
    }

    if( err == SUCCESS )
        write( "result = 0 'SUCCESS'" );
    else if( err == TIMEOUT ) {
        write( QString("result = %1 '%2'")
            .arg( err ).arg( getNPErrorString() ) );
        write( "Test inconclusive." );
        write( "Check connections and try again." );
    }
    else {
        write( QString("result = %1 '%2'")
            .arg( err ).arg( getNPErrorString() ) );
        QString s;
        int     ngood = 0;
        for( int is = 0; is < nShnk; ++is ) {
            if( mask & (1<<is) ) {
                s += QString(" %1").arg( is );
                ++ngood;
            }
        }
        write( QString("Zero-based good shank list = { %1 }")
            .arg( s.trimmed() ) );
        if( ngood < nShnk )
            write( "You should not use this probe." );
    }

    _closeProbe();
}


void IMBISTCtl::test_bistPSB()
{
    if( !stdStart( 7 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistPSB(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value() );

    stdFinish( err );
}


// @@@ FIX Temporary skip of signal test.
#if 0
void IMBISTCtl::test_bistSignal()
{
    if( !stdStart( 8, 40 ) )
        return;

    write( "Signal test -- not yet implemented --" );
    _closeProbe();
}
#endif


// @@@ FIX Experiment to observe readout on all electrodes.
#if 0
void IMBISTCtl::test_bistSignal()
{
    if( !stdStart( 8, 40 ) )
        return;

    NP_ErrorCode    err;
    bool            pass = false;

    std::vector<bistElectrodeStats> S( 960 );

    err = bistSignal(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value(),
            &pass,
            &S[0] );

    if( err != SUCCESS ) {

        write( QString("Error %1 running test: '%2'")
                .arg( err ).arg( getNPErrorString() ) );
    }

    write( QString("Signal test result = %1")
            .arg( pass ? "PASSED" : "FAILED" ) );

    for( int i = 0; i < 960; ++i ) {
        write( QString("F %1 A %2 min %3 max %4 ave %5")
        .arg( S[i].peakfreq_Hz, 0, 'f', 4 )
        .arg( S[i].peakamplitude, 0, 'f', 4 )
        .arg( S[i].min, 0, 'f', 4 )
        .arg( S[i].max, 0, 'f', 4 )
        .arg( S[i].avg, 0, 'f', 4 ) );
    }

    _closeProbe();
}
#endif


// The real signal test
#if 1
void IMBISTCtl::test_bistSignal()
{
    if( !stdStart( 8, 40 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistSignal(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value() );

    stdFinish( err );
}
#endif


void IMBISTCtl::test_bistNoise()
{
    if( !stdStart( 9, 40 ) )
        return;

    NP_ErrorCode    err;

    err = np_bistNoise(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            bistUI->dockSB->value() );

    stdFinish( err );
}

#endif  // HAVE_IMEC


