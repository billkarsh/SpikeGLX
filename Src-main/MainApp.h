#ifndef MAINAPP_H
#define MAINAPP_H

#include "Main_Actions.h"
#include "Main_Msg.h"
#include "Main_WinMenu.h"

#include <QApplication>

namespace Ui {
class RunStartupWin;
}

class Run;
class ConsoleWindow;
class MetricsWindow;
class Par2Window;
class ConfigCtl;
class AOCtl;
class CmdSrvDlg;
class RgtSrvDlg;
class CalSRRun;
class SvyPrbRun;

class QSettings;

/* ---------------------------------------------------------------- */
/* App persistent data -------------------------------------------- */
/* ---------------------------------------------------------------- */

struct AppData {
    QStringList     slDataDir;
    QString         empty,
                    lastViewedFile;
    bool            multidrive,
                    debug,
                    editLog;
    mutable QMutex  remoteMtx;

    int nDirs() const;
    const QString& getDataDir( int i ) const;
    void makePathAbsolute( QString &path ) const;

    void dataDirDlg();
    void updateDataDir( QStringList &sl, bool isMulti );
    void setDir( const QString &path, int i );
    void setMulti( bool enable );
    void explore( QObject *sender, int idir ) const;

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

private:
    void resize_slDataDir( int n );
    void loadDataDir( QSettings &S );
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
    Run                 *run;
    ConsoleWindow       *consoleWindow;
    MetricsWindow       *mxWin;
    Par2Window          *par2Win;
    ConfigCtl           *configCtl;
    AOCtl               *aoCtl;
    CmdSrvDlg           *cmdSrv;
    RgtSrvDlg           *rgtSrv;
    CalSRRun            *calSRRun;
    SvyPrbRun           *svyPrbRun;
    QWidget             *rsWin;
    Ui::RunStartupWin   *rsUI;
    AppData             appData;
    bool                initialized;

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
    bool isInitialized() const      {return initialized;}
    bool isDebugMode() const        {return appData.debug;}
    bool isLogEditable() const      {return appData.editLog;}
    bool isConsoleHidden() const;
    bool isShiftPressed() const;

    void dataDirCtlUpdate( QStringList &sl, bool isMD );
    void remoteSetsMultiDriveEnable( bool enable );
    bool remoteSetsDataDir( const QString &path, int i );
    int nDataDirs() const                       {return appData.nDirs();}
    const QString& dataDir( int i = 0 ) const   {return appData.getDataDir( i );}
    void makePathAbsolute(QString &path) const  {appData.makePathAbsolute( path );}

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
    void options_PickDataDir()
        {appData.dataDirDlg();}
    void options_ExploreDataDir( int idir = 0 )
        {appData.explore( sender(), idir );}
    void options_AODlg();

// Tools
    void tools_VerifySha1();
    void tools_ShowPar2Win();
    void tools_CalSRate();
    void tools_ImClose();
    void tools_ImBist();
    void tools_ImHst();
    void tools_ImFirmware();
    void tools_ToggleDebug();
    void tools_ToggleEditLog();
    void tools_SaveLogFile();

// Window
    void window_ShowHideConsole();
    void window_ShowHideGraphs();
    void window_SpikeView();
    void window_ColorTTL();
    void window_MoreTraces();
    void window_RunMetrics();

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
    QString remoteSetsRunName( const QString &name );
    QString remoteStartsRun();
    void remoteStopsRun();
    void remoteSetsDigitalOut( const QString &chan, bool onoff );
    void remoteShowsConsole( bool show );

// Run synchronizes with app
    void rsInit();
    void rsAuxStep();
    void rsProbeStep();
    void rsStartStep();
    void rsAbort();
    void rsStarted();
    void rsStopped();
    void runDaqError( const QString &e );
    void runLogErrorToDisk( const QString &e );
    void runUpdateCalTimer();
    void runCalFinished();
    void runUpdateSvyTimer();
    void runSvyFinished();

// -------
// Private
// -------

private:
    void showStartupMessages();
    void loadSettings();
    bool runCmdStart( QString *err = 0 );
};

#endif  // MAINAPP_H


