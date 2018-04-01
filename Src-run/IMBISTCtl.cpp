
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

IMBISTCtl::IMBISTCtl( QObject *parent )
    :   QObject( parent )
{
    dlg = new HelpButDialog(
                "Imec Diagnostic Help",
                "CommonResources/BIST_Help.html" );

    bistUI = new Ui::IMBISTDlg;
    bistUI->setupUi( dlg );
    ConnectUI( bistUI->goBut, SIGNAL(clicked()), this, SLOT(go()) );
    ConnectUI( bistUI->clearBut, SIGNAL(clicked()), this, SLOT(clear()) );
    ConnectUI( bistUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    close();

    dlg->exec();
}


IMBISTCtl::~IMBISTCtl()
{
    close();

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

    write( "-----------------------------------" );
    write( QString("Test %1").arg( itest+4 ) );

    if( open() ) {

        switch( itest ) {
            case 0: test4(); break;
            case 1: test5(); break;
            case 2: test6(); break;
            case 3: test7(); break;
            case 4: test8(); break;
            case 5: test9(); break;
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


bool IMBISTCtl::open()
{
    if( isClosed ) {

        write( "Connection: opening..." );

        int err = IM.neuropix_open();

        if( err != OPEN_SUCCESS ) {
            write( QString("IMEC open error %1.").arg( err ) );
            return false;
        }

        write( "Connection: open" );
        isClosed = false;
    }
    else
        write( "Connection: already open" );

    write( "Starting test..." );

    return true;
}


void IMBISTCtl::close()
{
    IM.neuropix_close();
    isClosed = true;
}


void IMBISTCtl::test4()
{
    write( "Watch LCD display screen on Xilinx" );

    int err = IM.neuropix_test4();

    write( QString("result = %1").arg( err ) );
}


void IMBISTCtl::test5()
{
    write( "Look for blinking orange on HS (you have 10 seconds)" );

    unsigned char   brd_ver, fpga_ver;
    int             err = IM.neuropix_test5( brd_ver, fpga_ver, 10 );

    write( QString("result = %1  board_vers = %2  fpga_vers = %3")
            .arg( err )
            .arg( brd_ver )
            .arg( fpga_ver ) );
}


void IMBISTCtl::test6()
{
    unsigned char   prbs_err;
    int             err = IM.neuropix_startTest6();

    write( QString("Start result = %1").arg( err ) );

    if( err != BISTTEST6_SUCCESS )
        return;

    QThread::msleep( 5000 );

    err = IM.neuropix_stopTest6( prbs_err );

    write( QString("Stop  result = %1  bit error count = %2")
        .arg( err )
        .arg( prbs_err ) );
}


void IMBISTCtl::test7()
{
    int err = IM.neuropix_startTest7();

    write( QString("Start result = %1").arg( err ) );

    if( err != BISTTEST_SUCCESS )
        return;

    QThread::msleep( 5000 );

    err = IM.neuropix_stopTest7();

    write( QString("Stop  result = %1  pats = %2  errs = %3  mask = %4")
        .arg( err )
        .arg( IM.neuropix_test7GetTotalChecked() )
        .arg( IM.neuropix_test7GetErrorCounter() )
        .arg( IM.neuropix_test7GetErrorMask() ) );
}


void IMBISTCtl::test8()
{
    int err;

    err = IM.neuropix_mode( ASIC_RECORDING );
    write( QString("Set recording mode: result = %1").arg( err ) );

    if( err != DIGCTRL_SUCCESS )
        return;

    err = IM.neuropix_nrst( true );
    write( QString("Set nrst(true): result = %1").arg( err ) );

    if( err != DIGCTRL_SUCCESS )
        return;

    for( int line = 0; line < 4; ++line ) {

        write( QString("Line %1...").arg( line ) );

        err = IM.neuropix_startTest8( true, line );
        write( QString("  Start testing: result = %1").arg( err ) );

        if( err != BISTTEST8_SUCCESS )
            continue;

        QThread::msleep( 1000 );

        err = IM.neuropix_stopTest8();
        write( QString("  Stop  testing: result = %1").arg( err ) );

        if( err != BISTTEST8_SUCCESS )
            return;

        bool    spi_adc_err, spi_ctr_err, spi_syn_err;

        err = IM.neuropix_test8GetErrorCounter(
                true, spi_adc_err, spi_ctr_err, spi_syn_err );

        write( QString("  adc errors = %1  counter errors = %2  sync errors = %3")
                .arg( spi_adc_err )
                .arg( spi_ctr_err )
                .arg( spi_syn_err ) );
    }
}


void IMBISTCtl::test9()
{
    int err = IM.neuropix_mode( ASIC_RECORDING );

    write( QString("Set recording mode: result = %1").arg( err ) );

    if( err != DIGCTRL_SUCCESS )
        return;

    err = IM.neuropix_nrst( false );
    write( QString("Set nrst(false): result = %1").arg( err ) );

    if( err != DIGCTRL_SUCCESS )
        return;

    err = IM.neuropix_resetDatapath();
    write( QString("ResetDataPath: result = %1").arg( err ) );

    if( err != SUCCESS )
        return;

    err = IM.neuropix_startTest9( true );
    write( QString("Start testing: result = %1").arg( err ) );

    if( err != BISTTEST9_SUCCESS )
        return;

    err = IM.neuropix_nrst( true );
    write( QString("Set nrst(true): result = %1").arg( err ) );

    if( err != DIGCTRL_SUCCESS )
        return;

    write( "Run ten seconds..." );

    QThread::msleep( 10000 );

    unsigned int    syn_errors, syn_total,
                    ctr_errors, ctr_total,
                    te_spi0_errors, te_spi1_errors,
                    te_spi2_errors, te_spi3_errors,
                    te_spi_total;

    IM.neuropix_test9GetResults(
        syn_errors, syn_total,
        ctr_errors, ctr_total,
        te_spi0_errors, te_spi1_errors,
        te_spi2_errors, te_spi3_errors,
        te_spi_total );

    write( QString("  sync checks = %1  sync errors = %2")
            .arg( syn_total )
            .arg( syn_errors ) );

    write( QString("  counter checks = %1  counter errors = %2")
            .arg( ctr_total )
            .arg( ctr_errors ) );

    write( QString("  spi checks = %1 ...").arg( te_spi_total ) ) ;

    write( QString("  spi {0 1 2 3} errors = {%1  %2  %3  %4}")
            .arg( te_spi0_errors )
            .arg( te_spi1_errors )
            .arg( te_spi2_errors )
            .arg( te_spi3_errors ) );

    err = IM.neuropix_stopTest9();
    write( QString("Stop  testing: result = %1").arg( err ) );
}

#endif  // HAVE_IMEC


