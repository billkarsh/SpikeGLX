
#include "ui_WavePlanDialog.h"

#include "WavePlanCtl.h"
#include "Stim.h"
#include "Util.h"

#include <QFileDialog>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

WavePlanCtl::WavePlanCtl() : QDialog(0)
{
    wvUI = new Ui::WavePlanDialog;
    wvUI->setupUi( this );

    newBut();

    wvUI->dataTE->setPlainText(
        "Syntax Example (replace with yours):\n"
        "do N {\n"
        "  level( V, ms )\n"
        "  ramp( V1, V2, ms )\n"
        "  sin( A, B, Hz, ms )\n"
        "}" );

    ConnectUI( wvUI->devCB, SIGNAL(currentIndexChanged( int )), this, SLOT(devChanged()) );
    ConnectUI( wvUI->devVCB, SIGNAL(currentIndexChanged( int )), this, SLOT(devVChanged()) );
    ConnectUI( wvUI->newBut, SIGNAL(clicked()), this, SLOT(newBut()) );
    ConnectUI( wvUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( wvUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( wvUI->checkBut, SIGNAL(clicked()), this, SLOT(checkBut()) );
    ConnectUI( wvUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( wvUI->buttonBox, SIGNAL(rejected()), this, SLOT(closeBut()) );

    exec();
}


WavePlanCtl::~WavePlanCtl()
{
    if( wvUI ) {
        delete wvUI;
        wvUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void WavePlanCtl::devChanged()
{
    if( wvUI->devCB->currentText() == "OneBox" ) {
        wvUI->devVCB->setCurrentIndex( wvUI->devVCB->findText( "5" ) );
        wvUI->devVCB->setEnabled( false );
    }
    else
        wvUI->devVCB->setEnabled( true );
}


void WavePlanCtl::devVChanged()
{
    wvUI->waveVSB->setMaximum( wvUI->devVCB->currentText().toDouble() );
}


void WavePlanCtl::newBut()
{
    type    = "txt";
    binsamp = 0;

    wvUI->waveLbl->clear();
    wvUI->devCB->setCurrentIndex( wvUI->devCB->findText( "NI" ) );
    wvUI->freqCB->setCurrentIndex( wvUI->freqCB->findText( "10000" ) );
    wvUI->devVCB->setCurrentIndex( wvUI->devVCB->findText( "5" ) );
    wvUI->waveVSB->setValue( 2.5 );
    wvUI->dataTE->clear();
    wvUI->dataTE->setEnabled( true );
    info( "New" );

    devVChanged();
}


void WavePlanCtl::loadBut()
{
// ----
// Meta
// ----

// file

    QString wavedir = makeWaves();

    QString fn = QFileDialog::getOpenFileName(
                    this,
                    "Select Wave Metafile",
                    wavedir,
                    "Waves (*.meta)" );

    if( !fn.length() )
        return;

    QFileInfo   fi( fn );

    if( fi.dir().absolutePath() != wavedir ) {
        beep( "Please load and save in _Waves directory" );
        return;
    }

    fn = fi.baseName();

    wvUI->waveLbl->setText( fn );

// load

    WaveMeta    W;
    QString     s, err = W.readMetaFile( fn );
    int         sel;

    if( err.isEmpty() )
        err = W.stdMetaCheck( OBX_WAV_FRQ_MAX, OBX_WAV_BUF_MAX, fn );

// infer device

    if( W.dev_vmx != 5 )
        wvUI->devCB->setCurrentIndex( wvUI->devCB->findText( "NI" ) );

// freq

    s   = QString("%1").arg( W.freq );
    sel = wvUI->freqCB->findText( s );

    if( sel >= 0 )
        wvUI->freqCB->setCurrentIndex( sel );
    else {
        wvUI->freqCB->setEditable( true );
        wvUI->freqCB->setCurrentText( s );
    }

// wave_vmx

    wvUI->waveVSB->setValue( W.wav_vmx );

// dev_vmx

    s   = QString("%1").arg( W.dev_vmx );
    sel = wvUI->devVCB->findText( s );

    if( sel >= 0 )
        wvUI->devVCB->setCurrentIndex( sel );
    else {
        wvUI->devVCB->setEditable( true );
        wvUI->devVCB->setCurrentText( s );
    }

// ----
// Data
// ----

    type    = W.data_type;
    binsamp = W.nsamp;

    if( W.data_type != "txt" ) {

        wvUI->dataTE->setEnabled( false );
        wvUI->dataTE->setPlainText(
            QString("BINARY DATA: %1 %2 samples")
            .arg( W.nsamp ).arg( W.data_type ) );

        if( !err.isEmpty() )
            beep( err );
        else
            info( QString("%1 ms").arg( 1000.0 * W.nsamp / W.freq, 0, 'f', 3 ) );
    }
    else {
        QString texterr = W.readTextFile( s, fn );

        wvUI->dataTE->setEnabled( true );

        if( texterr.isEmpty() )
            wvUI->dataTE->setPlainText( s );
        else
            wvUI->dataTE->setPlainText( texterr );

        beep( err );
    }
}


void WavePlanCtl::helpBut()
{
    showHelp( "WavePlan_Help" );
}


bool WavePlanCtl::checkBut()
{
    info( "" );

// freq

    double  v = wvUI->freqCB->currentText().toDouble();
    if( v < 10000 || v > 500000 ) {
        beep( "Frequency not in range [10, 500] kHz" );
        return false;
    }

// data

    if( type == "txt" ) {

        WaveMeta    W;
        vec_i16     buf;
        QString     err;

        W.freq      = v;
        W.wav_vmx   = wvUI->waveVSB->value();
        W.dev_vmx   = wvUI->devVCB->currentText().toDouble();
        W.data_type = "txt";

        err = W.parseText( buf, wvUI->dataTE->toPlainText(), OBX_WAV_BUF_MAX );

        if( err.isEmpty() && W.nsamp == 0 )
            err = "Wave text: Empty.";

        if( !err.isEmpty() ) {
            beep( err );
            return false;
        }

        info(
            QString("OK: %1 samples %2 ms")
            .arg( W.nsamp )
            .arg( 1000.0 * W.nsamp / W.freq, 0, 'f', 3 ) );
    }
    else {
        info(
            QString("%1 ms")
            .arg( 1000.0 * binsamp / v, 0, 'f', 3 ) );
    }

    return true;
}


void WavePlanCtl::saveBut()
{
    if( !checkBut() )
        return;

// ----
// Meta
// ----

// file

    QString fnold   = wvUI->waveLbl->text(),
            wavedir = makeWaves();

    QString fn = QFileDialog::getSaveFileName(
                    this,
                    "Save Wave Metafile",
                    wavedir,
                    "Waves (*.meta)" );

    if( !fn.length() )
        return;

    QFileInfo   fi( fn );

    if( fi.dir().absolutePath() != wavedir ) {
        beep( "Please load and save in _Waves directory" );
        return;
    }

    fn = fi.baseName();

    wvUI->waveLbl->setText( fn );

    WaveMeta    W;
    QString     err;

    W.freq      = wvUI->freqCB->currentText().toDouble();
    W.wav_vmx   = wvUI->waveVSB->value();
    W.dev_vmx   = wvUI->devVCB->currentText().toDouble();
    W.nsamp     = binsamp;
    W.data_type = type;

    err = W.writeMetaFile( fn );
    if( !err.isEmpty() ) {
        beep( err );
        return;
    }

    if( type == "txt" ) {
        err = W.writeTextFile( fn, wvUI->dataTE->toPlainText() );
        if( !err.isEmpty() ) {
            beep( err );
            return;
        }
    }
    else if( fn != fnold ) {
        QFile::copy(
            wavedir + "/" + fnold + ".bin",
            wavedir + "/" + fn + ".bin" );
    }

    info( "Saved" );
}


void WavePlanCtl::closeBut()
{
    reject();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString WavePlanCtl::makeWaves()
{
    QString path = QString("%1/_Waves").arg( appPath() );
    QDir().mkdir( path );
    return path;
}


void WavePlanCtl::info( const QString &msg )
{
    wvUI->statusLbl->setText( msg );
}


void WavePlanCtl::beep( const QString &msg )
{
    info( msg );

    if( !msg.isEmpty() )
        ::Beep( 440, 200 );
}


