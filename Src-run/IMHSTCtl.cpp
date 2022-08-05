
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMHSTCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "MainApp.h"

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


