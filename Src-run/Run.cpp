
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
#include "Version.h"

#include <QAction>
#include <QMessageBox>




/* ---------------------------------------------------------------- */
/* Ctor ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Run::Run( MainApp *app )
    :   QObject(0), app(app), niQ(0),
        graphsWindow(0), graphFetcher(0),
        imReader(0), niReader(0),
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


void Run::grfSetStreams( QVector<GFStream> &gfs )
{
    QMutexLocker    ml( &runMtx );

    for( int is = 0, ns = gfs.size(); is < ns; ++is ) {

        GFStream    &S = gfs[is];

        if( S.stream == "nidq" )
            S.aiQ = niQ;
        else
            S.aiQ = imQ[DAQ::Params::streamID( S.stream )];
    }

    if( graphFetcher )
        graphFetcher->setStreams( gfs );
}


void Run::grfHardPause( bool pause )
{
    QMutexLocker    ml( &runMtx );

    if( graphFetcher )
        graphFetcher->hardPause( pause );
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


void Run::grfUpdateRHSFlags()
{
    QMutexLocker    ml( &runMtx );

    if( graphsWindow ) {

        QMetaObject::invokeMethod(
            graphsWindow,
            "updateRHSFlags",
            Qt::QueuedConnection );
    }
}


void Run::grfUpdateWindowTitles()
{
// ------------
// Craft a name
// ------------

    DAQ::Params &p  = app->cfgCtl()->acceptedParams;
    QString     run = p.sns.runName,
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

quint64 Run::getImScanCount( uint ip ) const
{
    QMutexLocker    ml( &runMtx );

    if( ip < (uint)imQ.size() )
        return imQ[ip]->curCount();

    return 0;
}


quint64 Run::getNiScanCount() const
{
    QMutexLocker    ml( &runMtx );

    if( niQ )
        return niQ->curCount();

    return 0;
}


const AIQ* Run::getImQ( uint ip ) const
{
    QMutexLocker    ml( &runMtx );

    if( ip < (uint)imQ.size() )
        return imQ[ip];

    return 0;
}


const AIQ* Run::getNiQ() const
{
    QMutexLocker    ml( &runMtx );

    return niQ;
}

/* ---------------------------------------------------------------- */
/* Run control ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool Run::startRun( QString &errTitle, QString &errMsg )
{
    QMutexLocker    ml( &runMtx );

// ----------
// OK to run?
// ----------

    if( running ) {
        errTitle    = "Already running";
        errMsg      = "Stop previous run before starting another.";
        return false;
    }

// -------------
// General setup
// -------------

    app->runIniting();

    DAQ::Params &p = app->cfgCtl()->acceptedParams;

// ------
// Graphs
// ------

    createGraphsWindow( p );

// -----------
// IMEC stream
// -----------

    int streamSecs = streamSpanMax( p );

    if( p.im.enabled ) {

        for( int ip = 0; ip < p.im.nProbes; ++ip ) {

            imQ.push_back(
                new AIQ(
                    p.im.all.srate,
                    p.im.each[ip].imCumTypCnt[CimCfg::imSumAll],
                    streamSecs ) );
        }

        imReader = new IMReader( p, imQ );
        ConnectUI( imReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    }

// -----------
// NIDQ stream
// -----------

    if( p.ni.enabled ) {

        niQ =
            new AIQ(
                p.ni.srate,
                p.ni.niCumTypCnt[CniCfg::niSumAll],
                streamSecs );

        niReader = new NIReader( p, niQ );
        ConnectUI( niReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    }

// -------
// Trigger
// -------

    trg = new Trigger( p, graphsWindow, imQ, niQ );
    ConnectUI( trg->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );

// -----
// Start
// -----

    gate = new Gate( p, imReader, niReader, trg->worker );
    ConnectUI( gate->worker, SIGNAL(runStarted()), this, SLOT(gettingSamples()) );
    ConnectUI( gate->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
    ConnectUI( gate->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );

    running = true;

    grfUpdateWindowTitles();

    Systray() << "Acquisition starting up ...";
    Status() << "Acquisition starting up ...";
    Log() << "Acquisition starting up ...";

    ml.unlock();    // ensure runMtx available to startup agents

    gate->startRun();

    return true;
}


void Run::stopRun()
{
    aoStopDev();

    QMutexLocker    ml( &runMtx );

    if( !running )
        return;

    running = false;

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

    for( int ip = 0, np = imQ.size(); ip < np; ++ip )
        delete imQ[ip];

    imQ.clear();

// Note: graphFetcher (e.g. putScans), gate and trg (e.g. setTriggerLED)
// talk to graphsWindow. Therefore, we must wait for those threads to
// complete before tearing graphsWindow down.

    if( graphsWindow ) {
        app->modelessClosed( graphsWindow );
        delete graphsWindow;
        graphsWindow = 0;
    }

    grfUpdateWindowTitles();

    Systray() << "Acquisition stopped.";
    Status() << "Acquisition stopped.";
    Log() << "Acquisition stopped.";

    app->runStopped();
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


bool Run::imecPause( bool pause, int ipChanged )
{
    QMutexLocker    ml( &runMtx );

    if( imReader )
        return imReader->worker->pause( pause, ipChanged );

    return false;
}

/* ---------------------------------------------------------------- */
/* GraphFetcher ops ----------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::grfSoftPause( bool pause )
{
    QMutexLocker    ml( &runMtx );

    if( graphFetcher )
        graphFetcher->softPause( pause );
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
quint64 Run::dfGetImFileStart( uint ip ) const
{
    QMutexLocker    ml( &runMtx );

    if( trg && ip < (uint)imQ.size() )
        return trg->worker->curImFileStart( ip );

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
/* Owned gate and trigger ops ------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::rgtSetGate( bool hi )
{
    QMutexLocker    ml( &runMtx );

    if( gate ) {

        DAQ::Params &p = app->cfgCtl()->acceptedParams;

        if( p.mode.mGate == DAQ::eGateTCP )
            dynamic_cast<GateTCP*>(gate->worker)->rgtSetGate( hi );
    }
}


void Run::rgtSetTrig( bool hi )
{
    QMutexLocker    ml( &runMtx );

    if( trg ) {

        DAQ::Params &p = app->cfgCtl()->acceptedParams;

        if( p.mode.mTrig == DAQ::eTrigTCP )
            dynamic_cast<TrigTCP*>(trg->worker)->rgtSetTrig( hi );
    }
}


void Run::rgtSetMetaData( const KeyValMap &kvm )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->setMetaData( kvm );
}

/* ---------------------------------------------------------------- */
/* Audio ops ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void Run::aoStart()
{
    aoStop();

    if( isRunning() ) {

        aoStartDev();
        grfUpdateRHSFlags();
    }
}


void Run::aoStop()
{
    if( aoStopDev() )
        grfUpdateRHSFlags();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::gettingSamples()
{
    QMutexLocker    ml( &runMtx );

    if( !running )
        return;

    graphFetcher = new GraphFetcher();

    runMtx.unlock();
        graphsWindow->initColorTTL();
        graphsWindow->initGFStreams();
    runMtx.lock();

    if( app->getAOCtl()->doAutoStart() )
        QMetaObject::invokeMethod( this, "aoStart", Qt::QueuedConnection );

    app->runStarted();
}


void Run::workerStopsRun()
{
    stopRun();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::aoStartDev()
{
    AOCtl   *aoC = app->getAOCtl();

    if( !aoC->devStart( imQ, niQ ) ) {
        Error() << "Could not start audio drivers.";
        aoC->devStop();
    }
}


// Return true if was running.
//
bool Run::aoStopDev()
{
    AOCtl   *aoC = app->getAOCtl();
    bool    wasRunning = false;

    if( aoC->readyForScans() ) {
        aoC->devStop();
        wasRunning  = true;
    }

    return wasRunning;
}


void Run::createGraphsWindow( const DAQ::Params &p )
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


// Return smaller of {secMax seconds, pctMax% of RAM}.
//
int Run::streamSpanMax( const DAQ::Params &p )
{
    double  pctMax  = 0.25,
            bps     = 0.0,
            ram;
    int     secMax  = 30,
            sec;

#ifdef Q_OS_WIN64
    ram = pctMax * getRAMBytes();
#else
    ram = pctMax * 4.0 * 1024.0 * 1024.0 * 1024.0;
#endif

    if( p.im.enabled ) {

        for( int ip = 0; ip < p.im.nProbes; ++ip ) {

            bps += p.im.all.srate
                    * p.im.each[ip].imCumTypCnt[CimCfg::imSumAll];
        }
    }

    if( p.ni.enabled )
        bps += p.ni.srate * p.ni.niCumTypCnt[CniCfg::niSumAll];

    bps *= 2.0;
    sec  = qMin( int(ram/bps), secMax );

    if( sec < secMax )
        Warning() << "Stream length limited to " << sec << " seconds.";

    return sec;
}


