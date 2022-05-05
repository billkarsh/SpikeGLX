
#include "DataFile.h"
#include "DataFile_Helpers.h"
#include "DFName.h"
#include "Util.h"
#include "MainApp.h"
#include "Subset.h"
#include "Version.h"

#include <QDir>


/* ---------------------------------------------------------------- */
/* DataFile ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DataFile::DataFile( int ip )
    :   scanCt(0), mode(Undefined),
        trgStream(DAQ::Params::jsip2stream( 0, 0 )),
        trgChan(-1), dfw(0), wrAsync(true), sRate(0),
        ip(ip), nSavedChans(0)
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
/* openForRead ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::openForRead( const QString &filename, QString &error )
{
// ----
// Init
// ----

    closeAndFinalize();

// ------
// Valid?
// ------

    QString bFile = DFName::forceBinSuffix( filename );

    if( !DFName::isValidInputFile( bFile, {}, &error ) ) {
        error = "openForRead error: " + error;
        Error() << error;
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

    kvp.fromMetaFile( metaName = DFName::forceMetaSuffix( filename ) );

// ----------
// Parse meta
// ----------

    subclassParseMetaData();

    scanCt = kvp["fileSizeBytes"].toULongLong()
                / (sizeof(qint16) * nSavedChans);

// -----------
// Channel ids
// -----------

    chanIds.clear();

// Load subset string

    KVParams::const_iterator    it = kvp.find( "snsSaveChanSubset" );

    if( it == kvp.end() ) {
        error =
        QString("openForRead error: Missing snsSaveChanSubset tag '%1'.")
            .arg( filename );
        Error() << error;
        return false;
    }

    if( Subset::isAllChansStr( it->toString() ) )
        Subset::defaultVec( chanIds, nSavedChans );
    else if( !Subset::rngStr2Vec( chanIds, it->toString() ) ) {
        error =
        QString("openForRead error: Bad snsSaveChanSubset tag '%1'.")
            .arg( filename );
        Error() << error;
        return false;
    }

// Trigger Channel

    int trgMode = DAQ::stringToTrigMode( kvp["trigMode"].toString() );

    trgChan = -1;

    if( trgMode == DAQ::eTrigTTL ) {

        trgStream = kvp["trgTTLStream"].toString();

        if( kvp["trgTTLIsAnalog"].toBool() )
            trgChan = kvp["trgTTLAIChan"].toInt();
        else if( DAQ::Params::stream_isNI( trgStream ) ) {
            trgChan = cumTypCnt()[CniCfg::niSumAnalog]
                        + kvp["trgTTLBit"].toInt()/16;
        }
        else if( DAQ::Params::stream_isOB( trgStream ) )
            trgChan = cumTypCnt()[CimCfg::obSumAnalog];
        else
            trgChan = cumTypCnt()[CimCfg::imSumNeural];
    }
    else if( trgMode == DAQ::eTrigSpike ) {
        trgStream   = kvp["trgSpikeStream"].toString();
        trgChan     = kvp["trgSpikeAIChan"].toInt();
    }

    if( trgChan != -1 ) {
        if( trgStream != streamFromObj() || !chanIds.contains( trgChan ) )
            trgChan = -1;
    }

// ----------
// State data
// ----------

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

bool DataFile::openForWrite(
    const DAQ::Params   &p,
    int                 ig,
    int                 it,
    const QString       &forceName )
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

    QString brevname;

    if( forceName.isEmpty() ) {

        brevname = QString("%1_g%2_t%3.%4.bin")
                    .arg( p.sns.runName )
                    .arg( ig ).arg( it )
                    .arg( fileLblFromObj() );
    }
    else {
        brevname = QString("%1.%2.bin")
                    .arg( forceName )
                    .arg( fileLblFromObj() );
    }

    int nSaved = subclassGetSavChanCount( p );

    if( !nSaved ) {

        Error() <<
            QString("openForWrite error: Zero channels for file '%1'.")
            .arg( brevname );
        return false;
    }

// ----
// Open
// ----

    MainApp *app    = mainApp();
    QString bName;
    int     idir    = 0,
            ndir    = app->nDataDirs();
    bool    isIM    = p.stream_isIM( subtypeFromObj() );

    if( isIM && ndir > 1 )
        idir = ip % ndir;

    if( !forceName.isEmpty() )
        bName = brevname;
    else if( !p.sns.fldPerPrb || !isIM ) {

        bName = QString("%1/%2_g%3/%4")
                .arg( app->dataDir( idir ) )
                .arg( p.sns.runName ).arg( ig )
                .arg( brevname );
    }
    else {

        bName = QString("%1/%2_g%3/%2_g%3_%4")
                .arg( app->dataDir( idir ) )
                .arg( p.sns.runName ).arg( ig )
                .arg( streamFromObj() );

        QDir().mkdir( bName );

        bName += "/" + brevname;
    }

    metaName = DFName::forceMetaSuffix( bName );

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
// Experiment parameters are also stored in _Calibration/
// - imec_onebox_settings.ini
// - inec_probe_settings.ini.
//
//  [DAQSettings]
//    niAiRangeMin=-2.5
//    niAiRangeMax=2.5
//    niSampRate=19737
//    niMNGain=200
//    niMAGain=1
//    niDev1=Dev1
//    niDev2=Dev1
//    niClockSource=Whisper : 25000
//    niClockLine1=PFI2
//    niClockLine2=PFI2
//    niMNChans1=0:5
//    niMAChans1=6:7
//    niXAChans1=
//    niXDChans1=
//    niMNChans2=0:5
//    niMAChans2=6:7
//    niXAChans2=
//    niXDChans2=
//    niMuxFactor=32
//    niAiTermConfig=-1
//    niEnabled=true
//    niDualDevMode=false
//    niStartEnable=true
//    niStartLine=Dev1/port0/line0
//    niSnsShankMapFile=
//    niSnsChanMapFile=
//    niSnsSaveChanSubset=all
//    syncSourcePeriod=1
//    syncSourceIdx=0
//    syncNiThresh=1.1
//    syncImInputSlot=2
//    syncNiChanType=1
//    syncNiChan=224
//    trgTimTL0=10
//    trgTimTH=10
//    trgTimTL=1
//    trgTimNH=3
//    trgTimIsHInf=false
//    trgTimIsNInf=false
//    trgTTLThresh=1.1
//    trgTTLMarginS=1
//    trgTTLRefractS=0.5
//    trgTTLTH=0.5
//    trgTTLStream=nidq
//    trgTTLMode=0
//    trgTTLAIChan=4
//    trgTTLBit=0
//    trgTTLInarow=5
//    trgTTLNH=10
//    trgTTLIsAnalog=true
//    trgTTLIsNInf=true
//    trgSpikeThresh=-0.0001
//    trgSpikePeriEvtS=1
//    trgSpikeRefractS=0.5
//    trgSpikeStream=nidq
//    trgSpikeAIChan=4
//    trgSpikeInarow=5
//    trgSpikeNS=10
//    trgSpikeIsNInf=false
//    gateMode=0
//    trigMode=0
//    manOvShowBut=false
//    manOvInitOff=false
//    snsNotes=
//    snsRunName=myRun
//    snsReqMins=10
//
//  [DAQ_Imec_All]
//    imCalPolicy=1
//    imTrgSource=0
//    imTrgRising=true
//    imBistAtDetect=false
//    imNProbes=1
//    imEnabled=true
//    obNOnebox=0
//
//  [SerialNumberToProbe]
//    SN21\imRoFile=
//    SN21\imStdby=
//    SN21\imLEDEnable=false
//    SN21\imSnsShankMapFile=
//    SN21\imSnsChanMapFile=
//    SN21\imSnsSaveChanSubset=all
//
//  [SerialNumberToOnebox]
//    SN0\obAiRangeMax=5
//    SN0\obXAChans=0:11
//    SN0\obDigital=true
//    SN0\obSnsShankMapFile=
//    SN0\obSnsChanMapFile=
//    SN0\obSnsSaveChanSubset=all
//

    nSavedChans = nSaved;

    subclassStoreMetaData( p );

    kvp["nDataDirs"]        = ndir;
    kvp["nSavedChans"]      = nSavedChans;
    kvp["gateMode"]         = DAQ::gateModeToString( p.mode.mGate );
    kvp["trigMode"]         = DAQ::trigModeToString( p.mode.mTrig );
    kvp["fileName"]         = bName;
    kvp["fileCreateTime"]   = dateTime2Str( tCreate, Qt::ISODate );
    kvp["syncSourcePeriod"] = p.sync.sourcePeriod;
    kvp["syncSourceIdx"]    = p.sync.sourceIdx;
    kvp["typeImEnabled"]    = p.stream_nIM();
    kvp["typeObEnabled"]    = p.stream_nOB();
    kvp["typeNiEnabled"]    = p.stream_nNI();

    // All metadata are single lines of text
    QString noReturns = p.sns.notes;
    noReturns.replace( QRegExp("[\r\n]"), "\\n" );
    kvp["userNotes"]        = noReturns;

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
        kvp["trgTTLStream"]     = p.trgTTL.stream;
        kvp["trgTTLMode"]       = p.trgTTL.mode;
        kvp["trgTTLAIChan"]     = p.trgTTL.chan;
        kvp["trgTTLBit"]        = p.trgTTL.bit;
        kvp["trgTTLInarow"]     = p.trgTTL.inarow;
        kvp["trgTTLNH"]         = p.trgTTL.nH;
        kvp["trgTTLThresh"]     = p.trgTTL.T;
        kvp["trgTTLIsAnalog"]   = p.trgTTL.isAnalog;
        kvp["trgTTLIsNInf"]     = p.trgTTL.isNInf;
    }
    else if( p.mode.mTrig == DAQ::eTrigSpike ) {

        kvp["trgSpikePeriEvtS"] = p.trgSpike.periEvtSecs;
        kvp["trgSpikeRefractS"] = p.trgSpike.refractSecs;
        kvp["trgSpikeStream"]   = p.trgSpike.stream;
        kvp["trgSpikeAIChan"]   = p.trgSpike.aiChan;
        kvp["trgSpikeInarow"]   = p.trgSpike.inarow;
        kvp["trgSpikeNS"]       = p.trgSpike.nS;
        kvp["trgSpikeThresh"]   = p.trgSpike.T;
        kvp["trgSpikeIsNInf"]   = p.trgSpike.isNInf;
    }

// ----------
// State data
// ----------

    nMeasMax = 2 * 30000/100;   // ~2sec worth of blocks

    mode = Output;

// ---------------------
// Preliminary meta data
// ---------------------

// Write everything except {size, duration, SHA1} tallies.

    kvp["appVersion"] = QString("%1").arg( VERSION, 0, 16 );

    return kvp.toMetaFile( metaName );
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

    metaName = DFName::forceMetaSuffix( bName );

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
    trgStream   = other.trgStream;
    trgChan     = other.trgChan;

    kvp                 = other.kvp;
    kvp["fileName"]     = bName;
    kvp["nSavedChans"]  = nSavedChans;

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

    subclassUpdateShankMap( other, idxOtherChans );
    subclassUpdateChanMap( other, idxOtherChans );

// ----------
// State data
// ----------

    nMeasMax = 2 * 30000/100;   // ~2sec worth of blocks

    mode = Output;

    return true;
}

/* ---------------------------------------------------------------- */
/* closeAndFinalize ----------------------------------------------- */
/* ---------------------------------------------------------------- */

// Write meta data file including all {size, duration, SHA1} tallies.
// In output mode, the file is actually overwritten.
//
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

        std::basic_string<char> hStr;
        sha.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

        kvp["fileSHA1"]         = hStr.c_str();
        kvp["fileTimeSecs"]     = fileTimeSecs();
        kvp["fileSizeBytes"]    = binFile.size();
        kvp["appVersion"]       = QString("%1").arg( VERSION, 0, 16 );

        ok = kvp.toMetaFile( metaName );

        Log() << ">> Completed " << binFile.fileName();
    }

// -----
// Reset
// -----

    binFile.close();
    metaName.clear();

    statsBytes.clear();
    kvp.clear();
    chanIds.clear();
    sha.Reset();

    scanCt      = 0;
    mode        = Undefined;
    trgStream   = DAQ::Params::jsip2stream( 0, 0 );
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

        if( !dfw )
            dfw = new DFWriter( this, 4000 );

        dfw->worker->enqueue( scans );

        if( dfw->worker->percentFull() >= 95.0 ) {

            Error() << "Datafile queue overflow; stopping run.";
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

    if( nSavedChans != n16 )
        Subset::subset( scans, scans, chanIds, n16 );

    return writeAndInvalScans( scans );
}

/* ---------------------------------------------------------------- */
/* readScans ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Read num2read scans starting from file offset scan0.
// Note that (scan0 == 0) is the start of this file.
//
// To apply 'const' to this method, seek() and read()
// have to strip constness from binFile, since they
// move the file pointer.
//
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

#if 0
//    double  q0=getTime();

    std::vector<const QFile*>   vF;
    vF.push_back( &binFile );

//    QFile   f2, f3, f4;
//    f2.setFileName( binFile.fileName() );
//    f2.open( QIODevice::ReadOnly );
//    vF.push_back( &f2 );

//    f3.setFileName( binFile.fileName() );
//    f3.open( QIODevice::ReadOnly );
//    vF.push_back( &f3 );

//    f4.setFileName( binFile.fileName() );
//    f4.open( QIODevice::ReadOnly );
//    vF.push_back( &f4 );

//    Log()<<1000*(getTime()-q0);

    qint64 nr = readThreaded(
                    vF, scan0 * bytesPerScan,
                    &dst[0], num2read * bytesPerScan );
#elif 0

    qint64 nr = readChunky( binFile, &dst[0], num2read * bytesPerScan );

#else

    qint64 nr = ((QFile*)&binFile)->read(
                    (char*)&dst[0], num2read * bytesPerScan );
#endif

    if( nr != qint64(num2read) * bytesPerScan ) {

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
/* setFirstSample ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This is an absolute time stamp relative to the stream start,
// that is, relative to the start of acquisition (sample #0).
//
void DataFile::setFirstSample( quint64 firstCt, bool write )
{
    kvp["firstSample"] = firstCt;

    if( write )
        kvp.toMetaFile( metaName );
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
    for( KeyValMap::const_iterator it = kvm.begin(); it != kvm.end(); ++it )
        kvp[QString("rmt_%1").arg( it.key() )] = it.value();
}

/* ---------------------------------------------------------------- */
/* notes ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString DataFile::notes() const
{
    KVParams::const_iterator    it = kvp.find( "userNotes" );

    if( it != kvp.end() ) {

        QString withReturns = it.value().toString();
        withReturns.replace( QRegExp("\\\\n"), "\n" );

        return withReturns;
    }

    return "";
}

/* ---------------------------------------------------------------- */
/* probeType ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Return one of:
// - found true probe type
// - (-3) if 3A type
// - (-999) if not identified as probe.
//
int DataFile::probeType() const
{
    KVParams::const_iterator    it = kvp.find( "imDatPrb_type" );

    if( it != kvp.end() )
        return it.value().toInt();
    else if( kvp.contains( "imProbeOpt" ) )
        return -3;

    return -999;
}

/* ---------------------------------------------------------------- */
/* streamCounts --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DataFile::streamCounts( int &nIm, int &nOb, int &nNi ) const
{
    KVParams::const_iterator    it = kvp.find( "typeImEnabled" );

    nOb = 0;

    if( it != kvp.end() ) {

        nIm = it.value().toInt();
        nNi = kvp["typeNiEnabled"].toInt();

        it = kvp.find( "typeObEnabled" );

        if( it != kvp.end() )
            nOb = it.value().toInt();
    }
    else {

        QString sTypes =  kvp["typeEnabled"].toString();

        nIm = sTypes.contains( "imec" );
        nNi = sTypes.contains( "nidq" );
    }
}

/* ---------------------------------------------------------------- */
/* firstCt -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This is an absolute time stamp relative to the stream start,
// that is, relative to the start of acquisition (sample #0).
//
quint64 DataFile::firstCt() const
{
    KVParams::const_iterator    it = kvp.find( "firstSample" );

    if( it != kvp.end() )
        return it.value().toULongLong();

    return 0;
}

/* ---------------------------------------------------------------- */
/* getParam ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QVariant DataFile::getParam( const QString &name ) const
{
    KVParams::const_iterator    it = kvp.find( name );

    if( it != kvp.end() )
        return it.value();

    return QVariant::Invalid;
}

/* ---------------------------------------------------------------- */
/* verifySHA1 ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::verifySHA1( const QString &filename )
{
    CSHA1       sha1;
    KVParams    kvp;

    if( !kvp.fromMetaFile( DFName::forceMetaSuffix( filename ) ) ) {

        Error()
            << "verifySHA1 could not read meta data for ["
            << filename
            << "].";
        return false;
    }

    if( !sha1.HashFile( STR2CHR( filename ) ) ) {

        Error()
            << "verifySHA1 could not read file '"
            << filename
            << "'.";
        return false;
    }

    std::basic_string<char> hStr;
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
double DataFile::writtenBytes() const
{
    double  sum = 0;

    statsMtx.lock();

    foreach( uint bytes, statsBytes )
        sum += bytes;

    statsBytes.clear();

    statsMtx.unlock();

    return sum;
}

/* ---------------------------------------------------------------- */
/* doFileWrite ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::doFileWrite( const vec_i16 &scans )
{
    int n2Write = int(scans.size()) * sizeof(qint16);

//    int nWrit = writeChunky( binFile, &scans[0], n2Write );
    int nWrit = binFile.write( (char*)&scans[0], n2Write );

    statsMtx.lock();
        statsBytes.push_back( nWrit );
    statsMtx.unlock();

    if( nWrit != n2Write ) {
        Error() << "File writing error: " << binFile.error();
        return false;
    }

    sha.Update( (const UINT_8*)&scans[0], n2Write );

    return true;
}


