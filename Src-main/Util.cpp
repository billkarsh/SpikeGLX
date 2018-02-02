
#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "ConfigCtl.h"

#include <QMessageBox>
#include <QThread>
#include <QDir>
#include <QDateTime>
#include <ctime>
#include <iostream>
#include <QHostInfo>
#include <QNetworkInterface>

/* ---------------------------------------------------------------- */
/* Internal macros ------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define TEMP_FILE_NAME_PREFIX       "SpikeGL_DSTemp_"
#define RD_CHUNK_SIZE   1024*1024
#define WR_CHUNK_SIZE    128*1024
//#define WR_CHUNK_SIZE    786*1024






/* ---------------------------------------------------------------- */
/* namespace Util ------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace Util {

/* ---------------------------------------------------------------- */
/* Log messages to console ---------------------------------------- */
/* ---------------------------------------------------------------- */

Log::Log() : stream( &str, QIODevice::WriteOnly ), doprt(true)
{
}


Log::~Log()
{
    if( doprt ) {

        QString msg =
            QString("[Thread %1 %2] %3")
                .arg( (quint64)QThread::currentThreadId() )
                .arg( QDateTime::currentDateTime()
                        .toString( "M/dd/yy hh:mm:ss.zzz" ) )
                .arg( str );

        if( mainApp() )
            mainApp()->msg.logMsg( msg, color );
        else
            std::cerr << STR2CHR( msg ) << "\n";
    }
}


Debug::~Debug()
{
    color = Qt::darkBlue;

    if( !mainApp() || !mainApp()->isDebugMode() )
        doprt = false;
}


Error::~Error()
{
    color = Qt::darkRed;

    MainApp *app = mainApp();

    if( !app )
        return;

    if( app->isConsoleHidden() )
        Systray( true ) << str;
    else {
        ConsoleWindow   *w = app->console();
        w->showNormal();
        w->activateWindow();
    }
}


Warning::~Warning()
{
    color = Qt::darkMagenta;

    if( mainApp() && mainApp()->isConsoleHidden() )
        Systray( true ) << str;
}

/* ---------------------------------------------------------------- */
/* Show messages in status bar ------------------------------------ */
/* ---------------------------------------------------------------- */

Status::Status( int timeout )
    :   stream( &str, QIODevice::WriteOnly ), timeout(timeout)
{
    stream.setRealNumberNotation( QTextStream::FixedNotation );
    stream.setRealNumberPrecision( 2 );
}


Status::~Status()
{
    if( !str.length() )
        return;

    if( mainApp() )
        mainApp()->msg.statusBarMsg( str, timeout );
    else
        std::cerr << "STATUSMSG: " << STR2CHR( str ) << "\n";
}

/* ---------------------------------------------------------------- */
/* Show messages in system tray ----------------------------------- */
/* ---------------------------------------------------------------- */

Systray::Systray( bool isError, int timeout )
    :   Status(timeout), isError(isError)
{
}


Systray::~Systray()
{
    if( mainApp() )
        mainApp()->msg.sysTrayMsg( str, timeout, isError );
    else
        std::cerr << "SYSTRAYMSG: " << STR2CHR( str ) << "\n";

    str.clear(); // bypass superclass posting
}

/* ---------------------------------------------------------------- */
/* Global data ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

MainApp *mainApp()
{
    return MainApp::instance();
}


int daqAINumFetchesPerSec()
{
#if defined(QT_DEBUG) || defined(_DEBUG)
    return 100;
#else
    return 1000;
#endif
}


int daqAIFetchPeriodMillis()
{
    return 1000 / daqAINumFetchesPerSec();
}

/* ---------------------------------------------------------------- */
/* Math ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool feq( double a, double b )
{
    double  diff = a - b;
    return (diff < EPSILON) && (-diff < EPSILON);
}


double uniformDev( double rmin, double rmax )
{
    static bool seeded = false;

    if( !seeded ) {
        seeded = true;
        qsrand( std::time(0) );
    }

    return rmin + (rmax-rmin) * qrand() / RAND_MAX;
}


// Bit position of LSB( X ).
// Right-most bit is bit-0.
// E.g. ffs( 0x8 ) = 4.
//
int ffs( int x )
{
    int r = 1;

    if( !x )
        return 0;

    if( !(x & 0xffff) ) {
        x >>= 16;
        r += 16;
    }

    if( !(x & 0xff) ) {
        x >>= 8;
        r += 8;
    }

    if( !(x & 0xf) ) {
        x >>= 4;
        r += 4;
    }

    if( !(x & 3) ) {
        x >>= 2;
        r += 2;
    }

    if( !(x & 1) ) {
//        x >>= 1;
        r += 1;
    }

    return r;
}

/* ---------------------------------------------------------------- */
/* Objects -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Error checking form of connect.
// Recommended laid-back, queue-order handling of GUI elements.
//
void ConnectUI(
    const QObject       *src,
    const QString       &sig,
    const QObject       *dst,
    const QString       &slot )
{
// Note definition of string-izing macros:
// #define SLOT(a) "1"#a
// #define SIGNAL(a) "2"#a
// Hence, we apply mid(1) for nicer reporting.
// Note too the "1" & "2" prefixes are said to
// merely distinguish strings.
//
    if( !QObject::connect(
            src, STR2CHR( sig ),
            dst, STR2CHR( slot ),
            Qt::QueuedConnection ) ) {

        QString sname = src->objectName(),
                dname = dst->objectName();

        QMessageBox::critical(
            0,
            "Signal connection error",
            QString(
            "Error ConnectUI %1::%2 to %3::%4\n\n"
            "Make sure referenced datatypes are registered.")
            .arg( sname.isEmpty() ? "(unnamed)" : sname )
            .arg( sig.mid( 1 ) )
            .arg( dname.isEmpty() ? "(unnamed)" : dname )
            .arg( slot.mid( 1 ) ) );

        QApplication::exit( 1 );
        // only reached if event loop not running
        std::exit( 1 );
    }
}


// Error checking form of connect.
// Custom connection type, defaults to faster AutoConnection.
//
void Connect(
    const QObject       *src,
    const QString       &sig,
    const QObject       *dst,
    const QString       &slot,
    Qt::ConnectionType  type )
{
// Note definition of string-izing macros:
// #define SLOT(a) "1"#a
// #define SIGNAL(a) "2"#a
// Hence, we apply mid(1) for nicer reporting.
// Note too the "1" & "2" prefixes are said to
// merely distinguish strings.
//
    if( !QObject::connect(
            src, STR2CHR( sig ),
            dst, STR2CHR( slot ),
            type ) ) {

        QString sname = src->objectName(),
                dname = dst->objectName();

        QMessageBox::critical(
            0,
            "Signal connection error",
            QString(
            "Error Connect %1::%2 to %3::%4\n\n"
            "Make sure referenced datatypes are registered.")
            .arg( sname.isEmpty() ? "(unnamed)" : sname )
            .arg( sig.mid( 1 ) )
            .arg( dname.isEmpty() ? "(unnamed)" : dname )
            .arg( slot.mid( 1 ) ) );

        QApplication::exit( 1 );
        // only reached if event loop not running
        std::exit( 1 );
    }
}


bool objectHasAncestor( const QObject *object, const QObject *ancestor )
{
    while( object ) {
        if( (object = object->parent()) == ancestor )
            return true;
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* Resources ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Get contents of resource file like ":/myText.html"
// as a QString.
//
void res2Str( QString &str, const QString resFile )
{
    QFile   f( resFile );

    if( f.open( QIODevice::ReadOnly | QIODevice::Text ) )
        str = QTextStream( &f ).readAll();
    else
        str.clear();
}

/* ---------------------------------------------------------------- */
/* Settings ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString appPath()
{
    QString path = QDir::currentPath();

#ifdef Q_OS_MAC
    QDir    D( path );

    if( D.cdUp() )
        path = D.canonicalPath();
#endif

    return path;
}


QString iniFile( const QString &fileName )
{
    return QString("%1/configs/%2.ini")
            .arg( appPath() )
            .arg( fileName );
}


bool toolPath( QString &path, const QString &toolName, bool bcreate )
{
    path = QString("%1/tools").arg( appPath() );

    if( bcreate ) {

        if( !QDir().mkpath( path ) ) {
            Error() << "Failed to create folder [" << path << "].";
            return false;
        }
    }

    path = QString("%1/%2").arg( path ).arg( toolName );

    return true;
}

/* ---------------------------------------------------------------- */
/* Files ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

qint64 readChunky( const QFile &f, void *dst, qint64 bytes )
{
    const qint64    chunk   = RD_CHUNK_SIZE;
    qint64          noffset = 0,
                    nrem    = bytes;

    while( nrem > 0 ) {

        qint64  nthis   = qMin( nrem, chunk ),
                n       = ((QFile*)&f)->read( (char*)dst + noffset, nthis );

        if( n > 0 ) {
            noffset += n;
            nrem    -= n;
        }
        else if( n < 0 )
            return -1;
        else
            break;
    }

    return noffset;
}


qint64 writeChunky( QFile &f, const void *src, qint64 bytes )
{
    const qint64    chunk   = WR_CHUNK_SIZE;
    qint64          noffset = 0,
                    nrem    = bytes;

    while( nrem > 0 ) {

        qint64  nthis   = qMin( nrem, chunk ),
                n       = f.write( (const char*)src + noffset, nthis );

        if( n > 0 ) {
            noffset += n;
            nrem    -= n;
        }
        else if( n < 0 )
            return -1;
        else
            break;
    }

    return noffset;
}


// BK: I've retained the temp-file stuff for future reference.
//
// Remove all SpikeGL temp files
//
#if 0
// Original version selectively removes this app's current file
// and any files more than one day old to avoid interference with
// other instances of the app on this machine.
//
void removeTempDataFiles()
{
    // Remove current file

    QFile::remove(
        QString("%1/%2%3.bin")
            .arg( QDir::tempPath() )
            .arg( TEMP_FILE_NAME_PREFIX )
            .arg( QCoreApplication::applicationPid() )
        );

    // Remove files >= 1 day old

    QStringList tempList =
                QDir::temp().entryList(
                QStringList(
                TEMP_FILE_NAME_PREFIX "*.bin" ) );

    foreach( const QString &filename, tempList ) {

        QString     fname = QString("%1/%2")
                                .arg( QDir::tempPath() )
                                .arg( filename );
        QFileInfo   fi( fname );

        if( fi.lastModified().daysTo( QDateTime::currentDateTime() ) >= 1 )
            QFile::remove( fname );
    }
}
#endif

// Current implementation aggressively deletes all SpikeGL temp files
// since we anticipate only singleton app on a given machine.
//
void removeTempDataFiles()
{
    QStringList tempList =
                QDir::temp().entryList(
                QStringList( TEMP_FILE_NAME_PREFIX "*" ) );

    foreach( const QString &filename, tempList ) {

        QFile::remove(
                QString("%1/%2")
                .arg( QDir::tempPath() )
                .arg( filename ) );
    }
}

/* ---------------------------------------------------------------- */
/* Timers --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Workaround for sleep funs being protected in QT 4.8

class CSleeper : public QThread
{
public:
    static void msleep( long msec ) {QThread::msleep( msec );}
    static void usleep( long usec ) {QThread::usleep( usec );}
};


void msleep( long msec )
{
    CSleeper    S;
    S.msleep( msec );
}


void usleep( long usec )
{
    CSleeper    S;
    S.usleep( usec );
}

/* ---------------------------------------------------------------- */
/* Sockets -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Following address classification according to:
// <https://en.wikipedia.org/wiki/Private_network>
//
static bool isLinkLocalAddr( const QHostAddress &h )
{
    quint32 ip4 = h.toIPv4Address();

    return
        ip4 >= QHostAddress("169.254.1.0").toIPv4Address()
    &&  ip4 <= QHostAddress("169.254.254.255").toIPv4Address();
}


#if 0   // correct...for future reference

// Following address classification according to:
// <https://en.wikipedia.org/wiki/Private_network>
//
static bool isPrivateAddr( const QHostAddress &h )
{
    quint32 ip4 = h.toIPv4Address();

    if( ip4 >= QHostAddress("10.0.0.0").toIPv4Address()
    &&  ip4 <= QHostAddress("10.255.255.255").toIPv4Address() ) {

        return true;
    }

    if( ip4 >= QHostAddress("172.16.0.0").toIPv4Address()
    &&  ip4 <= QHostAddress("172.31.255.255").toIPv4Address() ) {

        return true;
    }

    if( ip4 >= QHostAddress("192.168.0.0").toIPv4Address()
    &&  ip4 <= QHostAddress("192.168.255.255").toIPv4Address() ) {

        return true;
    }

    return false;
}

#endif


QString getMyIPAddress()
{
    QList<QHostAddress> HL = QNetworkInterface::allAddresses();

    foreach( QHostAddress h, HL ) {

        if( h.protocol() == QAbstractSocket::IPv4Protocol
            && h != QHostAddress::LocalHost
            && !isLinkLocalAddr( h ) ) {

            return h.toString();
        }
    }

    return "0.0.0.0";
}


QString getHostName()
{
    return QHostInfo::localHostName();
}

/* ---------------------------------------------------------------- */
/* Misc OS helpers ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void guiBreathe()
{
    qApp->processEvents();
}

/* ---------------------------------------------------------------- */
/* end namespace Util --------------------------------------------- */
/* ---------------------------------------------------------------- */

}   // namespace Util


