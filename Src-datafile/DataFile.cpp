
#include "DataFile.h"
#include "DataFile_Helpers.h"
#include "Util.h"
#include "MainApp.h"
#include "Subset.h"
#include "Version.h"

#include <QDateTime>


/* ---------------------------------------------------------------- */
/* DataFile ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DataFile::DataFile()
    :   scanCt(0), mode(Undefined), trgChan(-1),
        dfw(0), wrAsync(true), sRate(0), nSavedChans(0)
{
}


DataFile::~DataFile()
{
    if( dfw ) {
        delete dfw;
        dfw = 0;
    }
}

/* ---------------------------------------------------------------- */
/* isValidInputFile ----------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::isValidInputFile(
    const QString   &filename,
    QString         *error )
{
    if( error )
        error->clear();

    QString     bFile = filename,
                mFile;
    QFileInfo   fi( bFile );

// -------------------
// Binary file exists?
// -------------------

    if( fi.suffix() == "meta" ) {

        bFile = QString("%1/%2.bin")
                    .arg( fi.canonicalPath() )
                    .arg( fi.completeBaseName() );

        fi.setFile( bFile );
    }

    if( !fi.exists() ) {

        if( error ) {
            *error = QString("Binary file [%1] does not exist.")
                        .arg( fi.fileName() );
        }

        return false;
    }

// ------------------
// Binary file empty?
// ------------------

    qint64  binSize = fileSize( fi, error );

    if( binSize <= 0 )
        return false;

// -----------------
// Meta file exists?
// -----------------

    mFile = forceMetaSuffix( bFile );

    fi.setFile( mFile );

    if( !fi.exists() ) {

        if( error ) {
            *error = QString("Meta file [%1] does not exist.")
                        .arg( fi.fileName() );
        }

        return false;
    }

// -------------------
// Meta file readable?
// -------------------

    if( !isFileReadable( fi, error ) )
        return false;

// -------------------
// Meta content valid?
// -------------------

    KVParams    kvp;

    if( !kvp.fromMetaFile( mFile ) ) {

        if( error ) {
            *error = QString("Meta file [%1] is corrupt.")
                        .arg( fi.fileName() );
        }

        return false;
    }

    // version check

    QString vFile = kvp["version"].toString(),
            vReq  = "20160120";

    if( vFile.compare( vReq ) < 0 ) {

        if( error ) {
            *error =
                QString("File version >= %1 required. This file is %2.")
                .arg( vReq )
                .arg( vFile.isEmpty() ? "unversioned" : vFile );
        }

        return false;
    }

    // finalization keys

    if( !kvp.contains( "fileSHA1" )
        || !kvp.contains( "fileTimeSecs" )
        || !kvp.contains( "fileSizeBytes" ) ) {

        if( error )
            *error = QString("Meta file [%1] missing required key.")
                        .arg( fi.fileName() );

        return false;
    }

    if( kvp["fileSizeBytes"].toLongLong() != binSize ) {

        if( error )
            *error = QString("Recorded/actual file-size mismatch [%1].")
                        .arg( fi.fileName() );

        return false;
    }

// --
// OK
// --

    return true;
}

/* ---------------------------------------------------------------- */
/* openForRead ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::openForRead( const QString &filename )
{
// ----
// Init
// ----

    closeAndFinalize();

// ------
// Valid?
// ------

    QString bFile = forceBinSuffix( filename ),
            error;

    if( !isValidInputFile( bFile, &error ) ) {
        Error() << "openForRead error: " << error;
        return false;
    }

    // Now know files exist, are openable,
    // and meta data passes minor checking.

// ----------
// Open files
// ----------

    binFile.setFileName( bFile );
    binFile.open( QIODevice::ReadOnly );

// ---------
// Load meta
// ---------

    kvp.fromMetaFile( metaName = forceMetaSuffix( filename ) );

// ----------
// Parse meta
// ----------

    subclassParseMetaData();

    scanCt = kvp["fileSizeBytes"].toLongLong()
                / (sizeof(qint16) * nSavedChans);

// -----------
// Channel ids
// -----------

    chanIds.clear();

// Load subset string

    KVParams::const_iterator    it;
    bool                        subsetOK = false;

    if( (it = kvp.find( "snsSaveChanSubset" )) != kvp.end()
        && !Subset::isAllChansStr( it->toString() ) ) {

        subsetOK = Subset::rngStr2Vec( chanIds, it->toString() );
    }

// Otherwise assume using all

    if( !subsetOK )
        Subset::defaultVec( chanIds, nSavedChans );

// Trigger Channel

    int trgMode = DAQ::stringToTrigMode( kvp["trigMode"].toString() );

    trgChan = -1;

    if( trgMode == DAQ::eTrigTTL )
        trgChan = kvp["trgTTLAIChan"].toInt();
    else if( trgMode == DAQ::eTrigSpike )
        trgChan = kvp["trgSpikeAIChan"].toInt();

    if( trgChan != -1 && !chanIds.contains( trgChan ) )
        trgChan = -1;

// --------------------
// Parse bad data table
// --------------------

    badData.clear();

    if( (it = kvp.find( "badData" )) != kvp.end() ) {

        QStringList bdl = it->toString()
                            .split( "; ", QString::SkipEmptyParts );

        foreach( const QString &bad, bdl ) {

            QStringList e = bad.split( ",", QString::SkipEmptyParts );

            if( e.count() == 2 ) {

                quint64 scan, ct;
                bool    ok1, ok2;

                scan    = e.at( 0 ).toULongLong( &ok1 );
                ct      = e.at( 1 ).toULongLong( &ok2 );

                if( ok1 && ok2 )
                    pushBadData( scan, ct );
            }
        }
    }

// ----
// Done
// ----

    Debug()
        << "Opened ["
        << QFileInfo( bFile ).fileName() << "] "
        << nSavedChans << " chans @"
        << sRate  << " Hz, "
        << scanCt << " scans total.";

    mode = Input;

    return true;
}

/* ---------------------------------------------------------------- */
/* openForWrite --------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::openForWrite( const DAQ::Params &p, const QString &binName )
{
// ------------
// Capture time
// ------------

    QDateTime   tCreate( QDateTime::currentDateTime() );

// ----
// Init
// ----

    closeAndFinalize();

// --------------
// Channel count?
// --------------

    int nSaved = subclassGetSavChanCount( p );

    if( !nSaved ) {

        Error()
            << "openForWrite error: Zero channel count for file ["
            << QFileInfo( binName ).completeBaseName()
            << "].";
        return false;
    }

// ----
// Open
// ----

    QString bName = binName;

    mainApp()->makePathAbsolute( bName );

    metaName = forceMetaSuffix( bName );

    Debug() << "Outdir : " << mainApp()->runDir();
    Debug() << "Outfile: " << bName;

    binFile.setFileName( bName );

    if( !binFile.open( QIODevice::WriteOnly ) ) {

        Error() << "openForWrite error: Can't open [" << bName << "]";
        return false;
    }

// ---------
// Meta data
// ---------

// To check completeness, here is full list of daq.ini settings.
// No other ini file contains experiment parameters:
//
//    niAiRangeMin=-5
//    niAiRangeMax=5
//    niSampRate=19737
//    niMNGain=200
//    niMAGain=1
//    niDev1=Dev6
//    niDev2=Dev6
//    niClock1=PFI2
//    niClock2=PFI2
//    niMNChans1=0:5
//    niMAChans1=6,7
//    niXAChans1=
//    niXDChans1=
//    niMNChans2=0:5
//    niMAChans2=6,7
//    niXAChans2=
//    niXDChans2=
//    niMuxFactor=32
//    niAiTermConfig=-1             // aiTermination string
//    niDualDevMode=false
//    niSyncEnable=true
//    niSyncLine=Dev6/port0/line0
//    trgTimTL0=10
//    trgTimTH=10
//    trgTimTL=1
//    trgTimNH=3
//    trgTimIsHInf=false
//    trgTimIsNInf=false
//    trgTTLMarginS=0.01
//    trgTTLRefractS=0.5
//    trgTTLTH=0.5
//    trgTTLMode=0
//    trgTTLAIChan=4
//    trgTTLInarow=5
//    trgTTLNH=1
//    trgTTLThresh=15232
//    trgTTLIsNInf=true
//    trgSpikePeriEvtS=1
//    trgSpikeRefractS=0.5
//    trgSpikeAIChan=4
//    trgSpikeInarow=5
//    trgSpikeNS=1
//    trgSpikeThresh=15232
//    trgSpikeIsNInf=true
//    gateMode=0
//    trigMode=0
//    snsImChanMapFile=         // snsChanMap string
//    snsNiChanMapFile=         // snsChanMap string
//    snsImSaveChanSubset=all
//    snsNiSaveChanSubset=all
//    snsRunName=myRun          // outputFile baseName

    nSavedChans = nSaved;

    subclassStoreMetaData( p );

    if( p.im.enabled && p.ni.enabled )
        kvp["typeEnabled"] = "imec,nidq";
    else if( p.im.enabled )
        kvp["typeEnabled"] = "imec";
    else
        kvp["typeEnabled"] = "nidq";

    kvp["nSavedChans"]      = nSavedChans;
    kvp["gateMode"]         = DAQ::gateModeToString( p.mode.mGate );
    kvp["trigMode"]         = DAQ::trigModeToString( p.mode.mTrig );
    kvp["fileName"]         = bName;
    kvp["fileCreateTime"]   = tCreate.toString( Qt::ISODate );

    if( p.mode.mGate == DAQ::eGateImmed ) {
    }

    if( p.mode.mTrig == DAQ::eTrigImmed ) {
    }
    else if( p.mode.mTrig == DAQ::eTrigTimed ) {

        kvp["trgTimTL0"]    = p.trgTim.tL0;
        kvp["trgTimTH"]     = p.trgTim.tH;
        kvp["trgTimTL"]     = p.trgTim.tL;
        kvp["trgTimNH"]     = p.trgTim.nH;
        kvp["trgTimIsHInf"] = p.trgTim.isHInf;
        kvp["trgTimIsNInf"] = p.trgTim.isNInf;
    }
    else if( p.mode.mTrig == DAQ::eTrigTTL ) {

        kvp["trgTTLMarginS"]    = p.trgTTL.marginSecs;
        kvp["trgTTLRefractS"]   = p.trgTTL.refractSecs;
        kvp["trgTTLTH"]         = p.trgTTL.tH;
        kvp["trgTTLMode"]       = p.trgTTL.mode;
        kvp["trgTTLAIChan"]     = p.trgTTL.aiChan;
        kvp["trgTTLInarow"]     = p.trgTTL.inarow;
        kvp["trgTTLNH"]         = p.trgTTL.nH;
        kvp["trgTTLThresh"]     = p.trgTTL.T;
        kvp["trgTTLIsNInf"]     = p.trgTTL.isNInf;
    }
    else if( p.mode.mTrig == DAQ::eTrigSpike ) {

        kvp["trgSpikePeriEvtS"] = p.trgSpike.periEvtSecs;
        kvp["trgSpikeRefractS"] = p.trgSpike.refractSecs;
        kvp["trgSpikeAIChan"]   = p.trgSpike.aiChan;
        kvp["trgSpikeInarow"]   = p.trgSpike.inarow;
        kvp["trgSpikeNS"]       = p.trgSpike.nS;
        kvp["trgSpikeThresh"]   = p.trgSpike.T;
        kvp["trgSpikeIsNInf"]   = p.trgSpike.isNInf;
    }

// ----
// Done
// ----

    nMeasMax = 2 * 30000/100;   // ~2sec worth of blocks

    mode = Output;

    return true;
}

/* ---------------------------------------------------------------- */
/* openForExport -------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::openForExport(
    const DataFile      &other,
    const QString       &filename,
    const QVector<uint> &idxOtherChans )
{
    if( !other.isOpenForRead() ) {
        Error()
            << "INTERNAL ERROR: First parameter"
            " to DataFile::openForExport() needs"
            " to be another DataFile that is opened for reading.";
        return false;
    }

// ----
// Init
// ----

    closeAndFinalize();

// ----
// Open
// ----

    QString bName = filename;

    mainApp()->makePathAbsolute( bName );

    metaName = forceMetaSuffix( bName );

    Debug() << "Outdir : " << mainApp()->runDir();
    Debug() << "Outfile: " << bName;

    binFile.setFileName( bName );

    if( !binFile.open( QIODevice::WriteOnly ) ) {

        Error() << "openForExport error: Can't open [" << bName << "].";
        return false;
    }

// ---------
// Meta data
// ---------

    int nIndices = idxOtherChans.size();

    sRate       = other.sRate;
    nSavedChans = nIndices;
    trgChan     = other.trgChan;

    kvp                 = other.kvp;
    kvp["fileName"]     = bName;
    kvp["nSavedChans"]  = nSavedChans;

    kvp.remove( "badData" );

// Build channel ID list

    chanIds.clear();

    const QVector<uint> &src    = other.chanIds;
    uint                nSrc    = src.size();

    foreach( uint i, idxOtherChans ) {

        if( i < nSrc )
            chanIds.push_back( src[i] );
        else {
            Error()
                << "INTERNAL ERROR: The idxOtherChans passed to"
                " DataFile::openForExport must be indices into"
                " chanIds[] array, not array elements.";
        }
    }

    kvp["snsSaveChanSubset"] = Subset::vec2RngStr( chanIds );

    subclassSetSNSChanCounts( 0, &other );

// ----
// Done
// ----

    nMeasMax = 2 * 30000/100;   // ~2sec worth of blocks

    mode = Output;

    return true;
}

/* ---------------------------------------------------------------- */
/* closeAndFinalize ----------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::closeAndFinalize()
{
    bool    ok = true;

    if( mode == Undefined || !isOpen() )
        ok = false;
    else if( mode == Output ) {

        if( dfw ) {
            delete dfw;
            dfw = 0;
        }

        sha.Final();

        std::basic_string<TCHAR>    hStr;
        sha.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

        kvp["fileSHA1"]         = hStr.c_str();
        kvp["fileTimeSecs"]     = fileTimeSecs();
        kvp["fileSizeBytes"]    = binFile.size();
        kvp["version"]          = QString("%1").arg( VERSION, 0, 16 );

        int nb = badData.count();

        if( nb ) {

            QString     bdString;
            QTextStream ts( &bdString, QIODevice::WriteOnly );

            ts << badData[0].first << "," << badData[0].second;

            for( int i = 1; i < nb; ++i )
                ts << "; " << badData[i].first << "," << badData[i].second;

            kvp["badData"] = bdString;
        }

        ok = kvp.toMetaFile( metaName );

        Log() << ">> Completed " << binFile.fileName();
    }

// -----
// Reset
// -----

    binFile.close();
    metaName.clear();

    kvp.clear();
    badData.clear();
    chanIds.clear();
    meas.clear();
    sha.Reset();

    scanCt      = 0;
    mode        = Undefined;
    trgChan     = -1;
    dfw         = 0;
    wrAsync     = true;
    sRate       = 0;
    nSavedChans = 0;

    return ok;
}

/* ---------------------------------------------------------------- */
/* closeAsync ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// In asynch thread...
// - Add remote app params
// - Call closeAndFinalize
// - Delete DataFile
// - Delete threads.
// - Always return 0 (null).
//
DataFile *DataFile::closeAsync( const KeyValMap &kvm )
{
    DFCloseAsync( this, kvm );
    return 0;
}

/* ---------------------------------------------------------------- */
/* writeAndInvalScans --------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::writeAndInvalScans( vec_i16 &scans )
{
// -------------------
// Check stupid errors
// -------------------

    if( !isOpen() )
        return false;

    if( !scans.size() )
        return true;

    if( scans.size() % nSavedChans ) {

        Error()
            << "writeAndInval: Vector size not multiple of num chans ("
            << nSavedChans
            << ") [dataFile: "
            << QFileInfo( binFile.fileName() ).completeBaseName()
            << "].";
        return false;
    }

// --------------
// Update counter
// --------------

    scanCt += scans.size() / nSavedChans;

// -----
// Write
// -----

    if( wrAsync ) {

        if( !dfw )              // 40sec worth of blocks
            dfw = new DFWriter( this, int(40 * sRate/100) );

        dfw->worker->enqueue( scans, 0 );

        if( dfw->worker->percentFull() >= 95.0 ) {

            Error() << "Datafile queue overflow; stopping run...";
            return false;
        }

        return true;
    }

    return doFileWrite( scans );
}

/* ---------------------------------------------------------------- */
/* writeAndInvalSubset -------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::writeAndInvalSubset( const DAQ::Params &p, vec_i16 &scans )
{
    int n16 = subclassGetAcqChanCount( p );

    if( nSavedChans != n16 ) {

        vec_i16         S;
        QVector<uint>   iKeep;

        subclassListSavChans( iKeep, p );
        Subset::subset( S, scans, iKeep, n16 );

        return writeAndInvalScans( S );
    }
    else
        return writeAndInvalScans( scans );
}

/* ---------------------------------------------------------------- */
/* readScans ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

qint64 DataFile::readScans(
    vec_i16         &dst,
    quint64         scan0,
    quint64         num2read,
    const QBitArray &keepBits ) const
{
// ---------
// Preflight
// ---------

    if( scan0 >= scanCt )
        return -1;

    num2read = qMin( num2read, scanCt - scan0 );

// ----
// Seek
// ----

    int bytesPerScan = nSavedChans * sizeof(qint16);

    if( !((QFile*)&binFile)->seek( scan0 * bytesPerScan ) ) {

        Error()
            << "readScans error: Failed seek to pos ["
            << scan0 * bytesPerScan
            << "] file size ["
            << binFile.size()
            << "].";
        return -1;
    }

// ----
// Read
// ----

    dst.resize( num2read * nSavedChans );

//    qint64 nr = readChunky( dataFile, &dst[0], num2read * bytesPerScan );

    qint64 nr = ((QFile*)&binFile)->read(
                    (char*)&dst[0], num2read * bytesPerScan );

    if( nr != (qint64)num2read * bytesPerScan ) {

        Error()
            << "readScans error: Failed file read: returned ["
            << nr
            << "] bytes ["
            << num2read * bytesPerScan
            << "] pos ["
            << scan0 * bytesPerScan
            << "] file size ["
            << binFile.size()
            << "] msg ["
            << binFile.errorString()
            << "].";

        dst.clear();
        return -1;
    }

// ------
// Subset
// ------

    if( keepBits.size() && keepBits.count( true ) < nSavedChans ) {

        QVector<uint>   iKeep;

        Subset::bits2Vec( iKeep, keepBits );
        Subset::subset( dst, dst, iKeep, nSavedChans );
    }

    return num2read;
}

/* ---------------------------------------------------------------- */
/* setParam ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DataFile::setParam( const QString &name, const QVariant &value )
{
    kvp[name] = value;
}

/* ---------------------------------------------------------------- */
/* setRemoteParams ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void DataFile::setRemoteParams( const KeyValMap &kvm )
{
    for( KeyValMap::const_iterator it = kvm.begin();
        it != kvm.end();
        ++it ) {

        kvp[QString("rmt_%1").arg( it.key() )] = it.value();
    }
}

/* ---------------------------------------------------------------- */
/* getParam ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QVariant &DataFile::getParam( const QString &name ) const
{
    static QVariant invalid;

    KVParams::const_iterator    it = kvp.find( name );

    if( it != kvp.end() )
        return it.value();

    return invalid;
}

/* ---------------------------------------------------------------- */
/* verifySHA1 ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::verifySHA1( const QString &filename )
{
    CSHA1       sha1;
    KVParams    kvp;

    if( !kvp.fromMetaFile( forceMetaSuffix( filename ) ) ) {

        Error()
            << "verifySHA1 could not read meta data for ["
            << filename
            << "].";
        return false;
    }

    if( !sha1.HashFile( STR2CHR( filename ) ) ) {

        Error()
            << "verifySHA1 could not read file ["
            << filename
            << "].";
        return false;
    }

    std::basic_string<TCHAR>    hStr;
    sha1.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

    QString hash        = hStr.c_str();
    QString hashSaved   = kvp["fileSHA1"].toString().trimmed();

    if( hashSaved.length() != 40 ) {

        Error()
            << "verifySHA1: Bad meta data hash: ["
            << hashSaved
            << "].";
        return false;
    }

    hash = hash.trimmed();

    return 0 == hash.compare( hashSaved, Qt::CaseInsensitive );
}

/* ---------------------------------------------------------------- */
/* percentFull ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

double DataFile::percentFull() const
{
    return (dfw ? dfw->worker->percentFull() : 0);
}

/* ---------------------------------------------------------------- */
/* writeSpeedBps -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return bytes/time.
//
double DataFile::writeSpeedBps() const
{
    double  time    = 0,
            bytes   = 0;
    int     n;

    statsMtx.lock();

    n = (int)meas.size();

    for( int i = 0; i < n; ++i ) {
        time    += meas[i].x;
        bytes   += meas[i].y;
    }

    statsMtx.unlock();

    return (time > 0 ? bytes / time : 0);
}

/* ---------------------------------------------------------------- */
/* doFileWrite ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::doFileWrite( const vec_i16 &scans )
{
    double  t0      = getTime();
    int     n2Write = (int)scans.size() * sizeof(qint16);

//    int nWrit = writeChunky( binFile, &scans[0], n2Write );
    int nWrit = binFile.write( (char*)&scans[0], n2Write );

    if( nWrit != n2Write ) {
        Error() << "File writing error: " << binFile.error();
        return false;
    }

    statsMtx.lock();

    while( (int)meas.size() >= nMeasMax )
        meas.pop_front();

    meas.push_back( Vec2( getTime() - t0, n2Write ) );

    statsMtx.unlock();

    sha.Update( (const UINT_8*)&scans[0], n2Write );

    return true;
}


