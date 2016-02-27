
#include "Util.h"
#include "MainApp.h"
#include "CmdSrvDlg.h"
#include "RgtSrvDlg.h"
#include "Version.h"

#include <QAction>
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

    stopAcqAct = new QAction( "Stop Running Acquisition", this );
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

    togDebugAct = new QAction( "&Debug Mode", this );
    togDebugAct->setShortcut( QKeySequence( tr("Ctrl+D") ) );
    togDebugAct->setShortcutContext( Qt::ApplicationShortcut );
    togDebugAct->setCheckable( true );
    togDebugAct->setChecked( app->isDebugMode() );
    ConnectUI( togDebugAct, SIGNAL(triggered()), app, SLOT(options_ToggleDebug()) );

    selRunDirAct = new QAction( "Choose &Run Directory...", this );
    ConnectUI( selRunDirAct, SIGNAL(triggered()), app, SLOT(options_PickRunDir()) );

    aoDlgAct = new QAction( "AO Settings...", this );
    aoDlgAct->setShortcut( QKeySequence( tr("Ctrl+A") ) );
    aoDlgAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( aoDlgAct, SIGNAL(triggered()), app, SLOT(options_AODlg()) );

    cmdSrvOptAct = new QAction( "Command Server Settings...", this );
    ConnectUI( cmdSrvOptAct, SIGNAL(triggered()), app->cmdSrv, SLOT(showOptionsDlg()) );

    rgtSrvOptAct = new QAction( "Gate/Trigger Server Settings...", this );
    ConnectUI( rgtSrvOptAct, SIGNAL(triggered()), app->rgtSrv, SLOT(showOptionsDlg()) );

// -----
// Tools
// -----

    sha1Act = new QAction( "Verify SHA1...", this );
    ConnectUI( sha1Act, SIGNAL(triggered()), app, SLOT(tools_VerifySha1()) );

    par2Act = new QAction( "PAR2 Redundancy Tool...", this );
    ConnectUI( par2Act, SIGNAL(triggered()), app, SLOT(tools_ShowPar2Win()) );

    editLogAct = new QAction( "Edit Log", this );
    editLogAct->setCheckable( true );
    editLogAct->setChecked( app->isLogEditable() );
    ConnectUI( editLogAct, SIGNAL(triggered()), app, SLOT(tools_ToggleEditLog()) );

    logFileAct = new QAction( "Save Log File...", this );
    ConnectUI( logFileAct, SIGNAL(triggered()), app, SLOT(tools_SaveLogFile()) );

// ------
// Window
// ------

    bringFrontAct = new QAction( "Bring All to Front", this );
    ConnectUI( bringFrontAct, SIGNAL(triggered()), &app->win, SLOT(bringAllToFront()) );

    shwHidConsAct = new QAction( "Hide/Show Conso&le", this );
    shwHidConsAct->setShortcut( QKeySequence( tr("Ctrl+L") ) );
    shwHidConsAct->setShortcutContext( Qt::ApplicationShortcut );
    ConnectUI( shwHidConsAct, SIGNAL(triggered()), app, SLOT(window_ShowHideConsole()) );

    shwHidGrfsAct = new QAction( "Hide/Show &Graphs", this );
    shwHidGrfsAct->setShortcut( QKeySequence( tr("Ctrl+G") ) );
    shwHidGrfsAct->setShortcutContext( Qt::ApplicationShortcut );
    shwHidGrfsAct->setEnabled( false );
    ConnectUI( shwHidGrfsAct, SIGNAL(triggered()), app, SLOT(window_ShowHideGraphs()) );

// ----
// Help
// ----

    helpAct = new QAction( APPNAME" &Help", this );
    ConnectUI( helpAct, SIGNAL(triggered()), app, SLOT(help_HelpDlg()) );

    aboutAct = new QAction( "&About", this );
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
    m->addAction( togDebugAct );
    m->addSeparator();
    m->addAction( selRunDirAct );
    m->addAction( aoDlgAct );
    m->addSeparator();
    m->addAction( cmdSrvOptAct );
    m->addAction( rgtSrvOptAct );

    m = mb->addMenu( "&Tools" );
    m->addAction( sha1Act );
    m->addAction( par2Act );
    m->addSeparator();
    m->addAction( editLogAct );
    m->addAction( logFileAct );

    m = mb->addMenu( "&Window" );
    m->addAction( bringFrontAct );
    m->addSeparator();
    m->addAction( shwHidConsAct );
    m->addAction( shwHidGrfsAct );
    m->addSeparator();
    windowMenu = m;

    m = mb ->addMenu( "&Help" );
    m->addAction( helpAct );
    m->addSeparator();
    m->addAction( aboutAct );
    m->addAction( aboutQtAct );

    Connect( windowMenu, SIGNAL(aboutToShow()), &mainApp()->win, SLOT(checkMarkActiveWindow()) );
}


