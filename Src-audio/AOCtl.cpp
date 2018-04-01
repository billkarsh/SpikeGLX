
#include "ui_AOWindow.h"

#include "AOCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "HelpWindow.h"
#include "SignalBlocker.h"
#include "AODevRtAudio.h"
#include "AODevSim.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* User ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AOCtl::User::loadSettings( bool remote )
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, file );
    settings.beginGroup( "AOCtl" );

    stream      = settings.value( "stream", "nidq" ).toString();
    loCutStr    = settings.value( "loCut", "OFF" ).toString();
    hiCutStr    = settings.value( "hiCut", "INF" ).toString();
    volume      = settings.value( "volume", 1.0 ).toDouble();
    imLeft      = settings.value( "imLeft", 0 ).toInt();
    imRight     = settings.value( "imRight", 0 ).toInt();
    niLeft      = settings.value( "niLeft", 0 ).toInt();
    niRight     = settings.value( "niRight", 0 ).toInt();
    autoStart   = settings.value( "autoStart", false ).toBool();
}


void AOCtl::User::saveSettings( bool remote ) const
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, file );
    settings.beginGroup( "AOCtl" );

    settings.setValue( "stream", stream );
    settings.setValue( "loCut", loCutStr );
    settings.setValue( "hiCut", hiCutStr );
    settings.setValue( "volume", volume );
    settings.setValue( "imLeft", imLeft );
    settings.setValue( "imRight", imRight );
    settings.setValue( "niLeft", niLeft );
    settings.setValue( "niRight", niRight );
    settings.setValue( "autoStart", autoStart );
}

/* ---------------------------------------------------------------- */
/* Derived -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define MAX10BIT    512
#define MAX16BIT    32768


// Derive the data actually needed within the audio device callback
// functions. This must be called any time AO parameters change.
//
void AOCtl::Derived::usr2drv( AOCtl *aoC )
{
// --------
// Defaults
// --------

    const DAQ::Params   &p      = aoC->p;
    const AOCtl::User   &usr    = aoC->usr;

// ------
// Stream
// ------

    streamID    = (usr.stream == "nidq" ? -1 : p.streamID( usr.stream ));

    srate       = (streamID >= 0 ? p.im.srate : p.ni.srate);

    nNeural     = (streamID >= 0 ?
                    p.im.imCumTypCnt[CimCfg::imSumNeural]
                    : p.ni.niCumTypCnt[CniCfg::niSumNeural]);

    maxBits     = (streamID >= 0 ? MAX10BIT : MAX16BIT);

// Note that the latency metrics for simulated data acquisition
// tend to be higher because simulation is fast and the streams
// get filled fast (especially so for nidq).

    if( streamID >= 0 ) {
#ifdef HAVE_IMEC
    maxLatency = 60;
#else
    maxLatency = 1000;
#endif
    }
    else {
#ifdef HAVE_NIDAQmx
    maxLatency = 20;
#else
    maxLatency = 1000;
#endif
    }

// --------
// Channels
// --------

    if( streamID >= 0 ) {
        lChan   = usr.imLeft;
        rChan   = usr.imRight;
    }
    else {
        lChan   = usr.niLeft;
        rChan   = usr.niRight;
    }

// ------
// Filter
// ------

    loCut   = -1;
    hiCut   = -1;

    if( usr.loCutStr != "OFF" ) {

        if( usr.loCutStr != "0" ) {

            loCut = usr.loCutStr.toDouble();
            hipass.setBiquad( bq_type_highpass, loCut / srate );
        }

        if( usr.hiCutStr != "INF" ) {

            hiCut = usr.hiCutStr.toDouble();
            lopass.setBiquad( bq_type_lowpass, hiCut / srate );
        }
    }

// ------
// Volume
// ------

    lVol = usr.volume;
    rVol = usr.volume;

    // Boost by type

    if( lChan < nNeural )
        lVol *= 64;
    else
        lVol *= 4;

    if( rChan < nNeural )
        rVol *= 64;
    else
        rVol *= 4;
}


qint16 AOCtl::Derived::vol( qint16 val, double vol ) const
{
    return qBound( SHRT_MIN, int(val * vol), SHRT_MAX );
}

/* ---------------------------------------------------------------- */
/* AOCtl ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

AOCtl::AOCtl( const DAQ::Params &p, QWidget *parent )
    :   QWidget(parent), p(p), helpDlg(0), dlgShown(false)
{
#if 1
    aoDev = new AODevRtAudio( this, p );
#else
    aoDev = new AODevSim( this, p );
#endif

    ctorCheckAudioSupport();

    aoUI = new Ui::AOWindow;
    aoUI->setupUi( this );
    ConnectUI( aoUI->streamCB, SIGNAL(activated(QString)), this, SLOT(streamCBChanged()) );
    ConnectUI( aoUI->leftSB, SIGNAL(valueChanged(int)), this, SLOT(liveChange()) );
    ConnectUI( aoUI->rightSB, SIGNAL(valueChanged(int)), this, SLOT(liveChange()) );
    ConnectUI( aoUI->loCB, SIGNAL(currentIndexChanged(QString)), this, SLOT(loCBChanged(QString)) );
    ConnectUI( aoUI->hiCB, SIGNAL(currentIndexChanged(int)), this, SLOT(liveChange()) );
    ConnectUI( aoUI->volSB, SIGNAL(valueChanged(double)), this, SLOT(liveChange()) );
    ConnectUI( aoUI->helpBut, SIGNAL(clicked()), this, SLOT(showHelp()) );
    ConnectUI( aoUI->resetBut, SIGNAL(clicked()), this, SLOT(reset()) );
    ConnectUI( aoUI->stopBut, SIGNAL(clicked()), this, SLOT(stop()) );
    ConnectUI( aoUI->applyBut, SIGNAL(clicked()), this, SLOT(apply()) );

    restoreScreenState();
}


AOCtl::~AOCtl()
{
    saveScreenState();

    if( aoDev ) {
        delete aoDev;
        aoDev = 0;
    }

    if( helpDlg ) {
        delete helpDlg;
        helpDlg = 0;
    }

    if( aoUI ) {
        delete aoUI;
        aoUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if selected stream is imec.
//
// Callable from any thread.
//
bool AOCtl::uniqueAIs( QVector<int> &vAI ) const
{
    bool    isImec = (usr.stream != "nidq");

    vAI.clear();

// --------------------------------
// Fill channels for matched stream
// --------------------------------

    if( aoDev->readyForScans() ) {

        aoMtx.lock();

            if( isImec ) {

                vAI.push_back( usr.imLeft );

                if( nDevChans > 1 && usr.imRight != usr.imLeft )
                    vAI.push_back( usr.imRight );
            }
            else {

                vAI.push_back( usr.niLeft );

                if( nDevChans > 1 && usr.niRight != usr.niLeft )
                    vAI.push_back( usr.niRight );
            }

        aoMtx.unlock();
    }

    return isImec;
}


bool AOCtl::showDialog( QWidget *parent )
{
    ctorCheckAudioSupport();    // User may have corrected an issue

    if( !ctorErr.isEmpty() ) {

        QMessageBox::critical( parent, "Audio Hardware Error", ctorErr );
        return false;
    }

    if( !mainApp()->cfgCtl()->validated ) {

        QMessageBox::critical( parent, "Audio Settings Error",
            "Run parameters never validated;"
            " do that using Configure dialog." );
        return false;
    }

    bool    stereo = nDevChans > 1;

    aoUI->rChanLbl->setEnabled( stereo );
    aoUI->rightSB->setEnabled( stereo );

    if( !aoDev->readyForScans() )
        reset();

    show();

    dlgShown = true;

    return true;
}


void AOCtl::graphSetsChannel( int chan, bool isLeft, bool isImec )
{
// Initialize settings and validate

    if( !dlgShown )
        showDialog();

// Quietly set new params

    SignalBlocker   b0(aoUI->streamCB),
                    b1(aoUI->leftSB),
                    b2(aoUI->rightSB);

    aoUI->streamCB->setCurrentIndex( !isImec );
    streamCBChanged( false );

    if( isLeft )
        aoUI->leftSB->setValue( chan );
    else
        aoUI->rightSB->setValue( chan );

// Apply

    apply();
}


// Signal audio restart.
//
// It may take time for the queued restart request to take
// effect, so we moderate the frequency of issued requests.
//
void AOCtl::restart()
{
    static double   tLast = 0;

    double  t   = getTime(),
            dt  = t - tLast;

    if( dt < 1.0 )
        return;

    tLast = t;

    Run *run = mainApp()->getRun();

    if( !run->isRunning() )
        return;

    QMetaObject::invokeMethod( run, "aoStart", Qt::QueuedConnection );

    Debug() << QString("Audio restarted after %1 sec").arg( int(dt) );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return QString::null or error string.
// Used for remote SETAUDIOPARAMS command.
//
// Callable from any thread.
//
QString AOCtl::cmdSrvSetsAOParamStr( const QString &paramString )
{
    QMutexLocker    ml( &aoMtx );

// ----------
// Supported?
// ----------

    if( !ctorErr.isEmpty() )
        return ctorErr;

// -------------------------
// Save settings to own file
// -------------------------

// start with current set

    usr.saveSettings( true );

// overwrite remote subset

    str2RemoteIni( paramString );

// -----------------------
// Transfer them to dialog
// -----------------------

    reset( true );

// --------------------------
// Remote-specific validation
// --------------------------

// With a dialog, user is constrained to choose items
// we've put into CB controls. Remote case lacks that
// constraint, so we check existence of CB items here.

    if( usr.stream != aoUI->streamCB->currentText() ) {

        return QString("Audio does not support stream [%1].")
                .arg( usr.stream );
    }

    if( usr.loCutStr != aoUI->loCB->currentText() ) {

        return QString("Low filter cutoff [%1] not supported.")
                .arg( usr.loCutStr );
    }

    if( usr.hiCutStr != aoUI->hiCB->currentText() ) {

        return QString("High filter cutoff [%1] not supported.")
                .arg( usr.hiCutStr );
    }

// -------------------
// Standard validation
// -------------------

    QString err;

    ml.unlock();    // allow valid() to relock

    if( !valid( err ) )
        err = QString("Audio Parameter Error [%1]").arg( err );

    return err;
}


void AOCtl::streamCBChanged( bool live )
{
    int max, left, right;

    if( !aoUI->streamCB->currentIndex() ) {

        max     = p.im.imCumTypCnt[CimCfg::imSumAll] - 1;
        left    = usr.imLeft;
        right   = usr.imRight;
    }
    else {

        max     = p.ni.niCumTypCnt[CniCfg::niSumAll] - 1;
        left    = usr.niLeft;
        right   = usr.niRight;
    }

    if( left > max )
        left = 0;

    if( right > max )
        right = 0;

    aoUI->leftSB->setMaximum( max );
    aoUI->rightSB->setMaximum( max );

    aoUI->leftSB->setValue( left );
    aoUI->rightSB->setValue( right );

    if( live )
        liveChange();
}


void AOCtl::loCBChanged( const QString &str )
{
    aoUI->hiCB->setEnabled( str != "OFF" );

    liveChange();
}


void AOCtl::liveChange()
{
    if( aoDev->readyForScans() )
        apply();
}


void AOCtl::showHelp()
{
    if( !helpDlg ) {

        helpDlg = new HelpWindow(
                        "Audio Help",
                        "CommonResources/Audio.html",
                        this );
    }

    helpDlg->show();
    helpDlg->activateWindow();
}


void AOCtl::reset( bool remote )
{
    if( mainApp()->isInitialized() )
        stop();

// ---------
// Get state
// ---------

    usr.loadSettings( remote );

// ------
// Stream
// ------

    aoUI->streamCB->setCurrentIndex( usr.stream == "nidq" );

// ------
// Checks
// ------

    aoUI->autoChk->setChecked( usr.autoStart );

// ------
// Filter
// ------

    int sel;

// default OFF = first
    sel = aoUI->loCB->findText( usr.loCutStr );
    aoUI->loCB->setCurrentIndex( sel > -1 ? sel : 0 );

// default INF = last
    sel = aoUI->hiCB->findText( usr.hiCutStr );
    aoUI->hiCB->setCurrentIndex( sel > -1 ? sel : aoUI->hiCB->count()-1 );

    aoUI->hiCB->setEnabled( aoUI->loCB->currentIndex() > 0 );

// ------
// Volume
// ------

    aoUI->volSB->setValue( usr.volume );

// --------------------
// Observe dependencies
// --------------------

    streamCBChanged();
}


void AOCtl::stop()
{
    mainApp()->getRun()->aoStop();
}


void AOCtl::apply()
{
    Run     *run = mainApp()->getRun();
    QString err;

    run->aoStop();

    if( valid( err ) )
        run->aoStart();
    else
        QMessageBox::critical( this, "Audio Parameter Error", err );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void AOCtl::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QWidget::keyPressEvent( e );
}


void AOCtl::closeEvent( QCloseEvent *e )
{
    if( helpDlg ) {
        delete helpDlg;
        helpDlg = 0;
    }

    QWidget::closeEvent( e );

    if( e->isAccepted() )
        emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AOCtl::ctorCheckAudioSupport()
{
    nDevChans = aoDev->getOutChanCount( ctorErr );

    if( ctorErr.isEmpty() && nDevChans <= 0 )
        ctorErr = "No audio output hardware found on this machine.";

    if( !ctorErr.isEmpty() )
        Warning() << "Audio error: " << ctorErr;
}


void AOCtl::str2RemoteIni( const QString str )
{
    STDSETTINGS( settings, "aoCtlremote" );
    settings.beginGroup( "AOCtl" );

    QTextStream ts( str.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString     line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > -1 && eq < line.length() ) {

            QString n = line.left( eq ).trimmed(),
                    v = line.mid( eq + 1 ).trimmed();

            settings.setValue( n, v );
        }
    }
}


bool AOCtl::valid( QString &err )
{
    if( !ctorErr.isEmpty() ) {

        err = ctorErr;
        return false;
    }

    if( !mainApp()->cfgCtl()->validated ) {
        err =
            "Run parameters never validated;"
            " do that using Configure dialog.";
        return false;
    }

    QMutexLocker    ml( &aoMtx );

// If never visited dialog before, fill with last known

    if( !dlgShown )
        reset();

    usr.stream      = aoUI->streamCB->currentText();
    usr.loCutStr    = aoUI->loCB->currentText();
    usr.hiCutStr    = aoUI->hiCB->currentText();
    usr.volume      = aoUI->volSB->value();
    usr.autoStart   = aoUI->autoChk->isChecked();

    if( usr.stream == "nidq" ) {
        usr.niLeft  = aoUI->leftSB->value();
        usr.niRight = aoUI->rightSB->value();
    }
    else {
        usr.imLeft  = aoUI->leftSB->value();
        usr.imRight = aoUI->rightSB->value();
    }

// Stream available?

    if( (usr.stream == "nidq" && !p.ni.enabled)
        || (usr.stream != "nidq" && !p.im.enabled) ) {

        err = "Selected audio stream is not enabled.";
        return false;
    }

// Channels legal?

    if( usr.stream == "nidq" ) {

        int n16 = p.ni.niCumTypCnt[CniCfg::niSumAll];

        if( usr.niLeft > n16 ) {
            err = QString(
                    "Left channel (%1) exceeds nidq channel count (%2).")
                    .arg( usr.niLeft )
                    .arg( n16 );
            return false;
        }

        if( usr.niRight > n16 ) {
            err = QString(
                    "Right channel (%1) exceeds nidq channel count (%2).")
                    .arg( usr.niRight )
                    .arg( n16 );
            return false;
        }
    }
    else {

        int n16 = p.im.imCumTypCnt[CimCfg::imSumAll];

        if( usr.imLeft > n16 ) {
            err = QString(
                    "Left channel (%1) exceeds imec channel count (%2).")
                    .arg( usr.imLeft )
                    .arg( n16 );
            return false;
        }

        if( usr.imRight > n16 ) {
            err = QString(
                    "Right channel (%1) exceeds imec channel count (%2).")
                    .arg( usr.imRight )
                    .arg( n16 );
            return false;
        }
    }

// Good

    usr.saveSettings();
    return true;
}


void AOCtl::saveScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "AUDIO/geometry", saveGeometry() );
}


void AOCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "AUDIO/geometry" ).toByteArray() ) ) {

        // BK: Get size from form, or do nothing.
    }
}


