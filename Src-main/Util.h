#ifndef UTIL_H
#define UTIL_H

#include <QObject>
#include <QColor>
#include <QDateTime>
#include <QFile>
#include <QString>
#include <QTextStream>

class MainApp;

/* ---------------------------------------------------------------- */
/* Macros --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define EPSILON 0.0000001

#ifndef MIN
#define MIN( a, b ) ((a) <= (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX( a, b ) ((a) >= (b) ? (a) : (b))
#endif

#define STR1(x) #x
#define STR(x) STR1(x)

#define STR2CHR( qstring )  ((qstring).toUtf8().constData())

#define STDSETTINGS( S, name )  \
    QSettings S( configPath( name ), QSettings::IniFormat )

/* ---------------------------------------------------------------- */
/* namespace Util ------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace Util
{

/* ---------------------------------------------------------------- */
/* Log messages to console ---------------------------------------- */
/* ---------------------------------------------------------------- */

class Log
{
private:
    QTextStream stream;
protected:
    QString     str;
    QColor      color;
    bool        doprt,  // debug() silent unless verbose mode
                doeco,  // echo errors and warnings to metrics
                dodsk;  // also record errors in runDir
public:
    Log();
    virtual ~Log();

    template <class T>
    Log &operator<<( const T &t ) {stream << t; return *this;}
};

class Debug : public Log
{
public:
    virtual ~Debug();
};

class Error : public Log
{
public:
    virtual ~Error();
};

class Warning : public Log
{
public:
    virtual ~Warning();
};

/* ---------------------------------------------------------------- */
/* Show messages in status bar ------------------------------------ */
/* ---------------------------------------------------------------- */

class Status
{
private:
    QTextStream stream;
protected:
    QString str;
    int     timeout;
public:
    Status( int timeout = 0 );
    virtual ~Status();

    template <class T>
    Status &operator<<( const T &t ) {stream << t; return *this;}
};

/* ---------------------------------------------------------------- */
/* Show messages in system tray ----------------------------------- */
/* ---------------------------------------------------------------- */

class Systray : public Status
{
protected:
    bool    isError;
public:
    Systray( bool isError = false, int timeout = 0 );
    virtual ~Systray();
};

/* ---------------------------------------------------------------- */
/* Global data ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

MainApp *mainApp();

int daqAINumFetchesPerSec();
int daqAIFetchPeriodMillis();

/* ---------------------------------------------------------------- */
/* Math ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// True if {a,b} closer than EPSILON (0.0000001)
bool feq( double a, double b );

// Uniform random deviate in range [rmin, rmax]
double uniformDev( double rmin = 0.0, double rmax = 1.0 );

// Position of least significant bit (like libc::ffs)
int ffs( int x );

/* ---------------------------------------------------------------- */
/* Objects -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConnectUI(
    const QObject       *src,
    const QString       &sig,
    const QObject       *dst,
    const QString       &slot );

void Connect(
    const QObject       *src,
    const QString       &sig,
    const QObject       *dst,
    const QString       &slot,
    Qt::ConnectionType  type = Qt::AutoConnection );

// True if object has ancestor as direct or distant parent
bool objectHasAncestor( const QObject *object, const QObject *ancestor );

/* ---------------------------------------------------------------- */
/* Resources ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Convert resFile resource item to string
void res2Str( QString &str, const QString &resFile );

// Remove terminal slash
QString rmvLastSlash( const QString &path );

// Windows directory
QString osPath();

// Current working directory
QString appPath();

// Full path to configs ini file
QString configPath( const QString &fileName );

// Full path to calibration folder
QString calibPath();

// Full path to calibration ini file
QString calibPath( const QString &fileName );

// Full path to tool item
bool toolPath( QString &path, const QString &toolName, bool bcreate );

// Display given html file in user browser
void showHelp( const QString &fileName );

/* ---------------------------------------------------------------- */
/* Files ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class UtilReadWorker : public QObject
{
    Q_OBJECT

private:
    const QFile *f;
    void        *dst;
    qint64      foffstart,
                fofffinal,
                doffset,
                nread;
public:
    UtilReadWorker(
        const QFile *f,
        void        *dst,
        qint64      foffstart,
        qint64      fofffinal,
        qint64      doffset,
        qint64      nread )
    :   QObject(0), f(f), dst(dst),
        foffstart(foffstart), fofffinal(fofffinal),
        doffset(doffset), nread(nread)  {}
signals:
    void finished();
public slots:
    void run();
};

class UtilReadThread
{
public:
    QThread         *thread;
    UtilReadWorker  *worker;
public:
    UtilReadThread(
        const QFile *f,
        void        *dst,
        qint64      foffstart,
        qint64      fofffinal,
        qint64      doffset,
        qint64      nread );
    virtual ~UtilReadThread();
};

// Threaded version of QIODevice::read
qint64 readThreaded(
    std::vector<const QFile*>   &vF,
    qint64                      seekto,
    void                        *dst,
    qint64                      bytes );

// Efficient version of QIODevice::read
qint64 readChunky( const QFile &f, void *dst, qint64 bytes );

// Efficient version of QIODevice::write
qint64 writeChunky( QFile &f, const void *src, qint64 bytes );

// Amount of space available on disk
quint64 availableDiskSpace( int iDataDir = 0 );

// Remove TEMP files (SpikeGL_DSTemp_*.bin)
void removeTempDataFiles();

/* ---------------------------------------------------------------- */
/* Timers --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Thread-safe QDateTime::toString
QString dateTime2Str( const QDateTime &dt, Qt::DateFormat f = Qt::TextDate );
QString dateTime2Str( const QDateTime &dt, const QString &format );

// Seconds since last machine reboot
uint secsSinceBoot();

// Current seconds from high resolution timer
double getTime();

/* ---------------------------------------------------------------- */
/* Sockets -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Turn off socket's Nagle algorithm
void socketNoNagle( int sock );

QString getMyIPAddress();

QString getHostName();

/* ---------------------------------------------------------------- */
/* OpenGL --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString glGetErrorString( int err );

// True if named extension available
bool hasOpenGLExtension( const char *ext_name );

// Enable/disable vertical sync in OpenGL. Defaults to on in Windows,
// off on Linux. Make sure the GL context is current when you call this!
void setOpenGLVSyncMode( bool onoff );

/* ---------------------------------------------------------------- */
/* Execution environs --------------------------------------------- */
/* ---------------------------------------------------------------- */

// Is Windows 7 or later
bool isWindows7OrLater();

// Set application process to realtime priority
void setRTPriority();

// Set higher precision system timing on/off
void setPreciseTiming( bool on );

// Number of real CPUs (cores) on the system
int getNProcessors();

// Which processor calling thread is running on
int getCurProcessorIdx();

// Mask-bits set which processors to run on
void setProcessAffinityMask( uint mask );

// Mask-bits set which processors to run on.
// Return previous mask, or zero if error.
uint setCurrentThreadAffinityMask( uint mask );

// Installed RAM as seen by 32-bit application
double getRAMBytes32BitApp();

// Installed RAM as seen by 64-bit application
double getRAMBytes64BitApp();

/* ---------------------------------------------------------------- */
/* Misc OS helpers ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void guiBreathe( bool pass2GUI = true );

bool isMouseDown();

void Beep( quint32 hertz, quint32 msec );

/* ---------------------------------------------------------------- */
/* end namespace Util --------------------------------------------- */
/* ---------------------------------------------------------------- */

}   // namespace Util

using namespace Util;

#endif  // UTIL_H


