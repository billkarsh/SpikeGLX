
#include "ui_AOWindow.h"

#include "AOCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "GUIControls.h"
#include "SignalBlocker.h"
#include "AODevRtAudio.h"
#include "AODevSim.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* User ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AOCtl::User::loadSettings( int nImec, bool remote )
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, file );

    int n = 1 + nImec;

    each.resize( n );

    for( int i = 0; i < n; ++i ) {

        EachStream  &E  = each[i];
        int         k   = i - 1;

        settings.beginGroup( QString("AOCtl_Stream_%1").arg( k ) );
        E.left      = settings.value( "left", 0 ).toInt();
        E.right     = settings.value( "right", 0 ).toInt();
        E.loCutStr  = settings.value( "loCut", "OFF" ).toString();
        E.hiCutStr  = settings.value( "hiCut", "INF" ).toString();
        E.volume    = settings.value( "volume", 1.0 ).toDouble();
        settings.endGroup();
    }

    settings.beginGroup( "AOCtl_All" );
    stream      = settings.value( "stream", "nidq" ).toString();
    autoStart   = settings.value( "autoStart", false ).toBool();
}


void AOCtl::User::saveSettings( bool remote ) const
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, file );

    for( int i = 0, n = each.size(); i < n; ++i ) {

        const EachStream    &E  = each[i];
        int                 k   = i - 1;

        settings.beginGroup( QString("AOCtl_Stream_%1").arg( k ) );
        settings.setValue( "left", E.left );
        settings.setValue( "right", E.right );
        settings.setValue( "loCut", E.loCutStr );
        settings.setValue( "hiCut", E.hiCutStr );
        settings.setValue( "volume", E.volume );
        settings.endGroup();
    }

    settings.beginGroup( "AOCtl_All" );
    settings.setValue( "stream", stream );
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

    streamID = (usr.stream == "nidq" ? -1 : p.streamID( usr.stream ));

    if( streamID >= 0 ) {

        const CimCfg::AttrEach  &E = p.im.each[streamID];

        srate   = E.srate;
        nNeural = E.imCumTypCnt[CimCfg::imSumNeural];
        maxBits = MAX10BIT;
    }
    else {
        srate   = p.ni.srate;
        nNeural = p.ni.niCumTypCnt[CniCfg::niSumNeural];
        maxBits = MAX16BIT;
    }

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

    const EachStream    &E = usr.each[streamID+1];

    lChan = E.left;
    rChan = E.right;

// ------
// Filter
// ------

    loCut = -1;
    hiCut = -1;

    if( E.loCutStr != "OFF" ) {

        if( E.loCutStr != "0" ) {

            loCut = E.loCutStr.toDouble();
            hipass.setBiquad( bq_type_highpass, loCut / srate );
        }

        if( E.hiCutStr != "INF" ) {

            hiCut = E.hiCutStr.toDouble();
            lopass.setBiquad( bq_type_lowpass, hiCut / srate );
        }
    }

// ------
// Volume
// ------

    lVol = E.volume;
    rVol = E.volume;

    // Boost by type

    if( lChan < nNeural )
        lVol *= 64;
    else
        lVol *= 0.5;

    if( rChan < nNeural )
        rVol *= 64;
    else
        rVol *= 0.5;
}


qint16 AOCtl::Derived::vol( qint16 val, double vol ) const
{
    return qBound( SHRT_MIN, int(val * vol), SHRT_MAX );
}

/* ---------------------------------------------------------------- */
/* AOCtl ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

AOCtl::AOCtl( const DAQ::Params &p, QWidget *parent )
    :   QWidget(parent), p(p), dlgShown(false)
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
    ConnectUI( aoUI->leftSB, SIGNAL(valueChanged(int)), this, SLOT(leftSBChanged(int)) );
    ConnectUI( aoUI->rightSB, SIGNAL(valueChanged(int)), this, SLOT(rightSBChanged(int)) );
    ConnectUI( aoUI->loCB, SIGNAL(currentIndexChanged(QString)), this, SLOT(loCBChanged(QString)) );
    ConnectUI( aoUI->hiCB, SIGNAL(currentIndexChanged(QString)), this, SLOT(hiCBChanged(QString)) );
    ConnectUI( aoUI->volSB, SIGNAL(valueChanged(double)), this, SLOT(volSBChanged(double)) );
    ConnectUI( aoUI->helpBut, SIGNAL(clicked()), this, SLOT(help()) );
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

    if( aoUI ) {
        delete aoUI;
        aoUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if selected stream matches streamID {-1=nidq}.
//
// Callable from any thread.
//
bool AOCtl::uniqueAIs( QVector<int> &vAI, int streamID ) const
{
    vAI.clear();

// --------
// Matches?
// --------

    if( streamID < 0 ) {

        if( usr.stream != "nidq" )
            return false;
    }
    else if( usr.stream != QString("imec%1").arg( streamID ) )
        return false;

// --------------------------------
// Fill channels for matched stream
// --------------------------------

    if( aoDev->readyForScans() ) {

        aoMtx.lock();

            const EachStream    &E = usr.each[streamID+1];

            vAI.push_back( E.left );

            if( nDevChans > 1 && E.right != E.left )
                vAI.push_back( E.right );

        aoMtx.unlock();
    }

    return true;
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


// streamID should be -1 for nidq, or imec stream index.
//
void AOCtl::graphSetsChannel( int chan, bool isLeft, int streamID )
{
// Initialize settings and validate

    if( !dlgShown )
        showDialog();

    EachStream  &E = usr.each[streamID+1];

    if( isLeft )
        E.left  = chan;
    else
        E.right = chan;

    aoUI->streamCB->setCurrentIndex( streamID + 1 );
    streamCBChanged( false );
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
QString AOCtl::cmdSrvSetsAOParamStr(
    const QString   &groupStr,
    const QString   &paramStr )
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

    str2RemoteIni( groupStr, paramStr );

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

    int idx = aoUI->streamCB->currentIndex();

    if( usr.each[idx].loCutStr != aoUI->loCB->currentText() ) {

        return QString("Low filter cutoff [%1] not supported.")
                .arg( usr.each[idx].loCutStr );
    }

    if( usr.each[idx].hiCutStr != aoUI->hiCB->currentText() ) {

        return QString("High filter cutoff [%1] not supported.")
                .arg( usr.each[idx].hiCutStr );
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
    SignalBlocker   b0(aoUI->leftSB),
                    b1(aoUI->rightSB),
                    b2(aoUI->loCB),
                    b3(aoUI->hiCB),
                    b4(aoUI->volSB);

    int idx, max, left, right;

    idx = aoUI->streamCB->currentIndex();

// --------
// Channels
// --------

    max = (!idx ?
            p.ni.niCumTypCnt[CniCfg::niSumAll] :
            p.im.each[idx-1].imCumTypCnt[CimCfg::imSumAll]) - 1;

    const EachStream    &E = usr.each[idx];

    left    = E.left;
    right   = E.right;

    if( left > max )
        left = 0;

    if( right > max )
        right = 0;

    aoUI->leftSB->setMaximum( max );
    aoUI->rightSB->setMaximum( max );

    aoUI->leftSB->setValue( left );
    aoUI->rightSB->setValue( right );

// -------
// Filters
// -------

// default OFF = first
    int sel = aoUI->loCB->findText( E.loCutStr );
    aoUI->loCB->setCurrentIndex( sel > -1 ? sel : 0 );

// default INF = last
    sel = aoUI->hiCB->findText( E.hiCutStr );
    aoUI->hiCB->setCurrentIndex( sel > -1 ? sel : aoUI->hiCB->count()-1 );

    aoUI->hiCB->setEnabled( aoUI->loCB->currentIndex() > 0 );

// ------
// Volume
// ------

    aoUI->volSB->setValue( E.volume );

// ------
// Update
// ------

    if( live )
        liveChange();
}


void AOCtl::leftSBChanged( int val )
{
    int idx = aoUI->streamCB->currentIndex();

    usr.each[idx].left = val;

    liveChange();
}


void AOCtl::rightSBChanged( int val )
{
    int idx = aoUI->streamCB->currentIndex();

    usr.each[idx].right = val;

    liveChange();
}


void AOCtl::loCBChanged( const QString &str )
{
    int idx = aoUI->streamCB->currentIndex();

    usr.each[idx].loCutStr = str;

    aoUI->hiCB->setEnabled( str != "OFF" );

    liveChange();
}


void AOCtl::hiCBChanged( const QString &str )
{
    int idx = aoUI->streamCB->currentIndex();

    usr.each[idx].hiCutStr = str;

    liveChange();
}


void AOCtl::volSBChanged( double val )
{
    int idx = aoUI->streamCB->currentIndex();

    usr.each[idx].volume = val;

    liveChange();
}


void AOCtl::help()
{
    showHelp( "Audio_Help" );
}


void AOCtl::reset( bool remote )
{
    if( mainApp()->isInitialized() )
        stop();

// ---------
// Get state
// ---------

    usr.loadSettings( p.im.get_nProbes(), remote );

// ------
// Stream
// ------

    FillExtantStreamCB( aoUI->streamCB, p.ni.enabled, p.im.get_nProbes() );
    SelStreamCBItem( aoUI->streamCB, usr.stream );

// ----
// Auto
// ----

    aoUI->autoChk->setChecked( usr.autoStart );

// --------------------
// Observe dependencies
// --------------------

    streamCBChanged( false );
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


void AOCtl::str2RemoteIni( const QString &groupStr, const QString prmStr )
{
    STDSETTINGS( settings, "aoctlremote" );
    settings.beginGroup( groupStr );

    QTextStream ts( prmStr.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
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


void AOCtl::liveChange()
{
    if( aoDev->readyForScans() )
        apply();
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
    usr.autoStart   = aoUI->autoChk->isChecked();

// Stream available?

    if( (usr.stream == "nidq" && !p.ni.enabled)
        || (usr.stream != "nidq" && !p.im.enabled) ) {

        err = "Selected audio stream is not enabled.";
        return false;
    }

// Channels legal?

    int idx = aoUI->streamCB->currentIndex(),
        lim = p.im.get_nProbes() - 1,
        n16;

    if( idx > 0 && idx - 1 > lim ) {

        err = QString(
                "Imec stream %1 exceeds limit %2.")
                .arg( idx - 1 )
                .arg( lim );
        return false;
    }

    if( !idx )
        n16 = p.ni.niCumTypCnt[CniCfg::niSumAll];
    else
        n16 = p.im.each[idx-1].imCumTypCnt[CimCfg::imSumAll];

    const EachStream    &E = usr.each[idx];

    if( E.left > n16 ) {
        err = QString(
                "Left channel (%1) exceeds %2 channel count (%3).")
                .arg( E.left )
                .arg( usr.stream )
                .arg( n16 );
        return false;
    }

    if( E.right > n16 ) {
        err = QString(
                "Right channel (%1) exceeds %2 channel count (%3).")
                .arg( E.right )
                .arg( usr.stream )
                .arg( n16 );
        return false;
    }

// Good

    usr.saveSettings();
    return true;
}


void AOCtl::saveScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Audio/geometry", saveGeometry() );
}


void AOCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Audio/geometry" ).toByteArray() ) ) {

        // BK: Get size from form, or do nothing.
    }
}


