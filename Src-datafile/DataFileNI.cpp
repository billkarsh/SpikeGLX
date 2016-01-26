
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
//    snsRunName=myRun          // outputFile baseName
//    snsMaxGrfPerTab=0         // N.A.
//    snsSuppressGraphs=false   // N.A.
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

    const int   *type = p.ni.niCumTypCnt;

    kvp["acqApLfMnMaXaDw"] =
        QString("0,0,%1,%2,%3,%4")
        .arg( type[CniCfg::niTypeMN] )
        .arg( type[CniCfg::niTypeMA] - type[CniCfg::niTypeMN] )
        .arg( type[CniCfg::niTypeXA] - type[CniCfg::niTypeMA] )
        .arg( type[CniCfg::niTypeXD] - type[CniCfg::niTypeXA] );

    kvp["snsChanMap"]   = p.sns.chanMap.toString( p.sns.saveBits );

    if( p.sns.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = p.sns.uiSaveChanStr;
        Subset::bits2Vec( chanIds, p.sns.saveBits );
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
        nSaved = p.sns.saveBits.count( true );

    return nSaved;
}


// Note: The snsChanMap tag stores the original acquired channel set.
// It is independent of snsSaveChanSubset. On the other hand, this
// tag records counts of saved channels in each category, thus the
// binary stream format.
//
void DataFileNI::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *type;

    if( p )
        type = reinterpret_cast<const uint*>(p->ni.niCumTypCnt);
    else
        type = reinterpret_cast<const uint*>(((DataFileNI*)dfSrc)->niCumTypCnt);

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


void DataFileNI::subclassListSavChans(
    QVector<uint>       &v,
    const DAQ::Params   &p )
{
    Subset::bits2Vec( v, p.sns.saveBits );
}


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
void DataFileNI::parseChanCounts()
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


