
#include "Pixmaps/Icon.xpm"
#include "Pixmaps/ParWindowIcon.xpm"
#include "ui_RunStartupWin.h"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "MetricsWindow.h"
#include "FileViewerWindow.h"
#include "DFName.h"
#include "ConfigCtl.h"
#include "DataDirCtl.h"
#include "AOCtl.h"
#include "CmdSrvDlg.h"
#include "RgtSrvDlg.h"
#include "Run.h"
#include "CalSRateCtl.h"
#include "SvyPrb.h"
#include "IMBISTCtl.h"
#include "IMHSTCtl.h"
#include "IMFirmCtl.h"
#include "Sha1Verifier.h"
#include "Par2Window.h"
#include "Version.h"
#include "WavePlanCtl.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QScreen>
#include <QMessageBox>
#include <QAction>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>


/* ---------------------------------------------------------------- */
/* AppData -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int AppData::nDirs() const
{
    QMutexLocker    ml( &remoteMtx );

    int n = slDataDir.size();

    if( multidrive )
        return n;
    else
        return (n ? 1 : 0);
}


const QString& AppData::getDataDir( int i ) const
{
    QMutexLocker    ml( &remoteMtx );

    if( i < 0 || i >= slDataDir.size() )
        return empty;

    return slDataDir[i];
}


void AppData::makePathAbsolute( QString &path ) const
{
    if( !QFileInfo( path ).isAbsolute() ) {

        QRegExp re("([^/\\\\]+_[gG]\\d+)_[tT]\\d+");

        QMutexLocker    ml( &remoteMtx );

        if( path.contains( re ) ) {

            path = QString("%1/%2/%3")
                    .arg( slDataDir[0] ).arg( re.cap(1) ).arg( path );
        }
        else {
            path = QString("%1/%2")
                    .arg( slDataDir[0] ).arg( path );
        }
    }
}


void AppData::dataDirDlg()
{
    remoteMtx.lock();
    DataDirCtl  D( 0, slDataDir, multidrive );
    remoteMtx.unlock();

    D.run();
}


void AppData::updateDataDir( QStringList &sl, bool isMulti )
{
    QMutexLocker    ml( &remoteMtx );
    slDataDir   = sl;
    multidrive  = isMulti;
}


void AppData::setDir( const QString &path, int i )
{
    QMutexLocker    ml( &remoteMtx );
    while( slDataDir.size() < i + 1 )
        slDataDir.append( QString() );
    slDataDir[i] = path;
}


void AppData::setMulti( bool enable )
{
    QMutexLocker    ml( &remoteMtx );
    multidrive = enable;
}


void AppData::explore( QObject *sender, int idir ) const
{
    QMutexLocker    ml( &remoteMtx );

    QAction *s = dynamic_cast<QAction*>(sender);

    if( s ) {

        bool    ok = false;
        int     id = s->objectName().toInt( &ok );

        if( ok )
            idir = id;
    }

    if( idir < slDataDir.size() )
        QDesktopServices::openUrl( QUrl::fromUserInput( slDataDir[idir] ) );
}


void AppData::loadSettings( QSettings &S )
{
    S.beginGroup( "MainApp" );

    loadDataDir( S );

    lastViewedFile  = S.value( "lastViewedFile", getDataDir( 0 ) ).toString();
    debug           = S.value( "debug", false ).toBool();
    editLog         = S.value( "editLog", false ).toBool();

    S.endGroup();
}


void AppData::saveSettings( QSettings &S ) const
{
    S.beginGroup( "MainApp" );

    S.setValue( "lastViewedFile", lastViewedFile );
    S.setValue( "debug", debug );
    S.setValue( "editLog", editLog );

    QMutexLocker    ml( &remoteMtx );
    S.setValue( "dataDir", slDataDir );
    S.setValue( "multidrive", multidrive );

    S.endGroup();
}


void AppData::loadDataDir( QSettings &S )
{
    slDataDir   = S.value( "dataDir" ).toStringList();
    multidrive  = S.value( "multidrive", false ).toBool();

    if( slDataDir.isEmpty() || !QFileInfo( slDataDir[0] ).exists() ) {

        const char  *defDataDir = "SGL_DATA";

        multidrive = false;

#ifdef Q_OS_WIN
        slDataDir.prepend(
        QString("%1%2").arg( QDir::rootPath() ).arg( defDataDir ) );
#else
        slDataDir.prepend(
        QString("%1/%2").arg( QDir::homePath() ).arg( defDataDir ) );
#endif

        if( !QDir().mkpath( slDataDir[0] ) ) {

            QMessageBox::critical(
                0,
                "Error Creating Data Directory",
                QString("Could not create default dir '%1'.\n\n"
                "Use menu item [Options/Choose Data Directory]"
                " to set a valid data location.")
                .arg( slDataDir[0] ) );
        }
        else {
            QMessageBox::about(
                0,
                "Created Data Directory",
                QString("Default data dir was created for you '%1'.\n\n"
                "Select an alternate with menu item"
                " [Options/Choose Data Directory].")
                .arg( slDataDir[0] ) );
        }
    }
}

/* ---------------------------------------------------------------- */
/* Ctor ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

//#define NPDEBUG

#ifdef NPDEBUG
using namespace Neuropixels;
void npcallback( int level, time_t ts, const char *module, const char *msg )
{
    Log()<<QString("L%1 [%2] \"%3\"").arg( level ).arg( module ).arg( msg );
    QFile   fo( "api_out.txt" );
    fo.open( QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text );
    QTextStream to( &fo );
    to << QString("L%1 [%2] \"%3\"\n").arg( level ).arg( module ).arg( msg );
    fo.flush();
    fo.close();
}
#endif


MainApp::MainApp( int &argc, char **argv )
    :   QApplication(argc, argv, true),
        run(0), consoleWindow(0), mxWin(0), par2Win(0),
        configCtl(0), aoCtl(0), cmdSrv(new CmdSrvDlg),
        rgtSrv(new RgtSrvDlg), calSRRun(0), svyPrbRun(0),
        rsWin(0), rsUI(0), initialized(false), quitting(false)
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

// Run ctor is lightweight, and Run is called by Warning() and Error()
// logging utils so we create it early.

    run = new Run( this );

    act.initActions();
    win.setActionPtr( &act );

// -----------
// Main window
// -----------

    consoleWindow = new ConsoleWindow;
    consoleWindow->setAttribute( Qt::WA_DeleteOnClose, false );
    consoleWindow->show();

#ifdef Q_OS_MACX
// Add console window to the Window menu, an app-global menu on OS-X
    win.addToMenu( consoleWindow );
#endif

// ------------
// Message logs
// ------------

    msg.initMessenger( consoleWindow );

    Log() << VERS_SGLX_STR;
    Log() << "Application started";

    mxWin = new MetricsWindow;
    mxWin->setAttribute( Qt::WA_DeleteOnClose, false );
    ConnectUI( mxWin, SIGNAL(closed(QWidget*)), this, SLOT(modelessClosed(QWidget*)) );

// -------------
// Other helpers
// -------------

    configCtl = new ConfigCtl;

    aoCtl = new AOCtl( configCtl->acceptedParams );
    aoCtl->setAttribute( Qt::WA_DeleteOnClose, false );
    aoCtl->setWindowTitle( APPNAME " - Audio Settings" );
    ConnectUI( aoCtl, SIGNAL(closed(QWidget*)), this, SLOT(modelessClosed(QWidget*)) );

// ----
// Done
// ----

    initialized = true;

    Log() << "Application initialized";
    Status() <<  APPNAME << " initialized.";

    updateConsoleTitle( "READY" );

    // Don't start servers until app initialized

    cmdSrv->startServer( true );
    rgtSrv->startServer( true );

    showStartupMessages();

#ifdef NPDEBUG
    np_dbg_setlogcallback( DBG_VERBOSE, npcallback );
    np_dbg_setlevel( DBG_VERBOSE );
#endif
}


MainApp::~MainApp()
{
    if( par2Win ) {
        delete par2Win;
        par2Win = 0;
    }

    if( rgtSrv ) {
        delete rgtSrv;
        rgtSrv = 0;
    }

    if( cmdSrv ) {
        delete cmdSrv;
        cmdSrv = 0;
    }

    if( aoCtl ) {
        delete aoCtl;
        aoCtl = 0;
    }

    if( configCtl ) {
        delete configCtl;
        configCtl = 0;
    }

    if( mxWin ) {
        delete mxWin;
        mxWin = 0;
    }

    if( consoleWindow ) {
        delete consoleWindow;
        consoleWindow = 0;
    }

    if( run ) {
        delete run;
        run = 0;
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


void MainApp::dataDirCtlUpdate( QStringList &sl, bool isMD )
{
    appData.updateDataDir( sl, isMD );
    saveSettings();
    act.ddExploreUpdate();
}


void MainApp::remoteSetsMultiDriveEnable( bool enable )
{
    appData.setMulti( enable );
    saveSettings();
    act.ddExploreUpdate();
}


bool MainApp::remoteSetsDataDir( const QString &path, int i )
{
    if( i < 0 )
        return false;

    QString _path = rmvLastSlash( path );

    if( !QDir( _path ).exists() )
        return false;

    appData.setDir( _path, i );
    saveSettings();
    act.ddExploreUpdate();

    Log() << QString("Remote client set data dir (%1): '%2'")
                .arg( i ).arg( path );

    return true;
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
    STDSETTINGS( S, "mainapp" );

    appData.saveSettings( S );
    cmdSrv->saveSettings( S );
    rgtSrv->saveSettings( S );
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
        last = dataDir();
    else if( !fi.exists() ) {

        // Failing to locate the last choice, if last was a file
        // with pattern 'path/file' we will offer the 'path' part.
        // However, if last was already a 'path' then we fall back
        // to the dataDir.

        // Note that fi.isFile() and fi.isDir() look at the string
        // pattern AND check existence so are not what we want.
        // We do our own pattern match instead:
        //
        //      path + slash + one-or-more-not-slashes + dot + arb.

        QRegExp re("(.*)[/\\\\][^/\\\\]+\\..*");

        if( last.contains( re ) ) {

            fi.setFile( last = re.cap(1) );

            if( !fi.isDir() )
                last = dataDir();
        }
        else
            last = dataDir();
    }

// ----
// Pick
// ----

// BK: FileOpen dialog tends to clip all but right 11 or 12 characters
// BK: of the last-selected filename. Seems internal to the Qt dialog.

    QString fname =
        QFileDialog::getOpenFileName(
            consoleWindow,
            "Select a data file to open",
            last,
            APPNAME" Data (*.bin)" );

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

    if( !DFName::isValidInputFile( errorMsg, fname, {} ) ) {

        QMessageBox::critical(
            consoleWindow,
            "Error Opening File",
            QString("File cannot be used for input '%1':\n[%2]")
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

        if( !fvw->viewFile( errorMsg, fname ) ) {

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
// Experiment to parse imec stream files.
#if 0
static void test1()
{
    FILE *fi = fopen( "Y:/__billKarsh__/imstream.bin", "rb" );
    if( !fi )
        return;
    Log()<<"open";guiBreathe();
    FILE *fo = fopen( "zzz.txt", "w" );

    int maxBytes, headBytes = 80;

    fseek( fi, 0, SEEK_END );
    maxBytes = ftell( fi );
    fseek( fi, headBytes, SEEK_SET );
    Log()<< maxBytes;

    quint32 data[124];
    int     k = 0;

    for( int bytes = headBytes; bytes < maxBytes; bytes += sizeof(data) ) {

        if( 1 != fread( data, sizeof(data), 1, fi ) ) {
            Log() << "failed read";
            break;
        }

        if( data[3] & 0x02 )
            continue;

        ++k;

        fprintf( fo, "%d\t%d\n", k, data[2] % 8000 );

        if( k >= 50000 )
            break;
    }

    Log() << "done";

    fclose( fo );
    fclose( fi );
}
#endif
//=================================================================


//=================================================================
// Experiment to seek NO_LOCK errors.
#if 0
#include "IMEC/NeuropixAPI.h"
#include <QRandomGenerator>
static void test1()
{
    QRandomGenerator    RN;
    RN.seed( quint32(getTime()) );
    int iter = 0, totErr = 0, slot = 2;
    NP_ErrorCode    err = SUCCESS;

    err = np_openBS( slot );
    if( err != SUCCESS )
        slot = 3;

    for(;;) {

        qint32  delay = 0;

        np_openBS( slot );

//        if( iter & 1 )
//            QThread::msleep( delay = RN.bounded( 0, 4000 ) );

//        err = np_openProbe( slot, 2 );

//        if( err != SUCCESS )
//            ++totErr;

//        FILE *fo = fopen( "lock_test.txt", "a" );
//        fprintf( fo, "%d\t%d\t%d\n", iter, err, delay );
//        fclose( fo );

        Log()<< QString("iters %1  errs %2").arg( iter+1 ).arg( totErr );
        ++iter;

        np_closeBS( slot );

        guiBreathe();
//        QThread::msleep( 15*1000 );
    }
}
#endif
//=================================================================


//=================================================================
// Experiment to write 512-channel chanmap.
#if 0
static void test1()
{
    FILE *fo = fopen( "chmap512.txt", "w" );

    fprintf( fo, "(512,0,1,0,0)" );

    for( int i = 0; i < 512; ++i ) {
        fprintf( fo, "(MN0C%d;%d:%d)", i, i, i );
    }

    fprintf( fo, "\n" );
    fclose( fo );
}
#endif
//=================================================================


//=================================================================
// Experiment to write 384-channel shankmap.
#if 0
static void test1()
{
    FILE *fo = fopen( "shmap512.txt", "w" );

    fprintf( fo, "(1,3,172)" );

    int N = 0;
    for( int r = 0; r < 172; ++r ) {
        fprintf( fo, "(0:0:%d:1)", r );
        if( ++N == 512 ) break;
        fprintf( fo, "(0:1:%d:1)", r );
        if( ++N == 512 ) break;
        fprintf( fo, "(0:2:%d:1)", r );
        if( ++N == 512 ) break;
    }

    fprintf( fo, "\n" );
    fclose( fo );
}
#endif
//=================================================================


//=================================================================
// Experiment to write tetrode imro table.
#if 0
static void test1()
{
    FILE *fo = fopen( "Tetrode_1shank.imro", "w" );

    fprintf( fo, "(0,384)" );

    for( int r = 0; r < 384/8; ++r ) {
        fprintf( fo, "(%d 0 0 500 250 1)", 8*r );
        fprintf( fo, "(%d 0 0 500 250 1)", 8*r+1 );
        fprintf( fo, "(%d 0 0 500 250 1)", 8*r+2 );
        fprintf( fo, "(%d 0 0 500 250 1)", 8*r+3 );
        fprintf( fo, "(%d 1 0 500 250 1)", 8*r+4 );
        fprintf( fo, "(%d 1 0 500 250 1)", 8*r+5 );
        fprintf( fo, "(%d 1 0 500 250 1)", 8*r+6 );
        fprintf( fo, "(%d 1 0 500 250 1)", 8*r+7 );
    }

    fprintf( fo, "\n" );
    fclose( fo );
}
#endif
//=================================================================


//=================================================================
// Experiment to write longCol imro table.
#if 0
static void test1()
{
    FILE *fo = fopen( "LongCol_1shank.imro", "w" );

    fprintf( fo, "(0,384)" );

    for( int r = 0; r < 192; ++r ) {
        fprintf( fo, "(%d 0 0 500 250 1)", 2*r );
        fprintf( fo, "(%d 1 0 500 250 1)", 2*r+1 );
    }

    fprintf( fo, "\n" );
    fclose( fo );
}
#endif
//=================================================================


//=================================================================
// Experiment to check NHP128 mux table.
#if 0
#include "IMROTbl_T1200.h"
static void test1()
{
    IMROTbl_T1200       *R = new IMROTbl_T1200;
    std::vector<int>    T;
    int                 nADC, nGrp;
    bool                ok = true;

    R->muxTable( nADC, nGrp, T );

    for( int i = 0; i < 129; ++i ) {

        int N = 0;

        for( int j = 0; j < 144; ++j ) {

            if( T[j] == i )
                ++N;
        }

        if( i == 128 ) {

            if( N != 16 ) {
                Log() << QString("chn 128 appears %1 times").arg( N );
                ok = false;
            }
        }
        else if( N != 1 ) {
            Log() << QString("chn %1 appears %2 times").arg( i ).arg( N );
            ok = false;
        }
    }

    Log() << "mux test ok " << ok;

    delete R;
}
#endif
//=================================================================


//=================================================================
// Experiment to parse cam files.
#if 0
static void test1()
{
    QFile   fc( "C:/Users/karshb/Desktop/SGLBUILD/FIXU/CatGT/CatGT-win/catgt_SC048_010121_g0/SC048_010121_g0_tcat.nidq.iXA_4_3.txt" ),
            ft( "C:/Users/karshb/Desktop/SGLBUILD/FIXU/CatGT/CatGT-win/catgt_SC048_010121_g0/SC048_010121_g0_tcat.nidq.XD_7_2_0.txt" );
    fc.open( QIODevice::ReadOnly | QIODevice::Text );
    ft.open( QIODevice::ReadOnly | QIODevice::Text );

    QFile   fo( "C:/Users/karshb/Desktop/SGLBUILD/FIXU/CatGT/CatGT-win/catgt_SC048_010121_g0/out.txt" );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream to( &fo );

    for(;;) {

        QString lc = fc.readLine(),
                lt = ft.readLine();

        if( lc.isEmpty() || lt.isEmpty() )
            break;

        double  dc = lc.toDouble(),
                dt = lt.toDouble();

        to
            << QString("%1\t").arg( dc, 0, 'f', 6 )
            << QString("%1\n").arg( dc-dt, 0, 'f', 6 );
    }
}
#endif
//=================================================================


//=================================================================
// Experiment to test rgtSetXXX.
#if 0
#include "RgtServer.h"
static void test1()
{
    ns_RgtServer::rgtSetGate( true );
    QThread::msleep( 5*1000 );
    ns_RgtServer::rgtSetGate( false );
}
#endif
//=================================================================


//=================================================================
// Experiment to test opto_getAttens.
#if 0
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
static QString makeErrorString( NP_ErrorCode err )
{
    char    buf[2048];
    size_t  n = np_getLastErrorMessage( buf, sizeof(buf) );

    if( n >= sizeof(buf) )
        n = sizeof(buf) - 1;

    buf[n] = 0;

    return QString(" error %1 '%2'.").arg( err ).arg( QString(buf) );
}
static void test1()
{
    double          atten;
    QString         msg;
    NP_ErrorCode    err;

    err = np_openBS( 5 );

    if( err != SUCCESS ) {
        msg = QString("IMEC openBS( %1 )%2").arg( 5 ).arg( makeErrorString( err ) );
        goto exit;
    }

    err = np_openProbe( 5, 1, 0 );

    if( err != SUCCESS ) {
        Log() << QString("Continue but openProbe fail %2").arg( makeErrorString( err ) );
    }

    for( int site = 0; site < 14; ++site ) {

        err = np_getEmissionSiteAttenuation(
                5, 1, 0, wavelength_t(wavelength_red), site, &atten );

        if( err != SUCCESS ) {
            msg = QString("IMEC getEmissionSiteAttenuation(slot %1, port %2)%3")
                    .arg( 5 ).arg( 1 ).arg( makeErrorString( err ) );
            break;
        }

        msg += QString("%1 ").arg( atten, 0, 'f', 9 );
    }

exit:
    np_closeBS( 5 );
    Log() << msg;
}
#endif
//=================================================================


//=================================================================
// Experiment to parse UHD2 IMRO.
#if 0
#include "IMROTbl_T1110.h"
static void test1()
{
    IMROTbl_T1110   *R = new IMROTbl_T1110;

    QString msg;
    if( !R->loadFile( msg, "V:/AA_SGLX/UHD2/NPUHD2_outer_vstripe_ref0.imro" ) ) {
        Log()<<msg;
        delete R;
        return;
    }

    QFile   fo( "V:/AA_SGLX/UHD2/out.txt" );
    fo.open( QIODevice::WriteOnly | QIODevice::Text );
    QTextStream to( &fo );

    for(int ic = 0; ic < 384; ++ic ) {

        to << QString("%1\t%2\n").arg( ic ).arg( R->chToEl( ic ) );
    }

    delete R;
}
#endif
//=================================================================


//=================================================================
// Experiment to extract API latency.
#if 0
static void test1()
{
    double mv2bits = 1.0/(1200.0/250/1024);
    int low = -0.24*mv2bits;
    int hi = 0.44*mv2bits;
    QFile fi( "V:/AA_SGLX/hctest/latency_meas/cpp_latent_g0_t0.imec0.lf.bin" );
    fi.open( QIODevice::ReadOnly );
    qint64 N = fi.size() / 2;
    QVector<short> D(N);
    fi.read( (char*)&D[0], 2*N );
    int nC = 385;
    int nt = N/nC;
    int L;
    int state = -1; //-1=seekbase, 0=seeklow, 1=seekhi
    int mask = 1 << 6;

    double X;

    for( int it = 0; it < nt; ++it ) {

        int v = D[393-384+it*nC];

        if( state == -1 ) {

            if( v < low )
                state = 0;
        }

        if( state == 0 ) {

            if( v >= low ) {
                L = it;
                state = 1;
            }
        }

        if( state == 1 ) {

            if( v >= hi ) {
                X = (L + it)/2;

                for( int jt = it+1; jt < nt; ++jt ) {
                    if( D[768-384+jt*nC] & mask ) {
                        X = (jt - 0.5 - X) / 2.500;
                        if( X > 2.0 )
                            Log()<<X;
                        break;
                    }
                    if( jt-it > 0.10*2500 )
                        break;
                }

                state = -1;
            }
        }
    }
}
#endif
//=================================================================


//=================================================================
// Experiment to test sleep.
#if 0
static void test1()
{
    setPreciseTiming( true );

    for( int dt = 100000; dt >= 1; dt /= 1.5 ) {
        double  t = getTime();
        QThread::usleep( dt );
        Log() << dt << "    " << 1e6*(getTime()-t);
    }

    setPreciseTiming( false );
}
#endif
//=================================================================


//=================================================================
// Experiment to test ProbeTable ops.
#if 0
#include "ProbeTable.h"
static void test1()
{
    CProbeTbl::ss2ini();
    CProbeTbl::extini();
    CProbeTbl::sortini();
    CProbeTbl::ini2json();
    CProbeTbl::json2ini();
    CProbeTbl::sortjson2ini();
    CProbeTbl::parsejson();
}
#endif
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
    if( quitting )
        return;

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

        Log();
        Log();
        Log() << "Quitting...";

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

    quitting = true;
    msg.appQuiting();
    win.closeAll();

    quit(); // user requested
}


void MainApp::tools_VerifySha1()
{
// Sha1Verifier is self-deleting object

    new Sha1Verifier;
}


void MainApp::tools_ShowPar2Win()
{
    if( !par2Win ) {

        par2Win = new Par2Window;
        par2Win->setAttribute( Qt::WA_DeleteOnClose, false );
        par2Win->setWindowTitle( APPNAME " - Par2 Redundancy Tool" );
        par2Win->setWindowIcon( QPixmap(ParWindowIcon_xpm) );
        ConnectUI( par2Win, SIGNAL(closed(QWidget*)), this, SLOT(modelessClosed(QWidget*)) );
    }

    par2Win->showNormal();
    modelessOpened( par2Win );
}


void MainApp::tools_CalSRate()
{
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before starting calibration." );
        return;
    }

    CalSRateCtl rateCtl( consoleWindow );
}


void MainApp::tools_ImClose()
{
#ifdef HAVE_IMEC
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before close hardware." );
        return;
    }

    CimCfg::closeAllBS();
#endif
}


void MainApp::tools_ImBist()
{
#ifdef HAVE_IMEC
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before starting diagnostics." );
        return;
    }

    IMBISTCtl bistCtl( consoleWindow );
#endif
}


void MainApp::tools_ImHst()
{
#ifdef HAVE_IMEC
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before starting diagnostics." );
        return;
    }

    IMHSTCtl hstCtl( consoleWindow );
#endif
}


void MainApp::tools_ImFirmware()
{
#ifdef HAVE_IMEC
    if( run->isRunning() ) {

        QMessageBox::critical(
            consoleWindow,
            "Run in Progress",
            "Stop the current run before starting updater." );
        return;
    }

    IMFirmCtl firmCtl( consoleWindow );
#endif
}


void MainApp::tools_ToggleDebug()
{
    appData.debug = !appData.debug;

    Log() << "Debug mode: " << (appData.debug ? "on" : "off");

    saveSettings();
}


void MainApp::tools_ToggleEditLog()
{
    appData.editLog = !appData.editLog;

    Log() << "Log editing: " << (appData.editLog ? "on" : "off");

    consoleWindow->setReadOnly( !appData.editLog );

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

        consoleWindow->showNormal();

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
            run->grfSetFocusMain();

        Systray() << "Show console from system tray icon.";
    }
}


void MainApp::window_ShowHideGraphs()
{
    run->grfShowHideAll();
}


void MainApp::window_WVDlg()
{
    WavePlanCtl W( 0 );
}


void MainApp::window_AODlg()
{
    if( aoCtl->showDialog() )
        modelessOpened( aoCtl );
}


void MainApp::window_SpikeView()
{
    run->grfShowSpikes( -1, -1, -1 );
}


void MainApp::window_ColorTTL()
{
    run->grfShowColorTTL();
}


void MainApp::window_MoreTraces()
{
    run->grfMoreTraces();
}


void MainApp::window_RunMetrics()
{
    mxWin->showDialog();
    modelessOpened( mxWin );
}


void MainApp::help_HelpDlg()
{
    showHelp( "UserManual" );
}


void MainApp::help_ExploreApp()
{
    QDesktopServices::openUrl( QUrl::fromUserInput( appPath() ) );
}


void MainApp::help_About()
{
    QMessageBox about( consoleWindow );
    about.setWindowTitle( "About " APPNAME );
    about.setText(
        "<!DOCTYPE html>"
        "<html>"
        "<body>"
        "<h2>" VERS_SGLX_STR "</h2>"
        "<h3>Author: <a href=\"mailto:karshb@janelia.hhmi.org\">Bill Karsh</a></h3>"
        "<p>Based on the SpikeGL extracellular data acquisition system originally developed by Calin A. Culianu.</p>"
        "<p>Copyright (c) 2024, Howard Hughes Medical Institute, All rights reserved.</p>"
        "<p>Use is subject to Janelia Research Campus Software Copyright 1.2 license terms:<br><a href=\"http://license.janelia.org/license\">http://license.janelia.org/license</a>.</p>"
        "<p>QLed components are subject to GNU Library GPL v2.0 terms:<br><a href=\"https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt\">https://github.com/billkarsh/SpikeGLX/blob/master/QLed-LGPLv2-LICENSE.txt</a>.</p>"
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

void MainApp::mainProcessEvents()
{
    processEvents();
}


void MainApp::modelessOpened( QWidget *w, bool activate )
{
    win.addToMenu( w );

    if( activate )
        win.activateWindow( w );
}


void MainApp::modelessClosed( QWidget *w )
{
    win.removeFromMenu( w );
}


bool MainApp::remoteGetsIsConsoleHidden() const
{
    return isConsoleHidden();
}


QString MainApp::remoteSetsRunName( const QString &name )
{
    QString err;

    if( !run->isRunning()
        || name.compare(
            configCtl->acceptedParams.sns.runName,
            Qt::CaseInsensitive ) != 0 ) {

        if( configCtl->externSetsRunName( err, name, 0 ) )
            run->grfRemoteSetsRunLE( name );
    }

    return err;
}


QString MainApp::remoteStartsRun()
{
    QString err;

    configCtl->dialog()->close();

    runCmdStart( &err );

    return err;
}


void MainApp::remoteStopsRun()
{
    run->stopRun();
}


void MainApp::remoteShowsConsole( bool show )
{
    if( show ^ consoleWindow->isVisible() )
        window_ShowHideConsole();
}


// First call made during run startup.
// Modifications to run parameters can be
// made here, as for calibration runs.
//
void MainApp::rsInit()
{
    DAQ::Params &p = cfgCtl()->acceptedParams;

// --------------
// Initing dialog
// --------------

    if( rsWin ) {
        delete rsWin;
        rsWin = 0;
    }

    if( rsUI ) {
        delete rsUI;
        rsUI = 0;
    }

    rsWin = new QWidget;

    Qt::WindowFlags F = rsWin->windowFlags();
    F |= Qt::WindowStaysOnTopHint;
    F &= ~(Qt::WindowContextHelpButtonHint | Qt::WindowCloseButtonHint);
    rsWin->setWindowFlags( F );

    rsUI = new Ui::RunStartupWin;
    rsUI->setupUi( rsWin );
    ConnectUI( rsUI->abortBut, SIGNAL(clicked()), this, SLOT(rsAbort()) );

    rsUI->auxBar->setValue( 0 );
    rsUI->auxBar->setMaximum( p.stream_nNI() + 3 * p.im.enabled );

    int np = p.stream_nIM();
    if( np ) {
        rsUI->label->setText( QString("%1").arg( np ) );
        rsUI->prb0->setValue( 0 );
        rsUI->prb0->setMaximum( 11 * np );
    }
    else {
        QSize   sz = rsUI->probesBox->sizeHint();
        rsUI->probesBox->hide();
        rsWin->resize( rsWin->width(), rsWin->height() - sz.height() - 20 );
    }

    rsUI->startBar->setValue( 0 );
    rsUI->startBar->setMaximum( p.stream_nNI() + 3 * p.im.enabled );

    rsWin->layout()->update();
    rsWin->layout()->activate();
    QRect   s = rsWin->frameGeometry(),
            S = QGuiApplication::primaryScreen()->geometry();
    rsWin->move( (S.width() - s.width())/2, (S.height() - s.height())/2 );
    rsWin->show();

// -------------------
// Initialize app data
// -------------------

    mxWin->runInit();

    act.stopAcqAct->setEnabled( true );

    if( configCtl->acceptedParams.sync.isCalRun ) {

        calSRRun = new CalSRRun;
        calSRRun->initRun();
    }
    else if( configCtl->acceptedParams.im.prbAll.isSvyRun ) {

        svyPrbRun = new SvyPrbRun;
        svyPrbRun->initRun();
    }
}


void MainApp::rsAuxStep()
{
    if( rsWin )
        rsUI->auxBar->setValue( 1 + rsUI->auxBar->value() );
}


void MainApp::rsProbeStep()
{
    if( rsWin )
        rsUI->prb0->setValue( 1 + rsUI->prb0->value() );
}


void MainApp::rsStartStep()
{
    if( rsWin )
        rsUI->startBar->setValue( 1 + rsUI->startBar->value() );
}


void MainApp::rsAbort()
{
    if( rsWin ) {
        rsUI->statusLbl->setText( "Aborting...hold on..." );
        processEvents();
    }

    run->stopRun();
}


void MainApp::rsStarted()
{
    if( rsWin ) {

        mxWin->runStart();

        QString s = "Acquisition started";

        Systray() << s;
        Status() << s;
        Log() << "GATE: " << s;

        delete rsWin;
        delete rsUI;
        rsWin = 0;
        rsUI  = 0;
    }

    if( calSRRun ) {
        // Due to limited accuracy, long intervals are
        // best implemented as sequences of short ones.
        QTimer::singleShot( 60000, this, SLOT(runUpdateCalTimer()) );
        calSRRun->initTimer();
    }
    else if( svyPrbRun ) {
        QTimer::singleShot(
            svyPrbRun->msPerBnk( true ), this, SLOT(runUpdateSvyTimer()) );
    }
}


// Last call made upon run termination.
// Final cleanup tasks can be placed here.
//
void MainApp::rsStopped()
{
    if( rsWin ) {
        delete rsWin;
        rsWin = 0;
    }

    if( rsUI ) {
        delete rsUI;
        rsUI = 0;
    }

    mxWin->runEnd();

    act.stopAcqAct->setEnabled( false );
    act.shwHidGrfsAct->setEnabled( false );
    act.spikeViewAct->setEnabled( false );
    act.colorTTLAct->setEnabled( false );
    act.moreTracesAct->setEnabled( false );

    if( calSRRun ) {

        QMetaObject::invokeMethod(
            calSRRun, "finish",
            Qt::QueuedConnection );
    }
    else if( svyPrbRun ) {

        QMetaObject::invokeMethod(
            svyPrbRun, "finish",
            Qt::QueuedConnection );
    }
}


void MainApp::runDaqError( const QString &e )
{
    run->stopRun();
    QMessageBox::critical( 0, "DAQ Error", e );
}


void MainApp::runLogErrorToDisk( const QString &e )
{
    if( run->isRunning() ) {

        QFile f( QString("%1/%2.errors.txt")
                .arg( dataDir() )
                .arg( configCtl->acceptedParams.sns.runName ) );
        f.open( QIODevice::Append | QIODevice::Text );
        QTextStream ts( &f );
        ts << e << "\n";
    }
}


void MainApp::runUpdateCalTimer()
{
    if( calSRRun ) {

        int elapsed = calSRRun->elapsedMS(),
            target  = 60000*configCtl->acceptedParams.sync.calMins;

        if( elapsed >= target ) {
            remoteStopsRun();
            return;
        }

        QTimer::singleShot(
            qBound( 10, target - elapsed, 60000 ),
            this, SLOT(runUpdateCalTimer()) );
    }
}


void MainApp::runCalFinished()
{
    calSRRun->deleteLater();
    calSRRun = 0;
}


void MainApp::runUpdateSvyTimer()
{
    if( svyPrbRun ) {

        if( svyPrbRun->nextBank() ) {
            QTimer::singleShot(
                svyPrbRun->msPerBnk(), this, SLOT(runUpdateSvyTimer()) );
        }
        else
            remoteStopsRun();
    }
}


void MainApp::runSvyFinished()
{
    svyPrbRun->deleteLater();
    svyPrbRun = 0;
}


#ifdef DO_SNAPSHOTS
void MainApp::runSnapBefore()
{
    QDir    dir("C://SGLTEST");
    dir.removeRecursively();
    dir.mkdir("C://SGLTEST");

    QStringList cmd;
    cmd << "tasklist /svc >> C://SGLTEST/before_run.out";
    cmd << "\n";
    cmd << "get-service >> C://SGLTEST/before_run.out";
    QProcess::execute( "powershell", cmd );
}


void MainApp::runSnapStarted()
{
    QStringList cmd;
    cmd << "tasklist /svc >> C://SGLTEST/started_run.out";
    cmd << "\n";
    cmd << "get-service >> C://SGLTEST/started_run.out";
    QProcess::execute( "powershell", cmd );
}


void MainApp::runSnapStopping()
{
    QFile   f("C://SGLTEST/stopping_run.out");
    if( f.exists() )
        return;

    QStringList cmd;
    cmd << "tasklist /svc >> C://SGLTEST/stopping_run.out";
    cmd << "\n";
    cmd << "get-service >> C://SGLTEST/stopping_run.out";
    QProcess::execute( "powershell", cmd );
}
#endif

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void MainApp::showStartupMessages()
{
    cmdSrv->showStartupMessage();
    rgtSrv->showStartupMessage();
}


// Called only from ctor.
//
void MainApp::loadSettings()
{
    STDSETTINGS( S, "mainapp" );

    appData.loadSettings( S );
    cmdSrv->loadSettings( S );
    rgtSrv->loadSettings( S );
}


bool MainApp::runCmdStart( QString *err )
{
    QString locErr;
    bool    showDlg = !err,
            ok;

    if( showDlg )
        err = &locErr;

#ifdef DO_SNAPSHOTS
    QTimer::singleShot( 0, this, SLOT(runSnapBefore()) );
#endif

    ok = run->startRun( *err );

    if( !ok && showDlg )
        QMessageBox::critical( 0, "Run Startup Errors", locErr );

#ifdef DO_SNAPSHOTS
    QTimer::singleShot( 10, this, SLOT(runSnapStarted()) );
#endif

    return ok;
}


