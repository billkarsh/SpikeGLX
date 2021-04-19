#ifndef RGTSERVER_H
#define RGTSERVER_H

#include "KVParams.h"

#include <QTcpServer>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace ns_RgtServer
{

#define RGT_DEF_PORT    52521
#define RGT_TOUT_MS     1000

/* ---------------------------------------------------------------- */
/* Remote messages to server app ---------------------------------- */
/* ---------------------------------------------------------------- */

// Remote app commands gate high or low.
// Blocks until transaction complete or timeout.
// Returns true if success.
//
bool rgtSetGate(
    bool            hi,
    const QString   &host = "127.0.0.1",
    ushort          port = RGT_DEF_PORT,
    int             timeout_msecs = RGT_TOUT_MS,
    QString         *err = 0 );

// Remote app commands trigger high or low.
// Blocks until transaction complete or timeout.
// Returns true if success.
//
bool rgtSetTrig(
    bool            hi,
    const QString   &host = "127.0.0.1",
    ushort          port = RGT_DEF_PORT,
    int             timeout_msecs = RGT_TOUT_MS,
    QString         *err = 0 );

// Remote app sends supplementary ini-style metadata.
// Blocks until transaction complete or timeout.
// Returns true if success.
//
bool rgtSetMetaData(
    const KeyValMap &kvm,
    const QString   &host = "127.0.0.1",
    ushort          port = RGT_DEF_PORT,
    int             timeout_msecs = RGT_TOUT_MS,
    QString         *err = 0 );

/* ---------------------------------------------------------------- */
/* Server-side message handling ----------------------------------- */
/* ---------------------------------------------------------------- */

class RgtServer : public QTcpServer
{
    Q_OBJECT

private:
    int timeout_msecs;

public:
    RgtServer( QObject *parent );

    bool beginListening(
        const QString   &iface = "127.0.0.1",
        ushort          port = RGT_DEF_PORT,
        int             timeout_ms = RGT_TOUT_MS );

signals:
    void rgtSetGate( bool hi );
    void rgtSetTrig( bool hi );
    void rgtSetMetaData( const KeyValMap &kvm );

protected:
    void incomingConnection( qintptr sockFd );  // from QTcpServer

private:
    void processConnection( QTcpSocket &sock );
};

}   // namespace ns_RgtServer

#endif  // RGTSERVER_H


