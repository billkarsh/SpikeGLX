#ifndef MAINAPP_H
#define MAINAPP_H

#include "Main_Actions.h"
#include "Main_Msg.h"
#include "Main_WinMenu.h"

#include <QApplication>

class Run;
class ConsoleWindow;
class MetricsWindow;
class Par2Window;
class ConfigCtl;
class AOCtl;
class CmdSrvDlg;
class RgtSrvDlg;
class CalSRRun;

class QProgressDialog;
class QSettings;

/* ---------------------------------------------------------------- */
/* App persistent data -------------------------------------------- */
/* ---------------------------------------------------------------- */

struct AppData {
    QString dataDir,
            lastViewedFile;
    bool    debug,
            editLog;
};

/* ---------------------------------------------------------------- */
/* MainApp -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Global object/data repository:
// - Create app's main objects
// - Store them
// - Pass messages among them
// - Clean up
//
class MainApp : public QApplication
{
    Q_OBJECT

    friend int main( int, char** );
    friend class Main_Actions;

private:
    Run             *run;
    ConsoleWindow   *consoleWindow;
    MetricsWindow   *mxWin;
    Par2Window      *par2Win;
    ConfigCtl       *configCtl;
    AOCtl           *aoCtl;
    CmdSrvDlg       *cmdSrv;
    RgtSrvDlg       *rgtSrv;
    CalSRRun        *calSRRun;
    QProgressDialog *runInitingDlg;
    mutable QMutex  remoteMtx;
    AppData         appData;
    bool            initialized;

public:
    Main_Actions    act;
    Main_Msg        msg;
    Main_WinMenu    win;

// ------------
// Construction
// ------------

private:
    MainApp( int &argc, char **argv );  // < constructed by main()
public:
    virtual ~MainApp();

// ------------------
// Main object access
// ------------------

public:
    static MainApp *instance()
        {return dynamic_cast<MainApp*>(qApp);}

    Run *getRun() const
        {return run;}

    ConsoleWindow *console() const
        {return const_cast<ConsoleWindow*>(consoleWindow);}

    MetricsWindow *metrics() const
        {return const_cast<MetricsWindow*>(mxWin);}

    ConfigCtl *cfgCtl() const
        {return configCtl;}

    AOCtl *getAOCtl() const
        {return aoCtl;}

// ----------
// Properties
// ----------

public:
    bool isInitialized() const
        {QMutexLocker ml(&remoteMtx); return initialized;}
    bool isDebugMode() const            {return appData.debug;}
    bool isConsoleHidden() const;
    bool isShiftPressed() const;
    bool isLogEditable() const          {return appData.editLog;}

    bool remoteSetsDataDir( const QString &path );
    QString dataDir() const
        {QMutexLocker ml(&remoteMtx); return appData.dataDir;}
    void makePathAbsolute( QString &path );

    void saveSettings() const;

// ----------------
// Event processing
// ----------------

    void giveFocus2Console();
    void updateConsoleTitle( const QString &status );

// ------------------
// Menu item handlers
// ------------------

public slots:
// File menu
    void file_Open();
    void file_NewRun();
    bool file_AskStopRun();
    void file_AskQuit();

// Options
    void options_PickDataDir();
    void options_ExploreRunDir();
    void options_AODlg();

// Tools
    void tools_VerifySha1();
    void tools_ShowPar2Win();
    void tools_CalSRate();
    void tools_ImClose();
    void tools_ImBist();
    void tools_ImFirmware();
    void tools_ToggleDebug();
    void tools_ToggleEditLog();
    void tools_SaveLogFile();

// Window
    void window_ShowHideConsole();
    void window_ShowHideGraphs();
    void window_RunMetrics();
    void window_MoreTraces();

// Help
    void help_HelpDlg();
    void help_ExploreApp();
    void help_About();
    void help_AboutQt();

// -----
// Slots
// -----

public slots:
// Worker yields to GUI thread
    void mainProcessEvents();

// Window needs app service
    void modelessOpened( QWidget *w, bool activate = true );
    void modelessClosed( QWidget *w );

// CmdSrv
    bool remoteGetsIsConsoleHidden() const;
    void remoteSetsRunName( const QString &name );
    QString remoteStartsRun();
    void remoteStopsRun();
    void remoteSetsDigitalOut( const QString &chan, bool onoff );
    void remoteShowsConsole( bool show );

// Run synchronizes with app
    void runIniting();
    void runInitSetLabel( const QString &s, bool zero = false );
    void runInitSetValue( int val );
    void runInitAbortedByUser();
    void runStarted();
    void runStopped();
    void runDaqError( const QString &e );
    void runLogErrorToDisk( const QString &e );
    void runUpdateCalTimer();
    void runCalFinished();

// -------
// Private
// -------

private:
    void showStartupMessages();
    void loadDataDir( QSettings &settings );
    void loadSettings();
    bool runCmdStart( QString *errTitle = 0, QString *errMsg = 0 );
};

#endif  // MAINAPP_H


