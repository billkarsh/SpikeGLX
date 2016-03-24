
#include "Socket.h"

#include <cstring>
#include <cstdlib>

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* WINDOWS -------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef WIN32    // WINDOWS

#include <winsock.h>
#include <io.h>

static const char *WSAGetLastErrorMessage(
    const char  *prefix = "",
    int         errorid = 0 );

typedef int socklen_t;

#define SHUT_RDWR                   2
#define CLOSE( x )                  closesocket( x )
#define IOCTL( x, y, z )            ioctlsocket( x, y, z )
#define LASTERROR_STR()             WSAGetLastErrorMessage()
#define LASTERROR_IS_CONNCLOSED()               \
    (WSAGetLastError() == WSAENETRESET ||       \
     WSAGetLastError() == WSAECONNABORTED ||    \
     WSAGetLastError() == WSAECONNRESET)

static bool     StartupCalled = false;
static WSADATA  wsaData;

/* ---------------------------------------------------------------- */
/* DoCleanup ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Called from atexit() as C-fun

extern "C" static void DoCleanup()
{
    if( StartupCalled )
        WSACleanup();

    StartupCalled = false;
}

/* ---------------------------------------------------------------- */
/* DO_STARTUP ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

static inline void DO_STARTUP()
{
    if( !StartupCalled ) {

        if( WSAStartup( 1<<8 | 1, &wsaData ) ) {

            throw SockErr(
                    "Could not start up winsock dll,"
                    " WSAStartup() failed." );
        }

        ::atexit( DoCleanup );
        StartupCalled = true;
    }
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* UNIX ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#else   // UNIX

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/ioctl.h>

#define DO_STARTUP()                do { } while (0)
#define CLOSE( x )                  ::close( x )
#define IOCTL( x, y, z )            ::ioctl( x, y, z )
#define LASTERROR_STR()             std::strerror( errno )
#define LASTERROR_IS_CONNCLOSED()   \
    (errno == ECONNRESET || errno == ENOTCONN || errno == EPIPE)

#endif  // WIN32 or UNIX

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Common --------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Socket::Socket( int type )
    :   m_addr(0), m_host("localhost"), m_error("Success"),
        m_sock(-1), m_type(type), m_port(-1),
        m_tcpNDelay(true), m_reuseAddr(true)
{
    DO_STARTUP();
    m_addr = new struct sockaddr_in;
    m_addr->sin_family = AF_INET;
}


Socket::~Socket()
{
    tcpDisconnect();
    delete m_addr;
    m_addr = 0;
}


bool Socket::tcpConnect( const std::string &host, uint16 port )
{
    tcpDisconnect();

    if( host.length() )
        setHost( host );

    if( port )
        setPort( port );

    switch( m_type ) {

        case UDP:
            m_sock = ::socket( PF_INET, SOCK_DGRAM, IPPROTO_IP );
            break;

        default:
            m_sock = ::socket( PF_INET, SOCK_STREAM, IPPROTO_IP );
            break;
    }

    if( !isValid() ) {
        m_error = LASTERROR_STR();
        return false;
    }

    if( m_type == TCP ) {

        setTCPNDelay();
        resolveHostAddr();

        if( ::connect(
                m_sock,
                reinterpret_cast<struct sockaddr*>(m_addr),
                sizeof(*m_addr) ) ) {

          m_error = LASTERROR_STR();
          CLOSE( m_sock );
          m_sock = -1;

          return false;
        }
    }

    return true;
}


void Socket::tcpDisconnect()
{
    if( isValid() ) {

        ::shutdown( m_sock, SHUT_RDWR );
        CLOSE( m_sock );
    }

    m_sock = -1;
}


void Socket::setSocketOption( int option, bool enable )
{
    if( !isValid() )
        return;

    switch( option ) {

        case TCPNoDelay:
            m_tcpNDelay = enable;
            setTCPNDelay();
            break;

        case ReuseAddr:
            m_reuseAddr = enable;
            setReuseAddr();
            break;
    }
}


bool Socket::bind(
    const std::string   &iface,
    uint16              port )
{
    setReuseAddr();

    struct sockaddr_in  addr;

    addr.sin_addr.s_addr    = inet_addr( iface.c_str() );
    addr.sin_port           = htons( port );
    addr.sin_family         = AF_INET;

    if( ::bind(
            m_sock,
            reinterpret_cast<struct sockaddr*>(&addr),
            sizeof(addr) ) != 0 ) {

        m_error = LASTERROR_STR();

        return false;
    }

    return true;
}


uint Socket::sendData(
    const void  *src,
    uint        srcBytes ) throw( const SockErr& )
{
    if( !srcBytes )
        return 0;

    int count = 0;

// TODO: non-blocking IO here!?

    if( m_type == UDP ) {

        // Datagram/UDP
        resolveHostAddr();

        count = ::sendto(
                    m_sock,
                    static_cast<const char*>(src),
                    srcBytes,
                    0,
                    reinterpret_cast<struct sockaddr*>(m_addr),
                    sizeof(*m_addr) );
    }
    else {

        // Stream/TCP
        count = ::send(
                    m_sock,
                    static_cast<const char*>(src),
                    srcBytes,
                    0 );
    }

    if( count < 0 ) {

        if( LASTERROR_IS_CONNCLOSED() )
            throw ConnectionClosed();

        m_error = LASTERROR_STR();
        throw SockErr( m_error );
    }
    else if( count == 0 ) {

        throw ConnectionClosed(
                std::string("EOF on socket to ") + m_host );
    }

    return count;
}


uint Socket::receiveData(
    void    *dst,
    uint    dstBytes ) throw( const SockErr& )
{
    if( !dstBytes )
        return 0;

    int count = 0;

// TODO: non-blocking IO here!?

    if( m_type == UDP ) {

        // Datagram/UDP
        socklen_t   fromlen = sizeof(*m_addr);

        resolveHostAddr();

        count = ::recvfrom(
                    m_sock,
                    static_cast<char*>(dst),
                    dstBytes,
                    0,
                    reinterpret_cast<struct sockaddr*>(m_addr),
                    &fromlen );
    }
    else {

        // Stream/TCP
        count = ::recv(
                    m_sock,
                    static_cast<char*>(dst),
                    dstBytes,
                    0 );
    }

    if( count < 0 ) {

        if( LASTERROR_IS_CONNCLOSED() )
            throw ConnectionClosed();

        m_error = LASTERROR_STR();
        throw SockErr( m_error );
    }
    else if( count == 0 ) {

        throw ConnectionClosed(
                std::string("EOF on socket to ") + m_host );
    }

    return count;
}


bool Socket::waitData( uint waitMS ) throw( const SockErr& )
{
    if( !isValid() )
        return false;

    int sec = waitMS / 1000,
        ms  = waitMS - 1000 * sec;

    struct timeval  tv;
    tv.tv_sec   = sec;
    tv.tv_usec  = 1000 * ms;

    fd_set  readfds;
    FD_ZERO( &readfds );
    FD_SET( m_sock, &readfds );

    int ret = select( m_sock + 1, &readfds, 0, 0, &tv );

// Ready
    if( ret > 0 )
        return true;

// !Ready
    if( ret == 0 )
        return false;

// Error
    if( LASTERROR_IS_CONNCLOSED() )
        throw ConnectionClosed();

// Else
    m_error = LASTERROR_STR();
    throw SockErr( m_error );

    return false;
}


uint Socket::nReadyForRead() const
{
    if( !isValid() )
        return 0;

#ifdef WIN32
    ulong   n = 0;
#else
    int     n = 0;
#endif

    IOCTL( m_sock, FIONREAD, &n );

    return (uint)n;
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Socket::resolveHostAddr()
{
    resolveHostAddr( *m_addr, m_host, m_port );
}


void Socket::resolveHostAddr(
    struct sockaddr_in  &addr,
    const std::string   &host,
    uint16              port )
{
    struct hostent  *he = gethostbyname( host.c_str() );

    if( !he ) {

        throw HostNotFound(
                host +
                " is not found by the resolver (" +
                LASTERROR_STR() +
                ")." );
    }

    memcpy(
        &addr.sin_addr.s_addr,
        he->h_addr,
        sizeof(addr.sin_addr.s_addr) );

    addr.sin_port   = htons( port );
    addr.sin_family = AF_INET;
}


void Socket::setTCPNDelay() const
{
#ifdef WIN32
    BOOL    flag    = m_tcpNDelay;
    int     ret     = ::setsockopt(
                        m_sock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        reinterpret_cast<char*>(&flag),
                        sizeof(flag) );
#else
    long    flag    = m_tcpNDelay;
    int     ret     = ::setsockopt(
                        m_sock,
                        IPPROTO_TCP,
                        TCP_NODELAY,
                        &flag,
                        sizeof(flag) );
#endif

    if( ret != 0 )
        throw SockErr( "Could not set TCP No Delay." );
}


void Socket::setReuseAddr() const
{
#ifdef WIN32
    BOOL    flag    = m_reuseAddr;
    int     ret     = ::setsockopt(
                        m_sock,
                        SOL_SOCKET ,
                        SO_REUSEADDR,
                        reinterpret_cast<char*>(&flag),
                        sizeof(flag) );
#else
    long    flag    = m_reuseAddr;
    int     ret     = ::setsockopt(
                        m_sock,
                        SOL_SOCKET,
                        SO_REUSEADDR,
                        &flag,
                        sizeof(flag) );
#endif

    if( ret != 0 )
        throw SockErr( "Could not set SO_REUSEADDR." );
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* WINDOWS -------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef WIN32

#include <strstream>
#include <algorithm>

/* ---------------------------------------------------------------- */
/* ErrorEntry ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// List of Winsock error constants mapped to an interpretation string.
// Note that this list must remain sorted by the error constant
// values, because we do a binary search on the list when looking up
// items.

static struct ErrorEntry {
    const char* msg;
    int         nID;

    ErrorEntry( int id, const char* msg = 0 ) : msg(msg), nID(id) {}

    bool operator< ( const ErrorEntry &rhs )  {return nID < rhs.nID;}

} gaErrorList[] = {
    ErrorEntry(0,                  "No error"),
    ErrorEntry(WSAEINTR,           "Interrupted system call"),
    ErrorEntry(WSAEBADF,           "Bad file number"),
    ErrorEntry(WSAEACCES,          "Permission denied"),
    ErrorEntry(WSAEFAULT,          "Bad address"),
    ErrorEntry(WSAEINVAL,          "Invalid argument"),
    ErrorEntry(WSAEMFILE,          "Too many open sockets"),
    ErrorEntry(WSAEWOULDBLOCK,     "Operation would block"),
    ErrorEntry(WSAEINPROGRESS,     "Operation now in progress"),
    ErrorEntry(WSAEALREADY,        "Operation already in progress"),
    ErrorEntry(WSAENOTSOCK,        "Socket operation on non-socket"),
    ErrorEntry(WSAEDESTADDRREQ,    "Destination address required"),
    ErrorEntry(WSAEMSGSIZE,        "Message too long"),
    ErrorEntry(WSAEPROTOTYPE,      "Protocol wrong type for socket"),
    ErrorEntry(WSAENOPROTOOPT,     "Bad protocol option"),
    ErrorEntry(WSAEPROTONOSUPPORT, "Protocol not supported"),
    ErrorEntry(WSAESOCKTNOSUPPORT, "Socket type not supported"),
    ErrorEntry(WSAEOPNOTSUPP,      "Operation not supported on socket"),
    ErrorEntry(WSAEPFNOSUPPORT,    "Protocol family not supported"),
    ErrorEntry(WSAEAFNOSUPPORT,    "Address family not supported"),
    ErrorEntry(WSAEADDRINUSE,      "Address already in use"),
    ErrorEntry(WSAEADDRNOTAVAIL,   "Can't assign requested address"),
    ErrorEntry(WSAENETDOWN,        "Network is down"),
    ErrorEntry(WSAENETUNREACH,     "Network is unreachable"),
    ErrorEntry(WSAENETRESET,       "Net connection reset"),
    ErrorEntry(WSAECONNABORTED,    "Software caused connection abort"),
    ErrorEntry(WSAECONNRESET,      "Connection reset by peer"),
    ErrorEntry(WSAENOBUFS,         "No buffer space available"),
    ErrorEntry(WSAEISCONN,         "Socket is already connected"),
    ErrorEntry(WSAENOTCONN,        "Socket is not connected"),
    ErrorEntry(WSAESHUTDOWN,       "Can't send after socket shutdown"),
    ErrorEntry(WSAETOOMANYREFS,    "Too many references, can't splice"),
    ErrorEntry(WSAETIMEDOUT,       "Connection timed out"),
    ErrorEntry(WSAECONNREFUSED,    "Connection refused"),
    ErrorEntry(WSAELOOP,           "Too many levels of symbolic links"),
    ErrorEntry(WSAENAMETOOLONG,    "File name too long"),
    ErrorEntry(WSAEHOSTDOWN,       "Host is down"),
    ErrorEntry(WSAEHOSTUNREACH,    "No route to host"),
    ErrorEntry(WSAENOTEMPTY,       "Directory not empty"),
    ErrorEntry(WSAEPROCLIM,        "Too many processes"),
    ErrorEntry(WSAEUSERS,          "Too many users"),
    ErrorEntry(WSAEDQUOT,          "Disc quota exceeded"),
    ErrorEntry(WSAESTALE,          "Stale NFS file handle"),
    ErrorEntry(WSAEREMOTE,         "Too many levels of remote in path"),
    ErrorEntry(WSASYSNOTREADY,     "Network system is unavailable"),
    ErrorEntry(WSAVERNOTSUPPORTED, "Winsock version out of range"),
    ErrorEntry(WSANOTINITIALISED,  "WSAStartup not yet called"),
    ErrorEntry(WSAEDISCON,         "Graceful shutdown in progress"),
    ErrorEntry(WSAHOST_NOT_FOUND,  "Host not found"),
    ErrorEntry(WSANO_DATA,         "No host data of that type was found")
};

static const int kNumMessages = sizeof(gaErrorList) / sizeof(ErrorEntry);

/* ---------------------------------------------------------------- */
/* WSAGetLastErrorMessage ----------------------------------------- */
/* ---------------------------------------------------------------- */

// A function similar in spirit to Unix's perror() that tacks a canned
// interpretation of the value of WSAGetLastError() onto the end of a
// passed string, separated by a ": ".  Generally, you should implement
// smarter error handling than this, but for default cases and simple
// programs, this function is sufficient.
//
// This function returns a pointer to an internal static buffer, so you
// must copy the data from this function before you call it again.  It
// follows that this function is also not thread-safe.

const char* WSAGetLastErrorMessage(
    const char  *prefix,
    int         errorid )
{
// Build basic error string

    static char acErrorBuffer[256];

    std::ostrstream outs( acErrorBuffer, sizeof(acErrorBuffer) );

    if( prefix && *prefix )
        outs << prefix << ": ";

// Tack appropriate canned message onto end of supplied message
// prefix. Note that we do a binary search here: gaErrorList must
// be sorted by error constant value.

    ErrorEntry  Target( errorid ? errorid : WSAGetLastError() );
    ErrorEntry  *pEnd   = gaErrorList + kNumMessages;
    ErrorEntry  *it     = std::lower_bound( gaErrorList, pEnd, Target );

    if( it != pEnd && it->nID == Target.nID )
        outs << it->msg;
    else
        outs << "unknown error";

    outs << " (" << Target.nID << ")";

// Terminate message

    outs << std::ends;
    acErrorBuffer[sizeof(acErrorBuffer) - 1] = 0;

    return acErrorBuffer;
}

#endif  // WINDOWS


