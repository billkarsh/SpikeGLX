
#include "Util.h"
#include "MainApp.h"
#include "CmdSrvDlg.h"
#include "RgtSrvDlg.h"
#include "Version.h"

#include <QMenuBar>
#include <QMainWindow>



void Main_Actions::initActions()
{
    MainApp *app = mainApp();

// ----
// File
// ----

    fileOpenAct = new QAction( "&Open File Viewer...", this );
    fileOpenAct->setShortcut( QKeySequence( tr("Ctrl+O") ) );
    fileOpenAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( fileOpenAct, SIGNAL(triggered()), app, SLOT(file_Open()) );

    newAcqAct = new QAction( "&New Acquisition...", this );
    newAcqAct->setShortcut( QKeySequence( tr("Ctrl+N") ) );
    newAcqAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( newAcqAct, SIGNAL(triggered()), app, SLOT(file_NewRun()) );

    stopAcqAct = new QAction( "&Stop Running Acquisition", this );
    stopAcqAct->setShortcut( QKeySequence( tr("ESC") ) );
    // 'esc' shortcut is standard for 'close this dialog' so we
    // do not give it application-wide scope. Moreover, we add
    // 'close on esc' keyPressEvent() handlers to widgets that
    // are not QDialog class to honor user expectation.
    stopAcqAct->setEnabled( false );
    ConnectUI( stopAcqAct, SIGNAL(triggered()), app, SLOT(file_AskStopRun()) );

    quitAct = new QAction( "&Quit", this );
    quitAct->setShortcut( QKeySequence( tr("Ctrl+Q") ) );
    quitAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( quitAct, SIGNAL(triggered()), app, SLOT(file_AskQuit()) );

// -------
// Options
// -------

    selDataDirAct = new QAction( "Choose &Data Directory...", this );
    ConnectUI( selDataDirAct, SIGNAL(triggered()), app, SLOT(options_PickDataDir()) );

    aoDlgAct = new QAction( "&Audio Settings...", this );
    aoDlgAct->setShortcut( QKeySequence( tr("Ctrl+A") ) );
    aoDlgAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( aoDlgAct, SIGNAL(triggered()), app, SLOT(options_AODlg()) );

    cmdSrvOptAct = new QAction( "&Command Server Settings...", this );
    ConnectUI( cmdSrvOptAct, SIGNAL(triggered()), app->cmdSrv, SLOT(showOptionsDlg()) );

    rgtSrvOptAct = new QAction( "&Gate/Trigger Server Settings...", this );
    ConnectUI( rgtSrvOptAct, SIGNAL(triggered()), app->rgtSrv, SLOT(showOptionsDlg()) );

// -----
// Tools
// -----

    sha1Act = new QAction( "&Verify SHA1...", this );
    ConnectUI( sha1Act, SIGNAL(triggered()), app, SLOT(tools_VerifySha1()) );

    par2Act = new QAction( "&PAR2 Redundancy Tool...", this );
    ConnectUI( par2Act, SIGNAL(triggered()), app, SLOT(tools_ShowPar2Win()) );

    calSRateAct = new QAction( "Sample &Rates From Run...", this );
    ConnectUI( calSRateAct, SIGNAL(triggered()), app, SLOT(tools_CalSRate()) );

    imCloseAct = new QAction( "&Close All Imec Slots", this );
    ConnectUI( imCloseAct, SIGNAL(triggered()), app, SLOT(tools_ImClose()) );

    imBistAct = new QAction( "&BIST (Imec Probe Diagnostics)...", this );
    ConnectUI( imBistAct, SIGNAL(triggered()), app, SLOT(tools_ImBist()) );

    imHstAct = new QAction( "&HST (Imec 1.0 Headstage Diagnostics)...", this );
    ConnectUI( imHstAct, SIGNAL(triggered()), app, SLOT(tools_ImHst()) );

    imFirmAct = new QAction( "&Update Imec Firmware...", this );
    ConnectUI( imFirmAct, SIGNAL(triggered()), app, SLOT(tools_ImFirmware()) );

    togDebugAct = new QAction( "Verbose Log (&Debug Mode)", this );
    togDebugAct->setShortcut( QKeySequence( tr("Ctrl+D") ) );
    togDebugAct->setShortcutContext( Qt::ApplicationShortcut );
    togDebugAct->setCheckable( true );
    togDebugAct->setChecked( app->isDebugMode() );
    ConnectUI( togDebugAct, SIGNAL(triggered()), app, SLOT(tools_ToggleDebug()) );

    editLogAct = new QAction( "Edit &Log", this );
    editLogAct->setCheckable( true );
    editLogAct->setChecked( app->isLogEditable() );
    ConnectUI( editLogAct, SIGNAL(triggered()), app, SLOT(tools_ToggleEditLog()) );

    logFileAct = new QAction( "&Save Log File...", this );
    ConnectUI( logFileAct, SIGNAL(triggered()), app, SLOT(tools_SaveLogFile()) );

// ------
// Window
// ------

    bringFrontAct = new QAction( "&Bring All to Front", this );
    ConnectUI( bringFrontAct, SIGNAL(triggered()), &app->win, SLOT(bringAllToFront()) );

    shwHidConsAct = new QAction( "Hide/Sho&w Console", this );
    shwHidConsAct->setShortcut( QKeySequence( tr("Ctrl+W") ) );
    shwHidConsAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( shwHidConsAct, SIGNAL(triggered()), app, SLOT(window_ShowHideConsole()) );

    shwHidGrfsAct = new QAction( "Hide/Show &Graphs", this );
    shwHidGrfsAct->setShortcut( QKeySequence( tr("Ctrl+G") ) );
    shwHidGrfsAct->setShortcutContext( Qt::ApplicationShortcut );
    shwHidGrfsAct->setEnabled( false );
    ConnectUI( shwHidGrfsAct, SIGNAL(triggered()), app, SLOT(window_ShowHideGraphs()) );

    runMetricsAct = new QAction( "Run &Metrics", this );
    runMetricsAct->setShortcut( QKeySequence( tr("Ctrl+M") ) );
    runMetricsAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( runMetricsAct, SIGNAL(triggered()), app, SLOT(window_RunMetrics()) );

    moreTracesAct = new QAction( "More &Traces", this );
    moreTracesAct->setShortcut( QKeySequence( tr("Ctrl+T") ) );
    moreTracesAct->setShortcutContext( Qt::ApplicationShortcut );
    moreTracesAct->setEnabled( false );
    ConnectUI( moreTracesAct, SIGNAL(triggered()), app, SLOT(window_MoreTraces()) );

// ----
// Help
// ----

    helpAct = new QAction( APPNAME" &Help", this );
    ConnectUI( helpAct, SIGNAL(triggered()), app, SLOT(help_HelpDlg()) );

    exploreAppAct = new QAction( "&Explore " APPNAME" Folder", this );
    ConnectUI( exploreAppAct, SIGNAL(triggered()), app, SLOT(help_ExploreApp()) );

    aboutAct = new QAction( "&About " APPNAME, this );
    ConnectUI( aboutAct, SIGNAL(triggered()), app, SLOT(help_About()) );

    aboutQtAct = new QAction( "About &Qt", this );
    ConnectUI( aboutQtAct, SIGNAL(triggered()), app, SLOT(help_AboutQt()) );
}


void Main_Actions::initMenus( QMainWindow *w )
{
#ifdef Q_OS_MACX
    QMenuBar    *mb = new QMenuBar;
#else
    QMenuBar    *mb = w->menuBar();
#endif

    QMenu   *m;

    m = mb->addMenu( "&File" );
    m->addAction( fileOpenAct );
    m->addAction( newAcqAct );
    m->addAction( stopAcqAct );
    m->addSeparator();
    m->addAction( quitAct );

    m = mb->addMenu( "&Options" );
    m->addAction( selDataDirAct );
    ddExploreMenu = m->addMenu( "&Explore Data Directory" );
    m->addSeparator();
    m->addAction( aoDlgAct );
    m->addSeparator();
    m->addAction( cmdSrvOptAct );
    m->addAction( rgtSrvOptAct );

    m = mb->addMenu( "&Tools" );
    m->addAction( sha1Act );
    m->addAction( par2Act );
    m->addSeparator();
    m->addAction( calSRateAct );
    m->addSeparator();
#ifdef HAVE_IMEC
    m->addAction( imCloseAct );
    m->addAction( imBistAct );
    m->addAction( imHstAct );
    m->addAction( imFirmAct );
    m->addSeparator();
#endif
    m->addAction( togDebugAct );
    m->addAction( editLogAct );
    m->addAction( logFileAct );

    m = mb->addMenu( "&Window" );
    m->addAction( bringFrontAct );
    m->addSeparator();
    m->addAction( shwHidConsAct );
    m->addAction( shwHidGrfsAct );
    m->addSeparator();
    m->addAction( runMetricsAct );
    m->addAction( moreTracesAct );
    m->addSeparator();
    windowMenu = m;

    m = mb ->addMenu( "&Help" );
    m->addAction( helpAct );
    m->addAction( exploreAppAct );
    m->addSeparator();
    m->addAction( aboutAct );
    m->addAction( aboutQtAct );

    Connect( windowMenu, SIGNAL(aboutToShow()), &mainApp()->win, SLOT(checkMarkActiveWindow()) );

    ddExploreUpdate();
}


void Main_Actions::ddExploreUpdate()
{
    MainApp *app = mainApp();

// Delete old actions

    for( int ia = 0, na = vddExploreAct.size(); ia < na; ++ia ) {

        QAction *A = vddExploreAct[ia];
        ddExploreMenu->removeAction( A );
        delete A;
    }

// Add new actions

    int na = app->nDataDirs();

    vddExploreAct.resize( na );

    for( int ia = 0; ia < na; ++ia ) {

        QAction *A = new QAction( QString("%1: %2").arg( ia ).arg( app->dataDir( ia ) ) );
        A->setObjectName( QString::number( ia ) );
        ConnectUI( A, SIGNAL(triggered()), app, SLOT(options_ExploreDataDir()) );

        ddExploreMenu->addAction( A );
        vddExploreAct[ia] = A;
    }
}


