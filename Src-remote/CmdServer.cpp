
#include "CmdServer.h"
#include "Util.h"
#include "MainApp.h"
#include "Version.h"
#include "ConfigCtl.h"
#include "AOCtl.h"
#include "AIQ.h"
#include "Run.h"
#include "Subset.h"
#include "Sha1Verifier.h"
#include "Par2Window.h"

#include <QDir>
#include <QDirIterator>
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
        haddr = iface;

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
        errMsg = "SHA1: Sum does not match sum in meta file.";
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


bool CmdWorker::okStreamID( const QString &cmd, int ip )
{
    const ConfigCtl *C = mainApp()->cfgCtl();

    if( !C->validated ) {
        errMsg =
        QString("%1: Run parameters never validated.").arg( cmd );
        return false;
    }

    const DAQ::Params   &p = C->acceptedParams;

    if( ip >= 0 ) {

        int np = p.im.get_nProbes();

        if( np <= 0 ) {
            errMsg =
            QString("%1: No IM streams enabled/configured.").arg( cmd );
            return false;
        }

        if( ip >= np ) {
            errMsg =
            QString("%1: IM streamID must be in range [0..%2].")
            .arg( cmd ).arg( np - 1 );
            return false;
        }
    }
    else if( !p.ni.enabled ) {
        errMsg =
        QString("%1: NI stream is not enabled/configured.").arg( cmd );
        return false;
    }

    return true;
}


void CmdWorker::getParams( QString &resp )
{
    ConfigCtl   *C = mainApp()->cfgCtl();

    if( !C->validated ) {
        errMsg =
        QString("GETPARAMS: Run parameters never validated.");
        return;
    }

    QMetaObject::invokeMethod(
        C, "cmdSrvGetsParamStr",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, resp) );
}


void CmdWorker::getImProbeSN( QString &resp, int ip )
{
    if( !okStreamID( "GETIMPROBESN", ip ) )
        return;

    const CimCfg::ImProbeDat    &P =
            mainApp()->cfgCtl()->prbTab.get_iProbe( ip );

    resp = QString("%1 %2\n").arg( P.sn ).arg( P.type );
}


void CmdWorker::getSampleRate( QString &resp, int ip )
{
    if( !okStreamID( "GETSAMPLERATE", ip ) )
        return;

    const DAQ::Params   &p = mainApp()->cfgCtl()->acceptedParams;

    resp = QString("%1\n")
            .arg( ip >= 0 ? p.im.each[ip].srate : p.ni.srate, 0, 'f', 6 );
}


void CmdWorker::getAcqChanCounts( QString &resp, int ip )
{
    if( !okStreamID( "GETACQCHANCOUNTS", ip ) )
        return;

    const DAQ::Params   &p = mainApp()->cfgCtl()->acceptedParams;

    if( ip >= 0 ) {

        const int   *type = p.im.each[ip].imCumTypCnt;

        int AP, LF, SY;

        AP = type[CimCfg::imTypeAP];
        LF = type[CimCfg::imTypeLF] - type[CimCfg::imTypeAP];
        SY = type[CimCfg::imTypeSY] - type[CimCfg::imTypeLF];

        resp = QString("%1 %2 %3\n")
                .arg( AP ).arg( LF ).arg( SY );
    }
    else {

        const int   *type = p.ni.niCumTypCnt;

        int MN, MA, XA, XD;

        MN = type[CniCfg::niTypeMN];
        MA = type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN];
        XA = type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA];
        XD = type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA];

        resp = QString("%1 %2 %3 %4\n")
                .arg( MN ).arg( MA ).arg( XA ).arg( XD );
    }
}


void CmdWorker::getSaveChans( QString &resp, int ip )
{
    if( !okStreamID( "GETSAVECHANS", ip ) )
        return;

    ConfigCtl   *C = mainApp()->cfgCtl();

    if( ip >= 0 ) {
        QMetaObject::invokeMethod(
            C, "cmdSrvGetsSaveChansIm",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QString, resp),
            Q_ARG(uint, ip) );
    }
    else {
        QMetaObject::invokeMethod(
            C, "cmdSrvGetsSaveChansNi",
            Qt::BlockingQueuedConnection,
            Q_RETURN_ARG(QString, resp) );
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


void CmdWorker::setDataDir( const QString &path )
{
    QFileInfo   info( path );

    if( info.isDir() && info.exists() ) {
        mainApp()->remoteSetsDataDir( path );
        Log() << "Remote client set data dir: " << path;
    }
    else {
        errMsg =
            QString("SETDATADIR: Not a directory or does not exist '%1'.")
            .arg( path );
    }
}


bool CmdWorker::enumDir( const QString &path )
{
    QDir    dir( path );

    if( !dir.exists() ) {
        errMsg = "ENUMDATADIR: Directory not found: " + path;
        return false;
    }
    else {

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
    }

    return true;
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::setParams()
{
    ConfigCtl   *C = mainApp()->cfgCtl();

    if( !C->validated ) {
        errMsg = "SETPARAMS: Run parameters never validated.";
        return;
    }

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( !params.isEmpty() ) {

            QMetaObject::invokeMethod(
                C, "cmdSrvSetsParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, params) );
        }
        else
            errMsg = QString("SETPARAMS: Param string is empty.");
    }
}


// Read one param line at a time from client,
// append to str,
// then set params en masse.
//
void CmdWorker::SetAudioParams( const QString &group )
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg = "SETAUDIOPARAMS: Run parameters never validated.";
        return;
    }

    if( SU.send( "READY\n", true ) ) {

        QString params, line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            params += QString("%1\n").arg( line );
        }

        if( !params.isEmpty() ) {

            QMetaObject::invokeMethod(
                app->getAOCtl(),
                "cmdSrvSetsAOParamStr",
                Qt::BlockingQueuedConnection,
                Q_RETURN_ARG(QString, errMsg),
                Q_ARG(QString, group),
                Q_ARG(QString, params) );
        }
        else
            errMsg = QString("SETAUDIOPARAMS: Param string is empty.");
    }
}


// Expected tok parameter is Boolean 0/1.
//
void CmdWorker::setAudioEnable( const QStringList &toks )
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg = "SETAUDIOENABLE: Run parameters never validated.";
        return;
    }

    if( toks.size() > 0 ) {

        bool    b = toks.front().toInt();

        QMetaObject::invokeMethod(
            app->getRun(),
            (b ? "aoStart" : "aoStop"),
            Qt::QueuedConnection );
    }
    else
        errMsg = "SETAUDIOENABLE: Requires parameter {0 or 1}.";
}


// Expected tok parameter is Boolean 0/1.
//
void CmdWorker::setRecordingEnabled( const QStringList &toks )
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg = "SETRECORDENAB: Run parameters never validated.";
        return;
    }

    if( toks.size() > 0 ) {

        bool    b = toks.front().toInt();

        QMetaObject::invokeMethod(
            app->getRun(),
            "dfSetRecordingEnabled",
            Qt::QueuedConnection,
            Q_ARG(bool, b),
            Q_ARG(bool, true) );
    }
    else
        errMsg = "SETRECORDENAB: Requires parameter {0 or 1}.";
}


// Expected tok parameter is run name.
//
void CmdWorker::setRunName( const QStringList &toks )
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg = "SETRUNNAME: Run parameters never validated.";
        return;
    }

    if( toks.size() > 0 ) {

        QString s = toks.join(" ").trimmed();

        QMetaObject::invokeMethod(
            app, "remoteSetsRunName",
            Qt::QueuedConnection,
            Q_ARG(QString, s) );
    }
    else
        errMsg = "SETRUNNAME: Requires name parameter.";
}


// Read one param line at a time from client,
// append to KVParams,
// then set meta data en masse.
//
void CmdWorker::setMetaData()
{
    Run *run = mainApp()->getRun();

    if( !run->isRunning() ) {
        errMsg = "SETMETADATA: Run not yet started.";
        return;
    }

    if( SU.send( "READY\n", true ) ) {

        KVParams    kvp;
        QString     line;

        while( !(line = SU.readLine()).isNull() ) {

            if( !line.length() )
                break; // done on blank line

            kvp.parseOneLine( line );
        }

        if( kvp.size() ) {

            QMetaObject::invokeMethod(
                run, "rgtSetMetaData",
                Qt::QueuedConnection,
                Q_ARG(KeyValMap, kvp) );
        }
        else
            errMsg = QString("SETMETADATA: Meta data empty.");
    }
}


void CmdWorker::startRun()
{
    MainApp *app = mainApp();

    if( !app->cfgCtl()->validated ) {
        errMsg =
        QString("STARTRUN: Run parameters never validated.");
        return;
    }

    QMetaObject::invokeMethod(
        app, "remoteStartsRun",
        Qt::BlockingQueuedConnection,
        Q_RETURN_ARG(QString, errMsg) );
}


void CmdWorker::stopRun()
{
    QMetaObject::invokeMethod(
        mainApp(), "remoteStopsRun",
        Qt::QueuedConnection );
}


// Expected tok params:
// 0) Boolean 0/1
// 1) channel string
//
void CmdWorker::setDigOut( const QStringList &toks )
{
    if( toks.size() >= 2 ) {

        QString devRem;
        int     lineRem;
        errMsg = CniCfg::parseDIStr( devRem, lineRem, toks.at( 1 ) );

        if( !errMsg.isEmpty() ) {
            Error() << "SETDIGOUT: " + errMsg;
            return;
        }


        MainApp         *app    = mainApp();
        const ConfigCtl *C      = app->cfgCtl();

        if( !C->validated ) {
            Error() <<
            (errMsg = "SETDIGOUT: Run parameters never validated.");
            return;
        }

        const DAQ::Params  &p = C->acceptedParams;

        if( p.ni.startEnable ) {

            QString devStart;
            int     lineStart;
            CniCfg::parseDIStr( devStart, lineStart, p.ni.startLine );

            if( !devStart.compare( devRem, Qt::CaseInsensitive ) ) {
                Error() <<
                (errMsg =
                "SETDIGOUT: Cannot use start line for digital output.");
                return;
            }
        }

        QMetaObject::invokeMethod(
            app, "remoteSetsDigitalOut",
            Qt::QueuedConnection,
            Q_ARG(QString, toks.at( 1 )),
            Q_ARG(bool, toks.at( 0 ).toInt()) );
    }
    else
        Error() << (errMsg = "SETDIGOUT: Requires at least 2 params.");
}


// Expected tok params:
// 0) streamID
// 1) starting scan index
// 2) scan count
// 3) <channel subset pattern "id1#id2#...">
// 4) <integer downsample factor>
//
// Send( 'BINARY_DATA %d %d uint64(%ld)'\n", nChans, nScans, headCt ).
// Write binary data stream.
//
void CmdWorker::fetch( const QStringList &toks )
{
    if( toks.size() >= 3 ) {

        MainApp             *app    = mainApp();
        const DAQ::Params   &p      = app->cfgCtl()->acceptedParams;

        int ip = toks.at( 0 ).toInt();

        if( ip >= 0 ) {

            int np = p.im.get_nProbes();

            if( ip >= np ) {
                errMsg =
                QString("FETCH: StreamID must be in range [-1..%1].")
                .arg( np - 1 );
                return;
            }
        }

        const AIQ*  aiQ =
                (ip >= 0 ?
                app->getRun()->getImQ( ip ) :
                app->getRun()->getNiQ());

        if( !aiQ )
            Warning() << (errMsg = "FETCH: Not running.");
        else {

            const QBitArray &allBits =
                    (ip >= 0 ?
                    p.im.each[ip].sns.saveBits :
                    p.ni.sns.saveBits);

            QBitArray   chanBits;
            int         nChans  = aiQ->nChans();
            uint        dnsmp   = 1;

            // -----
            // Chans
            // -----

            if( toks.size() >= 4 ) {

                QString err =
                    Subset::cmdStr2Bits(
                        chanBits, allBits, toks.at( 3 ), nChans );

                if( !err.isEmpty() ) {
                    errMsg = err;
                    Warning() << err;
                    return;
                }
            }
            else
                chanBits = allBits;

            // ----------
            // Downsample
            // ----------

            if( toks.size() >= 5 )
                dnsmp = toks.at( 4 ).toUInt();

            // ---------------------------------
            // Fetch whole timepoints from queue
            // ---------------------------------

            vec_i16 data;
            quint64 fromCt  = toks.at( 1 ).toLongLong();
            int     nMax    = toks.at( 2 ).toInt(),
                    size,
                    ret;

            try {
                data.reserve( nChans * nMax );
            }
            catch( const std::exception& ) {
                Warning() << (errMsg = "FETCH: Low mem.");
                return;
            }

            ret = aiQ->getNScansFromCt( data, fromCt, nMax );

            if( ret < 0 ) {
                Warning() << (errMsg = "FETCH: Too late.");
                return;
            }

            if( ret == 0 ) {
                Warning() << (errMsg = "FETCH: Low mem.");
                return;
            }

            size = data.size();

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

                // ----
                // Send
                // ----

                size = data.size();

                SU.send(
                    QString("BINARY_DATA %1 %2 uint64(%3)\n")
                    .arg( nChans )
                    .arg( size / nChans )
                    .arg( fromCt ),
                    true );

                SU.sendBinary( &data[0], size*sizeof(qint16) );
            }
            else
                Warning() << (errMsg = "FETCH: No data read from queue.");
        }
    }
    else
        Warning() << (errMsg = "FETCH: Requires at least 3 params.");
}


void CmdWorker::consoleShow( bool show )
{
    QMetaObject::invokeMethod(
        mainApp(), "remoteShowsConsole",
        Qt::QueuedConnection,
        Q_ARG(bool, show) );
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
            errMsg = "SHA1: Required .meta file not found.";
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


void CmdWorker::par2Start( QStringList toks )
{
    if( toks.count() >= 2 ) {

        // --------------------
        // Parse command letter
        // --------------------

        Par2Worker::Op  op;

        char cmd = toks.front().trimmed().at( 0 ).toLower().toLatin1();
        toks.pop_front();

        switch( cmd ) {

            case 'c':
                op = Par2Worker::Create;
                break;
            case 'v':
                op = Par2Worker::Verify;
                break;
            case 'r':
                op = Par2Worker::Repair;
                break;
            default:
                errMsg =
                    "PAR2: Operation must be"
                    " one of {\"c\", \"v\", \"r\"}";
                return;
        }

        // ---------------
        // Parse file name
        // ---------------

        QString file = toks.join(" ");

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
            guiBreathe();

        // -------
        // Cleanup
        // -------

        delete thread;
        par2 = 0;
    }
    else
        errMsg = "PAR2: Requires at least 2 arguments.";
}


// Return true if cmd handled here.
//
bool CmdWorker::doQuery( const QString &cmd, const QStringList &toks )
{

#define RUN         (mainApp()->getRun())
#define STREAMID    (toks.front().toInt())

// --------
// Dispatch
// --------

    QString resp;
    bool    handled = true;

    if( cmd == "GETVERSION" )
        resp = QString("%1\n").arg( VERSION_STR );
    else if( cmd == "ISINITIALIZED" )
        resp = QString("%1\n").arg( mainApp()->isInitialized() );
    else if( cmd == "GETTIME" )
        resp = QString("%1\n").arg( getTime(), 0, 'f', 3 );
    else if( cmd == "GETDATADIR" )
        resp = QString("%1\n").arg( mainApp()->dataDir() );
    else if( cmd == "GETPARAMS" )
        getParams( resp );
    else if( cmd == "GETIMPROBESN" )
        getImProbeSN( resp, STREAMID );
    else if( cmd == "GETSAMPLERATE" )
        getSampleRate( resp, STREAMID );
    else if( cmd == "GETACQCHANCOUNTS" )
        getAcqChanCounts( resp, STREAMID );
    else if( cmd == "GETSAVECHANS" )
        getSaveChans( resp, STREAMID );
    else if( cmd == "ISRUNNING" )
        resp = QString("%1\n").arg( RUN->isRunning() );
    else if( cmd == "ISSAVING" )
        resp = QString("%1\n").arg( RUN->dfIsSaving() );
    else if( cmd == "ISUSRORDER" ) {

        int ip = STREAMID;

        if( okStreamID( "ISUSRORDER", ip ) )
            resp = QString("%1\n").arg( RUN->grfIsUsrOrder( ip ) );
    }
    else if( cmd == "GETCURRUNFILE" )
        resp = QString("%1\n").arg( RUN->dfGetCurNiName() );
    else if( cmd == "GETFILESTART" ) {

        int ip = STREAMID;

        if( okStreamID( "GETFILESTART", ip ) )
            resp = QString("%1\n").arg( RUN->dfGetFileStart( ip ) );
    }
    else if( cmd == "GETSCANCOUNT" ) {

        int ip = STREAMID;

        if( okStreamID( "GETSCANCOUNT", ip ) )
            resp = QString("%1\n").arg( RUN->getScanCount( ip ) );
    }
    else if( cmd == "ISCONSOLEHIDDEN" )
        isConsoleHidden( resp );
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
// --------
// Dispatch
// --------

    bool    handled = true;

    if( cmd == "NOOP" ) {
        // do nothing, will just send OK in caller
    }
    else if( cmd == "SETDATADIR" )
        setDataDir( toks.join( " " ).trimmed() );
    else if( cmd == "ENUMDATADIR" )
        enumDir( mainApp()->dataDir() );
    else if( cmd == "SETPARAMS" )
        setParams();
    else if( cmd == "SETAUDIOPARAMS" )
        SetAudioParams( toks.front().trimmed() );
    else if( cmd == "SETAUDIOENABLE" )
        setAudioEnable( toks );
    else if( cmd == "SETRECORDENAB" )
        setRecordingEnabled( toks );
    else if( cmd == "SETRUNNAME" )
        setRunName( toks );
    else if( cmd == "SETMETADATA" )
        setMetaData();
    else if( cmd == "STARTRUN" )
        startRun();
    else if( cmd == "STOPRUN" )
        stopRun();
    else if( cmd == "SETDIGOUT" )
        setDigOut( toks );
    else if( cmd == "FETCH" )
        fetch( toks );
    else if( cmd == "CONSOLEHIDE" )
        consoleShow( false );
    else if( cmd == "CONSOLESHOW" )
        consoleShow( true );
    else if( cmd == "VERIFYSHA1" )
        verifySha1( toks.join( " " ).trimmed() );
    else if( cmd == "PAR2" )
        par2Start( toks );
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
                        QRegExp("\\s+"),
                        QString::SkipEmptyParts );

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

    return errMsg.isEmpty();
}


