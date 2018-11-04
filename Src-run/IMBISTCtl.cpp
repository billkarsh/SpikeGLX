
#ifdef HAVE_IMEC

#include "ui_IMBISTDlg.h"

#include "IMBISTCtl.h"
#include "HelpButDialog.h"
#include "Util.h"
#include "MainApp.h"

#include <QFileDialog>




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

// @@@ FIX Header temporary
#include <QMessageBox>
void IMBISTCtl::go()
{
// @@@ FIX Disabled BIST for now
// ------------------------------------------
    QMessageBox::information(
    dlg,
    "Feature Disabled",
    "BIST features under development." );
    return;
// ------------------------------------------

    int itest = bistUI->testCB->currentIndex();

    write( "-----------------------------------" );
    write( QString("Test %1").arg( bistUI->testCB->currentText() ) );

    if( _openSlot() ) {

        switch( itest ) {
#if 0   // @@@ FIX NeuropixAPI private members issue
            case 0: test_bistBS(); break;
            case 1: test_bistHB(); break;
            case 2: test_bistPRBS(); break;
            case 3: test_bistI2CMM(); break;
            case 4: test_bistEEPROM(); break;
            case 5: test_bistSR(); break;
            case 6: test_bistPSB(); break;
            case 7: test_bistSignal(); break;
            case 8: test_bistNoise(); break;
            case 9: test_HSTestVDDDA1V2(); break;
            case 10: test_HSTestVDDDD1V2(); break;
            case 11: test_HSTestVDDDA1V8(); break;
            case 12: test_HSTestVDDDD1V8(); break;
            case 13: test_HSTestOscillator(); break;
            case 14: test_HSTestMCLK(); break;
            case 15: test_HSTestPCLK(); break;
            case 16: test_HSTestPSB(); break;
            case 17: test_HSTestI2C(); break;
            case 18: test_HSTestNRST(); break;
            case 19: test_HSTestREC_NRESET(); break;
#endif  // @@@ FIX NeuropixAPI private members issue
        }
    }

    write( "-----------------------------------" );
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
                    mainApp()->runDir(),
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

    if( !openSlots.contains( slot ) ) {

        write( "Connection: opening slot..." );

        NP_ErrorCode    err = openBS( slot );

        if( err != SUCCESS ) {
            write(
                QString("IMEC openBS( %1 ) error %2 '%3'.")
                .arg( slot )
                .arg( err ).arg( np_GetErrorMessage( err ) ) );
            return false;
        }

        write( "Connection: slot open" );
        openSlots.append( slot );
    }
    else
        write( "Connection: slot already open" );

    return true;
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


void IMBISTCtl::_closeSlots()
{
    foreach( int is, openSlots )
        closeBS( is );

    openSlots.clear();
}


bool IMBISTCtl::stdStart()
{
    bool    ok = _openProbe();

    if( ok )
        write( "Starting test..." );

    return ok;
}


void IMBISTCtl::stdFinish( int err )
{
    write( QString("result = %1").arg( err ) );
    close( bistUI->slotSB->value(), -1 );
}


#if 0   // @@@ FIX NeuropixAPI private members issue
void IMBISTCtl::test_bistBS()
{
    if( !stdStart() )
        return;

    int err = IM.bistBS(
                bistUI->slotSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistHB()
{
    if( !stdStart() )
        return;

    int err = IM.bistHB(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}

void IMBISTCtl::test_bistPRBS()
{
    if( !stdStart() )
        return;

    int err;

    err = IM.bistStartPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value() );

    if( err != SUCCESS ) {

        write( QString("Error %1 starting test: '%2'")
                .arg( err ).arg( np_GetErrorMessage( err ) );
        close( bistUI->slotSB->value(), -1 );
        return;
    }

    write( "Run ten seconds..." );

    QThread::msleep( 10000 );

    quint8  prbs_err;

    err = IM.bistStopPRBS(
            bistUI->slotSB->value(),
            bistUI->portSB->value(),
            prbs_err );

    write(
        QString("stop result = %1; serDes errors = %2")
        .arg( err ).arg( prbs_err ) );

    close( bistUI->slotSB->value(), -1 );
}


void IMBISTCtl::test_bistI2CMM()
{
    if( !stdStart() )
        return;

    int err = IM.bistI2CMM(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistEEPROM()
{
    if( !stdStart() )
        return;

    int err = IM.bistEEPROM(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistSR()
{
    if( !stdStart() )
        return;

    int err = IM.bistSR(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistPSB()
{
    if( !stdStart() )
        return;

    int err = IM.bistPSB(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistSignal()
{
    if( !stdStart() )
        return;

    int err = IM.bistSignal(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_bistNoise()
{
    if( !stdStart() )
        return;

    int err = IM.bistNoise(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDA1V2()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestVDDA1V2(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDD1V2()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestVDDD1V2(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDA1V8()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestVDDA1V8(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestVDDDD1V8()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestVDDD1V8(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestOscillator()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestOscillator(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestMCLK()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestMCLK(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestPCLK()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestPCLK(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestPSB()
{
    if( !stdStart() )
        return;

    quint32 errormask;

    int err = IM.HSTestPSB(
                bistUI->slotSB->value(),
                bistUI->portSB->value(),
                errormask );

    write(
        QString("result = %1; error mask = x%1")
        .arg( err ).arg( errormask, 8, 16, QChar('0') ) );

    close( bistUI->slotSB->value(), -1 );
}


void IMBISTCtl::test_HSTestI2C()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestI2C(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestNRST()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestNRST(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}


void IMBISTCtl::test_HSTestREC_NRESET()
{
    if( !stdStart() )
        return;

    int err = IM.HSTestREC_NRESET(
                bistUI->slotSB->value(),
                bistUI->portSB->value() );

    stdFinish( err );
}
#endif  // @@@ FIX NeuropixAPI private members issue

#endif  // HAVE_IMEC


