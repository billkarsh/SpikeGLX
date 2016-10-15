
#include "Run.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "AIQ.h"
#include "IMReader.h"
#include "NIReader.h"
#include "GateTCP.h"
#include "TrigTCP.h"
#include "GraphsWindow.h"
#include "GraphFetcher.h"
#include "AOCtl.h"
#include "AOFetcher.h"
#include "Version.h"

#include <QAction>
#include <QMessageBox>




/* ---------------------------------------------------------------- */
/* Ctor ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Run::Run( MainApp *app )
    :   QObject(0), app(app), imQ(0), niQ(0),
        graphsWindow(0), graphFetcher(0),
        aoFetcher(0), imReader(0), niReader(0),
        gate(0), trg(0), running(false)
{
}

/* ---------------------------------------------------------------- */
/* Owned GraphsWindow ops ----------------------------------------- */
/* ---------------------------------------------------------------- */

bool Run::grfIsUsrOrderIm()
{
    QMutexLocker    ml( &runMtx );
    bool            isUsrOrder = false;

    if( graphsWindow ) {

        QMetaObject::invokeMethod(
            graphsWindow,
            "remoteIsUsrOrderIm",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, isUsrOrder) );
    }

    return isUsrOrder;
}


bool Run::grfIsUsrOrderNi()
{
    QMutexLocker    ml( &runMtx );
    bool            isUsrOrder = false;

    if( graphsWindow ) {

        QMetaObject::invokeMethod(
            graphsWindow,
            "remoteIsUsrOrderNi",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, isUsrOrder) );
    }

    return isUsrOrder;
}


void Run::grfRemoteSetsRunName( const QString &fn )
{
    QMutexLocker    ml( &runMtx );

    if( graphsWindow )
        graphsWindow->remoteSetRunLE( fn );
}


void Run::grfPause( bool paused )
{
    QMutexLocker    ml( &runMtx );

    if( graphFetcher )
        graphFetcher->pause( paused );
}


void Run::grfSetFocus()
{
    QMutexLocker    ml( &runMtx );

    if( graphsWindow )
        graphsWindow->setFocus( Qt::OtherFocusReason );
}


void Run::grfShowHide()
{
    QMutexLocker    ml( &runMtx );

    if( !graphsWindow )
        return;

    if( graphsWindow->isHidden() ) {
        graphsWindow->eraseGraphs();
        graphsWindow->show();
    }
    else {
        bool    hadfocus = (QApplication::focusWidget() == graphsWindow);

        graphsWindow->hide();

        if( hadfocus )
            app->giveFocus2Console();
    }
}


void Run::grfUpdateWindowTitles()
{
// ------------
// Craft a name
// ------------

    Params  &p  = app->cfgCtl()->acceptedParams;
    QString run = p.sns.runName,
            stat;

    if( running )
        stat = "RUNNING - " + run;
    else
        stat = "READY";

// -----------------------
// Apply to console window
// -----------------------

    app->updateConsoleTitle( stat );

// ----------------------
// Apply to graphs window
// ----------------------

    if( graphsWindow ) {

        graphsWindow->setWindowTitle(
                QString(APPNAME " Graphs - %1").arg( stat ) );

        app->win.copyWindowTitleToAction( graphsWindow );
    }
}

/* ---------------------------------------------------------------- */
/* Owned AIStream ops --------------------------------------------- */
/* ---------------------------------------------------------------- */

quint64 Run::getImScanCount() const
{
    if( isRunning() && imQ ) {

        QMutexLocker    ml( &runMtx );
        return imQ->curCount();
    }

    return 0;
}


quint64 Run::getNiScanCount() const
{
    if( isRunning() && niQ ) {

        QMutexLocker    ml( &runMtx );
        return niQ->curCount();
    }

    return 0;
}


const AIQ* Run::getImQ() const
{
    if( isRunning() ) {

        QMutexLocker    ml( &runMtx );
        return imQ;
    }

    return 0;
}


const AIQ* Run::getNiQ() const
{
    if( isRunning() ) {

        QMutexLocker    ml( &runMtx );
        return niQ;
    }

    return 0;
}

/* ---------------------------------------------------------------- */
/* Run control ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool Run::startRun( QString &errTitle, QString &errMsg )
{
// ----------
// OK to run?
// ----------

    if( isRunning() ) {
        errTitle    = "Already running";
        errMsg      = "Stop previous run before starting another.";
        return false;
    }

// -------------
// General setup
// -------------

    QMutexLocker    ml( &runMtx );
    Params          &p = app->cfgCtl()->acceptedParams;

    app->runIniting();

// ------
// Graphs
// ------

    createGraphsWindow( p );
    guiBreathe();

// -----------
// IMEC stream
// -----------

    int streamSecs = streamSpanMax( p );

    if( p.im.enabled ) {

        imQ = new AIQ( p.im.srate, p.im.imCumTypCnt[CimCfg::imSumAll], streamSecs );

        imReader = new IMReader( p, imQ );
        ConnectUI( imReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    }

// -----------
// NIDQ stream
// -----------

    if( p.ni.enabled ) {

        niQ = new AIQ( p.ni.srate, p.ni.niCumTypCnt[CniCfg::niSumAll], streamSecs );

        niReader = new NIReader( p, niQ );
        ConnectUI( niReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    }

// -------
// Trigger
// -------

    trg = new Trigger( p, graphsWindow, imQ, niQ );
    Connect( trg->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()), Qt::QueuedConnection );

// -----
// Start
// -----

    gate = new Gate( p, imReader, niReader, trg->worker );
    ConnectUI( gate->worker, SIGNAL(runStarted()), app, SLOT(runStarted()) );
    ConnectUI( gate->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    Connect( gate->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()), Qt::QueuedConnection );

    gate->startRun();
    running = true;

    grfUpdateWindowTitles();

    Systray() << "Acquisition starting up ...";
    Status() << "Acquisition starting up ...";
    Log() << "Acquisition starting up ...";

    graphFetcher = new GraphFetcher( graphsWindow, imQ, niQ );

    ml.unlock();    // allow AO to use runMtx

    if( app->getAOCtl()->doAutoStart() )
        QMetaObject::invokeMethod( this, "aoStart", Qt::QueuedConnection );

    return true;
}


void Run::stopRun()
{
    QMutexLocker    ml( &runMtx );

    if( !running )
        return;

    running = false;

    if( aoFetcher ) {
        delete aoFetcher;
        aoFetcher = 0;
    }

    if( graphFetcher ) {
        delete graphFetcher;
        graphFetcher = 0;
    }

// Note: gate sends messages to trg, so must delete gate before trg.

    if( gate ) {
        delete gate;
        gate = 0;
    }

    if( trg ) {
        delete trg;
        trg = 0;
    }

    if( niReader ) {
        delete niReader;
        niReader = 0;
    }

    if( imReader ) {
        delete imReader;
        imReader = 0;
    }

    if( niQ ) {
        delete niQ;
        niQ = 0;
    }

    if( imQ ) {
        delete imQ;
        imQ = 0;
    }

// Note: graphFetcher (e.g. putScans), gate and trg (e.g. setTriggerLED)
// talk to graphsWindow. Therefore, we must wait for those threads to
// complete before tearing graphsWindow down.

    if( graphsWindow ) {
        app->modelessClosed( graphsWindow );
        delete graphsWindow;
        graphsWindow = 0;
    }

    app->runStopped();

    grfUpdateWindowTitles();

    Systray() << "Acquisition stopped.";
    Status() << "Acquisition stopped.";
    Log() << "Acquisition stopped.";
}


// Return true (and stop) if can stop now.
//
bool Run::askThenStopRun()
{
    if( !isRunning() )
        return true;

    int yesNo = QMessageBox::question(
        0,
        "Stop Current Acquisition",
        "Acquisition in progress.\n"
        "Stop it before proceeding?",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No );

    guiBreathe();

    if( yesNo == QMessageBox::Yes ) {

        QMessageBox *M = new QMessageBox(
            QMessageBox::Information,
            "Closing Files",
            "Closing files...please wait.",
            QMessageBox::NoButton,
            0 );

        M->show();
        guiBreathe();

        stopRun();

        delete M;

        return true;
    }

    return false;
}


bool Run::imecPause( bool pause, bool changed )
{
    QMutexLocker    ml( &runMtx );

    if( !running || !imReader )
        return false;

    return imReader->worker->pause( pause, changed );
}

/* ---------------------------------------------------------------- */
/* Owned Datafile ops --------------------------------------------- */
/* ---------------------------------------------------------------- */

// Called by remote process.
//
bool Run::dfIsSaving() const
{
// BK: This is of dubious utility...should be deprecated.
    QMutexLocker    ml( &runMtx );

    return trg && !trg->worker->allFilesClosed();
}


bool Run::dfIsInUse( const QFileInfo &fi ) const
{
    QMutexLocker    ml( &runMtx );

    return trg && trg->worker->isInUse( fi );
}


void Run::dfSetRecordingEnabled( bool enabled, bool remote )
{
    QMutexLocker    ml( &runMtx );

    if( remote && graphsWindow ) {

        QMetaObject::invokeMethod(
            graphsWindow,
            "remoteSetRecordingEnabled",
            Qt::QueuedConnection,
            Q_ARG(bool, enabled) );

        return;
    }

    if( trg )
        trg->worker->setGateEnabled( enabled );
}


void Run::dfResetGTCounters()
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->resetGTCounters();
}


void Run::dfForceGTCounters( int g, int t )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->forceGTCounters( g, t );
}


// Called by remote process.
//
QString Run::dfGetCurNiName() const
{
// BK: This is of dubious utility...should be deprecated.
    QMutexLocker    ml( &runMtx );

    return (trg ? trg->worker->curNiFilename() : QString::null);
}


// Called by remote process.
//
quint64 Run::dfGetImFileStart() const
{
    QMutexLocker    ml( &runMtx );

    if( trg && imQ )
        return trg->worker->curImFileStart();

    return 0;
}


// Called by remote process.
//
quint64 Run::dfGetNiFileStart() const
{
    QMutexLocker    ml( &runMtx );

    if( trg && niQ )
        return trg->worker->curNiFileStart();

    return 0;
}

/* ---------------------------------------------------------------- */
/* Owned AOFetcher ops -------------------------------------------- */
/* ---------------------------------------------------------------- */

bool Run::isAOFetcher() const
{
    QMutexLocker    ml( &runMtx );
    return aoFetcher != 0;
}


void Run::aoStart()
{
    aoStop();

// BK: Does AO have dependency on niQ??
// BK: Currently allow only for NI
// BK: Should further check that src channel is in niQ

    const ConfigCtl *C = app->cfgCtl();

    if( isRunning() && C->validated && C->acceptedParams.ni.enabled ) {

        QMutexLocker    ml( &runMtx );
        aoFetcher = new AOFetcher( app->getAOCtl(), niQ );
    }
}


void Run::aoStop()
{
    QMutexLocker    ml( &runMtx );

    if( aoFetcher ) {
        delete aoFetcher;
        aoFetcher = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Owned gate and trigger ops ------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::rgtSetGate( bool hi )
{
    if( isRunning() ) {

        QMutexLocker    ml( &runMtx );
        Params          &p = app->cfgCtl()->acceptedParams;

        if( p.mode.mGate == eGateTCP && gate )
            dynamic_cast<GateTCP*>(gate->worker)->rgtSetGate( hi );
    }
}


void Run::rgtSetTrig( bool hi )
{
    if( isRunning() ) {

        QMutexLocker    ml( &runMtx );
        Params          &p = app->cfgCtl()->acceptedParams;

        if( p.mode.mTrig == eTrigTCP && trg )
            dynamic_cast<TrigTCP*>(trg->worker)->rgtSetTrig( hi );
    }
}


void Run::rgtSetMetaData( const KeyValMap &kvm )
{
    if( isRunning() ) {

        QMutexLocker    ml( &runMtx );

        if( trg )
            trg->worker->setMetaData( kvm );
    }
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::workerStopsRun()
{
    stopRun();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::createGraphsWindow( DAQ::Params &p )
{
    graphsWindow = new GraphsWindow( p );
    graphsWindow->setAttribute( Qt::WA_DeleteOnClose, false );

    app->act.shwHidGrfsAct->setEnabled( true );
    graphsWindow->show();

    app->modelessOpened( graphsWindow, false );

// Iff app built with Qt Creator, then graphs window
// will not get any mouse events until a modal dialog
// shows on top and is then destroyed!! That's what we
// do here...make an invisible message box.

    {
        QMessageBox XX( 0 );
        XX.setWindowModality( Qt::ApplicationModal );
        XX.setAttribute( Qt::WA_DontShowOnScreen, true );
//        XX.move( QApplication::desktop()->screen()->rect().topLeft() );
        XX.show();
        // auto-destroyed
    }
}


// Return smaller of {30 seconds, 25% of RAM}.
//
int Run::streamSpanMax( DAQ::Params &p )
{
    double  ram = 0.25,
            bps = 0.0;
    int     sec;

#ifdef Q_OS_WIN64
    ram *= getRAMBytes();
#else
    ram *= 4.0 * 1024.0 * 1024.0 * 1024.0;
#endif

    if( p.im.enabled )
        bps += p.im.srate * p.im.imCumTypCnt[CimCfg::imSumAll];

    if( p.ni.enabled )
        bps += p.ni.srate * p.ni.niCumTypCnt[CniCfg::niSumAll];

    bps *= 2.0;
    sec  = qMin( int(ram/bps), 30 );

    if( sec < 30 )
        Warning() << "Stream length limited to " << sec << " seconds.";

    return sec;
}


