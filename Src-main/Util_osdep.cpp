
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* ---------------------------------------------------------------- */
/* Includes general ----------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "Util.h"
#include "MainApp.h"

#include <QThread>
#include <QGLContext>

/* ---------------------------------------------------------------- */
/* Includes single OS --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN
    #include <QDir>
#elif defined(Q_WS_X11)
    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <X11/Xlib.h>
#elif defined(Q_WS_MACX)
    #include <agl.h>
    #include <gl.h>
#elif defined(Q_OS_LINUX)
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/sysinfo.h>
    #include <errno.h>
    #include <sched.h>
    #include <time.h>
    #include <unistd.h>
    #include <GL/gl.h>
#elif defined(Q_OS_DARWIN)
    #include <CoreServices/CoreServices.h>
    #include <GL/gl.h>
#endif

/* ---------------------------------------------------------------- */
/* Includes multi OS ---------------------------------------------- */
/* ---------------------------------------------------------------- */

#if !defined(Q_OS_WIN) && !defined(Q_OS_LINUX)
    #include <QTime>
#endif

#if !defined(Q_OS_WIN)
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
#endif

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Record time at startup using static object c'tor
static struct Init_getTime {
    Init_getTime() {getTime();}
} init_getTime;






/* ---------------------------------------------------------------- */
/* namespace Util ------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace Util {

/* ---------------------------------------------------------------- */
/* Resources ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

QString osPath()
{
    return QString::fromUtf8( qgetenv( "windir" ) ).replace( "\\", "/" );
}

#else

QString osPath()
{
    return QString();
}

#endif

/* ---------------------------------------------------------------- */
/* availableDiskSpace --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

quint64 availableDiskSpace( int iDataDir )
{
    quint64 availableBytes;

    if( GetDiskFreeSpaceEx(
            mainApp()->dataDir( iDataDir ).toStdWString().data(),
            (PULARGE_INTEGER)&availableBytes,
            NULL,
            NULL ) ) {

        return availableBytes;
    }

// Failing any actual query, return either a show-stopping zero
// or a modest number, such as 4GB.

    return quint64(4096) * 1024 * 1024;
}

#else

quint64 availableDiskSpace( int iDataDir )
{
// Failing any actual query, return either a show-stopping zero
// or a modest number, such as 4GB.

    return quint64(4096) * 1024 * 1024;
}

#endif

/* ---------------------------------------------------------------- */
/* secsSinceBoot -------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

uint secsSinceBoot()
{
    return GetTickCount() / 1000;
}

#elif defined(Q_OS_LINUX)

uint secsSinceBoot()
{
    struct sysinfo  si;
    sysinfo( &si );
    return si.uptime;
}

#else /* !Q_OS_WIN && !Q_OS_LINUX */

uint secsSinceBoot()
{
    return getTime();
}

#endif

/* ---------------------------------------------------------------- */
/* getTime -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

double getTime()
{
    static __int64  freq    = 0;
    static __int64  t0      = 0;
    __int64         tNow;

    QueryPerformanceCounter( (LARGE_INTEGER*)&tNow );

    if( !t0 )
        t0 = tNow;

    if( !freq )
        QueryPerformanceFrequency( (LARGE_INTEGER*)&freq );

    return double(tNow - t0) / double(freq);
}

#elif defined(Q_OS_LINUX)

double getTime()
{
    static double   t0 = -9999.;
    struct timespec ts;

    clock_gettime( CLOCK_MONOTONIC, &ts );

    double  t = double(ts.tv_sec) + double(ts.tv_nsec) / 1e9;

    if( t0 < 0.0 )
        t0 = t;

    return t - t0;
}

#else /* !Q_OS_WIN && !Q_OS_LINUX */

double getTime()
{
    static QTime    t;
    static bool     started = false;

    if( !started ) {
        t.start();
        started = true;
    }

    return t.elapsed() / 1000.0;
}

#endif

/* ---------------------------------------------------------------- */
/* socketNoNagle -------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

void socketNoNagle( int sock )
{
    BOOL    flag = 1;
    int     ret = setsockopt(
                    sock,
                    IPPROTO_TCP,
                    TCP_NODELAY,
                    reinterpret_cast<char*>(&flag),
                    sizeof(flag) );

    if( ret )
        Warning() << "Error turning off Nagling for socket " << sock;
}

#else /* !Q_OS_WIN */

void socketNoNagle( int sock )
{
    long    flag = 1;
    int     ret = setsockopt(
                    sock,
                    IPPROTO_TCP,
                    TCP_NODELAY,
                    reinterpret_cast<void*>(&flag),
                    sizeof(flag) );

    if( ret )
        Warning() << "Error turning off Nagling for socket " << sock;
}

#endif

/* ---------------------------------------------------------------- */
/* glGetErrorString ----------------------------------------------- */
/* ---------------------------------------------------------------- */

QString glGetErrorString( int err )
{
    switch( err ) {
        case GL_INVALID_OPERATION:  return "Invalid Operation";
        case GL_INVALID_ENUM:       return "Invalid Enum";
        case GL_NO_ERROR:           return "No Error";
        case GL_INVALID_VALUE:      return "Invalid Value";
        case GL_OUT_OF_MEMORY:      return "Out of Memory";
        case GL_STACK_OVERFLOW:     return "Stack Overflow";
        case GL_STACK_UNDERFLOW:    return "Stack Underflow";
    }

    return QString("UNKNOWN: %1").arg( err );
}

/* ---------------------------------------------------------------- */
/* hasOpenGLExtension --------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return pointer to first occurrence of 'ch' in str,
// or point to terminal NULL.
//
static const GLubyte *strChr( const GLubyte *str, GLubyte ch )
{
    while( str && *str && *str != ch )
        ++str;

    return str;
}


#ifdef Q_WS_X11

bool hasOpenGLXExtension( const char *ext_name )
{
    static const char   *glx_exts = 0;

    if( !glx_exts ) {

        Display *dis = XOpenDisplay( (char*)0 );

        if( dis ) {

            int         screen = DefaultScreen( dis );
            const char  *str = glXQueryExtensionsString( dis, screen );

            if( str )
                glx_exts = strdup( str );

            XCloseDisplay( dis );
        }
    }

    if( glx_exts ) {

        const char  *x0, *xlim, *s1, *s2;
        const char  space = ' ';

        // loop through all space-delimited expressions [x0..xlim)

        x0      = glx_exts;
        xlim    = strchr( x0 + 1, space );

        for( ; *x0; x0 = xlim + 1, xlim = strchr( x0, space ) ) {

            // compare strings

            s1 = x0;
            s2 = ext_name;

            for( ; *s1 && *s2 && *s1 == *s2 && s1 < xlim; ++s1, ++s2 )
                ;

            if( !*s2 && (!*s1 || *s1 == space) )
                return true;
        }
    }

    return false;
}

#else /* !Q_WS_X11 */

bool hasOpenGLXExtension( const char * )
{
    return false;
}

#endif


bool hasOpenGLExtension( const char *ext_name )
{
    static const GLubyte    *ext_str = 0;
    static const GLubyte    space = static_cast<GLubyte>(' ');

    if( !ext_str )
        ext_str = glGetString( GL_EXTENSIONS );

    if( !ext_str ) {
        Warning()
            << "Could not get GL_EXTENSIONS! ("
            << glGetErrorString( glGetError() ) << ")";
    }
    else {
        const GLubyte   *x0, *xlim, *s1;
        const char      *s2;

        // loop through all space-delimited expressions [x0..xlim)

        x0      = ext_str;
        xlim    = strChr( x0 + 1, space );

        for( ; *x0; x0 = xlim + 1, xlim = strChr( x0, space ) ) {

            // compare strings

            s1 = x0;
            s2 = ext_name;

            for( ; *s1 && *s2 && *s1 == *s2 && s1 < xlim; ++s1, ++s2 )
                ;

            if( !*s2 && (!*s1 || *s1 == space) )
                return true;
        }
    }

    return hasOpenGLXExtension( ext_name );
}

/* ---------------------------------------------------------------- */
/* setOpenGLVSyncMode --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

typedef BOOL (APIENTRY *wglswapfn_t)( int );

void setOpenGLVSyncMode( bool onoff )
{
    static Qt::HANDLE   lastThread = 0;    // limit report verbosity

    Qt::HANDLE  thread = QThread::currentThreadId();

    wglswapfn_t wglSwapIntervalEXT =
                (wglswapfn_t)QGLContext::currentContext()
                ->getProcAddress( "wglSwapIntervalEXT" );

    if( wglSwapIntervalEXT ) {

        wglSwapIntervalEXT( (onoff ? 1 : 0) );

        if( thread != lastThread ) {
            Log()
                << "OpenGL VSync mode "
                << (onoff ? "enabled" : "disabled")
                << " using wglSwapIntervalEXT().";
            lastThread = thread;
        }
    }
    else if( thread != lastThread ) {
        Warning()
            << "OpenGL VSync mode could not be set"
            " because wglSwapIntervalEXT function not found.";
        lastThread = thread;
    }
}

#elif defined(Q_WS_X11)

typedef int (*swap_t)( int );

void setOpenGLVSyncMode( bool onoff )
{
    static Qt::HANDLE   lastThread = 0;    // limit report verbosity

    Qt::HANDLE  thread = QThread::currentThreadId();

    if( hasOpenGLExtension( "GLX_SGI_swap_control" ) ) {

        if( thread != lastThread ) {
            Log()
                << "Found GLX_SGI_swap_control extension, turning "
                "OpenGL VSync mode "
                << (onoff ? "on" : "off")
                << ".";
            thread = lastThread;
        }

        swap_t  fun = (swap_t)glXGetProcAddressARB(
                        (const GLubyte*)"glXSwapIntervalSGI" );

        if( fun )
            fun( onoff ? 1 : 0 );
        else if( thread != lastThread ) {
            Warning() <<  "Error: glXSwapIntervalSGI function not found.";
            thread = lastThread;
        }
    }
    else if( thread != lastThread ) {
        Warning()
            << "OpenGL VSync mode could not be set"
            " because GLX_SGI_swap_control function not found.";
        thread = lastThread;
    }
}

#elif defined(Q_WS_MACX)

void setOpenGLVSyncMode( bool onoff )
{
    static Qt::HANDLE   lastThread = 0;    // limit report verbosity

    Qt::HANDLE  thread = QThread::currentThreadId();

    GLint       tmp = (onoff ? 1 : 0);
    AGLContext  ctx = aglGetCurrentContext();

    if( aglEnable( ctx, AGL_SWAP_INTERVAL ) == GL_FALSE ) {

        if( thread != lastThread ) {
            Warning()
                << "OpenGL VSync mode could not be set because"
                " aglEnable(AGL_SWAP_INTERVAL) returned false.";
            thread = lastThread;
        }
    }
    else {
        if( aglSetInteger( ctx, AGL_SWAP_INTERVAL, &tmp ) == GL_FALSE ) {

            if( thread != lastThread ) {
                Warning()
                    << "OpenGL VSync mode could not be set because"
                    " aglSetInteger returned false.";
                thread = lastThread;
            }
        }
        else if( thread != lastThread ) {
            Log()
                << "OpenGL VSync mode "
                << (onoff ? "enabled" : "disabled")
                << " using aglSetInteger().";
            thread = lastThread;
        }
    }
}

#else /* !Q_OS_WIN && !Q_WS_X11 && !Q_WS_MACX */

#warning Warning: setOpenGLVSyncMode() not implemented on this platform!

#endif

/* ---------------------------------------------------------------- */
/* isWindows7OrLater ---------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

bool isWindows7OrLater()
{
    OSVERSIONINFOEXW    V;
    memset( &V, 0, sizeof(V) );
    V.dwOSVersionInfoSize   = sizeof(V);
    V.dwMajorVersion        = HIBYTE(_WIN32_WINNT_WIN7);
    V.dwMinorVersion        = LOBYTE(_WIN32_WINNT_WIN7);

    DWORDLONG   M = 0;
    M = VerSetConditionMask( M, VER_MAJORVERSION, VER_GREATER_EQUAL );
    M = VerSetConditionMask( M, VER_MINORVERSION, VER_GREATER_EQUAL );


    return VerifyVersionInfoW( &V, VER_MAJORVERSION | VER_MINORVERSION, M );
}

#else /* !Q_OS_WIN */

bool isWindows7OrLater()
{
    return false;
}

#endif

/* ---------------------------------------------------------------- */
/* setRTPriority -------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

void setRTPriority()
{
    Log() << "Setting realtime process priority.";

    if( !SetPriorityClass( GetCurrentProcess(), REALTIME_PRIORITY_CLASS ) )
        Warning() << "SetPriorityClass() failed: " << int(GetLastError());
}

#elif defined(Q_OS_LINUX)

void setRTPriority()
{
    if( geteuid() == 0 ) {

        Log() << "Running as root, setting realtime process priority";

#if 0
// This function prevents swapping out app memory pages...
// but it will starve OS for memory.

        if( mlockall( MCL_CURRENT | MCL_FUTURE ) ) {
            int e = errno;
            Warning() <<  "Error from mlockall(): " << strerror( e );
        }
#endif

        struct sched_param  p;

        p.sched_priority = sched_get_priority_max( SCHED_RR );

        if( sched_setscheduler( 0, SCHED_RR, &p ) ) {
            int e = errno;
            Warning() << "Error from sched_setscheduler(): " << strerror( e );
        }
    }
    else
        Warning() << "Not running as root, cannot set realtime priority";
}

#else /* !Q_OS_WIN && !Q_OS_LINUX */

void setRTPriority()
{
    Warning() << "Cannot set realtime process priority on this platform.";
}

#endif

/* ---------------------------------------------------------------- */
/* setPreciseTiming ----------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

void setPreciseTiming( bool on )
{
    if( on )
        timeBeginPeriod( 1 );
    else
        timeEndPeriod( 1 );
}

#else

void setPreciseTiming( bool on )
{
    Q_UNUSED( on )
}

#endif

/* ---------------------------------------------------------------- */
/* getNProcessors ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Notes:
// getNProcessors returns 4 for a single CPU system having
// 2 cores/4 threads. Therefore, these are threads, a.k.a.
// virtual/logical "cores".
//
int getNProcessors()
{
    static int  nProcs = 0;

    if( !nProcs ) {
        SYSTEM_INFO si;
        GetSystemInfo( &si );
        nProcs = si.dwNumberOfProcessors;
    }

    return nProcs;
}

#elif defined(Q_OS_LINUX)

int getNProcessors()
{
    static int  nProcs = 0;

    if( !nProcs )
        nProcs = sysconf( _SC_NPROCESSORS_ONLN );

    return nProcs;
}

#elif defined(Q_OS_DARWIN)

int getNProcessors()
{
    static int  nProcs = 0;

    if( !nProcs )
        nProcs = MPProcessorsScheduled();

    return nProcs;
}

#else /* !Q_OS_WIN && !Q_OS_LINUX && !Q_OS_DARWIN */

int getNProcessors()
{
    return 1;
}

#endif

/* ---------------------------------------------------------------- */
/* getCurProcessorIdx --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

int getCurProcessorIdx()
{
    return GetCurrentProcessorNumber();
}

#else

int getCurProcessorIdx()
{
    return 0;
}

#endif

/* ---------------------------------------------------------------- */
/* setProcessAffinityMask ----------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Notes:
// GetProcessAffinityMask returns 0xF for a single CPU system
// having 2 cores/4 threads. Therefore, the mask bits refer to
// threads (virtual/logical "cores").
//
// setProcessAffinityMask( 0 ) has no effect.
// setProcessAffinityMask( 0xFF ) on a 4 thread system is an error:
//  the mask bits must be a subset of the system mask bits returned
//  by GetProcessAffinityMask.
//
void setProcessAffinityMask( uint mask )
{
    if( !SetProcessAffinityMask( GetCurrentProcess(), mask ) ) {
        Warning()
            << "Win32 Error setting process affinity mask: "
            << GetLastError();
    }
    else {
        Log()
            << "Process affinity mask set to: "
            << QString("0x%1").arg( mask, 0, 16, QChar('0') );
    }
}

#elif defined(Q_OS_LINUX)

void setProcessAffinityMask( uint mask )
{
    cpu_set_t   cpuset;
    int         nMaskBits = sizeof(mask) * 8;

    CPU_ZERO( &cpuset );

    for( int i = 0; i < nMaskBits; ++i ) {

        if( mask & (1 << i) )
            CPU_SET( i, &cpuset );
    }

    int err = sched_setaffinity( 0, sizeof(cpuset), &cpuset );

    if( err ) {
        Warning()
            << "sched_setaffinity("
            << QString("0x%1").arg( mask, 0, 16, QChar('0') )
            << ") error: " << strerror(errno);
    }
    else {
        Log()
            << "Process affinity mask set to: "
            << QString("0x%1").arg( mask, 0, 16, QChar('0') );
    }
}

#else /* !Q_OS_WIN && !Q_OS_LINUX */

void setProcessAffinityMask( uint )
{
    Warning() << "setProcessAffinityMask unimplemented on this system.";
}

#endif

/* ---------------------------------------------------------------- */
/* setCurrentThreadAffinityMask ----------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

uint setCurrentThreadAffinityMask( uint mask )
{
    return
        static_cast<uint>(
        SetThreadAffinityMask( GetCurrentThread(), DWORD_PTR(mask) ));
}

#else /* !Q_OS_WIN */

uint setCurrentThreadAffinityMask( uint )
{
    Warning() << "setCurrentThreadAffinityMask not implemented.";
    return 0;
}

#endif

/* ---------------------------------------------------------------- */
/* getRAMBytes32BitApp -------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Installed RAM as seen by 32-bit application.
//
// <http://putridparrot.com/blog/memory-limits-for-a-32-bit-net-application/>
//
double getRAMBytes32BitApp()
{
    double          G;
    MEMORYSTATUSEX  M;

    ZeroMemory( &M, sizeof(MEMORYSTATUSEX) );
    M.dwLength = sizeof(MEMORYSTATUSEX);

    if( GlobalMemoryStatusEx( &M ) ) {

        G = double(M.ullTotalVirtual) / (1024.0 * 1024.0 * 1024.0);

        if( G < 2.5 )
            G = 1.2;    // 32-bit OS: effective max
        else
            G = 2.8;    // 64-bit OS: effective max
    }
    else {
        Warning() << "getRAMBytes32BitApp did not succeed.";
        G = 1.2;
    }

    return G * 1024.0 * 1024.0 * 1024.0;
}

#else /* !Q_OS_WIN */

double getRAMBytes32BitApp()
{
    Warning() << "getRAMBytes32BitApp not implemented.";
    return 0.0;
}

#endif

/* ---------------------------------------------------------------- */
/* getRAMBytes64BitApp -------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Installed RAM as seen by 64-bit application.
//
double getRAMBytes64BitApp()
{
    MEMORYSTATUSEX  M;

    ZeroMemory( &M, sizeof(MEMORYSTATUSEX) );
    M.dwLength = sizeof(MEMORYSTATUSEX);

    if( GlobalMemoryStatusEx( &M ) )
        return double(M.ullAvailPhys);

    Warning() << "getRAMBytes64BitApp did not succeed.";
    return 4.0 * 1024.0 * 1024.0 * 1024.0;
}

#else /* !Q_OS_WIN */

double getRAMBytes64BitApp()
{
    Warning() << "getRAMBytes64BitApp not implemented.";
    return 0.0;
}

#endif

/* ---------------------------------------------------------------- */
/* isMouseDown ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// The QT mouseButtons() function works in the content areas of
// windows, but does not report clicks in a window title bar.
// Those are handled by the OS. Here we demonstrate how to read
// the button states in Windows.

#ifdef Q_OS_WIN

bool isMouseDown()
{
    short   L = GetKeyState( VK_LBUTTON ),
            R = GetKeyState( VK_RBUTTON );

    return ((L | R) & 0x8000) != 0;
}

#else /* !Q_OS_WIN */

bool isMouseDown()
{
//    return Qt::NoButton != qApp->mouseButtons();
    return false;
}

#endif

/* ---------------------------------------------------------------- */
/* Beep ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

void Beep( quint32 hertz, quint32 msec )
{
    ::Beep( hertz, msec );
}

#else /* !Q_OS_WIN */

void Beep( quint32 hertz, quint32 msec )
{
    Q_UNUSED( hertz )
    Q_UNUSED( msec )
}

#endif

/* ---------------------------------------------------------------- */
/* end namespace Util --------------------------------------------- */
/* ---------------------------------------------------------------- */

}   // namespace Util


