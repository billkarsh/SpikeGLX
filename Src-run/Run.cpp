
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
#include "ColorTTLCtl.h"
#include "SOCtl.h"
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
    app->act.spikeViewAct->setEnabled( true );
    app->act.colorTTLAct->setEnabled( true );
    app->act.moreTracesAct->setEnabled( igw == 0 );
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
bool Run::grfIsUsrOrder( int js, int ip )
{
    GraphsWindow    *gw         = 0;
    bool            isUsrOrder  = false;

    runMtx.lock();
        if( !vGW.empty() )
            gw = vGW[0].gw;
    runMtx.unlock();

    if( gw ) {
        QMetaObject::invokeMethod(
            gw, "remoteIsUsrOrder",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(bool, isUsrOrder),
            Q_ARG(int, js),
            Q_ARG(int, ip) );
    }

    return isUsrOrder;
}


// Main window only.
//
void Run::grfRemoteSetsRunLE( const QString &fn )
{
    GraphsWindow    *gw = 0;

    runMtx.lock();
        if( !vGW.empty() )
            gw = vGW[0].gw;
    runMtx.unlock();

    if( gw )
        gw->remoteSetRunLE( fn );
}


void Run::grfSetStreams( std::vector<GFStream> &gfs, int igw )
{
    QMutexLocker    ml( &runMtx );

    for( int is = 0, ns = gfs.size(); is < ns; ++is ) {

        GFStream    &S = gfs[is];

        if( DAQ::Params::stream_isNI( S.stream ) )
            S.aiQ = niQ;
        else if( DAQ::Params::stream_isOB( S.stream ) )
            S.aiQ = obQ[DAQ::Params::stream2ip( S.stream )];
        else
            S.aiQ = imQ[DAQ::Params::stream2ip( S.stream )];
    }

    if( igw < int(vGW.size()) ) {
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


// igw = -1 for all fetchers.
//
void Run::grfWaitPaused( int igw )
{
    QMutexLocker    ml( &runMtx );

    int ngw = vGW.size();

    if( igw >= 0 && igw < ngw && vGW[igw].gf ) {
        GraphFetcher    *gf = vGW[igw].gf;
        if( gf )
            gf->waitPaused();
    }
    else {
        for( int igw = 0; igw < ngw; ++igw ) {
            GraphFetcher    *gf = vGW[igw].gf;
            if( gf )
                gf->waitPaused();
        }
    }
}


void Run::grfSetFocusMain()
{
    GraphsWindow    *gw = 0;

    runMtx.lock();
        if( !vGW.empty() )
            gw = vGW[0].gw;
    runMtx.unlock();

    if( gw )
        gw->setFocus( Qt::OtherFocusReason );
}


void Run::grfShowSpikes( int gp, int ip, int ch )
{
    GraphsWindow    *gw = 0;

    runMtx.lock();

    if( !imQf.size() ) {

        runMtx.unlock();

        QMessageBox::information( 0, "Feature Not Enabled",
            "The SpikeViewer reads data from the Filtered AP-band Buffers."
            " You have to enable this feature on the IM Setup tab before"
            " starting the run." );

        return;
    }

    if( !vGW.empty() )
        gw = vGW[0].gw;

    runMtx.unlock();

    if( gw )
        gw->getSOCtl()->setChan( gp, ip, ch );
}


void Run::grfShowColorTTL()
{
    GraphsWindow    *gw = 0;

    runMtx.lock();
        if( !vGW.empty() )
            gw = vGW[0].gw;
    runMtx.unlock();

    if( gw )
        gw->getTTLColorCtl()->showDialog();
}


void Run::grfRefresh()
{
    QMutexLocker    ml( &runMtx );

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw )
        vGW[igw].gw->eraseGraphs();
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


void Run::grfMoreTraces()
{
    QMutexLocker    ml( &runMtx );

    if( vGW.size() == 1 ) {
        vGW.push_back( GWPair( mainApp()->cfgCtl()->acceptedParams, 1 ) );
        vGW[1].startFetching( runMtx );
    }
}


void Run::grfUpdateRHSFlagsAll()
{
    QMutexLocker    ml( &runMtx );

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

        QMetaObject::invokeMethod(
            vGW[igw].gw, "updateRHSFlags",
            Qt::QueuedConnection );
    }
}


void Run::grfUpdateProbe( int ip, bool shankMap, bool chanMap )
{
    QMutexLocker    ml( &runMtx );

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

        QMetaObject::invokeMethod(
            vGW[igw].gw, "updateProbe",
            Qt::QueuedConnection,
            Q_ARG(int, ip),
            Q_ARG(bool, shankMap),
            Q_ARG(bool, chanMap) );
    }
}


void Run::grfUpdateWindowTitles()
{
// ------------
// Craft a name
// ------------

    QString stat;

    if( running )
        stat = "RUNNING - " + app->cfgCtl()->acceptedParams.sns.runName;
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
        mainApp()->act.moreTracesAct->setEnabled( true );
    }
}

/* ---------------------------------------------------------------- */
/* Owned AIStream ops --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN64

// Return length in range [2..8] seconds,
// and no greater than 40% of available RAM.
//
int Run::streamSpanMax( const DAQ::Params &p, bool warn )
{
    double  fracMax = 0.40,
            ram     = fracMax * getRAMBytes64BitApp(),
            bps     = 0.0;
    int     secsMax = 8,
            secs;

    for( int iq = 0, nq = p.stream_nq(); iq < nq; ++iq ) {
        int ip, js = p.iq2jsip( ip, iq );
        bps += p.stream_rate( js, ip ) * p.stream_nChans( js, ip );
    }

    bps *= 2.0;
    secs = qBound( 2, int(ram/bps), secsMax );

    if( warn )
        Warning() << "Stream length limited to " << secs << " seconds.";

    return secs;
}

#else

// Return smaller of {secsMax seconds, fracMax of available RAM}.
//
// Note: Running with
//   + 1 second long NI stream of 8 analog chans
//   + two shank viewers
//   + audio
// takes about 128 MB RAM as measured by enabling NI PERFMON switch.
// We therefore set baseline "startup" memory use to 130 MB.
//
int Run::streamSpanMax( const DAQ::Params &p, bool warn )
{
    double  startup = 0.13 * 1024.0 * 1024.0 * 1024.0,
            fracMax = 0.40,
            ram     = fracMax * (getRAMBytes32BitApp() - startup),
            bps     = 0.0;
    int     secsMax = 30,
            secs;

    for( int iq = 0, nq = p.stream_nq(); iq < nq; ++iq ) {
        int ip, js = p.iq2jsip( ip, iq );
        bps += p.stream_rate( js, ip ) * p.stream_nChans( js, ip );
    }

    bps *= 2.0;
    secs = qBound( 2, int(ram/bps), secsMax );

    if( warn && secs < secsMax )
        Warning() << "Stream length limited to " << secs << " seconds.";

    return secs;
}

#endif


quint64 Run::getSampleCount( int js, int ip ) const
{
    const AIQ*  aiq = getQ( js, ip );

    QMutexLocker    ml( &runMtx );

    return (aiq ? aiq->endCount() : 0);
}


const AIQ* Run::getQ( int js, int ip ) const
{
    QMutexLocker    ml( &runMtx );

    switch( js ) {
        case jsNI: return niQ;
        case jsOB:
            if( ip < obQ.size() )
                return obQ[ip];
            break;
        case jsIM:
            if( ip < imQ.size() )
                return imQ[ip];
            break;
        case -jsIM:
            if( ip < imQf.size() )
                return imQf[ip];
            break;
    }

    return 0;
}


// Get a stream-based time for profiling lag in imec streams.
// NO MUTEX: Should only be called by imec worker thread.
//
double Run::getStreamTime() const
{
    if( imQ.size() > 1 )
        return imQ[0]->endTime();
    else if( obQ.size() )
        return obQ[0]->endTime();
    else if( niQ )
        return niQ->endTime();

    return getTime();
}

/* ---------------------------------------------------------------- */
/* Run control ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool Run::isRunning() const
{
    if( runMtx.tryLock( 10 ) ) {

        bool    result = running;
        runMtx.unlock();
        return result;
    }
    else
        return true;
}


bool Run::startRun( QString &err )
{
    QMutexLocker    ml( &runMtx );

// ----------
// OK to run?
// ----------

    if( running ) {
        err = "Already running; stop previous run before starting another.";
        return false;
    }

// ------------
// Map OneBoxes
// ------------

    QStringList sl;

    if( !app->cfgCtl()->prbTab.map1bxSlots( sl ) ) {
        err = sl[0];
        return false;
    }

// -------------
// General setup
// -------------

    app->rsInit();

    DAQ::Params &p = app->cfgCtl()->acceptedParams;

    setPreciseTiming( true );

// ------
// Graphs
// ------

    vGW.push_back( GWPair( p, 0 ) );

// -------
// Streams
// -------

    int streamSecs  = streamSpanMax( p ),
        nIM         = p.stream_nIM(),
        nOB         = p.stream_nOB(),
        nNI         = p.stream_nNI();

    for( int ip = 0; ip < nIM; ++ip ) {
        imQ.push_back( new AIQ(
        p.stream_rate( jsIM, ip ), p.stream_nChans( jsIM, ip ), streamSecs ) );
    }

    if( p.im.prbAll.qf_on ) {
        for( int ip = 0; ip < nIM; ++ip ) {
            imQf.push_back( new AIQ(
            p.stream_rate( jsIM, ip ), p.stream_nChans( jsIM, ip ),
            p.im.prbAll.qf_secsStr.toDouble() ) );
        }
    }

    for( int ip = 0; ip < nOB; ++ip ) {
        obQ.push_back( new AIQ(
        p.stream_rate( jsOB, ip ), p.stream_nChans( jsOB, ip ), streamSecs ) );
    }

    if( nNI ) {
        niQ = new AIQ(
        p.stream_rate( jsNI, 0 ), p.stream_nChans( jsNI, 0 ), streamSecs );
    }

    if( nIM || nOB ) {
        imReader = new IMReader( p, imQ, imQf, obQ );
        ConnectUI( imReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
        ConnectUI( imReader->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );
    }

    if( nNI ) {
        niReader = new NIReader( p, niQ );
        ConnectUI( niReader->worker, SIGNAL(daqError(QString)), app, SLOT(runDaqError(QString)) );
        ConnectUI( niReader->worker, SIGNAL(finished()), this, SLOT(workerStopsRun()) );
    }

// -------
// Trigger
// -------

    trg = new Trigger( p, vGW[0].gw, imQ, obQ, niQ );
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

    vGW[0].gw->getSOCtl()->stopFetching();

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

    for( int ip = 0, np = obQ.size(); ip < np; ++ip )
        delete obQ[ip];
    obQ.clear();

    for( int ip = 0, np = imQf.size(); ip < np; ++ip )
        delete imQf[ip];
    imQf.clear();

    for( int ip = 0, np = imQ.size(); ip < np; ++ip )
        delete imQ[ip];
    imQ.clear();

// Note: graphFetcher (e.g., putSamps), gate and trg (e.g., setTriggerLED)
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

    app->rsStopped();
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
        "Are you sure you want to stop?",
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

    if( igw < int(vGW.size()) ) {
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


void Run::dfSetNextFileName( const QString &name )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->setNextFileName( name );
}


void Run::dfSetTriggerOffBeep( quint32 hertz, quint32 msec )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->setTriggerOffBeep( hertz, msec );
}


void Run::dfSetTriggerOnBeep( quint32 hertz, quint32 msec )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->setTriggerOnBeep( hertz, msec );
}


void Run::dfSetSBTT( int ip, const QString &SBTT )
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        trg->worker->setSBTT( ip, SBTT );
}


void Run::dfSetRecordingEnabled( bool enabled, bool remote )
{
    QMutexLocker    ml( &runMtx );

    if( remote ) {

        for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

            QMetaObject::invokeMethod(
                vGW[igw].gw, "remoteSetRecordingEnabled",
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


void Run::dfHaltiq( int iq )
{
    if( trg )
        trg->worker->haltiq( iq );
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

    return (trg ? trg->worker->curNiFilename() : QString());
}


// Called by remote process.
//
quint64 Run::dfGetFileStart( int js, int ip ) const
{
    QMutexLocker    ml( &runMtx );

    if( trg )
        return trg->worker->curFileStart( js, ip );

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

        if( p.mode.mGate == DAQ::eGateTCP ) {
            GateTCP* G = dynamic_cast<GateTCP*>(gate->worker);
            if( G )
                G->rgtSetGate( hi );
        }
    }
}


void Run::rgtSetTrig( bool hi )
{
    QMutexLocker    ml( &runMtx );

    if( trg ) {

        DAQ::Params &p = app->cfgCtl()->acceptedParams;

        if( p.mode.mTrig == DAQ::eTrigTCP ) {
            TrigTCP* T = dynamic_cast<TrigTCP*>(trg->worker);
            if( T )
                T->rgtSetTrig( hi );
        }
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
/* Anatomy ops ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString Run::setAnatomyPP( const QString &s )
{
    QString err;

// Split header-other:
// [probe-id,shank-id](startpos,endpos,R,G,B,rgnname)(startpos,endpos,R,G,B,rgnname)…()

    QStringList sl = s.split(
                        QRegExp("\\s*\\]\\s*"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

    if( n != 2 )
        return "SETANATOMYPP: Missing [header].";

// Parse header

    QStringList slh = sl[0].split(
                        QRegExp("\\[\\s*|\\s*,\\s*"),
                        QString::SkipEmptyParts );
    n = slh.size();

    if( n != 2 )
        return "SETANATOMYPP: Missing header param.";

    int ip = slh[0].toInt(),
        sk = slh[1].toInt();

    DAQ::Params &p = app->cfgCtl()->acceptedParams;
    int         np = p.im.get_nProbes(),
                ns;

    if( np && ip >= np ) {
        return QString("SETANATOMYPP: Probe index %1 >= %2.")
                .arg( ip ).arg( np );
    }

    if( np && sk >= (ns = p.im.prbj[ip].roTbl->nShank()) ) {
        return QString("SETANATOMYPP: Probe %1: shank index %2 >= %3.")
                .arg( ip ).arg( sk ).arg( ns );
    }

// Add elements to metadata

    KeyValMap   kvm;
    kvm[QString("~anatomy_shank%1").arg( sk )] = sl[1];
    rgtSetMetaData( kvm );

// Send elements to shank

    QMutexLocker    ml( &runMtx );

    for( int igw = 0, ngw = vGW.size(); igw < ngw; ++igw ) {

        QMetaObject::invokeMethod(
            vGW[igw].gw, "remoteSetAnatomyPP",
            Qt::QueuedConnection,
            Q_ARG(QString, sl[1]),
            Q_ARG(int, ip),
            Q_ARG(int, sk) );
    }

    return err;
}

/* ---------------------------------------------------------------- */
/* Opto ops ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString Run::opto_getAttens( int ip, int color )
{
    QMutexLocker    ml( &runMtx );

    if( imReader ) {
        if( ip < imQ.size() )
            return imReader->worker->opto_getAttens( ip, color );
        else
            return "OPTOGETATTENS: ip out of range.";
    }
    else
        return "OPTOGETATTENS: imec not enabled in this run.";
}


QString Run::opto_emit( int ip, int color, int site )
{
    QMutexLocker    ml( &runMtx );

    if( imReader ) {
        if( ip < imQ.size() )
            return imReader->worker->opto_emit( ip, color, site );
        else
            return "OPTOEMIT: ip out of range.";
    }
    else
        return "OPTOEMIT: imec not enabled in this run.";
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

    app->rsStarted();
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

    if( !aoC->devStart( imQ, imQf, obQ, niQ ) ) {
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


