
#include "DataFileNI.h"
#include "Subset.h"




// Type = {0=neural, 1=aux, 2=dig}.
//
int DataFileNI::origID2Type( int ic ) const
{
    if( ic >= niCumTypCnt[CniCfg::niTypeXA] )
        return 2;

    if( ic >= niCumTypCnt[CniCfg::niTypeMN] )
        return 1;

    return 0;
}


double DataFileNI::origID2Gain( int ic ) const
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


ChanMapNI DataFileNI::chanMap() const
{
    ChanMapNI chanMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "snsChanMap" )) != kvp.end() )
        chanMap.fromString( it.value().toString() );

    return chanMap;
}


void DataFileNI::subclassParseMetaData()
{
// base class
    sRate           = kvp["niSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toUInt();

// subclass
    niRange.rmin    = kvp["niAiRangeMin"].toDouble();
    niRange.rmax    = kvp["niAiRangeMax"].toDouble();
    mnGain          = kvp["niMNGain"].toDouble();
    maGain          = kvp["niMAGain"].toDouble();

    parseChanCounts();
}


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
//    niEnabled=true                // implicit (enab if file)
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
//    snsChanMapFile=           // snsChanMap string
//    snsSaveChanSubset=all
//    snsRunName=myRun          // outputFile baseName
//
void DataFileNI::subclassStoreMetaData( const DAQ::Params &p )
{
    sRate   = p.ni.srate;

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

    const int   *cum = p.ni.niCumTypCnt;

    kvp["acqMnMaXaDw"] =
        QString("%1,%2,%3,%4")
        .arg( cum[CniCfg::niTypeMN] )
        .arg( cum[CniCfg::niTypeMA] - cum[CniCfg::niTypeMN] )
        .arg( cum[CniCfg::niTypeXA] - cum[CniCfg::niTypeMA] )
        .arg( cum[CniCfg::niTypeXD] - cum[CniCfg::niTypeXA] );

    kvp["snsChanMap"] =
        p.sns.niChans.chanMap.toString( p.sns.niChans.saveBits );

    if( p.sns.niChans.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = p.sns.niChans.uiSaveChanStr;
        Subset::bits2Vec( chanIds, p.sns.niChans.saveBits );
    }
    else {

        kvp["snsSaveChanSubset"] = "all";
        Subset::defaultVec( chanIds, nSavedChans );
    }

    subclassSetSNSChanCounts( &p, 0 );
}


int DataFileNI::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.ni.niCumTypCnt[CniCfg::niSumAll];
}


int DataFileNI::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.sns.niChans.saveBits.count( true );

    return nSaved;
}


// snsMnMaXaDw = saved stream channel counts.
//
void DataFileNI::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *cum;

    if( p )
        cum = reinterpret_cast<const uint*>(p->ni.niCumTypCnt);
    else
        cum = reinterpret_cast<const uint*>(((DataFileNI*)dfSrc)->niCumTypCnt);

    int niEachTypeCnt[CniCfg::niNTypes],
        i = 0,
        n = chanIds.size();

    memset( niEachTypeCnt, 0, CniCfg::niNTypes*sizeof(int) );

    while( i < n && chanIds[i] < cum[CniCfg::niTypeMN] ) {
        ++niEachTypeCnt[CniCfg::niTypeMN];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CniCfg::niTypeMA] ) {
        ++niEachTypeCnt[CniCfg::niTypeMA];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CniCfg::niTypeXA] ) {
        ++niEachTypeCnt[CniCfg::niTypeXA];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CniCfg::niTypeXD] ) {
        ++niEachTypeCnt[CniCfg::niTypeXD];
        ++i;
    }

    kvp["snsMnMaXaDw"] =
        QString("%1,%2,%3,%4")
        .arg( niEachTypeCnt[CniCfg::niTypeMN] )
        .arg( niEachTypeCnt[CniCfg::niTypeMA] )
        .arg( niEachTypeCnt[CniCfg::niTypeXA] )
        .arg( niEachTypeCnt[CniCfg::niTypeXD] );
}


void DataFileNI::subclassListSavChans(
    QVector<uint>       &v,
    const DAQ::Params   &p )
{
    Subset::bits2Vec( v, p.sns.niChans.saveBits );
}


// acqMnMaXaDw = acquired stream channel counts.
//
void DataFileNI::parseChanCounts()
{
    const QStringList   sl = kvp["acqMnMaXaDw"].toString().split(
                                QRegExp("^\\s*|\\s*,\\s*"),
                                QString::SkipEmptyParts );

// --------------------------------
// First count each type separately
// --------------------------------

    niCumTypCnt[CniCfg::niTypeMN] = sl[0].toInt();
    niCumTypCnt[CniCfg::niTypeMA] = sl[1].toInt();
    niCumTypCnt[CniCfg::niTypeXA] = sl[2].toInt();
    niCumTypCnt[CniCfg::niTypeXD] = sl[3].toInt();

// ---------
// Integrate
// ---------

    for( int i = 1; i < CniCfg::niNTypes; ++i )
        niCumTypCnt[i] += niCumTypCnt[i - 1];
}


