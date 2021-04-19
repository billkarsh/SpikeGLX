
#include "RgtServer.h"
#include "Util.h"
#include "SockUtil.h"

#include <QHostAddress>


#define GREETING    "XOXO"
#define SETGATEHI   "SETGATE 1"
#define SETGATELO   "SETGATE 0"
#define SETTRIGHI   "SETTRIG 1"
#define SETTRIGLO   "SETTRIG 0"
#define SETMETA     "SETMETA"
#define METAEND     "METAEND"
#define OK          "OK"


namespace ns_RgtServer
{

/* ---------------------------------------------------------------- */
/* Remote messages to server app ---------------------------------- */
/* ---------------------------------------------------------------- */

static bool prologue(
    const QString   &cmd,
    QString         *err,
    SockUtil        &SU,
    const QString   &host,
    ushort          port )
{
    Debug() << QString("Remote cmd [%1]: Started...").arg( cmd );

// -------
// Connect
// -------

    if( !SU.connectToServer( host, port ) ) {

        QString e = QString("Remote cmd [%1]: Could not connect to (%2:%3).")
                    .arg( cmd ).arg( host ).arg( port );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

// ------------
// Get greeting
// ------------

    QString line = SU.readLine();

    if( line.isNull() ) {

        QString e = QString("Remote cmd [%1]: No greeting.")
                    .arg( cmd );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

    if( !line.startsWith( GREETING ) ) {

        QString e = QString("Remote cmd [%1]: Bad greeting [%2].")
                    .arg( cmd ).arg( line );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

// ------------
// Send command
// ------------

    if( !SU.send( QString("%1\n").arg( cmd ) ) ) {

        QString e = QString("Remote cmd [%1]: Failed sending command.")
                    .arg( cmd );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

    return true;
}


static bool epilogue(
    const QString   &cmd,
    QString         *err,
    SockUtil        &SU )
{
// -------------------
// Get acknowledgement
// -------------------

    QString line = SU.readLine();

    if( line.isNull() ) {

        QString e = QString("Remote cmd [%1]: No ACK.")
                    .arg( cmd );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

    if( !line.startsWith( OK ) ) {

        QString e = QString("Remote cmd [%1]: ACK not OK [%2].")
                    .arg( cmd ).arg( line );
        SU.appendError( err, e );
        Error() << e;
        return false;
    }

// ----
// Done
// ----

    Debug() << QString("Remote cmd [%1]: Success.").arg( cmd );

    return true;
}


bool rgtSetGate(
    bool            hi,
    const QString   &host,
    ushort          port,
    int             timeout_msecs,
    QString         *err )
{
    QString     cmd = (hi ? SETGATEHI : SETGATELO);
    QTcpSocket  sock;
    SockUtil    SU( &sock, timeout_msecs, "RemoteApp", err );

    if( !prologue( cmd, err, SU, host, port ) )
        return false;

    return epilogue( cmd, err, SU );
}


bool rgtSetTrig(
    bool            hi,
    const QString   &host,
    ushort          port,
    int             timeout_msecs,
    QString         *err )
{
    QString     cmd = (hi ? SETTRIGHI : SETTRIGLO);
    QTcpSocket  sock;
    SockUtil    SU( &sock, timeout_msecs, "RemoteApp", err );

    if( !prologue( cmd, err, SU, host, port ) )
        return false;

    return epilogue( cmd, err, SU );
}


bool rgtSetMetaData(
    const KeyValMap &kvm,
    const QString   &host,
    ushort          port,
    int             timeout_msecs,
    QString         *err )
{
    QTcpSocket  sock;
    SockUtil    SU( &sock, timeout_msecs, "RemoteApp", err );

    if( !prologue( SETMETA, err, SU, host, port ) )
        return false;

    for( KeyValMap::const_iterator it = kvm.begin(); it != kvm.end(); ++it ) {

        if( !SU.send( QString("%1 = %2\n")
                        .arg( it.key() )
                        .arg( it.value().toString() ) ) ) {

            return false;
        }
    }

    if( !SU.send( METAEND "\n" ) )
        return false;

    return epilogue( SETMETA, err, SU );
}

/* ---------------------------------------------------------------- */
/* Server-side message handling ----------------------------------- */
/* ---------------------------------------------------------------- */

RgtServer::RgtServer( QObject *parent )
    :   QTcpServer(parent), timeout_msecs(RGT_TOUT_MS)
{
}


bool RgtServer::beginListening(
    const QString   &iface,
    ushort          port,
    int             timeout_ms )
{
    QHostAddress    haddr;

    timeout_msecs = timeout_ms;

    if( iface == "0.0.0.0" )
        haddr = QHostAddress::Any;
    else if( iface == "localhost" || iface == "127.0.0.1" )
        haddr = QHostAddress::LocalHost;
    else
        haddr = iface;

    if( !listen( haddr, port ) ) {
        Error() << QString("Gate/Trigger server could not listen on (%1:%2) [%3].")
                    .arg( haddr.toString() )
                    .arg( port )
                    .arg( errorString() );
        return false;
    }

    Log() << QString("Gate/Trigger server listening on (%1:%2).")
                .arg( haddr.toString() )
                .arg( port );

    return true;
}


void RgtServer::incomingConnection( qintptr sockFd )
{
    QTcpSocket  sock;

    sock.setSocketDescriptor( sockFd );

    processConnection( sock );
}


void RgtServer::processConnection( QTcpSocket &sock )
{
    QString     line, cmd, err;
    SockUtil    SU( &sock, timeout_msecs, "RgtSrv", &err );

// -----------
// Test socket
// -----------

    if( !SU.sockValid() ) {

        Error() << QString("RgtSrv test err %1%2 [%3]")
                    .arg( SU.tag() ).arg( SU.addr() ).arg( err );
        return;
    }

// -------------
// Send greeting
// -------------

    if( !SU.send( GREETING "\n" ) ) {

        Error() << QString("RgtSrv send greeting err %1%2 [%3]")
                    .arg( SU.tag() ).arg( SU.addr() ).arg( err );
        return;
    }

// -------------
// Parse command
// -------------

    line = SU.readLine();

    if( line.isNull() ) {

        Error() << QString("RgtSrv empty cmd err %1%2 [%3]")
                    .arg( SU.tag() ).arg( SU.addr() ).arg( err );
        return;
    }

    cmd = line.trimmed();

    if( cmd.startsWith( "SETGATE" ) )
        emit rgtSetGate( cmd.startsWith( SETGATEHI ) );
    else if( cmd.startsWith( "SETTRIG" ) )
        emit rgtSetTrig( cmd.startsWith( SETTRIGHI ) );
    else if( cmd.startsWith( "SETMETA" ) ) {

        KVParams    kvp;

        while( !(line = SU.readLine()).startsWith( METAEND ) )
            kvp.parseOneLine( line );

        emit rgtSetMetaData( kvp );
    }
    else {
        Error() << QString("RgtSrv unknown cmd err %1%2 [%3]")
                    .arg( SU.tag() ).arg( SU.addr() ).arg( cmd );
        return;
    }

// -----------
// Acknowledge
// -----------

    if( !SU.send( OK "\n" ) ) {

        Error() << QString("RgtSrv send OK err %1%2 [%3]")
                    .arg( SU.tag() ).arg( SU.addr() ).arg( err );
        return;
    }

// ----
// Done
// ----

    Debug() << QString("RgtSrv processed %1%2 [%3]")
                .arg( SU.tag() ).arg( SU.addr() ).arg( cmd );
}

}   // namespace ns_RgtServer


