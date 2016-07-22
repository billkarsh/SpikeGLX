
#include "Pixmaps/Icon.xpm"
#include "Pixmaps/ParWindowIcon.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "FileViewerWindow.h"
#include "DataFile.h"
#include "ConfigCtl.h"
#include "AOCtl.h"
#include "CmdSrvDlg.h"
#include "RgtSrvDlg.h"
#include "Run.h"
#include "IMBISTCtl.h"
#include "Sha1Verifier.h"
#include "Par2Window.h"
#include "HelpWindow.h"
#include "Version.h"

#include <QDesktopWidget>
#include <QProgressDialog>
#include <QMessageBox>
#include <QAction>
#include <QDir>
#include <QFileDialog>
#include <QPushButton>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* Ctor ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

MainApp::MainApp( int &argc, char **argv )
    :   QApplication(argc, argv, true),
        consoleWindow(0), par2Win(0), helpWindow(0),
        fvwHelpWin(0), configCtl(0), aoCtl(0), run(0),
        cmdSrv(new CmdSrvDlg), rgtSrv(new RgtSrvDlg),
        runInitingDlg(0), initialized(false)
{
// --------------
// App attributes
// --------------

    setQuitOnLastWindowClosed( false );

    // default for all top windows
    setWindowIcon( QPixmap(Icon_xpm) );

// ---------
// App state
// ---------

    loadSettings();

// -----------------
// Low-level helpers
// -----------------

    act.initActions();
    win.setActionPtr( &act );

// -----------
// Main window
// -----------

    consoleWindow = new ConsoleWindow;
    consoleWindow->setAttribute( Qt::WA_DeleteOnClose, false );
    consoleWindow->resize( 800, 300 );
    consoleWindow->show();

#ifdef Q_OS_MACX
// Add console window to the Window menu, an app-global menu on OS-X
    win.addToMenu( consoleWindow );
#endif

// ------------
// Message logs
// ------------

    msg.initMessenger( consoleWindow );

    Log() << VERSION_STR;
    Log() << "Application started";

// -------------
// Other helpers
// -------------

    configCtl = new ConfigCtl;

    aoCtl = new AOCtl( configCtl->acceptedParams );
    aoCtl->setAttribute( Qt::WA_DeleteOnClose, false );
    aoCtl->setWindowTitle( APPNAME " - AO Controller" );
    ConnectUI( aoCtl, SIGNAL(closed()), this, SLOT(aoCtlClosed()) );

    run = new Run( this );

    cmdSrv->startServer( true );
    rgtSrv->startServer( true );

// ----
// Done
// ----

    remoteMtx.lock();
    initialized = true;
    remoteMtx.unlock();

    Log() << "Application initialized";
    Status() <<  APPNAME << " initialized.";

    updateConsoleTitle( "READY" );

    showStartupMessages();
}


MainApp::~MainApp()
{
    saveSettings();

    if( par2Win ) {
        delete par2Win;
        par2Win = 0;
    }

    if( fvwHelpWin ) {
        delete fvwHelpWin;
        fvwHelpWin = 0;
    }

    if( helpWindow ) {
        delete helpWindow;
        helpWindow = 0;
    }

    if( rgtSrv ) {
        delete rgtSrv;
        rgtSrv = 0;
    }

    if( cmdSrv ) {
        delete cmdSrv;
        cmdSrv = 0;
    }

    if( run ) {
        delete run;
        run = 0;
    }

    if( aoCtl ) {
        delete aoCtl;
        aoCtl = 0;
    }

    if( configCtl ) {
        delete configCtl;
        configCtl = 0;
    }

    if( consoleWindow ) {
        delete consoleWindow;
        consoleWindow = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Properties ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool MainApp::isConsoleHidden() const
{
    return !consoleWindow || consoleWindow->isHidden();
}


bool MainApp::isShiftPressed() const
{
    return (keyboardModifiers() & Qt::ShiftModifier);
}


bool MainApp::remoteSetsRunDir( const QString &path )
{
    if( !QDir( path ).exists() )
        return false;

    remoteMtx.lock();
    appData.runDir = path;
    remoteMtx.unlock();

    return true;
}


void MainApp::makePathAbsolute( QString &path )
{
    if( !QFileInfo( path ).isAbsolute() ) {

        remoteMtx.lock();
        path = QString("%1/%2").arg( appData.runDir ).arg( path );
        remoteMtx.unlock();
    }
}


struct FV_IsViewingFile : public WindowTester
{
    QString fn;
    FV_IsViewingFile( const QString &fn ) : fn(fn) {}

    bool operator()( QWidget *w ) const
    {
        FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(w);
        if( f )
            return f->file() == fn;
        return false;
    }
};


void MainApp::saveSettings() const
{
    STDSETTINGS( settings, "mainapp" );

    settings.beginGroup( "MainApp" );

    settings.setValue( "lastViewedFile", appData.lastViewedFile );
    settings.setValue( "debug", appData.debug );
    settings.setValue( "editLog", appData.editLog );

    remoteMtx.lock();
    settings.setValue( "runDir", appData.runDir );
    remoteMtx.unlock();

    settings.endGroup();

    cmdSrv->saveSettings( settings );
    rgtSrv->saveSettings( settings );
}

/* ---------------------------------------------------------------- */
/* Event processing ----------------------------------------------- */
/* ---------------------------------------------------------------- */

void MainApp::giveFocus2Console()
{
    if( consoleWindow )
        consoleWindow->setFocus( Qt::OtherFocusReason );
}


void MainApp::updateConsoleTitle( const QString &status )
{
    QString title = QString(APPNAME " Console - %1").arg( status );

    consoleWindow->setWindowTitle( title );
    msg.updateSysTrayTitle( title );

#ifdef Q_OS_MACX
    win.copyWindowTitleToAction( consoleWindow );
#endif
}

/* ---------------------------------------------------------------- */
/* Menu item handlers --------------------------------------------- */
/* ---------------------------------------------------------------- */

void MainApp::file_Open()
{
// --------------
// Not during run
// --------------

    if( run->isRunning() ) {

        int yesNo = QMessageBox::warning(
            consoleWindow,
            "Run in Progress",
            "Application is acquiring data...\n\n"
            "Opening a file now may slow performance or cause a failure.\n"
            "It's safer to stop data acquisition before proceeding.\n\n"
            "Open the file anyway?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo != QMessageBox::Yes )
            return;
    }

// ------------------
// Previous choice...
// ------------------

    QString     last = appData.lastViewedFile;
    QFileInfo   fi( last );

    if( last.isEmpty() )
        last = runDir();
    else if( !fi.exists() ) {

        // Failing to locate the last choice, if last was a file
        // with pattern 'path/file' we will offer the 'path' part.
        // However, if last was already a 'path' then we fall back
        // to the runDir.

        // Note that fi.isFile() and fi.isDir() look at the string
        // pattern AND check existence so are not what we want.
        // We do our own pattern match instead:
        //
        //      path + slash + one-or-more-not-slashes + dot + arb.

        QRegExp re("(.*)[/\\\\][^/\\\\]+\\..*");

        if( last.contains( re ) ) {

            fi.setFile( last = re.cap(1) );

            if( !fi.isDir() )
                last = runDir();
        }
        else
            last = runDir();
    }

// ----
// Pick
// ----

    QString filters = APPNAME" Data (*.bin)";

    QString fname =
        QFileDialog::getOpenFileName(
            consoleWindow,
            "Select a data file to open",
            last,
            filters );

    // user closed dialog box
    if( !fname.length() )
        return;

// --------
// Remember
// --------

    appData.lastViewedFile = fname;
    saveSettings();

// ------
// Valid?
// ------

    QString errorMsg;

    if( !DataFile::isValidInputFile( fname, &errorMsg ) ) {

        QMessageBox::critical(
            consoleWindow,
            "Error Opening File",
            QString("%1 cannot be used for input:\n[%2]")
            .arg( QFileInfo( fname ).fileName() )
            .arg( errorMsg ) );
        return;
    }

// -------------------
// Open in file viewer
// -------------------

    QWidget *fvPrev = win.find_if( FV_IsViewingFile( fname ) );

    if( fvPrev ) {
        // already in a viewer: just bring to top
        win.activateWindow( fvPrev );
    }
    else {
        // create new viewer
        FileViewerWindow    *fvw = new FileViewerWindow;

        if( !fvw->viewFile( fname, &errorMsg ) ) {

            QMessageBox::critical(
                consoleWindow,
                "Error Opening File",
                errorMsg );

            Error() << "FileViewer open error: " << errorMsg;
            delete fvw;
            return;
        }
    }
}


//=================================================================
//#include "Biquad.h"
//static void test1()
//{
//    Biquad  *flt = new Biquad( bq_type_highpass, 300/25000.0 );
//    FILE    *f = fopen( "C:/SGL_TEST/flttest.txt", "w" );
//    int     level = uniformDev( -30000, 30000 ),
//            ntpts = 1000;
//    vec_i16 V( ntpts, level );

//    fprintf( f, "level %d\n", level );
//    flt->apply1Blockwise( &V[0], ntpts, 1, 0 );

//    for( int k = 0; k < ntpts; ++k )
//        fprintf( f, "%d\t%d\n", k, V[k] );

//    fclose( f );
//    delete flt;
//}
//=================================================================


void MainApp::file_NewRun()
{
//test1();return;

    if( !file_AskStopRun() )
        return;

    if( configCtl->showDialog() )
        runCmdStart();
}


bool MainApp::file_AskStopRun()
{
    return run->askThenStopRun();
}


void MainApp::file_AskQuit()
{
    if( run->isRunning() ) {

        int yesNo = QMessageBox::question(
            0,
            "Confirm Exit",
            "Data acquisition in progress.\n\n"
            "Stop acquisition and quit?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo != QMessageBox::Yes )
            return;

        processEvents();

        QMessageBox *M = new QMessageBox(
            QMessageBox::Information,
            "Closing Files",
            "Closing files...please wait.",
            QMessageBox::NoButton,
            0 );

        M->show();
        processEvents();

        run->stopRun();

        delete M;
        processEvents();
    }

    msg.appQuiting();

    quit(); // user requested
}


void MainApp::options_ToggleDebug()
{
    appData.debug = !appData.debug;

    Log() << "Debug mode: " << (appData.debug ? "on" : "off");

    saveSettings();
}


void MainApp::options_PickRunDir()
{
    remoteMtx.lock();
    QString dir = appData.runDir;
    remoteMtx.unlock();

    dir = QFileDialog::getExistingDirectory(
            0,
            "Choose Run Directory",
            dir,
            QFileDialog::DontResolveSymlinks
            | QFileDialog::ShowDirsOnly );

    if( !dir.isEmpty() ) {

        remoteMtx.lock();
        appData.runDir = dir;
        remoteMtx.unlock();

        saveSettings();
    }
}


void MainApp::options_AODlg()
{
    if( aoCtl->showDialog() ) {

        win.addToMenu( aoCtl );
        win.activateWindow( aoCtl );
    }
}


void MainApp::tools_ImBist()
{
#ifdef HAVE_IMEC
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before running diagnostics." );
        return;
    }

    IMBISTCtl( consoleWindow );
#endif
}


void MainApp::tools_VerifySha1()
{
// Sha1Client is self-deleting object

    new Sha1Verifier();
}


void MainApp::tools_ShowPar2Win()
{
    if( !par2Win ) {

        par2Win = new Par2Window;
        par2Win->setAttribute( Qt::WA_DeleteOnClose, false );
        par2Win->setWindowTitle( APPNAME " - Par2 Redundancy Tool" );
        par2Win->setWindowIcon( QPixmap(ParWindowIcon_xpm) );
        ConnectUI( par2Win, SIGNAL(closed()), this, SLOT(par2WinClosed()) );
    }

    par2Win->show();
    win.addToMenu( par2Win );
    win.activateWindow( par2Win );
}


void MainApp::tools_ToggleEditLog()
{
    appData.editLog = !appData.editLog;

    Log() << "Log editing: " << (appData.editLog ? "on" : "off");

    consoleWindow->textEdit()->setReadOnly( !appData.editLog );

    saveSettings();
}


void MainApp::tools_SaveLogFile()
{
    consoleWindow->saveToFile();
}


void MainApp::window_ShowHideConsole()
{
    if( !consoleWindow )
        return;

    if( consoleWindow->isHidden() ) {

        consoleWindow->show();

#ifdef Q_OS_MACX
        win.addToMenu( consoleWindow );
#endif
    }
    else {

        bool hadfocus = (focusWidget() == consoleWindow);

        consoleWindow->hide();

#ifdef Q_OS_MACX
        win.removeFromMenu( consoleWindow );
#endif

        if( hadfocus )
            run->grfSetFocus();

        Systray() << "Show console from system tray icon.";
    }
}


void MainApp::window_ShowHideGraphs()
{
    run->grfShowHide();
}


void MainApp::help_HelpDlg()
{
    if( !helpWindow ) {

        helpWindow = new HelpWindow(
                            APPNAME" Help",
                            "CommonResources/Manual-Text.html" );
        ConnectUI( helpWindow, SIGNAL(closed()), this, SLOT(helpWindowClosed()) );
    }

    helpWindow->show();
    win.addToMenu( helpWindow );
    win.activateWindow( helpWindow );
}


void MainApp::help_About()
{
    QMessageBox about( consoleWindow );
    about.setWindowTitle( "About " APPNAME );
    about.setText(
        "<!DOCTYPE html>"
        "<html>"
        "<body>"
        "<h2>" VERSION_STR "</h2>"
        "<h3>Author: <a href=\"mailto:karshb@janelia.hhmi.org\">Bill Karsh</a></h3>"
        "<p>Based on the SpikeGL extracellular data acquisition system originally developed by Calin A. Culianu.</p>"
        "<p>Copyright (c) 2015, Howard Hughes Medical Institute, All rights reserved.</p>"
        "<p>Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:<br><a href=\"http://license.janelia.org/license\">http://license.janelia.org/license</a>.</p>"
        "</body>"
        "</html>"
    );

#if QT_VERSION >= 0x050300
    about.setTextInteractionFlags(
        Qt::TextBrowserInteraction |
        Qt::TextSelectableByKeyboard );
#endif

    about.exec();
}


void MainApp::help_AboutQt()
{
    QApplication::aboutQt();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void MainApp::aoCtlClosed()
{
    win.removeFromMenu( aoCtl );
}


void MainApp::par2WinClosed()
{
    win.removeFromMenu( par2Win );
}


void MainApp::helpWindowClosed()
{
    win.removeFromMenu( helpWindow );
}


void MainApp::fvwHelpWinClosed()
{
    win.removeFromMenu( fvwHelpWin );
}


void MainApp::showFVWHelpWin()
{
    if( !fvwHelpWin ) {

        fvwHelpWin = new HelpWindow(
                            APPNAME" File Viewer Help",
                            "CommonResources/Manual-Text.html" );
        ConnectUI( fvwHelpWin, SIGNAL(closed()), this, SLOT(fvwHelpWinClosed()) );
    }

    fvwHelpWin->show();
    win.addToMenu( fvwHelpWin );
    win.activateWindow( fvwHelpWin );
}


bool MainApp::remoteGetsIsConsoleHidden() const
{
    return isConsoleHidden();
}


void MainApp::remoteSetsRunName( const QString &name )
{
    configCtl->setRunName( name );
    run->grfRemoteSetsRunName( name );
}


QString MainApp::remoteStartsRun()
{
    QString eTitle, eMsg, err;

    if( !runCmdStart( &eTitle, &eMsg ) )
        err = QString("%1[%2]").arg( eTitle ).arg( eMsg );

    return err;
}


void MainApp::remoteStopsRun()
{
    run->stopRun();
}


void MainApp::remoteSetsDigitalOut( const QString &chan, bool onoff )
{
    CniCfg::setDO( chan, onoff );
}


void MainApp::remoteShowsConsole( bool show )
{
    if( show ^ consoleWindow->isVisible() )
        window_ShowHideConsole();
}


void MainApp::runIniting()
{
// --------------
// Initing dialog
// --------------

    if( runInitingDlg ) {
        delete runInitingDlg;
        runInitingDlg = 0;
    }

    runInitingDlg = new QProgressDialog(
        "DAQ Startup",
        "Abort", 0, 100, consoleWindow );

    runInitingDlg->setWindowFlags( runInitingDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    runInitingDlg->setWindowTitle( "Download Params" );
    runInitingDlg->setWindowModality( Qt::ApplicationModal );
    runInitingDlg->setAutoReset( false );
    ConnectUI( runInitingDlg, SIGNAL(canceled()), this, SLOT(runInitAbortedByUser()) );

    QSize   dlg = runInitingDlg->sizeHint();
    QRect   DT  = desktop()->screenGeometry();

    runInitingDlg->setMinimumWidth( 1.25 * dlg.width() );
    dlg = runInitingDlg->sizeHint();

    runInitingDlg->move(
        (DT.width()  - dlg.width()) / 2,
        (DT.height() - dlg.height())/ 2 );

    runInitingDlg->show();

// -------------------
// Initialize app data
// -------------------

    act.stopAcqAct->setEnabled( true );
}


void MainApp::runInitSetLabel( const QString &s )
{
    if( !runInitingDlg )
        return;

// If label string includes phrase "can't be aborted"
// then disable Abort button.

    QPushButton *B = runInitingDlg->findChild<QPushButton*>();

    if( B )
        B->setDisabled( s.contains( "can't be aborted" ) );

    runInitingDlg->setLabelText( QString("DAQ Startup (%1)").arg( s ) );
    runInitingDlg->setValue( 0 );
}


void MainApp::runInitSetValue( int val )
{
    if( !runInitingDlg )
        return;

    runInitingDlg->setValue( val );
}


void MainApp::runInitAbortedByUser()
{
    run->stopRun();
}


void MainApp::runStarted()
{
    if( runInitingDlg ) {

        QString s = "Acquisition started";

        Systray() << s;
        Status() << s;
        Log() << "GATE: " << s;

        runInitingDlg->setValue( 100 );
        delete runInitingDlg;
        runInitingDlg = 0;
    }
}


void MainApp::runStopped()
{
    if( runInitingDlg ) {
        delete runInitingDlg;
        runInitingDlg = 0;
    }

    act.stopAcqAct->setEnabled( false );
    act.shwHidGrfsAct->setEnabled( false );
}


void MainApp::runDaqError( const QString &e )
{
    QMessageBox::critical( 0, "DAQ Error", e );
    run->stopRun();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void MainApp::showStartupMessages()
{
    cmdSrv->showStartupMessage();
    rgtSrv->showStartupMessage();
}


void MainApp::loadRunDir( QSettings &settings )
{
    const char  *defRunDir = "SGL_DATA";

    remoteMtx.lock();

    appData.runDir =
        settings.value( "runDir" ).toString();

    if( appData.runDir.isEmpty()
        || !QFileInfo( appData.runDir ).exists() ) {

#ifdef Q_OS_WIN
        appData.runDir =
        QString("%1%2").arg( QDir::rootPath() ).arg( defRunDir );
#else
        appData.runDir =
        QString("%1/%2").arg( QDir::homePath() ).arg( defRunDir );
#endif

        if( !QDir( appData.runDir ).mkpath( appData.runDir ) ) {

            QMessageBox::critical(
                0,
                "Error Creating Data Directory",
                QString("Could not create default dir [%1].\n\n"
                "Use menu item [Options/Choose Run Directory]"
                " to set a valid data location.")
                .arg( appData.runDir ) );
        }
        else {
            QMessageBox::about(
                0,
                "Created Data Directory",
                QString("Default data dir [%1] was created for you.\n\n"
                "Select an alternate with menu item"
                " [Options/Choose Run Directory].")
                .arg( appData.runDir ) );
        }
    }

    remoteMtx.unlock();
}


void MainApp::loadSettings()
{
    STDSETTINGS( settings, "mainapp" );

    settings.beginGroup( "MainApp" );

    loadRunDir( settings );

    appData.lastViewedFile =
        settings.value( "lastViewedFile", runDir() ).toString();
    appData.debug =
        settings.value( "debug", false ).toBool();
    appData.editLog =
        settings.value( "editLog", false ).toBool();

    settings.endGroup();

    cmdSrv->loadSettings( settings );
    rgtSrv->loadSettings( settings );
}


bool MainApp::runCmdStart( QString *errTitle, QString *errMsg )
{
    QString locTitle, locMsg;
    bool    showDlg = !errTitle || !errMsg,
            ok;

    if( showDlg ) {
        errTitle    = &locTitle;
        errMsg      = &locMsg;
    }

    ok = run->startRun( *errTitle, *errMsg );

    if( !ok && showDlg )
        QMessageBox::critical( 0, locTitle, locMsg );

    return ok;
}


