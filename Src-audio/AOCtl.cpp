
#include "ui_AOWindow.h"

#include "AOCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "SignalBlocker.h"
#include "AODevRtAudio.h"
#include "AODevSim.h"

#include <QKeyEvent>
#include <QMessageBox>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* User ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void AOCtl::EachStream::loadSettings( QSettings &S )
{
    loCutStr  = S.value( "loCut", "OFF" ).toString();
    hiCutStr  = S.value( "hiCut", "INF" ).toString();
    volume    = S.value( "volume", 1.0 ).toDouble();
    left      = S.value( "left", 0 ).toInt();
    right     = S.value( "right", 0 ).toInt();
}


void AOCtl::EachStream::saveSettings( QSettings &S ) const
{
    S.setValue( "loCut", loCutStr );
    S.setValue( "hiCut", hiCutStr );
    S.setValue( "volume", volume );
    S.setValue( "left", left );
    S.setValue( "right", right );
}


void AOCtl::User::loadSettings( bool remote )
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "" );

    STDSETTINGS( settings, file );

    int nq = p.stream_nq();

    each.resize( nq );

    for( int iq = 0; iq < nq; ++iq ) {

        settings.beginGroup( QString("AOCtl_%1").arg( p.iq2stream( iq ) ) );
            each[iq].loadSettings( settings );
        settings.endGroup();
    }

    settings.beginGroup( "AOCtl_All" );
    stream      = settings.value( "stream", p.jsip2stream( jsNI, 0 ) ).toString();
    useQf       = settings.value( "useQflt", true ).toBool();
    clickplay   = settings.value( "clickplay", false ).toBool();
    autoStart   = settings.value( "autoStart", false ).toBool();
}


void AOCtl::User::saveSettings( bool remote ) const
{
    QString file = QString("aoctl%1").arg( remote ? "remote" : "");

    STDSETTINGS( settings, file );

    for( int iq = 0, nq = each.size(); iq < nq; ++iq ) {

        settings.beginGroup( QString("AOCtl_%1").arg( p.iq2stream( iq ) ) );
            each[iq].saveSettings( settings );
        settings.endGroup();
    }

    settings.beginGroup( "AOCtl_All" );
    settings.setValue( "stream", stream );
    settings.setValue( "useQflt", useQf );
    settings.setValue( "clickplay", clickplay );
    settings.setValue( "autoStart", autoStart );
}

/* ---------------------------------------------------------------- */
/* Derived -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

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

    streamjs    = p.stream2jsip( streamip, usr.stream );
    srate       = p.stream_rate( streamjs, streamip );

    if( streamjs == jsIM ) {

        const CimCfg::PrbEach   &E = p.im.prbj[streamip];

        nNeural = E.imCumTypCnt[CimCfg::imSumNeural];
        maxInt  = E.roTbl->maxInt();
    }
    else if( streamjs == jsOB ) {
        nNeural = 0;
        maxInt  = MAX16BIT;
    }
    else {
        nNeural = p.ni.niCumTypCnt[CniCfg::niSumNeural];
        maxInt  = MAX16BIT;
    }

// Note that the latency metrics for simulated data acquisition
// tend to be higher because simulation is fast and the streams
// get filled fast (especially so for nidq).

    if( streamjs > 0 ) {
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

    const EachStream    &E = usr.each[p.stream2iq( usr.stream )];

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

AOCtl::AOCtl( const DAQ::Params &p )
    :   QWidget(0), p(p), usr(p), dlgShown(false)
{
    setAttribute( Qt::WA_DeleteOnClose, false );

#ifdef Q_OS_WIN
    aoDev = new AODevRtAudio( this, p );
#else
    aoDev = new AODevSim( this, p );
#endif

    ctorCheckAudioSupport();

    aoUI = new Ui::AOWindow;
    aoUI->setupUi( this );
    ConnectUI( aoUI->streamCB, SIGNAL(activated(int)), this, SLOT(streamCBChanged()) );
    ConnectUI( aoUI->qfChk, SIGNAL(clicked(bool)), this, SLOT(qfChecked(bool)) );
    ConnectUI( aoUI->leftSB, SIGNAL(valueChanged(int)), this, SLOT(leftSBChanged(int)) );
    ConnectUI( aoUI->rightSB, SIGNAL(valueChanged(int)), this, SLOT(rightSBChanged(int)) );
    ConnectUI( aoUI->loCB, SIGNAL(currentTextChanged(QString)), this, SLOT(loCBChanged(QString)) );
    ConnectUI( aoUI->hiCB, SIGNAL(currentTextChanged(QString)), this, SLOT(hiCBChanged(QString)) );
    ConnectUI( aoUI->cpChk, SIGNAL(clicked(bool)), this, SLOT(cpChecked(bool)) );
    ConnectUI( aoUI->volSB, SIGNAL(valueChanged(double)), this, SLOT(volSBChanged(double)) );
    ConnectUI( aoUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( aoUI->resetBut, SIGNAL(clicked()), this, SLOT(reset()) );
    ConnectUI( aoUI->stopBut, SIGNAL(clicked()), this, SLOT(stop()) );
    ConnectUI( aoUI->applyBut, SIGNAL(clicked()), this, SLOT(apply()) );

    restoreScreenState();
}


void AOCtl::predelete()
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

// Return true if selected stream matches current.
//
// Callable from any thread.
//
bool AOCtl::uniqueAIs( std::vector<int> &vAI, const QString &stream ) const
{
    vAI.clear();

// --------
// Matches?
// --------

    if( usr.stream != stream )
        return false;

// --------------------------------
// Fill channels for matched stream
// --------------------------------

    if( aoDev->readyForScans() ) {

        QMutexLocker        ml( &aoMtx );
        const EachStream    &E = usr.each[p.stream2iq( stream )];

        vAI.push_back( E.left );

        if( nDevChans > 1 && E.right != E.left )
            vAI.push_back( E.right );
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

    showNormal();

    dlgShown = true;

    return true;
}


void AOCtl::graphSetsChannel( int chan, int LBR, const QString &stream )
{
// Initialize settings and validate

    if( !dlgShown )
        showDialog();

    EachStream  &E = usr.each[p.stream2iq( stream )];

    if( LBR <= 0 )
        E.left  = chan;

    if( LBR >= 0 )
        E.right = chan;

    p.streamCB_selItem( aoUI->streamCB, stream, true );
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

    if( tLast && dt < 1.0 )
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

void AOCtl::reset( bool remote )
{
    stop();

// ---------
// Get state
// ---------

    usr.loadSettings( remote );

// ------
// Stream
// ------

    p.streamCB_fillRuntime( aoUI->streamCB );
    p.streamCB_selItem( aoUI->streamCB, usr.stream, true );

    aoUI->qfChk->setChecked( usr.useQf );

// -----
// Modes
// -----

    aoUI->cpChk->setChecked( usr.clickplay );
    aoUI->autoChk->setChecked( usr.autoStart );

// --------------------
// Observe dependencies
// --------------------

    streamCBChanged( false );
}


// Return empty QString or error string.
// Used for remote SETAUDIOPARAMS command.
//
// Callable from any thread.
//
QString AOCtl::cmdSrvSetsAOParamStr(
    const QString   &groupStr,
    const QString   &paramStr )
{
// ----------
// Supported?
// ----------

    if( !ctorErr.isEmpty() )
        return ctorErr;

// ----
// Stop
// ----

    stop();

// -------------------------
// Save settings to own file
// -------------------------

    QMutexLocker    ml( &aoMtx );

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

    int iq = aoUI->streamCB->currentIndex();

    if( usr.each[iq].loCutStr != aoUI->loCB->currentText() ) {

        return QString("Low filter cutoff [%1] not supported.")
                .arg( usr.each[iq].loCutStr );
    }

    if( usr.each[iq].hiCutStr != aoUI->hiCB->currentText() ) {

        return QString("High filter cutoff [%1] not supported.")
                .arg( usr.each[iq].hiCutStr );
    }

// -------------------
// Standard validation
// -------------------

    QString err;

    ml.unlock();    // allow valid() to relock

    if( !valid( err, true ) )
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

    int iq = aoUI->streamCB->currentIndex(),
        ip,
        js = p.iq2jsip( ip, iq ),
        nC = p.stream_nChans( js, ip );

// --
// Qf
// --

    aoUI->qfChk->setEnabled( js == jsIM );

// --------
// Channels
// --------

    EachStream  &E = usr.each[iq];

    if( E.left >= nC )
        E.left = 0;

    if( E.right >= nC )
        E.right = 0;

    aoUI->leftSB->setMaximum( nC );
    aoUI->rightSB->setMaximum( nC );

    aoUI->leftSB->setValue( E.left );
    aoUI->rightSB->setValue( E.right );

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


void AOCtl::qfChecked( bool checked )
{
    usr.useQf = checked;
    liveChange();
}


void AOCtl::leftSBChanged( int val )
{
    usr.each[aoUI->streamCB->currentIndex()].left = val;
    liveChange();
}


void AOCtl::rightSBChanged( int val )
{
    usr.each[aoUI->streamCB->currentIndex()].right = val;
    liveChange();
}


void AOCtl::loCBChanged( const QString &str )
{
    usr.each[aoUI->streamCB->currentIndex()].loCutStr = str;
    aoUI->hiCB->setEnabled( str != "OFF" );
    liveChange();
}


void AOCtl::hiCBChanged( const QString &str )
{
    usr.each[aoUI->streamCB->currentIndex()].hiCutStr = str;
    liveChange();
}


void AOCtl::cpChecked( bool checked )
{
    usr.clickplay = checked;
    liveChange();
}


void AOCtl::volSBChanged( double val )
{
    usr.each[aoUI->streamCB->currentIndex()].volume = val;
    liveChange();
}


void AOCtl::helpBut()
{
    showHelp( "Audio_Help" );
}


void AOCtl::stop()
{
    MainApp *app = mainApp();

    if( app->isInitialized() )
        app->getRun()->aoStop();
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

    if( e->isAccepted() ) {
        dlgShown = false;
        emit closed( this );
    }
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
        Warning() << "Audio warning: " << ctorErr;
}


void AOCtl::str2RemoteIni( const QString &groupStr, const QString &prmStr )
{
    STDSETTINGS( settings, "aoctlremote" );
    settings.beginGroup( groupStr );

    QTextStream ts( prmStr.toUtf8(), QIODevice::ReadOnly | QIODevice::Text );
    QString     line;

    while( !(line = ts.readLine( 65536 )).isNull() ) {

        int eq = line.indexOf( "=" );

        if( eq > 0 && eq < line.length() - 1 ) {

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


bool AOCtl::valid( QString &err, bool remote )
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

    if( !remote && !dlgShown )
        reset();

    usr.stream      = aoUI->streamCB->currentText();
    usr.useQf       = aoUI->qfChk->isChecked();
    usr.clickplay   = aoUI->cpChk->isChecked();
    usr.autoStart   = aoUI->autoChk->isChecked();

// Stream available?

    int ip, js = p.stream2jsip( ip, usr.stream );

    if( js < 0 ) {
        err = "Selected audio stream is not enabled.";
        return false;
    }

// Channels legal?

    int iq = aoUI->streamCB->currentIndex(),
        nC = p.stream_nChans( js, ip );

    const EachStream    &E = usr.each[iq];

    if( E.left > nC ) {
        err = QString(
                "Left channel (%1) exceeds %2 channel count (%3).")
                .arg( E.left )
                .arg( usr.stream )
                .arg( nC );
        return false;
    }

    if( E.right > nC ) {
        err = QString(
                "Right channel (%1) exceeds %2 channel count (%3).")
                .arg( E.right )
                .arg( usr.stream )
                .arg( nC );
        return false;
    }

// Good

    usr.saveSettings();
    return true;
}


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void AOCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Audio/geometry" ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void AOCtl::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Audio/geometry", geomSave( this, 26, 5 ) );
}


