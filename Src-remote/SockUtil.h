#ifndef SOCKUTIL_H
#define SOCKUTIL_H

#include <QTcpSocket>
#include <QString>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Lightweight wrapper around a socket to
// simplify call sequences and report errors.
//
// No destructor, caller retains socket ownership.
//
class SockUtil
{
private:
    QTcpSocket  *sock;      // The socket, owned by caller
    QString     _tag,       // context string for err messages
                _addr;      // "(host:port)"; made by addr()
    QString     *errOut;    // caller's optional message copy
    int         timeout_ms;
    bool        autoAbort;  // abort() on send() error

public:
    SockUtil()  {}
    SockUtil(
        QTcpSocket      *sock,
        int             timeout_ms,
        const QString   &msgTag,
        QString         *errOut = 0,
        bool            autoAbort = false )
        {
            init( sock, timeout_ms, msgTag, errOut, autoAbort );
        }

    void init(
        QTcpSocket      *sock,
        int             timeout_ms,
        const QString   &msgTag,
        QString         *errOut = 0,
        bool            autoAbort = false )
        {
            this->sock          = sock;
            this->_tag          = msgTag;
            this->errOut        = errOut;
            this->timeout_ms    = timeout_ms;
            this->autoAbort     = autoAbort;
        }

    void setLowLatency();

    const QString &tag()   {return _tag;}
    const QString &addr();

    bool sockExists();
    bool sockValid();

    bool connectToServer( const QString &host, quint16 port );

    bool send( const QString &msg, bool debugInput = false );
    bool sendBinary( const void* src, qint64 bytes );

    QString readLine();

    static QString errorToString( QAbstractSocket::SocketError e );
    static void appendError( QString *eDst, const QString &eNew );

    static void shutdown( QTcpSocket *sock );
};

#endif  // SOCKUTIL_H


