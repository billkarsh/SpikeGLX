
#include "SockUtil.h"
#include "Util.h"

#ifdef Q_OS_LINUX
#include <sys/socket.h>
#endif

#include <QHostAddress>


#define LINELEN 65536


void SockUtil::setLowLatency()
{
#if QT_VERSION >= 0x040600
    sock->setSocketOption( QAbstractSocket::LowDelayOption, 1 );
#else
    socketNoNagle( sock->socketDescriptor() );
#endif
}


const QString &SockUtil::addr()
{
    if( _addr.isEmpty() && sock ) {

        _addr = QString("(%1:%2)")
                    .arg( sock->peerAddress().toString() )
                    .arg( sock->peerPort() );
    }

    return _addr;
}


// Lowest cost check against unexpected exit of our own comm. thread,
// hence, deletion of its socket. This doesn't test whether the socket
// is valid/connected, see sockValid().
//
bool SockUtil::sockExists()
{
    if( !sock ) {
        appendError( errOut, "Socket unexpectedly deleted." );
        return false;
    }

    return true;
}


// A more expensive test for the useability of a socket.
// It is unclear what isValid() really entails, so we
// check that and, perhaps redundantly, the state.
//
bool SockUtil::sockValid()
{
    if( !sockExists() )
        return false;

    if( !sock->isValid() ) {
        appendError( errOut, "Socket invalid" );
        return false;
    }

    if( sock->state() != QAbstractSocket::ConnectedState ) {
        appendError( errOut, "Socket disconnected" );
        return false;
    }

    return true;
}


bool SockUtil::connectToServer( const QString &host, quint16 port )
{
    sock->connectToHost( host, port );

    if( !sock->waitForConnected( timeout_ms ) || !sock->isValid() ) {

        QString err = errorToString( sock->error() );

        appendError( errOut, err );

        QString logMsg = QString("Err %1%2 [%3]")
                            .arg( _tag ).arg( addr() ).arg( err );

        if( sock->error() == QAbstractSocket::ConnectionRefusedError )
            Debug() << logMsg;
        else
            Error() << logMsg;

        return false;
    }

    return true;
}


bool SockUtil::send( const QString &msg, bool debugInput )
{
    if( debugInput ) {
        Debug()
            << "Snd "
            << _tag << addr()
            << " [" << msg.trimmed() << "]";
    }

    if( !sockExists() )
        return false;

    sock->write( STR2CHR( msg ) );

    if( sock->bytesToWrite()
        && !sock->waitForBytesWritten( timeout_ms ) ) {

        appendError( errOut, errorToString( sock->error() ) );

        if( autoAbort )
            sock->abort();

        return false;
    }

    return true;
}


bool SockUtil::sendBinary( const void* src, qint64 bytes )
{
    if( !sockExists() )
        return false;

    sock->write( (const char*)src, bytes );

    if( sock->bytesToWrite()
        && !sock->waitForBytesWritten( timeout_ms ) ) {

        appendError( errOut, errorToString( sock->error() ) );

        if( autoAbort )
            sock->abort();

        return false;
    }

    return true;
}


// Return data string, or,
// return empty QString to signal end of communication.
//
QString SockUtil::readLine()
{
    for( int ct = 0; !sock->canReadLine(); ++ct ) {

        if( !sockValid() )
            return QString();

        if( !sock->waitForReadyRead( timeout_ms ) || ct >= 3 ) {

            QString err;

            if( sock->error() != QAbstractSocket::UnknownSocketError )
                err = errorToString( sock->error() );
            else
                err = "timeout or peer shutdown during read";

            appendError( errOut, err );
            return QString();
        }
    }

    return sock->readLine( LINELEN ).trimmed();
}


// Note:
// -----
// Calling sock->error() on a new healthy socket returns value
// QAbstractSocket::UnknownSocketError (-1). Most likely this
// value reflects NO error rather than a mysterious error type.
//
QString SockUtil::errorToString( QAbstractSocket::SocketError e )
{
    switch( e ) {

        case QAbstractSocket::UnknownSocketError:
            return "Application (not socket) error";
        case QAbstractSocket::ConnectionRefusedError:
            return "Connection refused (or timed out)";
        case QAbstractSocket::RemoteHostClosedError:
            return "Remote host closed connection";
        case QAbstractSocket::HostNotFoundError:
            return "Host address not found";
        case QAbstractSocket::SocketAccessError:
            return "Socket (application) privilege error";
        case QAbstractSocket::SocketResourceError:
            return "Socket resources exhausted";
        case QAbstractSocket::SocketTimeoutError:
            return "Socket operation timed out";
        case QAbstractSocket::DatagramTooLargeError:
            return "Datagram too large";
        case QAbstractSocket::AddressInUseError:
            return "Bind() address already in use";
        case QAbstractSocket::SocketAddressNotAvailableError:
            return "Bind() address not on this host";
        case QAbstractSocket::UnsupportedSocketOperationError:
            return "Socket operation unsupported on this OS";
        case QAbstractSocket::ProxyAuthenticationRequiredError:
            return "Socket proxy authentication error";
        default:
            break;
    }

    return QString("Untranslated socket error (%1)").arg( e );
}


// Append (';;' separated) eNew to existing eDst (if exists).
//
void SockUtil::appendError( QString *eDst, const QString &eNew )
{
    if( eDst && !eNew.isEmpty() ) {

        if( !eDst->isEmpty() )
            eDst->append( ";;" );

        eDst->append( eNew );
    }
}


#ifdef Q_OS_LINUX
void SockUtil::shutdown( QTcpSocket *sock )
{
//#define SHUT_RDWR   2
    if( sock && sock->state() == QAbstractSocket::ConnectedState )
        shutdown( sock->socketDescriptor(), SHUT_RDWR );
}
#else
void SockUtil::shutdown( QTcpSocket * )
{
}
#endif


