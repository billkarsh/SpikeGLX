
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMROTbl.h"
#include "IMBISTCtl.h"
#include "Util.h"
#include "MainApp.h"

#include <QFileDialog>
#include <QThread>

using namespace Neuropixels;


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static QString getNPErrorString4()
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

IMBISTCtl::IMBISTCtl() : QDialog(0)
{
    bistUI = new Ui::IMBISTDlg;
    bistUI->setupUi( this );
    ConnectUI( bistUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( bistUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( bistUI->clearBut, SIGNAL(clicked()), this, SLOT(clear()) );
    ConnectUI( bistUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    exec();
}


IMBISTCtl::~IMBISTCtl()
{
    _closeSlots();

    if( bistUI ) {
        delete bistUI;
        bistUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMBISTCtl::go()
{
    int itest = bistUI->testCB->currentIndex();

    QGuiApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
    guiBreathe();
    guiBreathe();

    if( !_openSlot() || !okVersions() )
        goto exit;

    if( !itest ) {

        if( !test_bistBS() )
            goto exit;

        if( !probeType() || !openProbe() )
            goto exit;

        test_bistHB();
        test_bistPRBS();
        test_bistI2CMM();
        test_bistEEPROM();
        test_bistSR();
        test_bistPSB();
        test_bistSignal();
        test_bistNoise();

        write( "" );
        write( "If any tests failed, click Help button for assistance." );
    }
    else {

        if( itest > 1 ) {
            if( !probeType() || !openProbe() )
                goto exit;
        }

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

exit:
    closeProbe();
    QGuiApplication::restoreOverrideCursor();
}


void IMBISTCtl::helpBut()
{
    showHelp( "BIST_Help" );
}


void IMBISTCtl::clear()
{
    bistUI->outTE->clear();
}


void IMBISTCtl::save()
{
    QString fn = QFileDialog::getSaveFileName(
                    this,
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


bool IMBISTCtl::_openSlot()
{
    write( "Open slot..." );

    int     slot = bistUI->slotSB->value();
    bool    ok   = true;

    if( 0 ) {
    }
    else if( openSlots4.end() == std::find( openSlots4.begin(), openSlots4.end(), slot ) )
        openSlots4.push_back( slot );

    if( 0 ) {
    }
    else
        write( "API4 slot..." );

    return ok;
}


void IMBISTCtl::_closeSlots()
{
    for( int is : openSlots4 )
        np_closeBS( is );
}


bool IMBISTCtl::okVersions()
{
    basestationID   bs;
    firmware_Info   info;
    HardwareID      hID;
    QString         bsfw, bscfw;
    int             bstech,
                    slot = bistUI->slotSB->value();
    NP_ErrorCode    err;

    write( "Check slot firmware..." );

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
        write( getNPErrorString4() );
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
        write( getNPErrorString4() );
        return false;
    }
    bscfw = QString("%1.%2.%3")
            .arg( info.major ).arg( info.minor ).arg( info.build );

    err = np_getBSCHardwareID( slot, &hID );
    if( err != SUCCESS ) {
        write( "Error identifying module:" );
        write(
            QString("IMEC getBSCHardwareID(slot %1) error %2:")
            .arg( slot ).arg( err ) );
        write( getNPErrorString4() );
        return false;
    }
    bstech = IMROTbl::bscpnToTech( hID.ProductNumber );

    QStringList bs_bsc;
    IMROTbl::bscCheckTech( bs_bsc, bsfw, bscfw, bstech, slot );

    if( bs_bsc.size() ) {
        write( "ERROR: Wrong IMEC Firmware Version(s) ---" );
        foreach( const QString &s, bs_bsc )
            write( s );
        write("(1) Select menu item 'Tools/Update Imec PXIe Firmware'.");
        write("(2) Read the help for that dialog.");
        return false;
    }

    return true;
}


bool IMBISTCtl::probeType()
{
    write( "Check probe type..." );

    pn.clear();

    int slot = bistUI->slotSB->value(),
        port = bistUI->portSB->value(),
        dock = bistUI->dockSB->value();

    if( 0 ) {
    }
    else {
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
                .arg( err ).arg( getNPErrorString4() ) );

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
            goto exit;
        }

        // ---------------------------
        // Test for Quad port (2 or 4)
        // ---------------------------

        else if( prod.contains( "ext" ) ) {
            write("For Quad-probes (NP2020) only test ports (1 or 3).");
            return false;
        }

        write(
            QString("Headstage: pn %1 sn %2")
            .arg( hID.ProductNumber ).arg( hID.SerialNumber ) );

        // --
        // PN
        // --

        err = np_getProbeHardwareID( slot, port, dock, &hID );

        if( err != SUCCESS ) {
            write(
                QString("IMEC getProbeHardwareID(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( slot ).arg( port ).arg( dock )
                .arg( err ).arg( getNPErrorString4() ) );
            return false;
        }

        pn = hID.ProductNumber;

        write(
            QString("Probe: pn %1 sn %2")
            .arg( hID.ProductNumber ).arg( hID.SerialNumber ) );
    }

// ----
// Type
// ----

    if( !IMROTbl::pnToType( type, pn ) ) {
        write(
            QString("SpikeGLX probeType(slot %1, port %2, dock %3)"
            " error 'Probe part number %4 unsupported'.")
            .arg( slot ).arg( port ).arg( dock ).arg( pn ) );
        write("Try updating to a newer SpikeGLX/API version.");
        return false;
    }

exit:
    return EEPROMCheck();
}


bool IMBISTCtl::EEPROMCheck()
{
    testEEPROM = true;

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
            .arg( err ).arg( getNPErrorString4() ) );
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


bool IMBISTCtl::openProbe()
{
    write( "Open probe..." );

    if( 1 ) {

        int             slot = bistUI->slotSB->value(),
                        port = bistUI->portSB->value(),
                        dock = bistUI->dockSB->value();
        NP_ErrorCode    err  = np_openProbe( slot, port, dock );

        if( err != SUCCESS && err != ALREADY_OPEN ) {
            write(
                QString("IMEC openProbe(slot %1, port %2, dock %3) error %4 '%5'.")
                .arg( slot ).arg( port ).arg( dock )
                .arg( err ).arg( getNPErrorString4() ) );
            return false;
        }
    }

    return true;
}


void IMBISTCtl::closeProbe()
{
    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else
        np_closeBS( slot );
}


void IMBISTCtl::stdStart( int itest, int secs )
{
    write( "-----------------------------------" );
    write( QString("Test %1").arg( bistUI->testCB->itemText( itest ) ) );

    if( itest > 1 ) {

        if( secs ) {
            write( QString("Starting test, allow ~%1 seconds...")
                    .arg( secs ) );
        }
        else
            write( "Starting test..." );
    }
}


void IMBISTCtl::stdFinish4( NP_ErrorCode err )
{
    if( err == SUCCESS )
        write( "result = 0 'SUCCESS'" );
    else {
        write( QString("result = %1 '%2'")
            .arg( err ).arg( getNPErrorString4() ) );
    }

    stdFinish();
}


void IMBISTCtl::stdFinish()
{
    write( "-----------------------------------" );
}


bool IMBISTCtl::test_bistBS()
{
    stdStart( 1 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistBS( slot );

        stdFinish4( err );

        if( err == NO_SLOT ) {
            writeMapMsg( slot );
            write( "" );
            return false;
        }
    }

    return true;
}


void IMBISTCtl::test_bistHB()
{
    stdStart( 2, 5 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistHB(
                slot,
                bistUI->portSB->value(),
                bistUI->dockSB->value() );

        stdFinish4( err );
    }
}


void IMBISTCtl::test_bistPRBS()
{
    stdStart( 3, 10 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistStartPRBS( slot, bistUI->portSB->value() );

        if( err != SUCCESS ) {
            write( QString("Error %1 starting test: '%2'")
                    .arg( err ).arg( getNPErrorString4() ) );
            stdFinish();
            return;
        }

        QThread::msleep( 10000 );

        int prbs_err;

        err = np_bistStopPRBS( slot, bistUI->portSB->value(), &prbs_err );

        if( err != SUCCESS ) {
            write( QString("Error %1 stopping test: '%2'")
                    .arg( err ).arg( getNPErrorString4() ) );
        }

        write( QString("Test result: serDes error count = %1")
                .arg( prbs_err ) );

        if( !prbs_err )
            write( "result = 0 'SUCCESS'" );
        else
            write( "result = 'FAILED'" );
    }

    stdFinish();
}


void IMBISTCtl::test_bistI2CMM()
{
    stdStart( 4 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistI2CMM( slot, bistUI->portSB->value(), bistUI->dockSB->value() );

        stdFinish4( err );
    }
}


void IMBISTCtl::test_bistEEPROM()
{
    stdStart( 5 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err = SUCCESS;

        if( testEEPROM )
            err = np_bistEEPROM( slot, bistUI->portSB->value() );

        stdFinish4( err );
    }
}


void IMBISTCtl::test_bistSR()
{
    stdStart( 6 );

    IMROTbl *R      = IMROTbl::alloc( pn );
    int     slot    = bistUI->slotSB->value(),
            nShnk   = R->nShank();
    bool    testSR  = (R->nBanks() > 1);
    uint8_t mask    = 0;
    delete R;

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err = SUCCESS;

        if( testSR ) {
            err = np_bistSR(
                    slot,
                    bistUI->portSB->value(),
                    bistUI->dockSB->value(), &mask );
        }

        if( err == SUCCESS )
            write( "result = 0 'SUCCESS'" );
        else if( err == TIMEOUT ) {
            write( QString("result = %1 '%2'")
                .arg( err ).arg( getNPErrorString4() ) );
            write( "Test inconclusive." );
            write( "Check connections and try again." );
        }
        else {
            write( QString("result = %1 '%2'")
                .arg( err ).arg( getNPErrorString4() ) );

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
            if( ngood == 0 )
                write( "You cannot use this probe." );
            else if( ngood < nShnk )
                write( "You can use this probe by selecting sites only on good shanks." );
        }
    }

    stdFinish();
}


void IMBISTCtl::test_bistPSB()
{
    stdStart( 7 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistPSB( slot, bistUI->portSB->value(), bistUI->dockSB->value() );

        stdFinish4( err );
    }
}


// @@@ FIX Temporary skip of signal test.
#if 0
void IMBISTCtl::test_bistSignal()
{
    stdStart( 8, 40 );

    write( "Signal test -- not yet implemented --" );
    stdFinish();
}
#endif


// @@@ FIX Experiment to observe readout on all electrodes.
#if 0
void IMBISTCtl::test_bistSignal()
{
    stdStart( 8, 40 );

    int             slot = bistUI->slotSB->value();
    NP_ErrorCode    err;
    bool            pass = false;

    std::vector<bistElectrodeStats> S( 960 );

    err = bistSignal(
            slot,
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

    stdFinish();
}
#endif


// The real signal test
#if 1
void IMBISTCtl::test_bistSignal()
{
    stdStart( 8, 40 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistSignal( slot, bistUI->portSB->value(), bistUI->dockSB->value() );

        stdFinish4( err );
    }
}
#endif


void IMBISTCtl::test_bistNoise()
{
    stdStart( 9, 40 );

    int slot = bistUI->slotSB->value();

    if( 0 ) {
    }
    else {
        NP_ErrorCode    err;

        err = np_bistNoise( slot, bistUI->portSB->value(), bistUI->dockSB->value() );

        stdFinish4( err );
    }
}

#endif  // HAVE_IMEC


