#ifndef COMMANDSERVER_H
#define COMMANDSERVER_H

#include "SockUtil.h"

#include <QTcpServer>
#include <QStringList>

class Par2Worker;
class MainApp;
class ConfigCtl;
class Run;

namespace DAQ {
class Params;
}

class QTcpSocket;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define CMD_DEF_PORT    4142
#define CMD_TOUT_MS     10000


class CmdServer : protected QTcpServer
{
private:
    int timeout_msecs;

public:
    CmdServer( QObject *parent )
    :   QTcpServer(parent), timeout_msecs(CMD_TOUT_MS)  {}
    virtual ~CmdServer();

    bool beginListening(
        const QString   &iface = "0.0.0.0",
        ushort          port = CMD_DEF_PORT,
        uint            timeout_ms = CMD_TOUT_MS );

    static void deleteAllActiveConnections();

protected:
    virtual void incomingConnection( qintptr sockFd );  // from QTcpServer
};


// CmdServer creates a new CmdWorker instance
// to handle each remote connection. If CmdWorker
// remains innactive for timeout_ms (~10s) it
// deletes itself (see CmdWorker::run()).
//
class CmdWorker : public QObject
{
    Q_OBJECT

private:
    QString     errMsg;
    Par2Worker  *par2;
    QTcpSocket  *sock;
    SockUtil    SU;
    qintptr     sockFd,     // socket 'file descriptor'
                timeout;

public:
    CmdWorker( qintptr sockFd, int timeout )
    :   QObject(0), par2(0),
        sock(0), sockFd(sockFd),
        timeout(timeout)  {}
    virtual ~CmdWorker();

signals:
    void finished();

public slots:
    void run();
    void sha1Progress( int pct );
    void sha1Result( int res );
    void par2Report( const QString &s );
    void par2Error( const QString &s );

private:
    void sendOK();
    void sendError( const QString &errMsg );
    MainApp* okAppValidated( const QString &cmd );
    ConfigCtl* okCfgValidated( const QString &cmd );
    ConfigCtl* okjsip( const QString &cmd, int js, int ip );
    ConfigCtl* okStreamToks( const QString &cmd, int &js, int &ip, const QStringList &toks );
    Run* okRunStarted( const QString &cmd );
    void getGeomMap( QString &resp, const QStringList &toks );
    void getImecChanGains( QString &resp, const QStringList &toks );
    void getParams( QString &resp );
    void getParamsImAll( QString &resp );
    void getParamsImProbe( QString &resp, const QStringList &toks );
    void getParamsOneBox( QString &resp, const QStringList &toks );
    void getProbeList( QString &resp );
    void getRunName( QString &resp );
    void getStreamAcqChans( QString &resp, const QStringList &toks );
    void getStreamI16ToVolts( QString &resp, const QStringList &toks );
    void getStreamMaxInt( QString &resp, const QStringList &toks );
    void getStreamNP( QString &resp, const QStringList &toks );
    void getStreamSampleRate( QString &resp, const QStringList &toks );
    void getStreamSaveChans( QString &resp, const QStringList &toks );
    void getStreamSN( QString &resp, const QStringList &toks );
    void getStreamVoltageRange( QString &resp, const QStringList &toks );
    void isConsoleHidden( QString &resp );
    void mapSample( QString &resp, const QStringList &toks );
    void opto_getAttens( QString &resp, const QStringList &toks );
    void consoleShow( bool show );
    bool enumDir( const QString &path );
    void fetch( const QStringList &toks );
    void getStreamShankMap( const QStringList &toks );
    void opto_emit( QStringList toks );
    void par2Start( QStringList toks );
    void setAnatomyPP( const QStringList &toks );
    void setAudioEnable( const QStringList &toks );
    void setAudioParams( const QString &group );
    void setDataDir( QStringList toks );
    void setMetaData();
    void setMultiDriveEnable( const QStringList &toks );
    void setNextFileName( const QString &name );
    void setNIDO( const QStringList &toks );
    void setOBXAO( const QStringList &toks );
    void setParams();
    void setParamsImAll();
    void setParamsImProbe( const QStringList &toks );
    void setParamsOneBox( const QStringList &toks );
    void setRecordingEnabled( const QStringList &toks );
    void setRunName( const QStringList &toks );
    void setTriggerOffBeep( const QStringList &toks );
    void setTriggerOnBeep( const QStringList &toks );
    void startRun();
    void stopRun();
    void triggerGT( const QStringList &toks );
    void verifySha1( QString file );
    bool doQuery( const QString &cmd, const QStringList &toks );
    bool doCommand( const QString &cmd, const QStringList &toks );
    bool processLine( const QString &line );
};

#endif  // COMMANDSERVER_H


