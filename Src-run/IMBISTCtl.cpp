
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMBISTCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "MainApp.h"

#include <QFileDialog>
#include <QThread>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMBISTCtl::IMBISTCtl( QObject *parent ) : QObject( parent )
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
        test_bistBS();
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
#if 0   // @@@ FIX NeuropixAPI private members issue
            case 10: test_HSTestVDDDA1V2(); break;
            case 11: test_HSTestVDDDD1V2(); break;
            case 12: test_HSTestVDDDA1V8(); break;
            case 13: test_HSTestVDDDD1V8(); break;
            case 14: test_HSTestOscillator(); break;
            case 15: test_HSTestMCLK(); break;
            case 16: test_HSTestPCLK(); break;
            case 17: test_HSTestPSB(); break;
            case 18: test_HSTestI2C(); break;
            case 19: test_HSTestNRST(); break;
            case 20: test_HSTestREC_NRESET(); break;
#endif  // @@@ FIX NeuropixAPI private members issue
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
    te->moveCursor( QTextCursor::StartOfLine );
    guiBreathe();
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
    foreach( int is, openSlots )
        closeBS( is );

    openSlots.clear();
}


bool IMBISTCtl::_openProbe()
{
    int             slot = bistUI->slotSB->value(),
                    port = bistUI->portSB->value();
    NP_ErrorCode    err  = openProbe( slot, port );

    if( err != SUCCESS && err != ALREADY_OPEN ) {
        write(
            QString("IMEC openProbe(slot %1, port %2) error %3 '%4'.")
            .arg( slot ).arg( port )
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
        return false;
    }

    write( "Probe: open" );
    return true;
}


void IMBISTCtl::_closeProbe()
{
    write( "-----------------------------------" );
    closeBS( bistUI->slotSB->value() );
}


bool IMBISTCtl::stdStart( int itest, int secs )
{
    write( "-----------------------------------" );
    write( QString("Test %1").arg( bistUI->testCB->itemText( itest ) ) );

    bool    ok = _openSlot();

    if( ok && itest != 1 ) {

        ok = _openProbe();

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
    write( QString("result = %1 '%2'")
            .arg( err ).arg( np_GetErrorMessage( err ) ) );
    _closeProbe();
}


void IMBISTCtl::test_bistBS()
{
    if( !stdStart( 1 ) )
        return;

    NP_ErrorCode    err;

    err = bistBS( bistUI->slotSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistHB()
{
    if( !stdStart( 2, 5 ) )
        return;

    NP_ErrorCode    err;

    err = bistHB(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistPRBS()
{
    if( !stdStart( 3, 10 ) )
        return;

    NP_ErrorCode    err;

    err = bistStartPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    if( err != SUCCESS ) {

        write( QString("Error %1 starting test: '%2'")
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
        _closeProbe();
        return;
    }

    QThread::msleep( 10000 );

    quint8  prbs_err;

    err = bistStopPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            &prbs_err );

    if( err != SUCCESS ) {

        write( QString("Error %1 stopping test: '%2'")
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
    }

    write( QString("Test result: serDes error count = %1")
            .arg( prbs_err ) );
    _closeProbe();
}


void IMBISTCtl::test_bistI2CMM()
{
    if( !stdStart( 4 ) )
        return;

    NP_ErrorCode    err;

    err = bistI2CMM(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


// @@@ FIX The real EEPROM test, but imec needs to fix
#if 1
void IMBISTCtl::test_bistEEPROM()
{
    if( !stdStart( 5 ) )
        return;

    NP_ErrorCode    err;

    err = bistEEPROM(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}
#endif


// @@@ FIX Temporary skip of EEPROM test.
#if 0
void IMBISTCtl::test_bistEEPROM()
{
    if( !stdStart( 5 ) )
        return;

    write( "EEPROM test -- not yet implemented --" );
    _closeProbe();
}
#endif


void IMBISTCtl::test_bistSR()
{
    if( !stdStart( 6 ) )
        return;

    NP_ErrorCode    err;

    err = bistSR(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistPSB()
{
    if( !stdStart( 7 ) )
        return;

    NP_ErrorCode    err;

    err = bistPSB(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


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
            &pass,
            &S[0] );

    if( err != SUCCESS ) {

        write( QString("Error %1 running test: '%2'")
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
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


// @@@ FIX The real signal test, but imec needs to fix
#if 1
void IMBISTCtl::test_bistSignal()
{
    if( !stdStart( 8, 40 ) )
        return;

    NP_ErrorCode    err;
    bool            pass = false;

    err = bistSignal(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            &pass,
            NULL );

    if( err != SUCCESS ) {

        write( QString("Error %1 running test: '%2'")
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
    }

    write( QString("Signal test result = %1")
            .arg( pass ? "PASSED" : "FAILED" ) );
    _closeProbe();
}
#endif


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


void IMBISTCtl::test_bistNoise()
{
    if( !stdStart( 9, 40 ) )
        return;

    NP_ErrorCode    err;

    err = bistNoise(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


#if 0   // @@@ FIX NeuropixAPI private members issue
void IMBISTCtl::test_HSTestVDDDA1V2()
{
    if( !stdStart( 10 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestVDDA1V2(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDD1V2()
{
    if( !stdStart( 11 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestVDDD1V2(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDA1V8()
{
    if( !stdStart( 12 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestVDDA1V8(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDD1V8()
{
    if( !stdStart( 13 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestVDDD1V8(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestOscillator()
{
    if( !stdStart( 14 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestOscillator(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestMCLK()
{
    if( !stdStart( 15 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestMCLK(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestPCLK()
{
    if( !stdStart( 16 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestPCLK(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestPSB()
{
    if( !stdStart( 17 ) )
        return;

    quint32         errormask;
    NP_ErrorCode    err;

    err = IM.HSTestPSB(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            errormask );

    write(
        QString("result = %1; error mask = x%1")
        .arg( err ).arg( errormask, 8, 16, QChar('0') ) );
    _closeProbe();
}


void IMBISTCtl::test_HSTestI2C()
{
    if( !stdStart( 18 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestI2C(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestNRST()
{
    if( !stdStart( 19 ) )
        return;

    NP_ErrorCode    ere;

    err = IM.HSTestNRST(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestREC_NRESET()
{
    if( !stdStart( 20 ) )
        return;

    NP_ErrorCode    err;

    err = IM.HSTestREC_NRESET(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    stdFinish( err );
}
#endif  // @@@ FIX NeuropixAPI private members issue

#endif  // HAVE_IMEC


