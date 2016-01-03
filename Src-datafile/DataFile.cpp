
#include "DataFile.h"
#include "DataFile_Helpers.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Subset.h"

#include <QDateTime>




/* ---------------------------------------------------------------- */
/* DataFile ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DataFile::DataFile()
    :   sRate(0), scanCt(0), nSavedChans(0),
        mode(Undefined), dfw(0), wrAsync(true)
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
/* isValidInputFile ------------------------------------------------ */
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

    if( !kvp.contains( "fileSizeBytes" )
        || !kvp.contains( "nSavedChans" )
        || !kvp.contains( "niSampRate" ) ) {

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

    dataFile.setFileName( bFile );
    dataFile.open( QIODevice::ReadOnly );

// ---------
// Load meta
// ---------

    kvp.fromMetaFile( metaName = forceMetaSuffix( filename ) );

// ----------
// Parse meta
// ----------

    qint64  fsize = kvp["fileSizeBytes"].toLongLong();

    nSavedChans     = kvp["nSavedChans"].toUInt();
    sRate           = kvp["niSampRate"].toDouble();
    scanCt          = fsize / (sizeof(qint16) * nSavedChans);
    niRange.rmin    = kvp["niAiRangeMin"].toDouble();
    niRange.rmax    = kvp["niAiRangeMax"].toDouble();
    mnGain          = kvp["niMNGain"].toDouble();
    maGain          = kvp["niMAGain"].toDouble();
    trgMode         = DAQ::stringToTrigMode( kvp["trigMode"].toString() );

    parseChanCounts();

// -----------
// Channel ids
// -----------

    chanIds.clear();

// Load real subset if exists

    KVParams::const_iterator    it;
    bool                        subsetOK = false;

    if( (it = kvp.find( "snsSaveChanSubset" )) != kvp.end()
        && !Subset::isAllChansStr( it->toString() ) ) {

        subsetOK = Subset::rngStr2Vec( chanIds, it->toString() );
    }

// Otherwise assume using all

    if( !subsetOK )
        Subset::defaultVec( chanIds, nSavedChans );

// Finally, there must be nChans per timepoint!!
// So we replicate last channel to fill any gap.

    int nc = chanIds.size();

    if( nSavedChans > nc )
        chanIds.insert( chanIds.end(), nSavedChans - nc, chanIds[nc - 1] );

// Trigger Channel

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

    int nOnChans = p.sns.saveBits.count( true );

    if( !p.ni.niCumTypCnt[CniCfg::niSumAll] || !nOnChans ) {
        Error() << "openForWrite error: Save-channel count = zero.";
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

    dataFile.setFileName( bName );

    if( !dataFile.open( QIODevice::WriteOnly ) ) {

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
//    trigInitiallyOff=false    // N.A.
//    snsChanMapFile=           // snsChanMap string
//    snsSaveChanSubset=all
//    snsRunName=myRun          // outputFile
//    snsMaxGrfPerTab=0         // N.A.
//    snsSuppressGraphs=false   // N.A.

    sRate       = p.ni.srate;
    nSavedChans = nOnChans;

    kvp["niAiRangeMin"]         = p.ni.range.rmin;
    kvp["niAiRangeMax"]         = p.ni.range.rmax;
    kvp["niSampRate"]           = sRate;
    kvp["niMNGain"]             = p.ni.mnGain;
    kvp["niMAGain"]             = p.ni.maGain;
    kvp["niDev1"]               = p.ni.dev1;
    kvp["niDev1ProductName"]    = CniCfg::getProductName( p.ni.dev1 );
    kvp["niClock1"]             = p.ni.clockStr1;
    kvp["niMNChans1"]           = p.ni.uiMNStr1;
    kvp["niMAChans1"]           = p.ni.uiMAStr1;
    kvp["niXAChans1"]           = p.ni.uiXAStr1;
    kvp["niXDChans1"]           = p.ni.uiXDStr1;
    kvp["niMuxFactor"]          = p.ni.muxFactor;
    kvp["niAiTermination"]      = CniCfg::termConfigToString( p.ni.termCfg );
    kvp["niSyncEnable"]         = p.ni.syncEnable;
    kvp["niSyncLine"]           = p.ni.syncLine;
    kvp["gateMode"]             = DAQ::gateModeToString( p.mode.mGate );
    kvp["trigMode"]             = DAQ::trigModeToString( p.mode.mTrig );
    kvp["snsChanMap"]           = p.sns.chanMap.toString( p.sns.saveBits );
    kvp["nSavedChans"]          = nSavedChans;
    kvp["fileName"]             = bName;
    kvp["fileCreateTime"]       = tCreate.toString( Qt::ISODate );

    if( p.sns.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = p.sns.uiSaveChanStr;
        Subset::bits2Vec( chanIds, p.sns.saveBits );
    }
    else {

        kvp["snsSaveChanSubset"] = "all";
        Subset::defaultVec( chanIds, nSavedChans );
    }

    const int   *type = p.ni.niCumTypCnt;

    kvp["acqApLfMnMaXaDw"] =
        QString("0,0,%1,%2,%3,%4")
        .arg( type[CniCfg::niTypeMN] )
        .arg( type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN] )
        .arg( type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA] )
        .arg( type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

    setSaveChanCounts( p );

    if( p.ni.isDualDevMode ) {

        kvp["niDev2"]               = p.ni.dev2;
        kvp["niDev2ProductName"]    = CniCfg::getProductName( p.ni.dev2 );
        kvp["niClock2"]             = p.ni.clockStr2;
        kvp["niMNChans2"]           = p.ni.uiMNStr2();
        kvp["niMAChans2"]           = p.ni.uiMAStr2();
        kvp["niXAChans2"]           = p.ni.uiXAStr2();
        kvp["niXDChans2"]           = p.ni.uiXDStr2();
        kvp["niDualDevMode"]        = true;
    }

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

    meas.resizeAndErase( 10 * daqAINumFetchesPerSec() ); // ~10s worth

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

    dataFile.setFileName( bName );

    if( !dataFile.open( QIODevice::WriteOnly ) ) {

        Error() << "openForExport error: Can't open [" << bName << "]";
        return false;
    }

// ---------
// Meta data
// ---------

    int nOnChans = idxOtherChans.size();

    sRate       = other.sRate;
    nSavedChans = nOnChans;
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

    setSaveChanCounts( mainApp()->cfgCtl()->acceptedParams );

// ----
// Done
// ----

    meas.resizeAndErase( 10 * daqAINumFetchesPerSec() ); // ~10s worth

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
        kvp["fileSizeBytes"]    = dataFile.size();

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

        Log() << ">> Completed " << dataFile.fileName();
    }

// -----
// Reset
// -----

    dataFile.close();
    metaName.clear();

    kvp.clear();
    badData.clear();
    chanIds.clear();
    meas.erase();
    sha.Reset();

    sRate       = 0;
    scanCt      = 0;
    nSavedChans = 0;
    trgMode     = 0;
    trgChan     = -1;
    dfw         = 0;
    mode        = Undefined;

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
            << QFileInfo( dataFile.fileName() ).baseName()
            << "]";
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

        if( !dfw )
            dfw = new DFWriter( this );

        dfw->worker->enqueue( scans, 0 );

        return dfw->worker->percentFull() < 100.0;
    }

    return doFileWrite( scans );
}

/* ---------------------------------------------------------------- */
/* writeAndInvalSubset -------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::writeAndInvalSubset( const DAQ::Params &p, vec_i16 &scans )
{
    int n16 = p.ni.niCumTypCnt[CniCfg::niSumAll];

    if( nSavedChans != n16 ) {

        vec_i16         S;
        QVector<uint>   iKeep;

        Subset::bits2Vec( iKeep, p.sns.saveBits );
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
    const QBitArray &keepBits )
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

    if( !dataFile.seek( scan0 * bytesPerScan ) ) {

        Error()
            << "readScans error: Failed seek to pos ["
            << scan0 * bytesPerScan
            << "] file size ["
            << dataFile.size()
            << "].";
        return -1;
    }

// ----
// Read
// ----

    dst.resize( num2read * nSavedChans );

//    qint64 nr = readChunky( dataFile, &dst[0], num2read * bytesPerScan );
    qint64 nr = dataFile.read( (char*)&dst[0], num2read * bytesPerScan );

    if( nr != (qint64)num2read * bytesPerScan ) {

        Error()
            << "readScans error: Failed file read: returned ["
            << nr
            << "] bytes ["
            << num2read * bytesPerScan
            << "] pos ["
            << scan0 * bytesPerScan
            << "] file size ["
            << dataFile.size()
            << "] msg ["
            << dataFile.errorString()
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
/* origID2Type ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Type = {0=neural, 1=aux, 2=dig}.
//
int DataFile::origID2Type( int ic ) const
{
    if( ic >= niCumTypCnt[CniCfg::niTypeXA] )
        return 2;

    if( ic >= niCumTypCnt[CniCfg::niTypeMN] )
        return 1;

    return 0;
}

/* ---------------------------------------------------------------- */
/* origID2Gain ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

double DataFile::origID2Gain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        if( ic < niCumTypCnt[CniCfg::niTypeMN] )
            g = mnGain;
        else if( ic < niCumTypCnt[CniCfg::niTypeMA] )
            g = maGain;

        if( g < 0.01 )
            g = 0.01;
    }

    return g;
}

/* ---------------------------------------------------------------- */
/* chanMap -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ChanMap DataFile::chanMap() const
{
    ChanMap chanMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "snsChanMap" )) != kvp.end() )
        chanMap.fromString( it.value().toString() );

    return chanMap;
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
            << "verifySHA1 could not read meta data for: "
            << filename;
        return false;
    }

    if( !sha1.HashFile( STR2CHR( filename ) ) ) {

        Error()
            << "verifySHA1 could not read file: "
            << filename;
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
            << "]";
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
/* writeSpeedBytesSec --------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return bytes/time.
//
double DataFile::writeSpeedBytesSec() const
{
    double  time    = 0,
            bytes   = 0;
    Vec2    *p;
    uint    n;

    statsMtx.lock();

    n = meas.dataPtr1( p );

    for( int i = 0; i < (int)n; ++i ) {
        time    += p[i].x;
        bytes   += p[i].y;
    }

    n = meas.dataPtr2( p );

    for( int i = 0; i < (int)n; ++i ) {
        time    += p[i].x;
        bytes   += p[i].y;
    }

    statsMtx.unlock();

    return (time > 0 ? bytes / time : 0);
}

/* ---------------------------------------------------------------- */
/* parseChanCounts ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Notes:
// ------
// - This tag is perhaps redundant with the snsChanMap header
// in that it merely multiplies out the muxfactor. It's mainly
// a convenience, but has a nice symmetry with snsApLfMnMaXaDw,
// which is a truly useful tag.
//
// - The original category counts obtained here can be used to
// type (original) channel IDs, for purposes of filter or gain
// assignment.
//
void DataFile::parseChanCounts()
{
    const QStringList   sl = kvp["acqApLfMnMaXaDw"].toString().split(
                                QRegExp("^\\s*|\\s*,\\s*"),
                                QString::SkipEmptyParts );

// --------------------------------
// First count each type separately
// --------------------------------

    if( sl.size() >= 6 ) {

        niCumTypCnt[CniCfg::niTypeMN] = sl[2].toInt();
        niCumTypCnt[CniCfg::niTypeMA] = sl[3].toInt();
        niCumTypCnt[CniCfg::niTypeXA] = sl[4].toInt();
        niCumTypCnt[CniCfg::niTypeXD] = sl[5].toInt();
    }
    else {

        QVector<uint>   vc;
        int             muxFactor = kvp["niMuxFactor"].toInt();

        Subset::rngStr2Vec( vc, QString("%1,%2")
            .arg( kvp["niMNChans1"].toString() )
            .arg( kvp["niMNChans2"].toString() ) );
        niCumTypCnt[CniCfg::niTypeMN] = vc.size() * muxFactor;

        Subset::rngStr2Vec( vc, QString("%1,%2")
            .arg( kvp["niMAChans1"].toString() )
            .arg( kvp["niMAChans2"].toString() ) );
        niCumTypCnt[CniCfg::niTypeMA] = vc.size() * muxFactor;

        Subset::rngStr2Vec( vc, QString("%1,%2")
            .arg( kvp["niXAChans1"].toString() )
            .arg( kvp["niXAChans2"].toString() ) );
        niCumTypCnt[CniCfg::niTypeXA] = vc.size();

        Subset::rngStr2Vec( vc, QString("%1,%2")
            .arg( kvp["niXDChans1"].toString() )
            .arg( kvp["niXDChans2"].toString() ) );
        niCumTypCnt[CniCfg::niTypeXD] = (vc.size() + 15) / 16;
    }

// ---------
// Integrate
// ---------

    for( int i = 1; i < CniCfg::niNTypes; ++i )
        niCumTypCnt[i] += niCumTypCnt[i - 1];
}

/* ---------------------------------------------------------------- */
/* setSaveChanCounts ---------------------------------------------- */
/* ---------------------------------------------------------------- */

// Note: The snsChanMap tag stores the original acquired channel set.
// It is independent of snsSaveChanSubset. On the other hand, this
// tag records counts of saved channels in each category, thus the
// binary stream format.
//
void DataFile::setSaveChanCounts( const DAQ::Params &p )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *type = reinterpret_cast<const uint*>(p.ni.niCumTypCnt);

    int niEachTypeCnt[CniCfg::niNTypes],
        i = 0,
        n = chanIds.size();

    memset( niEachTypeCnt, 0, CniCfg::niNTypes*sizeof(int) );

    while( i < n && chanIds[i] < type[CniCfg::niTypeMN] ) {
        ++niEachTypeCnt[CniCfg::niTypeMN];
        ++i;
    }

    while( i < n && chanIds[i] < type[CniCfg::niTypeMA] ) {
        ++niEachTypeCnt[CniCfg::niTypeMA];
        ++i;
    }

    while( i < n && chanIds[i] < type[CniCfg::niTypeXA] ) {
        ++niEachTypeCnt[CniCfg::niTypeXA];
        ++i;
    }

    while( i < n && chanIds[i] < type[CniCfg::niTypeXD] ) {
        ++niEachTypeCnt[CniCfg::niTypeXD];
        ++i;
    }

    kvp["snsApLfMnMaXaDw"] =
        QString("0,0,%1,%2,%3,%4")
        .arg( niEachTypeCnt[CniCfg::niTypeMN] )
        .arg( niEachTypeCnt[CniCfg::niTypeMA] )
        .arg( niEachTypeCnt[CniCfg::niTypeXA] )
        .arg( niEachTypeCnt[CniCfg::niTypeXD] );
}

/* ---------------------------------------------------------------- */
/* doFileWrite ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::doFileWrite( const vec_i16 &scans )
{
    int     n2Write = (int)scans.size() * sizeof(qint16);
    Vec2    m( getTime(), n2Write );

    int nWrit = writeChunky( dataFile, &scans[0], n2Write );

    if( nWrit != n2Write ) {
        Error() << "File writing error: " << dataFile.error();
        return false;
    }

    m.x = getTime() - m.x;

    statsMtx.lock();
    meas.putData( &m, 1 );
    statsMtx.unlock();

    sha.Update( (const UINT_8*)&scans[0], n2Write );

    return true;
}


