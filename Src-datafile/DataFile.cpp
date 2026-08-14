
#include "DataFile.h"
#include "DataFile_Helpers.h"
#include "DFName.h"
#include "Util.h"
#include "MainApp.h"
#include "Subset.h"
#include "Version.h"

#include <QDir>
#include <QRegularExpression>


/* ---------------------------------------------------------------- */
/* ORec ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DataFile::ORec::~ORec()
{
    if( dfw )
        delete dfw;
}

/* ---------------------------------------------------------------- */
/* DataFile ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

DataFile::DataFile( int ip )
    :   sampCt(0), mode(Undefined),
        i_trgStream(DAQ::Params::jsip2stream( jsNI, 0 )),
        i_trgChan(-1), o_wrAsync(true),
        sRate(0), ip(ip), nSavedChans(0)
{
}


DataFile::~DataFile()
{
}

/* ---------------------------------------------------------------- */
/* openForRead ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::openForRead( QString &error, const QString &filename )
{
// ----
// Init
// ----

    closeAndFinalize();

// ------
// Valid?
// ------

    QString bFile = DFName::forceBinSuffix( filename );

    if( !DFName::isValidInputFile( error, bFile, {} ) ) {
        error = "openForRead error: " + error;
        Error() << error;
        return false;
    }

    // Now know files exist, are openable,
    // and metadata passes minor checking.

// ----------
// Open files
// ----------

    i_binFile.setFileName( bFile );

    if( !i_binFile.open( QIODevice::ReadOnly ) ) {
        error = QString("File error <%1> opening(read) '%2'.")
                .arg( i_binFile.errorString() ).arg ( bFile );
        Error() << error;
        return false;
    }

// ---------
// Load meta
// ---------

    kvp.fromMetaFile( metaName = DFName::forceMetaSuffix( filename ) );

// ----------
// Parse meta
// ----------

    if( !subclassParseMetaData( &error ) ) {
        error = "openForRead error: " + error + ".";
        Error() << error;
        return false;
    }

    sampCt = kvp["fileSizeBytes"].toULongLong()
                / (sizeof(qint16) * nSavedChans);

// -----------------
// Saved channel ids
// -----------------

    snsFileChans.clear();

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
        Subset::defaultVec( snsFileChans, nSavedChans );
    else if( !Subset::rngStr2Vec( snsFileChans, it->toString() ) ) {
        error =
        QString("openForRead error: Bad snsSaveChanSubset tag '%1'.")
            .arg( filename );
        Error() << error;
        return false;
    }

// Trigger Channel

    int trgMode = DAQ::stringToTrigMode( kvp["trigMode"].toString() );

    i_trgChan = -1;

    if( trgMode == DAQ::eTrigTTL ) {

        i_trgStream = kvp["trgTTLStream"].toString();

        if( kvp["trgTTLIsAnalog"].toBool() )
            i_trgChan = kvp["trgTTLAIChan"].toInt();
        else if( DAQ::Params::stream_isNI( i_trgStream ) ) {
            i_trgChan = cumTypCnt()[CniCfg::niSumAnalog]
                        + kvp["trgTTLBit"].toInt()/16;
        }
        else if( DAQ::Params::stream_isOB( i_trgStream ) )
            i_trgChan = cumTypCnt()[CimCfg::obSumAnalog];
        else
            i_trgChan = cumTypCnt()[CimCfg::imSumNeural];
    }
    else if( trgMode == DAQ::eTrigSpike ) {
        i_trgStream = kvp["trgSpikeStream"].toString();
        i_trgChan   = kvp["trgSpikeAIChan"].toInt();
    }

    if( i_trgChan != -1 ) {
        if( i_trgStream != streamFromObj() || !snsFileChans.contains( (uint)i_trgChan ) )
            i_trgChan = -1;
    }

// ----------
// State data
// ----------

    Debug()
        << "Opened ["
        << QFileInfo( bFile ).fileName() << "] "
        << nSavedChans << " chans @"
        << sRate  << " Hz, "
        << sampCt << " samps total.";

    mode = Input;

// ----------------
// Check xxErrFlags
// ----------------

    {
        QMap<QString,QVariant>::const_iterator
            it  = kvp.begin(),
            end = kvp.end();
        for( ; it != end; ++it ) {
            if( it.key().contains( "ErrFlags" ) ) {
                if( !it.value().toString().trimmed().startsWith( "0 " ) ) {
                    Warning() <<
                    QString("Metadata <%1=%2> in '%3'.")
                    .arg( it.key() )
                    .arg( it.value().toString().trimmed() )
                    .arg( QFileInfo( bFile ).fileName() );
                }
                break;
            }
        }
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* openForWrite --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Writing gets ability to save 1 bin/meta per probe shank.
// - Original ip1 used for multidisk saving: ip1 % ndir.
// - Original ip1 used for probe folder naming.
// - Original ip1 meta file always written as record.
// - Altered bin/meta files gets name ip2 = 1000 + 10*ip1 + s.
//
// Edited metadata in per-shank mode:
// - fileName
// - nSavedChans
// - snsSaveChanSubset
// - snsApLfSy
// - typeIMEnabled
// - ~snsChanMap
// - ~snsGeomMap
//
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

    o_nAcqChans = subclassGetAcqChanCount( p );
    subclassGetSavChanCount( p );

    if( !nSavedChans ) {
        Error() <<
            QString("openForWrite error: Zero channels for stream '%1'.")
            .arg( fileLblFromObj() );
        return false;
    }

// ---------
// Multidisk
// ---------

    MainApp *app    = mainApp();
    int     idir    = 0,
            ndir    = app->nDataDirs();
    bool    isIM    = p.stream_isIM( subtypeFromObj() );

    if( isIM && ndir > 1 )
        idir = ip % ndir;

// ------
// Naming
// ------

// (baseName)[lbl](ext)
// e.g.
// (dir/run_g0/run_g0_imec0/run_g0_t0.)[imec0.ap](.bin)
//
// Note: baseName only contains ip1; label may contain ip1 or ip2.

    if( !forceName.isEmpty() )
        o_baseName = forceName + ".";
    else if( !isIM || !p.sns.fldPerPrb ) {

        o_baseName =
            QString("%1/%2_g%3/%2_g%3_t%4.")
                .arg( app->dataDir( idir ) )
                .arg( p.sns.runName )
                .arg( ig )
                .arg( it );
    }
    else {

        o_baseName =
            QString("%1/%2_g%3/%2_g%3_%4")
                .arg( app->dataDir( idir ) )
                .arg( p.sns.runName )
                .arg( ig )
                .arg( streamFromObj() );

        QDir().mkdir( o_baseName );

        o_baseName += QString("/%1_g%2_t%3.")
                        .arg( p.sns.runName ).arg( ig ).arg( it );
    }

    metaName = o_baseName + fileLblFromObj() + ".meta";

// -----------------
// Standard metadata
// -----------------

// To check completeness, here is full list of daq.ini settings.
// Experiment parameters are also stored in _Calibration/
// - imec_onebox_settings.ini
// - inec_probe_settings.ini.
//
//  [DAQSettings]
//    niAiRangeMin=-2.5
//    niAiRangeMax=2.5
//    niSettle=7
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
//    syncCalMins=30
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
//    trgSpikeInarow=3
//    trgSpikeNS=10
//    trgSpikeIsNInf=false
//    gateMode=0
//    trigMode=0
//    manOvShowBut=false
//    manOvConfirm=false
//    manOvInitOff=false
//    snsNotes=
//    snsRunName=myRun
//    snsReqMins=10
//    snsPairChk=true
//    snsFldPerProbe=true
//    niLowLatency=false
//
//  [DAQ_Imec_All]
//    imQfSecs=.5
//    imQfLoCut=300
//    imQfHiCut=9000
//    imCalPolicy=0
//    imTrgSource=0
//    imSvySettleSec=2
//    imSvySecPerBnk=35
//    imSRAtDetect=true
//    imPSBAtDetect=true
//    imLowLatency=false
//    imTrgRising=true
//    imQfOn=true
//    imNProbes=1
//    obNOneBox=0
//    imEnabled=true
//    imVigilant=false
//
//  [SerialNumberToProbe]
//    SN21\imroFile=
//    SN21\imStdby=
//    SN21\imSvyMaxBnk=-1
//    SN21\imLEDEnable=false
//    SN21\imSnsChanMapFile=
//    SN21\imSnsSaveChanSubset=all
//
//  [SerialNumberToOneBox]
//    SN0\obAiRangeMax=5
//    SN0\obAOChans=0
//    SN0\obA2DThresh="0.25,0.25,0.25,0.25,0.25,0.25,0.25,0.25,0.25,0.25,0.25,0.25"
//    SN0\obXAChans=0:11
//    SN0\obDigital=true
//    SN0\obSnsChanMapFile=
//    SN0\obSnsSaveChanSubset=all
//

    kvp["appVersion"]       = QString("%1").arg( VERS_SGLX, 0, 16 );
    kvp["nDataDirs"]        = ndir;
    kvp["nSavedChans"]      = nSavedChans;
    kvp["gateMode"]         = DAQ::gateModeToString( p.mode.mGate );
    kvp["trigMode"]         = DAQ::trigModeToString( p.mode.mTrig );
    kvp["fileCreateTime"]   = dateTime2Str( tCreate, Qt::ISODate );
    kvp["syncSourcePeriod"] = p.sync.sourcePeriod;
    kvp["syncSourceIdx"]    = p.sync.sourceIdx;
    kvp["typeImEnabled"]    = p.stream_nIM();
    kvp["typeObEnabled"]    = p.stream_nOB();
    kvp["typeNiEnabled"]    = p.stream_nNI();

    // All metadata are single lines of text
    QString noReturns = p.sns.notes;
    noReturns.replace( QRegularExpression("[\r\n]"), "\\n" );
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

    subclassStoreMetaData( p );

// -----------------------------------------
// Adjust typeImEnabled for all files in run
// -----------------------------------------

    if( p.sns.sepShanks ) {

        int highestIP4 = -1;

        for( int ip = 0, np = p.stream_nIM(); ip < np; ++ip ) {
            if( p.im.prbj[ip].roTbl->nShank() > 1 )
                highestIP4 = ip;
        }

        if( highestIP4 >= 0 )
            kvp["typeImEnabled"] = 1000 + 10*highestIP4 + 4;
    }

// ------------------------
// Splitting for this file?
// ------------------------

    if( isIM && p.sns.sepShanks && p.im.prbj[ip].roTbl->nShank() > 1 ) {

        // ---------
        // Splitting
        // ---------

        // Split channel lists

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        std::sort( snsFileChans.begin(), snsFileChans.end() );

        QVector<uint>   shk[4];
        IMROTbl         *T  = E.roTbl;
        int             nAP = T->nAP(),
                        nSY = T->nSY(),
                        ic  = 0,
                        nc  = int(snsFileChans.size()),
                        C;

        // Sort neurals
        while( ic < nc && (C = snsFileChans[ic]) < nAP ) {
            shk[T->shnk( C )].push_back( C );
            ++ic;
        }

        // Append SY
        if( nSY == 4 && (nc - ic) == 4 ) {
            for( int is = 0; is < 4; ++is )
                shk[is].push_back( snsFileChans[ic + is] );
        }
        else {
            for( int is = 0; is < 4; ++is )
                shk[is].push_back( C );
        }

        // Create non-empty ORec

        for( int is = 0; is < 4; ++is ) {

            if( shk[is].size() <= 1 )
                continue;

            o_rec.push_back( std::make_unique<ORec>() );
            ORec    &R = *o_rec[o_rec.size() - 1];

            QString bName = o_baseName +
                                QString("imec%1.ap.bin").arg( 1000 + 10*ip + is );

            R.binFile.setFileName( bName );

            if( !R.binFile.open( QIODevice::WriteOnly ) ) {
                Error() <<
                QString("File error <%1> opening(write) '%2'.")
                .arg( R.binFile.errorString() ).arg( bName );
                return false;
            }

            R.iKeep     = shk[is];
            R.kvp       = kvp;
            R.metaName  = o_baseName +
                            QString("imec%1.ap.meta").arg( 1000 + 10*ip + is );

            GeomMap     G;
            QBitArray   apBits;

            Subset::vec2Bits( apBits, R.iKeep );
            E.roTbl->toGeomMap_snsFileChans( G, R.iKeep, 0 );
            G.andOutImStdby( E.stdbyBits, R.iKeep, 0 );

            R.kvp["fileName"]           = bName;
            R.kvp["nSavedChans"]        = R.iKeep.size();
            R.kvp["snsSaveChanSubset"]  = Subset::vec2RngStr( R.iKeep );
            R.kvp["snsApLfSy"]          = QString("%1,0,1").arg( R.iKeep.size() - 1 );
            R.kvp["~snsChanMap"]        = E.sns.chanMap.toString( apBits );
            R.kvp["~snsGeomMap"]        = G.toString();

            // 1st meta write
            if( !R.kvp.toMetaFile( R.metaName ) )
                return false;

            Debug() << "Outfile: " << bName;
        }

        // write meta master
        kvp.toMetaFile( metaName );
    }
    else {

        // ------
        // Single
        // ------

        o_rec.push_back( std::make_unique<ORec>() );
        ORec    &R = *o_rec[0];

        QString bName = o_baseName + fileLblFromObj() + ".bin";

        R.binFile.setFileName( bName );

        if( !R.binFile.open( QIODevice::WriteOnly ) ) {
            Error() <<
            QString("File error <%1> opening(write) '%2'.")
            .arg( R.binFile.errorString() ).arg( bName );
            return false;
        }

        if( nSavedChans < o_nAcqChans )
            R.iKeep = snsFileChans;

        R.kvp       = kvp;
        R.metaName  = metaName;

        R.kvp["fileName"] = bName;

        // 1st meta write
        if( !R.kvp.toMetaFile( R.metaName ) )
            return false;

        Debug() << "Outfile: " << bName;
    }

// ----------
// State data
// ----------

    mode = Output;

    return true;
}

/* ---------------------------------------------------------------- */
/* openForExport -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Special purpose method for FileViewerWindow exporter.
// Data from preexisting dfSrc file are copied to 'filename'.
// 'indicesOfSrcChans' are dfSrc::snsFileChan[] indices, not elements.
// For example, if srcFile contains channels: {0,1,2,3,6,7,8},
// export the last three by setting indicesOfSrcChans = {4,5,6}.
//
bool DataFile::openForExport(
    const DataFile      &dfSrc,
    const QString       &filename,
    const QVector<uint> &indicesOfSrcChans )
{
    if( !dfSrc.isOpenForRead() ) {
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

    o_rec.push_back( std::make_unique<ORec>() );
    ORec    &R = *o_rec[0];

    QString bName = filename;

    mainApp()->makePathAbsolute( bName );

    metaName = DFName::forceMetaSuffix( bName );

    Debug() << "Outfile: " << bName;

    R.binFile.setFileName( bName );

    if( !R.binFile.open( QIODevice::WriteOnly ) ) {
        Error() <<
        QString("File error <%1> opening(export bin) '%2'.")
        .arg( R.binFile.errorString() ).arg( bName );
        return false;
    }

// --------
// Metadata
// --------

    sRate       = dfSrc.sRate;
    nSavedChans = (int)indicesOfSrcChans.size();
    i_trgStream = dfSrc.i_trgStream;
    i_trgChan   = dfSrc.i_trgChan;

    kvp                 = dfSrc.kvp;
    kvp["fileName"]     = bName;
    kvp["nSavedChans"]  = nSavedChans;

// Build saved channel ID list

    snsFileChans.clear();

    const QVector<uint> &srcChans = dfSrc.snsFileChans;
    uint                nSrcChans = srcChans.size();

    foreach( uint i, indicesOfSrcChans ) {

        if( i < nSrcChans )
            snsFileChans.push_back( srcChans[i] );
        else {
            Error()
                << "INTERNAL ERROR: The indicesOfSrcChans passed to"
                " DataFile::openForExport must be indices into"
                " snsFileChans[] array, not array elements.";
        }
    }

    kvp["snsSaveChanSubset"] = Subset::vec2RngStr( snsFileChans );

    subclassSetSNSChanCounts( 0, &dfSrc );

    subclassUpdateGeomMap( dfSrc, indicesOfSrcChans );
    subclassUpdateShankMap( dfSrc, indicesOfSrcChans );
    subclassUpdateChanMap( dfSrc, indicesOfSrcChans );

// ----------
// State data
// ----------

    mode= Output;

    return true;
}

/* ---------------------------------------------------------------- */
/* closeAndFinalize ----------------------------------------------- */
/* ---------------------------------------------------------------- */

// Write metadata file including all {size, duration, SHA1} tallies.
// In output mode, the file is actually overwritten.
//
bool DataFile::closeAndFinalize()
{
    bool    ok = true;

    if( mode == Undefined )
        ok = false;
    else if( mode == Output ) {

        for( int j = 0, n = int(o_rec.size()); j < n; ++j ) {

            ORec    &R = *o_rec[j];

            R.sha.Final();
            std::basic_string<char> hStr;
            R.sha.ReportHashStl( hStr, CSHA1::REPORT_HEX_SHORT );

            R.kvp["fileSHA1"]         = hStr.c_str();
            R.kvp["fileTimeSecs"]     = fileTimeSecs();
            R.kvp["fileSizeBytes"]    = R.binFile.size();
            R.kvp["appVersion"]       = QString("%1").arg( VERS_SGLX, 0, 16 );

            ok = R.kvp.toMetaFile( R.metaName );

            Log() << ">> Completed " << R.binFile.fileName();

            if( R.dfw ) {
                delete R.dfw;
                R.dfw = 0;
            }
            R.binFile.close();
        }
    }

// -----
// Reset
// -----

    i_binFile.close();
    metaName.clear();

    kvp.clear();
    snsFileChans.clear();

    sampCt      = 0;
    mode        = Undefined;
    i_trgStream = DAQ::Params::jsip2stream( jsNI, 0 );
    i_trgChan   = -1;
    o_wrAsync   = true;
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
/* writeAndInvalSamps --------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::writeAndInvalSamps( vec_i16 &samps )
{
// -------------------
// Check stupid errors
// -------------------

    if( mode != Output )
        return false;

    int nsamp = int(samps.size());

    if( !nsamp )
        return true;

    if( nsamp % nSavedChans ) {
        Error()
            << "writeAndInval: Vector size not multiple of num chans ("
            << nSavedChans
            << ") [stream: "
            << fileLblFromObj()
            << "].";
        return false;
    }

// --------------
// Update counter
// --------------

    sampCt += nsamp / nSavedChans;

// -----
// Write
// -----

// The depth of the writer queue has been empirically set at 4000
// queue blocks, for spinning disks. Since writing generally uses
// a 0.10 second activity period, the queue size is ~400 seconds.

    if( o_wrAsync ) {

        for( int j = 0, n = int(o_rec.size()); j < n; ++j ) {

            ORec    &R = *o_rec[j];

            if( !R.dfw )
                R.dfw = new DFWriter( this, j, 4000 );

            if( R.iKeep.size() ) {
                vec_i16 rsamps;
                Subset::subset( rsamps, samps, R.iKeep, o_nAcqChans );
                R.dfw->worker->enqueue( rsamps );
            }
            else
                R.dfw->worker->enqueue( samps );

            if( R.dfw->worker->percentFull() >= 95.0 ) {
                Error() << "Datafile queue overflow; stopping run.";
                return false;
            }
        }

        return true;
    }

    return doFileWrite( samps, 0 );
}

/* ---------------------------------------------------------------- */
/* readSamps ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Read num2read samps starting from file offset samp0.
// If num2read > available, available count is used.
// Return number of samps actually read or -1 on failure.
//
// Notes:
// - Call after openForRead().
// - File starts at (samp0 == 0).
//
// To apply 'const' to this method, seek() and read()
// have to strip constness from i_binFile, since they
// move the file pointer.
//
qint64 DataFile::readSamps(
    vec_i16         &dst,
    quint64         samp0,
    quint64         num2read,
    const QBitArray &keepBits ) const
{
// ---------
// Preflight
// ---------

    if( samp0 >= sampCt )
        return -1;

    num2read = qMin( num2read, sampCt - samp0 );

// ----
// Seek
// ----

    int bytesPerSamp = nSavedChans * sizeof(qint16);

    if( !((QFile*)&i_binFile)->seek( samp0 * bytesPerSamp ) ) {

        Error()
            << "readSamps error: Failed seek to pos ["
            << samp0 * bytesPerSamp
            << "] file size ["
            << i_binFile.size()
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
    vF.push_back( &i_binFile );

//    QFile   f2, f3, f4;
//    f2.setFileName( i_binFile.fileName() );
//    f2.open( QIODevice::ReadOnly );
//    vF.push_back( &f2 );

//    f3.setFileName( i_binFile.fileName() );
//    f3.open( QIODevice::ReadOnly );
//    vF.push_back( &f3 );

//    f4.setFileName( i_binFile.fileName() );
//    f4.open( QIODevice::ReadOnly );
//    vF.push_back( &f4 );

//    Log()<<1000*(getTime()-q0);

    qint64 nr = readThreaded(
                    vF, samp0 * bytesPerSamp,
                    &dst[0], num2read * bytesPerSamp );
#elif 0

    qint64 nr = readChunky( i_binFile, &dst[0], num2read * bytesPerSamp );

#else

    qint64 nr = ((QFile*)&i_binFile)->read(
                    (char*)&dst[0], num2read * bytesPerSamp );
#endif

    if( nr != qint64(num2read) * bytesPerSamp ) {

        Error()
            << "readSamps error: Failed file read: returned ["
            << nr
            << "] bytes ["
            << num2read * bytesPerSamp
            << "] pos ["
            << samp0 * bytesPerSamp
            << "] file size ["
            << i_binFile.size()
            << "] msg ["
            << i_binFile.errorString()
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
    for( int i = 0, n = int(o_rec.size()); i < n; ++i ) {
        ORec    &R = *o_rec[i];
        R.kvp["firstSample"] = firstCt;
    }

    if( write ) {
        kvp.toMetaFile( metaName );
        for( int i = 0, n = int(o_rec.size()); i < n; ++i ) {
            ORec    &R = *o_rec[i];
            R.kvp.toMetaFile( R.metaName );
        }
    }
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
    for(KeyValMap::const_iterator it = kvm.begin(); it != kvm.end(); ++it) {

        if( it.key() == "~svySBTT" ||
            it.key().startsWith( "~anatomy" ) ||
            it.key().startsWith( "imErr" ) ||
            it.key().startsWith( "obErr" ) ) {

            kvp[it.key()] = it.value();
            for( int i = 0, n = int(o_rec.size()); i < n; ++i ) {
                ORec    &R = *o_rec[i];
                R.kvp[it.key()] = it.value();
            }
        }
        else {
            QString key = QString("rmt_%1").arg( it.key() );
            kvp[key] = it.value();
            for( int i = 0, n = int(o_rec.size()); i < n; ++i ) {
                ORec    &R = *o_rec[i];
                R.kvp[key] = it.value();
            }
        }
    }
}

/* ---------------------------------------------------------------- */
/* delParam ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DataFile::delParam( const QString &name )
{
    kvp.remove( name );
}

/* ---------------------------------------------------------------- */
/* notes ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QString DataFile::notes() const
{
    KVParams::const_iterator    it = kvp.find( "userNotes" );

    if( it != kvp.end() ) {

        QString withReturns = it.value().toString();
        withReturns.replace( QRegularExpression("\\\\n"), "\n" );

        return withReturns;
    }

    return "";
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

    return QVariant();
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
            << "verifySHA1 could not read metadata for ["
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
            << "verifySHA1: Bad metadata hash: ["
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
    double  pct = 0.0;

    for( int j = 0, n = int(o_rec.size()); j < n; ++j ) {
        DFWriter    *dfw = o_rec[j]->dfw;
        if( dfw )
            pct = qMax( pct, dfw->worker->percentFull() );
    }

    return pct;
}

/* ---------------------------------------------------------------- */
/* writeSpeedBps -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return bytes/time.
//
double DataFile::writtenBytes() const
{
    double  sum = 0;

    for( int j = 0, n = int(o_rec.size()); j < n; ++j ) {

        const ORec  &R = *o_rec[j];

        QMutexLocker    ml( &R.statsMtx );

        foreach( uint bytes, R.statsBytes )
            sum += bytes;

        R.statsBytes.clear();
    }

    return sum;
}

/* ---------------------------------------------------------------- */
/* doFileWrite ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataFile::doFileWrite( const vec_i16 &samps, int j )
{
    ORec    &R = *o_rec[j];

    int n2Write = int(samps.size()) * sizeof(qint16);

//    int nWrit = writeChunky( R->binFile, &samps[0], n2Write );
    int nWrit = R.binFile.write( (char*)&samps[0], n2Write );

    R.statsMtx.lock();
        R.statsBytes.push_back( nWrit );
    R.statsMtx.unlock();

    if( nWrit != n2Write ) {
        Error() <<
        QString("File error <%1> writing(bin) '%2'.")
        .arg( R.binFile.errorString() ).arg( R.binFile.fileName() );
        return false;
    }

    R.sha.Update( (const UINT_8*)&samps[0], n2Write );

    return true;
}


