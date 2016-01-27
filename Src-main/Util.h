#ifndef UTIL_H
#define UTIL_H

#include <QObject>
#include <QEvent>
#include <QFile>
#include <QFileInfo>
#include <QColor>
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
    QSettings S( iniFile( name ), QSettings::IniFormat )

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
    bool        doprt;
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

 void res2Str( QString &str, const QString resFile );

/* ---------------------------------------------------------------- */
/* Settings ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString iniFile( const QString &fileName );
bool toolPath( QString &path, const QString &toolName, bool bcreate );

/* ---------------------------------------------------------------- */
/* Files ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Filename without extension
QString baseName( const QString &fname );

QString forceBinSuffix( const QString &name );
QString forceMetaSuffix( const QString &name );

// Return true if file::open works (optional error string)
bool isFileReadable( const QFileInfo &fi, QString *error );

// Return file size or -1 (optional error string)
qint64 fileSize( const QFileInfo &fi, QString *error );

// Efficient version of QIODevice::read
qint64 readChunky( const QFile &f, void *dst, qint64 bytes );

// Efficient version of QIODevice::write
qint64 writeChunky( QFile &f, const void *src, qint64 bytes );

// Amount of space available on disk
quint64 availableDiskSpace();

// Remove TEMP files (SpikeGL_DSTemp_*.bin)
void removeTempDataFiles();

/* ---------------------------------------------------------------- */
/* Timers --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void msleep( long msec );
void usleep( long usec );

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

// Set application process to realtime priority
void setRTPriority();

// Number of real CPUs (cores) on the system
int getNProcessors();

// Mask-bits set which processors to run on
void setProcessAffinityMask( uint mask );

// Mask-bits set which processors to run on.
// Return previous mask, or zero if error.
uint setCurrentThreadAffinityMask( uint mask );

// Installed RAM
double getRAMBytes();

/* ---------------------------------------------------------------- */
/* Misc OS helpers ------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool isMouseDown();

/* ---------------------------------------------------------------- */
/* end namespace Util --------------------------------------------- */
/* ---------------------------------------------------------------- */

}   // namespace Util

using namespace Util;

#endif  // UTIL_H


