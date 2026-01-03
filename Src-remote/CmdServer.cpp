
#include "CmdServer.h"
#include "Util.h"
#include "MainApp.h"
#include "Version.h"
#include "ConfigCtl.h"
#include "AOCtl.h"
#include "AIQ.h"
#include "Run.h"
#include "Sync.h"
#include "Subset.h"
#include "Sha1Verifier.h"
#include "Par2Window.h"
#include "Stim.h"

#include <QDir>
#include <QDirIterator>
#include <QRegularExpression>
#include <QThread>


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static QMutex   kilMtx;
static bool     allstop = false;
static void     stopAll()   {QMutexLocker ml(&kilMtx); allstop=true;}
static bool     allStop()   {QMutexLocker ml(&kilMtx); return allstop;}

/* ---------------------------------------------------------------- */
/* class CmdServer ------------------------------------------------ */
/* ---------------------------------------------------------------- */

CmdServer::~CmdServer()
{
    Log() << "Command server stopped.";
}


bool CmdServer::beginListening(
    const QString   &iface,
    ushort          port,
    uint            timeout_ms )
{
    QHostAddress    haddr;

    timeout_msecs = timeout_ms;

    if( iface == "0.0.0.0" )
        haddr = QHostAddress::Any;
    else if( iface == "localhost" || iface == "127.0.0.1" )
        haddr = QHostAddress::LocalHost;
    else
        haddr.setAddress( iface );

    if( !listen( haddr, port ) ) {
        Error() << QString("CmdSrv could not listen on (%1:%2) [%3].")
                    .arg( haddr.toString() )
                    .arg( port )
                    .arg( errorString() );
        return false;
    }

    Log() << QString("CmdSrv listening on (%1:%2).")
                .arg( haddr.toString() )
                .arg( port );

    return true;
}


void CmdServer::deleteAllActiveConnections()
{
    stopAll();
}


// Create and start a self-destructing connection worker.
//
void CmdServer::incomingConnection( qintptr sockFd )
{
    QThread     *thread = new QThread;
    CmdWorker   *worker = new CmdWorker( sockFd, timeout_msecs );

    worker->moveToThread( thread );

    Connect( thread, SIGNAL(started()), worker, SLOT(run()) );
    Connect( worker, SIGNAL(finished()), worker, SLOT(deleteLater()) );
    Connect( worker, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );
    Connect( thread, SIGNAL(finished()), thread, SLOT(deleteLater()) );

    thread->start();
}

/* ---------------------------------------------------------------- */
/* class CmdWorker ------------------------------------------------ */
/* ---------------------------------------------------------------- */

CmdWorker::~CmdWorker()
{
    Debug() << "Del " << SU.tag() << SU.addr();

    if( par2 ) {
        delete par2;
        par2 = 0;
    }

    SockUtil::shutdown( sock );

    if( sock ) {
        delete sock;
        sock = 0;
    }
}


void CmdWorker::run()
{
// -----------------------
// Create/configure socket
// -----------------------

    sock = new QTcpSocket;
    sock->setSocketDescriptor( sockFd );
    sock->moveToThread( this->thread() );

    SU.init( sock, timeout, "CmdWorker", &errMsg, true );
    SU.setLowLatency();

    Debug() << "New " << SU.tag() << SU.addr();

// ---------------------
// Get/dispatch messages
// ---------------------

    const int   max_errCt   = 5;
    int         errCt       = 0;

    while( !allStop() && SU.sockValid() && errCt < max_errCt ) {

        QString line = SU.readLine();

        if( line.isNull() )
            break;

        Debug() << "Rcv " << SU.tag() << SU.addr() << " [" << line << "]";

        if( line.length() ) {

            if( processLine( line ) ) {
                sendOK();
                errCt = 0;
            }
            else {
                sendError( errMsg );
                ++errCt;
            }
        }
    }

// ------------
// Self cleanup
// ------------

    QMetaObject::invokeMethod(
        mainApp()->getRun(),
        "qf_remoteClientDisable",
        Qt::QueuedConnection );

    Debug() << "End " << SU.tag() << SU.addr();

    emit finished();
}


void CmdWorker::sha1Progress( int pct )
{
    SU.send( QString("%1\n").arg( pct ), true );
}


void CmdWorker::sha1Result( int res )
{
    if( Sha1Worker::Success != res )
        errMsg = "SHA1: Sum does not match sum in metafile.";
}


void CmdWorker::par2Report( const QString &s )
{
    if( s.size() && !SU.send( QString("%1\n").arg( s ), true ) )
        QMetaObject::invokeMethod( par2, "cancel", Qt::QueuedConnection );
}


void CmdWorker::par2Error( const QString &err )
{
    SU.appendError( &errMsg, err );
}


void CmdWorker::sendOK()
{
    SU.send( "OK\n", true );
}


void CmdWorker::sendError( const QString &errMsg )
{
    Error() << "Err " << SU.tag() << SU.addr() << " [" << errMsg << "]";

    if( SU.sockValid() )
        SU.send( QString("ERROR %1\n").arg( errMsg ), true );
}


MainApp* CmdWorker::okAppValidated( const QString &cmd )
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg = QString("%1: Run parameters never validated.").arg( cmd );
        app = 0;
    }

    return app;
}


ConfigCtl* CmdWorker::okCfgValidated( const QString &cmd )
{
    ConfigCtl   *C = mainApp()->cfgCtl();

    if( !C->validated ) {
        errMsg = QString("%1: Run parameters never validated.").arg( cmd );
        C = 0;
    }

    return C;
}


ConfigCtl* CmdWorker::okjsip( const QString &cmd, int js, int ip )
{
    ConfigCtl   *C = okCfgValidated( cmd );

    if( !C )
        return 0;

    const DAQ::Params   &p = C->acceptedParams;
    int                 np;

    switch( js ) {
        case jsNI:
            if( p.stream_nNI() )
                return C;
            errMsg = QString("%1: nidq stream not enabled.").arg( cmd );
            break;
        case jsOB:
            np = p.stream_nOB();
            if( !np ) {
                errMsg = QString("%1: obx stream not enabled.").arg( cmd );
                break;
            }
            if( ip >= 0 && ip < np )
                return C;
            errMsg = QString("%1: obx stream-ip must be in range[0..%2].").arg( cmd ).arg( np - 1 );
            break;
        case jsIM:
        case -jsIM:
            np = p.stream_nIM();
            if( !np ) {
                errMsg = QString("%1: imec stream not enabled.").arg( cmd );
                break;
            }
            if( ip >= 0 && ip < np )
                return C;
            errMsg = QString("%1: imec stream-ip must be in range[0..%2].").arg( cmd ).arg( np - 1 );
            break;
        default:
            errMsg = QString("%1: stream-js must be in set {-2,0,1,2}.").arg( cmd );
    }

    return 0;
}


ConfigCtl* CmdWorker::okStreamToks( const QString &cmd, int &js, int &ip, const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = QString("%1: Requires at least 2 params.").arg( cmd );
        return 0;
    }

    js = toks[0].toInt();
    ip = toks[1].toInt();

    return okjsip( cmd, js, ip );
}


Run* CmdWorker::okRunStarted( const QString &cmd )
{
    Run *run = mainApp()->getRun();

    if( !run->isRunning() ) {
        errMsg = QString("%1: Run not yet started.").arg( cmd );
        run = 0;
    }

    return run;
}


void CmdWorker::getGeomMap( QString &resp, const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "GETGEOMMAP: Requires parameter ip.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "GETGEOMMAP" );
    int         ip = toks[0].toInt(),
                np;

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;

    np = p.stream_nIM();

    if( !np )
        errMsg = "GETGEOMMAP: imec stream not enabled.";
    else if( ip < 0 || ip >= np )
        errMsg = QString("GETGEOMMAP: imec stream-ip must be in range[0..%1].").arg( np - 1 );
    else
        resp = p.im.prbj[ip].remoteGetGeomMap();
}


void CmdWorker::getImecChanGains( QString &resp, const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "GETIMECCHANGAINS: Requires params {ip, chan}.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "GETIMECCHANGAINS" );
    int         ip = toks[0].toInt(),
                ch = toks[1].toInt(),
                np;

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;

    np = p.stream_nIM();

    if( !np )
        errMsg = "GETIMECCHANGAINS: imec stream not enabled.";
    else if( ip < 0 || ip >= np )
        errMsg = QString("GETIMECCHANGAINS: imec stream-ip must be in range[0..%1].").arg( np - 1 );
    else {
        IMROTbl *R = p.im.prbj[ip].roTbl;
        resp = QString("%1 %2\n").arg( R->apGain( ch ) ).arg( R->lfGain( ch ) );
    }
}


void CmdWorker::getLastGT( QString &resp )
{
    ConfigCtl   *C = okCfgValidated( "GETLASTGT" );

    if( !C )
        return;

    int g, t;
    mainApp()->getRun()->dfGetLastGT( g, t );

    resp = QString("%1 %2\n").arg( g ).arg( t );
}


void CmdWorker::getParams( QString &resp )
{
    ConfigCtl   *C = okCfgValidated( "GETPARAMS" );

    if( !C )
        return;

    QMetaObject::invokeMethod(
        C, "cmdSrvGetsParamStr",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, resp),
        Q_ARG(int, 0),
        Q_ARG(int, 0) );
}


void CmdWorker::getParamsImAll( QString &resp )
{
    ConfigCtl   *C = okCfgValidated( "GETPARAMSIMALL" );

    if( !C )
        return;

    QMetaObject::invokeMethod(
        C, "cmdSrvGetsParamStr",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, resp),
        Q_ARG(int, 1),
        Q_ARG(int, 0) );
}


void CmdWorker::getParamsImProbe( QString &resp, const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "GETPARAMSIMPRB: Requires parameter ip.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "GETPARAMSIMPRB" );
    int         ip = toks[0].toInt(),
                np;

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;

    np = p.stream_nIM();

    if( !np )
        errMsg = "GETPARAMSIMPRB: imec stream not enabled.";
    else if( ip < 0 || ip >= np )
        errMsg = QString("GETPARAMSIMPRB: imec stream-ip must be in range[0..%1].").arg( np - 1 );
    else {
        QMetaObject::invokeMethod(
            C, "cmdSrvGetsParamStr",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QString, resp),
            Q_ARG(int, 2),
            Q_ARG(int, ip) );
    }
}


void CmdWorker::getParamsOneBox( QString &resp, const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "GETPARAMSOBX: Requires params {ip, slot}.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "GETPARAMSOBX" );

    if( !C )
        return;

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "GETPARAMSOBX: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("GETPARAMSOBX: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("GETPARAMSOBX: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    QMetaObject::invokeMethod(
        C, "cmdSrvGetsParamStr",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, resp),
        Q_ARG(int, 3),
        Q_ARG(int, ip) );
}


void CmdWorker::getProbeList( QString &resp )
{
    resp = "()";

    ConfigCtl   *C = mainApp()->cfgCtl();

    if( C->validated ) {

        const DAQ::Params   &p = C->acceptedParams;
        int                 np = p.stream_nIM();

        if( np ) {
            resp.clear();
            for( int ip = 0; ip < np; ++ip ) {
                IMROTbl *R = p.im.prbj[ip].roTbl;
                resp += QString("(%1,%2,%3)")
                        .arg( ip ).arg( R->nShank() ).arg( R->pn );
            }
        }
    }

    resp += "\n";
}


void CmdWorker::getRunName( QString &resp )
{
    ConfigCtl   *C = okCfgValidated( "GETRUNNAME" );

    if( !C )
        return;

    resp = QString("%1\n").arg( C->acceptedParams.sns.runName );
}


void CmdWorker::getStreamAcqChans( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMACQCHANS", js, ip, toks );

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;

    switch( js ) {
        case jsNI:
            {
                const int*  cum = p.ni.niCumTypCnt;
                int         MN, MA, XA, XD;

                MN = cum[CniCfg::niTypeMN];
                MA = cum[CniCfg::niTypeMA] - cum[CniCfg::niTypeMN];
                XA = cum[CniCfg::niTypeXA] - cum[CniCfg::niTypeMA];
                XD = cum[CniCfg::niTypeXD] - cum[CniCfg::niTypeXA];

                resp = QString("%1 %2 %3 %4\n")
                        .arg( MN ).arg( MA ).arg( XA ).arg( XD );
            }
            break;
        case jsOB:
            {
                const int*  cum = p.im.get_iStrOneBox( ip ).obCumTypCnt;
                int         XA, XD, SY;

                XA = cum[CimCfg::obTypeXA];
                XD = cum[CimCfg::obTypeXD] - cum[CimCfg::obTypeXA];
                SY = cum[CimCfg::obTypeSY] - cum[CimCfg::obTypeXD];

                resp = QString("%1 %2 %3\n")
                        .arg( XA ).arg( XD ).arg( SY );
            }
            break;
        case jsIM:
        case -jsIM:
            {
                const int*  cum = p.im.prbj[ip].imCumTypCnt;
                int         AP, LF, SY;

                AP = cum[CimCfg::imTypeAP];
                LF = cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP];
                SY = cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF];

                resp = QString("%1 %2 %3\n")
                        .arg( AP ).arg( LF ).arg( SY );
            }
            break;
    }
}


void CmdWorker::getStreamI16ToVolts( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMI16TOVOLTS", js, ip, toks );

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;
    double              M;

    switch( js ) {
        case jsNI:  M = p.ni.int16ToV( 1, toks[2].toInt() ); break;
        case jsOB:  M = p.im.get_iStrOneBox( ip ).int16ToV( 1 ); break;
        case jsIM:
        case -jsIM: M = p.im.prbj[ip].intToV( 1, toks[2].toInt() ); break;
        default: errMsg = "GETSTREAMI16TOVOLTS: js must be in set {-2,0,1,2}."; return;
    }

    resp = QString("%1\n").arg( M );
}


void CmdWorker::getStreamMaxInt( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMMAXINT", js, ip, toks );

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;
    int                 mx;

    switch( js ) {
        case jsNI:  mx = 32768; break;
        case jsOB:  mx = 32768; break;
        case jsIM:
        case -jsIM: mx = p.im.prbj[ip].roTbl->maxInt(); break;
        default: errMsg = "GETSTREAMMAXINT: js must be in set {-2,0,1,2}."; return;
    }

    resp = QString("%1\n").arg( mx );
}


void CmdWorker::getStreamNP( QString &resp, const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "GETSTREAMNP: Requires parameter js.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "GETSTREAMNP" );
    int         js = toks[0].toInt();

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;
    int                 np;

    switch( js ) {
        case jsNI:  np = p.stream_nNI(); break;
        case jsOB:  np = p.stream_nOB(); break;
        case jsIM:
        case -jsIM: np = p.stream_nIM(); break;
        default: errMsg = "GETSTREAMNP: js must be in set {-2,0,1,2}."; return;
    }

    resp = QString("%1\n").arg( np );
}


void CmdWorker::getStreamSampleRate( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMSAMPLERATE", js, ip, toks );

    if( !C )
        return;

    const DAQ::Params   &p = C->acceptedParams;
    resp = QString("%1\n").arg( p.stream_rate( qAbs(js), ip ), 0, 'f', 6 );
}


void CmdWorker::getStreamSaveChans( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMSAVECHANS", js, ip, toks );

    if( !C )
        return;

    switch( js ) {
        case jsNI:
            QMetaObject::invokeMethod(
                C, "cmdSrvGetsSaveChansNi",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, resp) );
            break;
        case jsOB:
            QMetaObject::invokeMethod(
                C, "cmdSrvGetsSaveChansOb",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, resp),
                Q_ARG(int, ip) );
            break;
        case jsIM:
        case -jsIM:
            QMetaObject::invokeMethod(
                C, "cmdSrvGetsSaveChansIm",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, resp),
                Q_ARG(int, ip) );
            break;
    }
}


void CmdWorker::getStreamSN( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMSN", js, ip, toks );

    if( !C )
        return;

    switch( js ) {
        case jsOB:
            {
                const DAQ::Params           &p = C->acceptedParams;
                const CimCfg::ImProbeDat    &P = C->prbTab.get_iOneBox(
                                                    p.im.obx_istr2isel( ip ) );
                resp = QString("%1 %2\n").arg( P.obsn ).arg( P.slot );
            }
            break;
        case jsIM:
        case -jsIM:
            {
                const CimCfg::ImProbeDat    &P = C->prbTab.get_iProbe( ip );
                resp = QString("%1 %2\n").arg( P.sn ).arg( P.type );
            }
            break;
        default:
            errMsg = "GETSTREAMSN: Only valid for js = {-2,1,2}.";
    }
}


void CmdWorker::getStreamVoltageRange( QString &resp, const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMVOLTAGERANGE", js, ip, toks );

    if( !C )
        return;

    double              V;
    const DAQ::Params   &p = C->acceptedParams;

    switch( js ) {
        case jsNI:
            resp = QString("%1 %2\n").arg( p.ni.range.rmin ).arg( p.ni.range.rmax );
            break;
        case jsOB:
            V       = p.im.get_iStrOneBox( ip ).range.rmax;
            resp    = QString("%1 %2\n").arg( -V ).arg( V );
            break;
        case jsIM:
        case -jsIM:
            V       = p.im.prbj[ip].roTbl->maxVolts();
            resp    = QString("%1 %2\n").arg( -V ).arg( V );
            break;
    }
}


void CmdWorker::isConsoleHidden( QString &resp )
{
    bool    b;

    QMetaObject::invokeMethod(
        mainApp(),
        "remoteGetsIsConsoleHidden",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(bool, b) );

    resp = QString("%1\n").arg( b );
}


// Expected tok params:
// 0) dstjs
// 1) dstip
// 2) src sample index
// 3) srcjs
// 4) srcip
//
void CmdWorker::mapSample( QString &resp, const QStringList &toks )
{
    if( toks.size() < 5 ) {
        errMsg = "MAPSAMPLE: Requires at least 5 params.";
        return;
    }

    quint64 srcC    = toks.at( 2 ).toLongLong();
    int     dstjs   = toks.at( 0 ).toInt(),
            dstip   = toks.at( 1 ).toInt(),
            srcjs   = toks.at( 3 ).toInt(),
            srcip   = toks.at( 4 ).toInt();

    ConfigCtl   *C;

    if( !okjsip( "MAPSAMPLE (dst)", dstjs, dstip ) )
        return;

    if( !(C = okjsip( "MAPSAMPLE (src)", srcjs, srcip )) )
        return;

    dstjs = qAbs( dstjs );
    srcjs = qAbs( srcjs );

    if( dstjs == srcjs && dstip == srcip ) {
        resp = toks.at( 2 ).trimmed() + "\n";
        return;
    }

    const DAQ::Params   &p      = C->acceptedParams;
    const Run           *run    = mainApp()->getRun();
    const AIQ           *dstQ   = run->getQ( dstjs, dstip ),
                        *srcQ   = run->getQ( srcjs, srcip );

    if( dstQ && srcQ ) {

        SyncStream  dstS, srcS;
        dstS.init( dstQ, dstjs, dstip, p );
        srcS.init( srcQ, srcjs, srcip, p );
        syncDstTAbs( srcC, &srcS, &dstS, p );

        resp = QString("%1\n").arg( dstS.TAbs2Ct( dstS.tAbs ) );
    }
    else
        errMsg = "MAPSAMPLE: Not running.";
}


// Expected tok params:
// 0) ip
// 1) color
//
void CmdWorker::opto_getAttens( QString &resp, const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "OPTOGETATTENS: Requires params {ip, color}.";
        return;
    }

    Run *run = okRunStarted( "OPTOGETATTENS" );

    if( !run )
        return;

    QMetaObject::invokeMethod(
        run, "opto_getAttens",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, resp),
        Q_ARG(int, toks.at( 0 ).toInt()),
        Q_ARG(int, toks.at( 1 ).toInt()) );

    if( resp.startsWith( "O" ) || resp.startsWith( "I" ) ) {
        errMsg = resp;
        resp.clear();
    }
    else
        resp += "\n";
}


void CmdWorker::consoleShow( bool show )
{
    QMetaObject::invokeMethod(
        mainApp(), "remoteShowsConsole",
        Qt::QueuedConnection,
        Q_ARG(bool, show) );
}


bool CmdWorker::enumDir( const QString &path )
{
    if( path.isEmpty() ) {
        errMsg = "ENUMDATADIR: Directory index out of range.";
        return false;
    }

    QDir    dir( path );

    if( !dir.exists() ) {
        errMsg = "ENUMDATADIR: Directory not found: " + path;
        return false;
    }

    QDirIterator    it( path );
    QString         pth = path + "/";

    while( it.hasNext() ) {

        it.next();

        QFileInfo   fi      = it.fileInfo();
        QString     entry   = fi.fileName();

        if( fi.isDir() ) {

            if( entry == "." || entry == ".." )
                continue;

            if( !enumDir( pth + entry ) )
                return false;
        }
        else if( !SU.send( QString("%1\n").arg( pth + entry ), true ) )
            return false;
    }

    return true;
}


// Expected tok params:
// 0) js
// 1) ip
// 2) starting sample index
// 3) max count
// 4) <channel subset pattern "id1#id2#...">
// 5) <integer downsample factor>
//
// Send( 'BINARY_DATA %d %d uint64(%ld)'\n", nChans, nSamps, headCt ).
// Write binary data stream.
//
void CmdWorker::fetch( const QStringList &toks )
{
    if( toks.size() < 4 ) {
        errMsg = "FETCH: Requires at least 4 params.";
        return;
    }

    int         js, ip;
    ConfigCtl   *C = okStreamToks( "FETCH", js, ip, toks );

    if( !C )
        return;

    const DAQ::Params   &p  = C->acceptedParams;
    const AIQ*          aiQ = mainApp()->getRun()->getQ( js, ip );

    if( !aiQ ) {
        errMsg = "FETCH: Not running or stream not enabled.";
        return;
    }

    if( js == -jsIM )
        aiQ->qf_remoteClient( true );

// -----
// Chans
// -----

    QBitArray   chanBits;
    int         nChans  = aiQ->nChans();
    uint        dnsmp   = 1;

    if( toks.size() < 5 || toks.at( 4 ) == "-1#" )
        chanBits.fill( true, nChans );
    else if( toks.at( 4 ) == "-2#" ) {

        switch( js ) {
            case jsNI:  chanBits = p.ni.sns.saveBits; break;
            case jsOB:  chanBits = p.im.get_iStrOneBox( ip ).sns.saveBits; break;
            case jsIM:
            case -jsIM: chanBits = p.im.prbj[ip].sns.saveBits; break;
        }
    }
    else {

        chanBits.fill( true, nChans );

        QString err =
            Subset::cmdStr2Bits(
                chanBits, chanBits, toks.at( 4 ), nChans );

        if( !err.isEmpty() ) {
            errMsg = err;
            return;
        }
    }

// ----------
// Downsample
// ----------

    if( toks.size() >= 6 )
        dnsmp = toks.at( 5 ).toUInt();

// ---------------------------------
// Fetch whole timepoints from queue
// ---------------------------------

    vec_i16 data;
    quint64 fromCt  = toks.at( 2 ).toLongLong();
    int     nMax    = toks.at( 3 ).toInt(),
            size,
            ret;

    try {
        data.reserve( nChans * nMax );
    }
    catch( const std::exception& ) {
        errMsg = "FETCH: Low mem.";
        return;
    }

    for( int itry = 0; itry < 3; ++itry ) {

        ret = aiQ->getNSampsFromCt( data, fromCt, nMax );

        if( ret < 0 ) {
            errMsg = "FETCH: Too late.";
            return;
        }

        if( ret == 0 ) {
            errMsg = "FETCH: Low mem.";
            return;
        }

        size = data.size();

        if( size )
            break;

        quint64  endCt = aiQ->endCount();

        // Try to give client at least a small amount of data.
        // 24 = 2 imec packets of samples.
        // Bound sleeps to < 2 millisec to keep latency lower.

        if( fromCt + 24 > endCt ) {

            int gap_us = 1e6*(fromCt + 24 - endCt)/aiQ->sRate();

            QThread::usleep( qBound( 250, gap_us, 2000 ) );
        }
    }

    if( size ) {

        // ----------------
        // Requested subset
        // ----------------

        if( chanBits.count( true ) < nChans ) {

            QVector<uint>   iKeep;

            Subset::bits2Vec( iKeep, chanBits );
            Subset::subset( data, data, iKeep, nChans );
            nChans = iKeep.size();
        }

        // ----------
        // Downsample
        // ----------

        if( dnsmp > 1 )
            Subset::downsample( data, data, nChans, dnsmp );

        size = data.size();
    }

// ----
// Send
// ----

    SU.send(
        QString("BINARY_DATA %1 %2 uint64(%3)\n")
        .arg( nChans )
        .arg( size / nChans )
        .arg( fromCt ),
        true );

    SU.sendBinary( &data[0], size*sizeof(qint16) );
}


void CmdWorker::getStreamShankMap( const QStringList &toks )
{
    int         js, ip;
    ConfigCtl   *C = okStreamToks( "GETSTREAMSHANKMAP", js, ip, toks );

    if( !C )
        return;

    const ShankMap      *sm;
    const DAQ::Params   &p = C->acceptedParams;
    QVector<short>      vs;
    short               *dst;
    int                 ne;

    switch( js ) {
        case jsNI: sm = &p.ni.sns.shankMap; break;
        default:
            errMsg = "GETSTREAMSHANKMAP: Only valid for js = {0}.";
            return;
    }

    ne = sm->e.size();

    SU.send(
        QString("SHANKMAP %1 %2 %3 %4\n")
        .arg( sm->ns ).arg( sm->nc ).arg( sm->nr ).arg( ne ), true );

    vs.resize( 4*ne );
    dst = &vs[0];

    for( int ie = 0; ie < ne; ++ie ) {
        const ShankMapDesc  &E = sm->e[ie];
        *dst++ = E.s;
        *dst++ = E.c;
        *dst++ = E.r;
        *dst++ = E.u;
    }

    SU.sendBinary( &vs[0], 4*ne*sizeof(qint16) );
}


// Expected tok params:
// 0) channel string
// 1) uint32 bits
//
void CmdWorker::niDOSet( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "NIDOSET: Requires params {lines, bits}.";
        return;
    }

    MainApp *app = okAppValidated( "NIDOSET" );

    if( !app )
        return;

    errMsg = CniCfg::setDO( toks.at( 0 ), toks.at( 1 ).toUInt() );

    if( !errMsg.isEmpty() )
        errMsg = "NIDOSET: Look at Log window (debug mode) for details.";
}


// Expected tok params:
// 0) outChan string
// 1) trigTerm string
//
void CmdWorker::niWaveArm( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "NIWVARM: Requires params {outChan, trigTerm}.";
        return;
    }

    if( !okRunStarted( "NIWVARM" ) )
        return;

    errMsg = CStim::ni_wave_arm( toks[0], toks[1] );

    if( !errMsg.isEmpty() )
        errMsg = "NIWVARM: " + errMsg;
}


// Expected tok params:
// 0) outChan string
// 1) wave string
// 2) loop Boolean 0/1.
//
void CmdWorker::niWaveLoad( const QStringList &toks )
{
    if( toks.size() < 3 ) {
        errMsg = "NIWVLOAD: Requires params {outChan, wavename, loop}.";
        return;
    }

    if( !okRunStarted( "NIWVLOAD" ) )
        return;

    errMsg = CStim::ni_wave_download_file( toks[0], toks[1], toks[2].toInt() );

    if( !errMsg.isEmpty() )
        errMsg = "NIWVLOAD: " + errMsg;
}


// Expected tok params:
// 0) outChan string
// 1) start Boolean 0/1.
//
void CmdWorker::niWaveStartStop( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "NIWVSTSP: Requires params {outChan, start}.";
        return;
    }

    if( !okRunStarted( "NIWVSTSP" ) )
        return;

    errMsg = CStim::ni_wave_start_stop( toks[0], toks[1].toInt() );

    if( !errMsg.isEmpty() )
        errMsg = "NIWVSTSP: " + errMsg;
}


// Expected tok params:
// 0) ip
// 1) slot
// 2) chn_vlt string
//
void CmdWorker::obxAOSet( const QStringList &toks )
{
    if( toks.size() < 3 ) {
        errMsg = "OBXAOSET: Requires params {ip, slot, chn_vlt}.";
        return;
    }

    if( !okRunStarted( "OBXAOSET" ) )
        return;

    ConfigCtl   *C = okCfgValidated( "OBXAOSET" );

    if( !C )
        return;

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "OBXAOSET: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("OBXAOSET: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("OBXAOSET: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    errMsg = CStim::obx_set_AO( ip, toks[2] );

    if( !errMsg.isEmpty() )
        errMsg = "OBXAOSET: " + errMsg;
}


// Expected tok params:
// 0) ip
// 1) slot
// 2) trig int
// 3) loop Boolean 0/1.
//
void CmdWorker::obxWaveArm( const QStringList &toks )
{
    if( toks.size() < 4 ) {
        errMsg = "OBXWVARM: Requires params {ip, slot, trig, loop}.";
        return;
    }

    if( !okRunStarted( "OBXWVARM" ) )
        return;

    ConfigCtl   *C = okCfgValidated( "OBXWVARM" );

    if( !C )
        return;

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "OBXWVARM: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("OBXWVARM: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("OBXWVARM: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    errMsg = CStim::obx_wave_arm( ip, toks[2].toInt(), toks[3].toInt() );

    if( !errMsg.isEmpty() )
        errMsg = "OBXWVARM: " + errMsg;
}


// Expected tok params:
// 0) ip
// 1) slot
// 2) wave string
//
void CmdWorker::obxWaveLoad( const QStringList &toks )
{
    if( toks.size() < 3 ) {
        errMsg = "OBXWVLOAD: Requires params {ip, slot, wavename}.";
        return;
    }

    if( !okRunStarted( "OBXWVLOAD" ) )
        return;

    ConfigCtl   *C = okCfgValidated( "OBXWVLOAD" );

    if( !C )
        return;

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "OBXWVLOAD: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("OBXWVLOAD: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("OBXWVLOAD: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    errMsg = CStim::obx_wave_download_file( ip, toks[2] );

    if( !errMsg.isEmpty() )
        errMsg = "OBXWVLOAD: " + errMsg;
}


// Expected tok params:
// 0) ip
// 1) slot
// 2) start Boolean 0/1.
//
void CmdWorker::obxWaveStartStop( const QStringList &toks )
{
    if( toks.size() < 3 ) {
        errMsg = "OBXWVSTSP: Requires params {ip, slot, start}.";
        return;
    }

    if( !okRunStarted( "OBXWVSTSP" ) )
        return;

    ConfigCtl   *C = okCfgValidated( "OBXWVSTSP" );

    if( !C )
        return;

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "OBXWVSTSP: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("OBXWVSTSP: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("OBXWVSTSP: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    errMsg = CStim::obx_wave_start_stop( ip, toks[2].toInt() );

    if( !errMsg.isEmpty() )
        errMsg = "OBXWVSTSP: " + errMsg;
}


// Expected tok params:
// 0) ip
// 1) color
// 2) site
//
void CmdWorker::opto_emit( QStringList toks )
{
    if( toks.size() < 3 ) {
        errMsg = "OPTOEMIT: Requires params {ip, color, site}.";
        return;
    }

    Run *run = okRunStarted( "OPTOEMIT" );

    if( !run )
        return;

    QMetaObject::invokeMethod(
        run, "opto_emit",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, errMsg),
        Q_ARG(int, toks.at( 0 ).toInt()),
        Q_ARG(int, toks.at( 1 ).toInt()),
        Q_ARG(int, toks.at( 2 ).toInt()) );
}


void CmdWorker::par2Start( QStringList toks )
{
    if( toks.count() < 2 ) {
        errMsg = "PAR2: Requires at least 2 parameters.";
        return;
    }

// --------------------
// Parse command letter
// --------------------

    Par2Worker::Op  op;

    char    cmd = toks.front().trimmed().at( 0 ).toLower().toLatin1();
    toks.pop_front();

    switch( cmd ) {
        case 'c': op = Par2Worker::Create; break;
        case 'v': op = Par2Worker::Verify; break;
        case 'r': op = Par2Worker::Repair; break;
        default:
            errMsg =
                "PAR2: Operation must be"
                " one of {\"c\", \"v\", \"r\"}";
            return;
    }

// ---------------
// Parse file name
// ---------------

    QString file = toks.join( " " ).trimmed().replace( "/", "\\" );

    mainApp()->makePathAbsolute( file );

// ------------------
// Create par2 worker
// ------------------

    QThread *thread = new QThread;

    par2 = new Par2Worker( file, op );
    par2->moveToThread( thread );
    Connect( thread, SIGNAL(started()), par2, SLOT(run()) );
    Connect( par2, SIGNAL(report(QString)), this, SLOT(par2Report(QString)) );
    Connect( par2, SIGNAL(error(QString)), this, SLOT(par2Error(QString)) );
    Connect( par2, SIGNAL(finished()), par2, SLOT(deleteLater()) );
    Connect( par2, SIGNAL(destroyed()), thread, SLOT(quit()), Qt::DirectConnection );

// ----------------
// Start processing
// ----------------

    thread->start();

    while( thread->isRunning() )
        guiBreathe( false );

// -------
// Cleanup
// -------

    delete thread;
    par2 = 0;
}


// Expected tok params:
// 0) pause  Boolean 0/1.
//
void CmdWorker::pauseGraphs( QStringList toks )
{
    if( toks.size() < 1 ) {
        errMsg = "PAUSEGRF: Requires params {pause}.";
        return;
    }

    Run *run = okRunStarted( "PAUSEGRF" );

    if( run )
        run->grfRemotePause( toks.at( 0 ).toInt() );
}


// Expected tok parameter is Pinpoint data string:
// [probe-id,shank-id](startpos,endpos,R,G,B,rgnname)(startpos,endpos,R,G,B,rgnname)...()
//    - probe-id: SpikeGLX logical probe id.
//    - shank-id: [0..n-shanks].
//    - startpos: region start in microns from tip.
//    - endpos:   region end in microns from tip.
//    - R,G,B:    region color as RGB, each [0..255].
//    - rgnname:  region name text.
//
void CmdWorker::setAnatomyPP( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETANATOMYPP: Requires string parameter.";
        return;
    }

    Run *run = okRunStarted( "SETANATOMYPP" );

    if( !run ) {
        // If not running, quietly ignore message
        errMsg.clear();
        return;
    }

    QString s = toks.join( " " ).trimmed();

    QMetaObject::invokeMethod(
        run, "setAnatomyPP",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, errMsg),
        Q_ARG(QString, s) );
}


// Expected tok parameter is Boolean 0/1.
//
void CmdWorker::setAudioEnable( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETAUDIOENABLE: Requires parameter {0 or 1}.";
        return;
    }

    MainApp *app = okAppValidated( "SETAUDIOENABLE" );

    if( !app )
        return;

    bool    b = toks.front().toInt();

    QMetaObject::invokeMethod(
        app->getRun(),
        (b ? "aoStart" : "aoStop"),
        Qt::QueuedConnection );
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setAudioParams( const QString &group )
{
    MainApp *app = okAppValidated( "SETAUDIOPARAMS" );

    if( !app )
        return;

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( params.isEmpty() )
            errMsg = "SETAUDIOPARAMS: Param string is empty.";
        else {
            QMetaObject::invokeMethod(
                app->getAOCtl(),
                "cmdSrvSetsAOParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, group),
                Q_ARG(QString, params) );

            if( !errMsg.isEmpty() )
                errMsg = "SETAUDIOPARAMS: " + errMsg;
        }
    }
}


// Expected tok params:
// 0) idir
// 1) directory path
//
void CmdWorker::setDataDir( QStringList toks )
{
    if( toks.size() < 2 ) {
        errMsg = "SETDATADIR: Requires params {idir, path}.";
        return;
    }

    int i = toks.first().toInt();
    toks.pop_front();

    QString     path = toks.join( " " ).trimmed().replace( "\\", "/" );
    QFileInfo   info( path );

    if( info.isDir() && info.exists() )
        mainApp()->remoteSetsDataDir( path, i );
    else {
        errMsg =
        QString("SETDATADIR: Not a directory or does not exist '%1'.")
        .arg( path );
    }
}


// Read one param line at a time from client,
// append to KVParams,
// then set metadata en masse.
//
void CmdWorker::setMetaData()
{
    Run *run = okRunStarted( "SETMETADATA" );

    if( !run )
        return;

    if( SU.send( "READY\n", true ) ) {

        KVParams    kvp;
        QString     line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            kvp.parseOneLine( line );
        }

        if( !kvp.size() ) {
            errMsg = "SETMETADATA: Sent metadata is empty.";
            return;
        }

        QMetaObject::invokeMethod(
            run, "rgtSetMetaData",
            Qt::QueuedConnection,
            Q_ARG(KeyValMap, kvp) );
    }
}


// Expected tok parameter is Boolean 0/1.
//
void CmdWorker::setMultiDriveEnable( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETMULTIDRIVEENABLE: Requires parameter {0 or 1}.";
        return;
    }

    mainApp()->remoteSetsMultiDriveEnable( toks.front().toInt() );
}


void CmdWorker::setNextFileName( const QString &name )
{
    Run *run = okRunStarted( "SETNEXTFILENAME" );

    if( !run )
        return;

    QFileInfo   fi( name );

    if( fi.fileName().isEmpty() || fi.isRelative() ) {
        errMsg = "SETNEXTFILENAME: Requires full path and name.";
        return;
    }

    QMetaObject::invokeMethod(
        run, "dfSetNextFileName",
        Qt::QueuedConnection,
        Q_ARG(QString, name) );
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setParams()
{
    ConfigCtl   *C = okCfgValidated( "SETPARAMS" );

    if( !C )
        return;

    if( mainApp()->getRun()->isRunning() ) {
        errMsg = "SETPARAMS: Cannot set params while running.";
        return;
    }

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( params.isEmpty() )
            errMsg = "SETPARAMS: Param string is empty.";
        else {
            QMetaObject::invokeMethod(
                C, "cmdSrvSetsParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, params),
                Q_ARG(int, 0),
                Q_ARG(int, 0) );

            if( !errMsg.isEmpty() )
                errMsg = "SETPARAMS: " + errMsg;
        }
    }
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setParamsImAll()
{
    ConfigCtl   *C = okCfgValidated( "SETPARAMSIMALL" );

    if( !C )
        return;

    if( mainApp()->getRun()->isRunning() ) {
        errMsg = "SETPARAMSIMALL: Cannot set params while running.";
        return;
    }

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( params.isEmpty() )
            errMsg = "SETPARAMSIMALL: Param string is empty.";
        else {
            QMetaObject::invokeMethod(
                C, "cmdSrvSetsParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, params),
                Q_ARG(int, 1),
                Q_ARG(int, 0) );

            if( !errMsg.isEmpty() )
                errMsg = "SETPARAMSIMALL: " + errMsg;
        }
    }
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setParamsImProbe( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETPARAMSIMPRB: Requires parameter ip.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "SETPARAMSIMPRB" );
    int         ip = toks[0].toInt(),
                np;

    if( !C )
        return;

    if( mainApp()->getRun()->dfIsSaving() ) {
        errMsg = "SETPARAMSIMPRB: Cannot set probe params while writing.";
        return;
    }

    const DAQ::Params   &p = C->acceptedParams;

    np = p.stream_nIM();

    if( !np )
        errMsg = "SETPARAMSIMPRB: imec stream not enabled.";
    else if( ip < 0 || ip >= np )
        errMsg = QString("SETPARAMSIMPRB: imec stream-ip must be in range[0..%1].").arg( np - 1 );
    else if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( params.isEmpty() )
            errMsg = "SETPARAMSIMPRB: Param string is empty.";
        else {
            QMetaObject::invokeMethod(
                C, "cmdSrvSetsParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, params),
                Q_ARG(int, 2),
                Q_ARG(int, ip) );

            if( !errMsg.isEmpty() )
                errMsg = "SETPARAMSIMPRB: " + errMsg;
        }
    }
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setParamsOneBox( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "SETPARAMSOBX: Requires params {ip, slot}.";
        return;
    }

    ConfigCtl   *C = okCfgValidated( "SETPARAMSOBX" );

    if( !C )
        return;

    if( mainApp()->getRun()->isRunning() ) {
        errMsg = "SETPARAMSOBX: Cannot set params while running.";
        return;
    }

    const DAQ::Params   &p   = C->acceptedParams;
    int                 ip   = toks[0].toInt();

    if( ip >= 0 ) {
        int np = p.stream_nOB();
        if( !np ) {
            errMsg = "SETPARAMSOBX: Obx stream not enabled.";
            return;
        }
        else if( ip >= np ) {
            errMsg = QString("SETPARAMSOBX: Obx stream-ip must be in range[0..%1].").arg( np - 1 );
            return;
        }
    }
    else {
        int slot = toks[1].toInt();
        ip = p.im.obx_slot2istr( slot );
        if( ip < 0 ) {
            errMsg = QString("SETPARAMSOBX: Slot %1 is not a selected OneBox.").arg( slot );
            return;
        }
    }

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( params.isEmpty() )
            errMsg = "SETPARAMSOBX: Param string is empty.";
        else {
            QMetaObject::invokeMethod(
                C, "cmdSrvSetsParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, params),
                Q_ARG(int, 3),
                Q_ARG(int, ip) );

            if( !errMsg.isEmpty() )
                errMsg = "SETPARAMSOBX: " + errMsg;
        }
    }
}


// Expected tok parameter is Boolean 0/1.
//
void CmdWorker::setRecordingEnabled( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETRECORDENAB: Requires parameter {0 or 1}.";
        return;
    }

    Run *run = okRunStarted( "SETRECORDENAB" );

    if( !run )
        return;

    bool    b = toks.front().toInt();

// Use BlockingQueuedConnection to hold off triggers until enabled

    QMetaObject::invokeMethod(
        run,
        "dfSetRecordingEnabled",
        Qt::BlockingQueuedConnection,
        Q_ARG(bool, b),
        Q_ARG(bool, true) );
}


// Expected tok parameter is run name.
//
void CmdWorker::setRunName( const QStringList &toks )
{
    if( toks.size() < 1 ) {
        errMsg = "SETRUNNAME: Requires name parameter.";
        return;
    }

    MainApp *app = okAppValidated( "SETRUNNAME" );

    if( !app )
        return;

    QString s = toks.join( " " ).trimmed();

    QMetaObject::invokeMethod(
        app, "remoteSetsRunName",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, errMsg),
        Q_ARG(QString, s) );

    if( !errMsg.isEmpty() )
        errMsg = "SETRUNNAME: " + errMsg;
}


// Expected tok params:
// 0) Hertz
// 1) millisec
//
void CmdWorker::setTriggerOffBeep( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "SETTRIGGEROFFBEEP: Requires params {hertz, millisec}.";
        return;
    }

    Run *run = okRunStarted( "SETTRIGGEROFFBEEP" );

    if( !run )
        return;

    QMetaObject::invokeMethod(
        run, "dfSetTriggerOffBeep",
        Qt::QueuedConnection,
        Q_ARG(quint32, toks.at( 0 ).toInt()),
        Q_ARG(quint32, toks.at( 1 ).toInt()) );
}


// Expected tok params:
// 0) Hertz
// 1) millisec
//
void CmdWorker::setTriggerOnBeep( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "SETTRIGGERONBEEP: Requires params {hertz, millisec}.";
        return;
    }

    Run *run = okRunStarted( "SETTRIGGERONBEEP" );

    if( !run )
        return;

    QMetaObject::invokeMethod(
        run, "dfSetTriggerOnBeep",
        Qt::QueuedConnection,
        Q_ARG(quint32, toks.at( 0 ).toInt()),
        Q_ARG(quint32, toks.at( 1 ).toInt()) );
}


void CmdWorker::startRun()
{
    MainApp *app = okAppValidated( "STARTRUN" );

    if( !app )
        return;

    QMetaObject::invokeMethod(
        app, "remoteStartsRun",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, errMsg) );

    if( !errMsg.isEmpty() )
        errMsg = "STARTRUN: " + errMsg;
}


void CmdWorker::stopRun()
{
    QMetaObject::invokeMethod(
        mainApp(), "remoteStopsRun",
        Qt::QueuedConnection );
}


// Expected tok params:
// 0) g {-1,0,1}
// 1) t {-1,0,1}
//
void CmdWorker::triggerGT( const QStringList &toks )
{
    if( toks.size() < 2 ) {
        errMsg = "TRIGGERGT: Requires params {g, t}.";
        return;
    }

    Run *run = okRunStarted( "TRIGGERGT" );

    if( !run )
        return;

    int g = toks.at( 0 ).toInt(),
        t = toks.at( 1 ).toInt();

    if( g >= 0 ) {
        QMetaObject::invokeMethod(
            run, "rgtSetGate",
            Qt::QueuedConnection,
            Q_ARG(bool, g) );
    }

    if( g != 0 && t >= 0 ) {
        QMetaObject::invokeMethod(
            run, "rgtSetTrig",
            Qt::QueuedConnection,
            Q_ARG(bool, t) );
    }
}


void CmdWorker::verifySha1( QString file )
{
    MainApp *app = mainApp();

    app->makePathAbsolute( file );

    QFileInfo   fi( file );
    KVParams    kvp;

// At this point file, hence fi, could be either
// the binary or meta member of the pair.

    if( fi.isDir() )
        errMsg = "SHA1: Specified path is a directory.";
    else if( !fi.exists() )
        errMsg = "SHA1: Specified file not found.";
    else {

        // Here we force fi to be the meta member.

        if( fi.suffix() == "meta" )
            file.clear();
        else {
            fi.setFile(
                QString("%1/%2.meta")
                .arg( fi.path() )
                .arg( fi.completeBaseName() ) );
        }

        if( !fi.exists() )
            errMsg = "SHA1: Required metafile not found.";
        else if( !kvp.fromMetaFile( fi.filePath() ) )
            errMsg = QString("SHA1: Can't read '%1'.").arg( fi.fileName() );
        else {

            if( file.isEmpty() ) {

                file = QString("%1/%2.bin")
                        .arg( fi.path() )
                        .arg( fi.completeBaseName() );
            }

            fi.setFile( file );

            if( app->getRun()->dfIsInUse( fi ) ) {
                errMsg =
                    QString("SHA1: File in use '%1'.").arg( fi.fileName() );
                return;
            }

            Sha1Worker  *v = new Sha1Worker( file, kvp );

            Connect( v, SIGNAL(progress(int)), this, SLOT(sha1Progress(int)) );
            Connect( v, SIGNAL(result(int)), this, SLOT(sha1Result(int)) );

            v->run();
            delete v;
        }
    }
}


// Return true if cmd handled here.
//
bool CmdWorker::doQuery( const QString &cmd, const QStringList &toks )
{

#define DIRID       (toks.front().toInt())
#define RUN         (mainApp()->getRun())

// --------
// Dispatch
// --------

    QString resp;
    bool    handled = true;

    if( cmd == "GETCURRUNFILE" )
        resp = QString("%1\n").arg( RUN->dfGetCurNiName() );
    else if( cmd == "GETDATADIR" )
        resp = QString("%1\n").arg( mainApp()->dataDir( DIRID ) );
    else if( cmd == "GETGEOMMAP" )
        getGeomMap( resp, toks );
    else if( cmd == "GETIMECCHANGAINS" )
        getImecChanGains( resp, toks );
    else if( cmd == "GETLASTGT" )
        getLastGT( resp );
    else if( cmd == "GETPARAMS" )
        getParams( resp );
    else if( cmd == "GETPARAMSIMALL" )
        getParamsImAll( resp );
    else if( cmd == "GETPARAMSIMPRB" )
        getParamsImProbe( resp, toks );
    else if( cmd == "GETPARAMSOBX" )
        getParamsOneBox( resp, toks );
    else if( cmd == "GETPROBELIST" )
        getProbeList( resp );
    else if( cmd == "GETRUNNAME" )
        getRunName( resp );
    else if( cmd == "GETSTREAMACQCHANS" )
        getStreamAcqChans( resp, toks );
    else if( cmd == "GETSTREAMFILESTART" ) {
        int js, ip;
        if( okStreamToks( cmd, js, ip, toks ) )
            resp = QString("%1\n").arg( RUN->dfGetFileStart( qAbs(js), ip ) );
    }
    else if( cmd == "GETSTREAMI16TOVOLTS" )
        getStreamI16ToVolts( resp, toks );
    else if( cmd == "GETSTREAMMAXINT" )
        getStreamMaxInt( resp, toks );
    else if( cmd == "GETSTREAMNP" )
        getStreamNP( resp, toks );
    else if( cmd == "GETSTREAMSAMPLECOUNT" ) {
        int js, ip;
        if( okStreamToks( cmd, js, ip, toks ) )
            resp = QString("%1\n").arg( RUN->getSampleCount( qAbs(js), ip ) );
    }
    else if( cmd == "GETSTREAMSAMPLERATE" )
        getStreamSampleRate( resp, toks );
    else if( cmd == "GETSTREAMSAVECHANS" )
        getStreamSaveChans( resp, toks );
    else if( cmd == "GETSTREAMSN" )
        getStreamSN( resp, toks );
    else if( cmd == "GETSTREAMVOLTAGERANGE" )
        getStreamVoltageRange( resp, toks );
    else if( cmd == "GETTIME" )
        resp = QString("%1\n").arg( getTime(), 0, 'f', 3 );
    else if( cmd == "GETVERSION" )
        resp = QString("%1\n").arg( VERS_SGLX_STR );
    else if( cmd == "ISCONSOLEHIDDEN" )
        isConsoleHidden( resp );
    else if( cmd == "ISINITIALIZED" )
        resp = QString("%1\n").arg( mainApp()->isInitialized() );
    else if( cmd == "ISRUNNING" )
        resp = QString("%1\n").arg( RUN->isRunning() );
    else if( cmd == "ISSAVING" )
        resp = QString("%1\n").arg( RUN->dfIsSaving() );
    else if( cmd == "ISUSRORDER" ) {
        int js, ip;
        if( okStreamToks( cmd, js, ip, toks ) )
            resp = QString("%1\n").arg( RUN->grfIsUsrOrder( qAbs(js), ip ) );
    }
    else if( cmd == "MAPSAMPLE" )
        mapSample( resp, toks );
    else if( cmd == "OPTOGETATTENS" )
        opto_getAttens( resp, toks );
    else
        handled = false;

// -----
// Reply
// -----

    if( handled ) {

        if( resp.isNull() ) {

            if( errMsg.isEmpty() )
                errMsg = "CmdWorker: Application response is empty.";
        }
        else
            SU.send( resp, true );
    }

    return handled;
}


// Return true if cmd handled here.
//
bool CmdWorker::doCommand( const QString &cmd, const QStringList &toks )
{

#define DIRID       (toks.front().toInt())

// --------
// Dispatch
// --------

    bool    handled = true;

    if( cmd == "NOOP" ) {
        // do nothing, will just send OK in caller
    }
    else if( cmd == "CONSOLEHIDE" )
        consoleShow( false );
    else if( cmd == "CONSOLESHOW" )
        consoleShow( true );
    else if( cmd == "ENUMDATADIR" )
        enumDir( mainApp()->dataDir( DIRID ) );
    else if( cmd == "FETCH" )
        fetch( toks );
    else if( cmd == "GETSTREAMSHANKMAP" )
        getStreamShankMap( toks );
    else if( cmd == "NIDOSET" )
        niDOSet( toks );
    else if( cmd == "NIWVARM" )
        niWaveArm( toks );
    else if( cmd == "NIWVLOAD" )
        niWaveLoad( toks );
    else if( cmd == "NIWVSTSP" )
        niWaveStartStop( toks );
    else if( cmd == "OBXAOSET" )
        obxAOSet( toks );
    else if( cmd == "OBXWVARM" )
        obxWaveArm( toks );
    else if( cmd == "OBXWVLOAD" )
        obxWaveLoad( toks );
    else if( cmd == "OBXWVSTSP" )
        obxWaveStartStop( toks );
    else if( cmd == "OPTOEMIT" )
        opto_emit( toks );
    else if( cmd == "PAR2" )
        par2Start( toks );
    else if( cmd == "PAUSEGRF" )
        pauseGraphs( toks );
    else if( cmd == "SETANATOMYPP" )
        setAnatomyPP( toks );
    else if( cmd == "SETAUDIOENABLE" )
        setAudioEnable( toks );
    else if( cmd == "SETAUDIOPARAMS" )
        setAudioParams( toks.front().trimmed() );
    else if( cmd == "SETDATADIR" )
        setDataDir( toks );
    else if( cmd == "SETMETADATA" )
        setMetaData();
    else if( cmd == "SETMULTIDRIVEENABLE" )
        setMultiDriveEnable( toks );
    else if( cmd == "SETNEXTFILENAME" )
        setNextFileName( toks.join( " " ).trimmed().replace( "\\", "/" ) );
    else if( cmd == "SETPARAMS" )
        setParams();
    else if( cmd == "SETPARAMSIMALL" )
        setParamsImAll();
    else if( cmd == "SETPARAMSIMPRB" )
        setParamsImProbe( toks );
    else if( cmd == "SETPARAMSOBX" )
        setParamsOneBox( toks );
    else if( cmd == "SETRECORDENAB" )
        setRecordingEnabled( toks );
    else if( cmd == "SETRUNNAME" )
        setRunName( toks );
    else if( cmd == "SETTRIGGEROFFBEEP" )
        setTriggerOffBeep( toks );
    else if( cmd == "SETTRIGGERONBEEP" )
        setTriggerOnBeep( toks );
    else if( cmd == "STARTRUN" )
        startRun();
    else if( cmd == "STOPRUN" )
        stopRun();
    else if( cmd == "TRIGGERGT" )
        triggerGT( toks );
    else if( cmd == "VERIFYSHA1" )
        verifySha1( toks.join( " " ).trimmed().replace( "\\", "/" ) );
    else if( cmd == "BYE"
            || cmd == "QUIT"
            || cmd == "EXIT"
            || cmd == "CLOSE" ) {

        Debug() << "Bye " << SU.tag() << SU.addr() << " [Closed by client.]";
        sock->close();
    }
    else
        handled = false;

    return handled;
}


// Some protocol notes:
// --------------------
// Both commands and queries have an errMsg (or evtError)
// box to report problems. Use SocketUtil::appendError()
// to concatenate errors in the order of occurrence. The
// individual error messages should generally not terminate
// with "\n", as that will be applied by CmdWorker::sendError().
//
// If errMsg should become non-empty, the operation has failed.
// On failure this function returns false; no query response is
// returned to the caller; only the errMsg is returned.
//
// No response (resp) is generated or returned for a "command".
//
// For a query, a response is required, and if it is ultimately
// null, an error to that effect is issued. String responses are
// expected to be "\n" terminated, and it is each query handler's
// responsibility to make it so.
//
bool CmdWorker::processLine( const QString &line )
{
// ------------
// Init outputs
// ------------

    errMsg.clear();

// -------------
// Parse command
// -------------

    QStringList toks = line.split(
                        QRegularExpression("\\s+"),
                        Qt::SkipEmptyParts );

    if( toks.empty() ) {
        errMsg = "CmdWorker: Missing command token.";
        return false;
    }

    QString cmd = toks.front().toUpper();
    toks.pop_front();

// --------
// Dispatch
// --------

    if( !doQuery( cmd, toks ) && !doCommand( cmd, toks ) )
        errMsg = QString("CmdWorker: Unknown command [%1].").arg( cmd );

    if( !errMsg.isEmpty() )
        Warning() << errMsg;

    return errMsg.isEmpty();
}


