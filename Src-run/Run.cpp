
#include "Run.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
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
/* struct GWPair -------------------------------------------------- */
/* ---------------------------------------------------------------- */

Run::GWPair::GWPair( const DAQ::Params &p, int igw )
{
    if( igw == 0 ) {

        // The main window (0) is created early in the run.
        // Its fetcher is not created until streaming starts.

        gf = 0;

        createWindow( p, 0 );

        // Iff app built with Qt Creator, then graphs window
        // will not get any mouse events until a modal dialog
        // shows on top and is then destroyed!! That's what we
        // do here...make an invisible message box.

        {
            QMessageBox XX( 0 );
            XX.setWindowModality( Qt::ApplicationModal );
            XX.setAttribute( Qt::WA_DontShowOnScreen, true );
            XX.show();
            // auto-destroyed
        }
    }
    else {
        createWindow( p, igw );
        setTitle( igw );
    }
}


void Run::GWPair::createWindow( const DAQ::Params &p, int igw )
{
    gw = new GraphsWindow( p, igw );
    gw->setAttribute( Qt::WA_DeleteOnClose, false );
    gw->show();

    MainApp *app = mainApp();
    app->act.shwHidGrfsAct->setEnabled( true );
    app->modelessOpened( gw, igw > 0 );
}


void Run::GWPair::setTitle( int igw )
{
    if( gw ) {

        MainApp *app = mainApp();

        gw->setWindowTitle(
                QString(APPNAME " Graphs%1 - RUNNING - %2")
                .arg( igw )
                .arg( app->cfgCtl()->acceptedParams.sns.runName ) );

        app->win.copyWindowTitleToAction( gw );
    }
}


void Run::GWPair::kill()
{
    stopFetching();

    if( gw ) {
        mainApp()->modelessClosed( gw );
        delete gw;
        gw = 0;
    }
}


void Run::GWPair::startFetching( QMutex &runMtx )
{
    gf = new GraphFetcher();

    runMtx.unlock();
        gw->initViews();
    runMtx.lock();
}


void Run::GWPair::stopFetching()
{
    if( gf ) {
        delete gf;
        gf = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Ctor ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Run::Run( MainApp *app )
    :   QObject(0), app(app), niQ(0),
        imReader(0), niReader(0),
        gate(0), trg(0), running(false)
{
}

/* ---------------------------------------------------------------- */
/* Owned GraphsWindow ops ----------------------------------------- */
/* ---------------------------------------------------------------- */

// Query main window.
//
bool Run::grfIsUsrOrderIm()
{
    QMutexLocker    ml( &runMtx );
    bool            isUsrOrder = false;

    if( !vGW.isEmpty() ) {

        GraphsWindow    *gw = vGW[0].gw;

        if( gw ) {

            QMetaObject::invokeMethod(
                gw,
                "remoteIsUsrOrderIm",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, isUsrOrder) );
        }
    }

    return isUsrOrder;
}


// Query main window.
//
bool Run::grfIsUsrOrderNi()
{
    QMutexLocker    ml( &runMtx );
    bool            isUsrOrder = false;

    if( !vGW.isEmpty() ) {

        GraphsWindow    *gw = vGW[0].gw;

        if( gw ) {

            QMetaObject::invokeMethod(
                gw,
                "remoteIsUsrOrderNi",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(bool, isUsrOrder) );
        }
    }

    return isUsrOrder;
}


// Main window only.
//
void Run::grfRemoteSetsRunName( const QString &fn )
{
    QMutexLocker    ml( &runMtx );

    if( !vGW.isEmpty() ) {

        GraphsWindow    *gw = vGW[0].gw;

        if( gw )
            gw->remoteSetRunLE( fn );
    }
}


void Run::grfSetStreams( QVector<GFStream> &gfs, int igw )
{
    QMutexLocker    ml( &runMtx );

    for( int is = 0, ns = gfs.size(); is < ns; ++is ) {

        GFStream    &S = gfs[is];

        if( S.stream == "nidq" )
            S.aiQ = niQ;
        else
            S.aiQ = imQ[DAQ::Params::streamID( S.stream )];
    }

    if( igw < vGW.size() ) {
        GraphFetcher    *gf = vGW[igw].gf;
        if( gf )
            gf->setStreams( gfs );
    }
}


// igw = -1 for all fetchers.
//
bool Run::grfHardPause( bool pause, int igw )
{
    QMutexLocker    ml( &runMtx );

    int ngw = vGW.size();

    if( igw >= 0 && igw < ngw && vGW[igw].gf )
        return vGW[igw].gf->hardPause( pause );
    else {
        for( int igw = 0; igw < ngw; ++igw ) {
            GraphFetcher    *gf = vGW[igw].gf;
            if( gf )
                gf->hardPause( pause );
        }
    }

    return true;
}


void Run::grfWaitPaused( int igw )
{
    QMutexLocker    ml( &runMtx );

    if( igw < vGW.size() ) {
        GraphFetcher    *gf = vGW[igw].gf;
        if( gf )
            gf->waitPaused();
    }
}


void Run::grfSetFocusMain()
{
    QMutexLocker    ml( &runMtx );

    if( !vGW.isEmpty() ) {

        GraphsWindow    *gw = vGW[0].gw;

        if( gw )
            gw->setFocus( Qt::OtherFocusReason );
    }
}


void Run::grfShowHideAll()
{
    QMutexLocker    ml( &runMtx );

    bool    anyHadFocus = false;

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

        GraphsWindow    *gw = vGW[igw].gw;

        if( gw->isHidden() ) {
            gw->eraseGraphs();
            gw->showNormal();
        }
        else {

            if( QApplication::focusWidget() == gw )
                anyHadFocus = true;

            gw->hide();
        }
    }

    if( anyHadFocus )
        app->giveFocus2Console();
}


void Run::grfUpdateRHSFlagsAll()
{
    QMutexLocker    ml( &runMtx );

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

        QMetaObject::invokeMethod(
            vGW[igw].gw,
            "updateRHSFlags",
            Qt::QueuedConnection );
    }
}


void Run::grfUpdateWindowTitles()
{
// ------------
// Craft a name
// ------------

    QString run = app->cfgCtl()->acceptedParams.sns.runName,
            stat;

    if( running )
        stat = "RUNNING - " + run;
    else
        stat = "READY";

// -----------------------
// Apply to console window
// -----------------------

    app->updateConsoleTitle( stat );

// -----------------------
// Apply to graphs windows
// -----------------------

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw )
        vGW[igw].setTitle( igw );
}


void Run::grfClose( GraphsWindow *gw )
{
// MS: For now, allow two GWPair maximum; this message must be for 2nd.

    Q_UNUSED( gw )

    if( vGW.size() > 1 ) {

        vGW[1].kill();
        vGW.resize( 1 );
    }
}

/* ---------------------------------------------------------------- */
/* Owned AIStream ops --------------------------------------------- */
/* ---------------------------------------------------------------- */

quint64 Run::getImScanCount( uint ip ) const
{
    QMutexLocker    ml( &runMtx );

    if( ip < (uint)imQ.size() )
        return imQ[ip]->endCount();

    return 0;
}


quint64 Run::getNiScanCount() const
{
    QMutexLocker    ml( &runMtx );

    if( niQ )
        return niQ->endCount();

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


// Get a stream-based time for profiling lag in imec streams.
// NO MUTEX: Should only be called by imec worker thread.
//
double Run::getStreamTime() const
{
    if( niQ )
        return niQ->endTime();
    else if( (uint)imQ.size() > 1 )
        return imQ[0]->endTime();

    return getTime();
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

    setPreciseTiming( true );

// ------
// Graphs
// ------

    vGW.push_back( GWPair( p, 0 ) );

// -----------
// IMEC stream
// -----------

    int streamSecs = streamSpanMax( p );

    if( p.im.enabled ) {

        for( int ip = 0, np = p.im.get_nProbes(); ip < np; ++ip ) {

            const CimCfg::AttrEach  &E = p.im.each[ip];

            imQ.push_back(
                new AIQ(
                    E.srate,
                    E.imCumTypCnt[CimCfg::imSumAll],
                    streamSecs ) );
        }

        imReader = new IMReader( p, imQ );
        ConnectUI( imReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
        ConnectUI( imReader->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );
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
        ConnectUI( niReader->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );
    }

// -------
// Trigger
// -------

    trg = new Trigger( p, vGW[0].gw, imQ, niQ );
    ConnectUI( trg->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
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

    QString s = "Acquisition starting up ...";

    Systray() << s;
    Status() << s;
    Log() << s;

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

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw )
        vGW[igw].stopFetching();

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

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw )
        vGW[igw].kill();

    vGW.clear();

    grfUpdateWindowTitles();

    setPreciseTiming( false );

    QString s = "Acquisition stopped.";

    Systray() << s;
    Status() << s;
    Log() << s;

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


void Run::imecUpdate( int ip )
{
    QMutexLocker    ml( &runMtx );

    if( imReader )
        imReader->worker->update( ip );
}

/* ---------------------------------------------------------------- */
/* GraphFetcher ops ----------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::grfSoftPause( bool pause, int igw )
{
    QMutexLocker    ml( &runMtx );

    if( igw < vGW.size() ) {
        GraphFetcher    *gf = vGW[igw].gf;
        if( gf )
            gf->softPause( pause );
    }
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

    if( remote ) {

        for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

            QMetaObject::invokeMethod(
                vGW[igw].gw,
                "remoteSetRecordingEnabled",
                Qt::QueuedConnection,
                Q_ARG(bool, enabled) );
        }

        return;
    }

    if( trg )
        trg->worker->setGateEnabled( enabled );
}


bool Run::dfIsRecordingEnabled()
{
    QMutexLocker    ml( &runMtx );

    return trg && trg->worker->isGateHi();
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
        grfUpdateRHSFlagsAll();
    }
}


void Run::aoStop()
{
    if( aoStopDev() )
        grfUpdateRHSFlagsAll();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Run::gettingSamples()
{
    QMutexLocker    ml( &runMtx );

    if( !running )
        return;

    vGW[0].startFetching( runMtx );

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
        Warning() << "Could not start audio drivers.";
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


// Return smaller of {secsMax seconds, fracMax of available RAM}.
//
// Note: Running with
//   + 1 second long NI stream of 8 analog chans
//   + two shank viewers
//   + audio
// takes about 128 MB RAM as measured by enabling NI PERFMON switch.
// We therefore set baseline "startup" memory use to 130 MB.
//
int Run::streamSpanMax( const DAQ::Params &p )
{
    double  startup = 0.13 * 1024.0 * 1024.0 * 1024.0,
            fracMax = 0.40,
            bps     = 0.0,
            ram;
    int     secsMax = 30,
            secs,
            np      = p.im.get_nProbes();

    ram = fracMax * (getRAMBytes32BitApp() - startup);

    for( int ip = 0; ip < np; ++ip ) {
        const CimCfg::AttrEach  &E = p.im.each[ip];
        bps += E.srate * E.imCumTypCnt[CimCfg::imSumAll];
    }

    if( p.ni.enabled )
        bps += p.ni.srate * p.ni.niCumTypCnt[CniCfg::niSumAll];

    bps *= 2.0;
    secs = qBound( 1, int(ram/bps), secsMax );

    if( secs < secsMax )
        Warning() << "Stream length limited to " << secs << " seconds.";

    return secs;
}


