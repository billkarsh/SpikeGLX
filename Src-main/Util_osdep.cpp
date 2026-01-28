
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

/* ---------------------------------------------------------------- */
/* Includes general ----------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "Util.h"
#include "MainApp.h"

#include <QThread>

/* ---------------------------------------------------------------- */
/* Includes single OS --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN
    #include <QDir>
    #include <GL/gl.h>
    #include <windows.h>
#elif defined(Q_WS_X11)
    #include <GL/gl.h>
    #include <GL/glx.h>
    #include <X11/Xlib.h>
#elif defined(Q_WS_MACX)
    #include <agl.h>
    #include <gl.h>
#elif defined(Q_OS_LINUX)
    #include <fstream>
    #include <sys/mman.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <sys/statvfs.h>
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

#elif defined(Q_OS_LINUX)

QString osPath()
{
    return "/etc";
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

#elif defined(Q_OS_LINUX)

// Helper to check if we are running inside WSL
//
bool isWSL()
{
    std::ifstream   file( "/proc/version" );
    std::string     line;

    if( std::getline( file, line ) ) {

        // Look for "microsoft" or "wsl" (case insensitive usually, but lower here)
        // Modern WSL2 kernels often contain "microsoft-standard-WSL2"

        std::transform( line.begin(), line.end(), line.begin(), ::tolower );

        if( line.find( "microsoft" ) != std::string::npos ||
            line.find( "wsl" ) != std::string::npos ) {
            return true;
        }
    }

    return false;
}

quint64 availableDiskSpace( int iDataDir )
{
// Get standard path (e.g., /home/Bill/Data)
    std::string localPath = mainApp()->dataDir( iDataDir ).toStdString();

    struct statvfs  statLocal;
    quint64         localAvail = 0;

    if( statvfs( localPath.c_str(), &statLocal ) == 0 )
        localAvail = quint64(statLocal.f_bavail) * quint64(statLocal.f_frsize);
    else {
        // Fallback if path doesn't exist
        return quint64(4096) * 1024 * 1024;
    }

// If we are in WSL, we MUST also check the physical host (C: drive)
// because the virtual disk might report 1TB free when physical disk is full.

    if( isWSL() ) {

        // Check /mnt/c which maps to Windows system drive

        struct statvfs  statHost;

        if( statvfs( "/mnt/c", &statHost ) == 0 ) {

            quint64 hostAvail = quint64(statHost.f_bavail) * quint64(statHost.f_frsize);

            if( hostAvail < localAvail )
                return hostAvail;
        }
    }

    return localAvail;
}

#else

quint64 availableDiskSpace( int iDataDir )
{
    Q_UNUSED( iDataDir )

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
/* setHighPriority ------------------------------------------------ */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

void setHighPriority( bool on )
{
    Log() << QString("Setting %1 process priority.")
                .arg( on ? "high" : "normal" );

    if( !SetPriorityClass( GetCurrentProcess(),
        (on ? HIGH_PRIORITY_CLASS : NORMAL_PRIORITY_CLASS) ) ) {

        Warning() << "SetPriorityClass() failed: " << int(GetLastError());
    }
}

#elif defined(Q_OS_LINUX)

void setHighPriority( bool on )
{
    if( geteuid() == 0 ) {

        Log() << "Running as root, setting high process priority";

#if 0
// This function prevents swapping out app memory pages...
// but it will starve OS for memory.

        if( mlockall( MCL_CURRENT | MCL_FUTURE ) ) {
            int e = errno;
            Warning() <<  "Error from mlockall(): " << strerror( e );
        }
#endif

        struct sched_param  p;

        if( on ) {
            p.sched_priority = sched_get_priority_min( SCHED_RR ) +
                                0.80 *
                                (sched_get_priority_max( SCHED_RR )
                                - sched_get_priority_min( SCHED_RR ));
        }
        else {
            p.sched_priority = (sched_get_priority_min( SCHED_RR )
                                + sched_get_priority_max( SCHED_RR )) / 2;
        }

        if( sched_setscheduler( 0, SCHED_RR, &p ) ) {
            int e = errno;
            Warning() << "Error from sched_setscheduler(): " << strerror( e );
        }
    }
    else
        Warning() << "Not running as root, cannot set high priority";
}

#else /* !Q_OS_WIN && !Q_OS_LINUX */

void setHighPriority( bool on )
{
    Q_UNUSED( on )
    Warning() << "Cannot set high process priority on this platform.";
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
/* getNAssignedThreads -------------------------------------------- */
/* ---------------------------------------------------------------- */

int getNAssignedThreads()
{
    return qBound( 1, (int)std::thread::hardware_concurrency(), 64 );
}


/* ---------------------------------------------------------------- */
/* getCurProcessorIdx --------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

int getCurProcessorIdx()
{
    return GetCurrentProcessorNumber();
}

#elif defined(Q_OS_LINUX)

int getCurProcessorIdx()
{
    return sched_getcpu();
}

#else

int getCurProcessorIdx()
{
    return 0;
}

#endif

/* ---------------------------------------------------------------- */
/* pCoreAffinityMask ---------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Notes:
// - Query key RelationProcessorCore defines union member Processor,
// which is type _PROCESSOR_RELATIONSHIP.
// - The enumerated records are physical processors, not cores.
// - struct _PROCESSOR_RELATIONSHIP does not contain EfficiencyClass
// member as of Qt 6.9.1, but we can see from microsoft pages that
// EfficiencyClass maps to Reserved[0].
// - Higher values of EfficiencyClass -> more power/less efficient.
// - There can be multiple classes and we want highest value ones.
// - So first pass assesses range of class values.
// - Second pass builds mask.
//
quint64 pCoreAffinityMask()
{
// Fetch processor data

    DWORD   len = 0;

    GetLogicalProcessorInformationEx(
        RelationProcessorCore, NULL, &len );

    std::vector<BYTE>   buffer( len );

    if( !len ||
        !GetLogicalProcessorInformationEx(
            RelationProcessorCore,
            reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(
                buffer.data()),
            &len) ) {

        Warning() << "Failed to query processor info.";
        return 0;
    }

// First pas: get range of classes

    auto    ptr = buffer.data();
    int     cmin = 99, cmax = 0;

    while( ptr < buffer.data() + len ) {

        auto info =
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

        if( info->Relationship == RelationProcessorCore ) {

            auto &core  = info->Processor;
            int  ceff   = core.Reserved[0];
            cmin = qMin( ceff, cmin );
            cmax = qMax( ceff, cmax );
        }

        ptr += info->Size;
    }

// Second pass: build cmax mask

    if( cmin == cmax )
        return 0;

    DWORD_PTR   mask = 0;

    ptr = buffer.data();

    while( ptr < buffer.data() + len ) {

        auto info =
        reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);

        if( info->Relationship == RelationProcessorCore ) {

            auto &core = info->Processor;

            if( core.Reserved[0] == cmax ) {
                // core.GroupMask[0].Mask = logical cores for this proc
                mask |= core.GroupMask[0].Mask;
            }
        }

        ptr += info->Size;
    }

    return mask;
}

#else

quint64 pCoreAffinityMask()
{
    return 0;
}

#endif

/* ---------------------------------------------------------------- */
/* setProcessAffinityMask ----------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

// Set which cores to run on, or zero for all available.
//
// Notes:
// GetProcessAffinityMask returns 0xF for a single CPU system
// having 2 cores/4 threads. Therefore, the mask bits refer to
// threads (virtual/logical "cores").
//
// setProcessAffinityMask( 0 ) has no effect.
// setProcessAffinityMask( 0xFF ) on a 4 thread system is an error:
// the mask bits must be a subset of the system mask bits returned
// by GetProcessAffinityMask.
//
void setProcessAffinityMask( quint64 mask )
{
    if( !mask ) {
        for( int i = 0, n = getNAssignedThreads(); i < n; ++i )
            mask |= (1LL << i);
    }

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

void setProcessAffinityMask( quint64 mask )
{
    if( !mask ) {
        for( int i = 0, n = getNAssignedThreads(); i < n; ++i )
            mask |= (1LL << i);
    }

    cpu_set_t   cpuset;
    int         nMaskBits = sizeof(mask) * 8;

    CPU_ZERO( &cpuset );

    for( int i = 0; i < nMaskBits; ++i ) {

        if( mask & (1LL << i) )
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

void setProcessAffinityMask( quint64 )
{
    Warning() << "setProcessAffinityMask unimplemented on this system.";
}

#endif

/* ---------------------------------------------------------------- */
/* setCurrentThreadAffinityMask ----------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef Q_OS_WIN

quint64 setCurrentThreadAffinityMask( quint64 mask )
{
    return
        static_cast<quint64>(
        SetThreadAffinityMask( GetCurrentThread(), DWORD_PTR(mask) ));
}

#else /* !Q_OS_WIN */

quint64 setCurrentThreadAffinityMask( quint64 )
{
    Warning() << "setCurrentThreadAffinityMask not implemented.";
    return 0;
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

#elif defined(Q_OS_LINUX)

double getRAMBytes64BitApp()
{
// Method 1: Try reading /proc/meminfo (Recommended)
// This is the most accurate way to get "Available" memory on Linux kernels 3.14+
// It accounts for cache that can be reclaimed, similar to Windows behavior.

    std::ifstream   file( "/proc/meminfo" );

    if( file.is_open() ) {

        std::string token;

        while( file >> token ) {

            if( token == "MemAvailable:" ) {
                quint64 memAvailableKb;
                if( file >> memAvailableKb ) {
                    // /proc/meminfo reports in kB, convert to bytes
                    return memAvailableKb * 1024.0;
                }
            }
        }

        file.close();
    }

// Method 2: Fallback to sysinfo()
// Used if /proc/meminfo fails or MemAvailable isn't present (very old kernels).
// Note: 'freeram' is strictly unused memory and does not count reclaimable cache,
// so this number will likely be lower than Method 1.

    struct sysinfo  info;
    if( sysinfo(&info) == 0 )
        return double(info.freeram) * double(info.mem_unit);

    Warning() << "getRAMBytes64BitApp did not succeed.";
    return 4.0 * 1024.0 * 1024.0 * 1024.0;
}

#else

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


