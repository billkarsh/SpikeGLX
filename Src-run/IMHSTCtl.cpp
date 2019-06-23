
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMHSTCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "MainApp.h"

#include <QFileDialog>
#include <QThread>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMHSTCtl::IMHSTCtl( QObject *parent ) : QObject( parent )
{
    dlg = new HelpButDialog( "HST_Help" );

    hstUI = new Ui::IMBISTDlg;
    hstUI->setupUi( dlg );
    ConnectUI( hstUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( hstUI->clearBut, SIGNAL(clicked()), this, SLOT(clear()) );
    ConnectUI( hstUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    hstUI->testCB->clear();
    hstUI->testCB->addItem( "Run All Tests" );
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
            case 1: test_supplyVoltages(); break;
            case 2: test_controlSignals(); break;
            case 3: test_masterClock(); break;
            case 4: test_PSBDataBus(); break;
            case 5: test_signalGenerator(); break;
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

QString IMHSTCtl::getErrorStr()
{
    char    buf[1024];
    getLastErrorMessage( buf, sizeof(buf) );
    return buf;
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
        closeBS( is );

    openSlots.clear();
}


bool IMHSTCtl::_openHST()
{
    int             slot = hstUI->slotSB->value(),
                    port = hstUI->portSB->value();
    NP_ErrorCode    err  = openProbeHSTest( slot, port );

    if( err != SUCCESS && err != ALREADY_OPEN ) {
        write(
            QString("IMEC openProbeHSTest(slot %1, port %2) error %3 '%4'.")
            .arg( slot ).arg( port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    err = HSTestI2C(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( err != SUCCESS ) {
        write(
            QString("IMEC HSTestI2C(slot %1, port %2) error '%3'.")
            .arg( slot ).arg( port )
            .arg( getErrorStr() ) );
        return false;
    }

    write( "HST: open" );
    return true;
}


void IMHSTCtl::_closeHST()
{
    write( "-----------------------------------" );
    closeBS( hstUI->slotSB->value() );
}


bool IMHSTCtl::stdStart( int itest, int secs )
{
    write( "-----------------------------------" );
    write( QString("Test %1").arg( hstUI->testCB->itemText( itest ) ) );

    bool    ok = _openSlot();

    if( ok ) {

        ok = _openHST();

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
    }

    return ok;
}


bool IMHSTCtl::stdTest( const QString &fun, NP_ErrorCode err )
{
    if( err == SUCCESS ) {
        write( QString("%1 result = SUCCESS").arg( fun ) );
        return true;
    }

    write( QString("%1 result = FAIL: '%2'")
        .arg( fun ).arg( getErrorStr() ) );

    _closeHST();
    return false;
}


void IMHSTCtl::test_runAll()
{
    test_supplyVoltages();
    test_controlSignals();
    test_masterClock();
    test_PSBDataBus();
    test_signalGenerator();
}


void IMHSTCtl::test_supplyVoltages()
{
    if( !stdStart( 1 ) )
        return;

    NP_ErrorCode    err;

    err = HSTestVDDA1V8(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestVDDDA1V8", err ) )
        return;

    err = HSTestVDDD1V8(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestVDDD1V8", err ) )
        return;

    err = HSTestVDDA1V2(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestVDDA1V2", err ) )
        return;

    err = HSTestVDDD1V2(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestVDDD1V2", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_controlSignals()
{
    if( !stdStart( 2 ) )
        return;

    NP_ErrorCode    err;

    err = HSTestNRST(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestNRST", err ) )
        return;

    err = HSTestREC_NRESET(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestREC_NRESET", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_masterClock()
{
    if( !stdStart( 3 ) )
        return;

    NP_ErrorCode    err;

    err = HSTestMCLK(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestMCLK", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_PSBDataBus()
{
    if( !stdStart( 4 ) )
        return;

    NP_ErrorCode    err;

    err = HSTestPSB(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestPSB", err ) )
        return;

    _closeHST();
}


void IMHSTCtl::test_signalGenerator()
{
    if( !stdStart( 5 ) )
        return;

    NP_ErrorCode    err;

    err = HSTestOscillator(
            hstUI->slotSB->value(),
            hstUI->portSB->value() );

    if( !stdTest( "HSTestOscillator", err ) )
        return;

    _closeHST();
}

#endif  // HAVE_IMEC


