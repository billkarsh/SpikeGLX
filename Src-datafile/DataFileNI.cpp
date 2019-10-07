
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


// Note: For FVW, map entries must match the saved chans.
//
ChanMap* DataFileNI::chanMap() const
{
    ChanMapNI   *chanMap = new ChanMapNI;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsChanMap" )) != kvp.end() )
        chanMap->fromString( it.value().toString() );

    return chanMap;
}


// Note: For FVW, map entries must match the saved chans.
//
ShankMap* DataFileNI::shankMap() const
{
    ShankMap    *shankMap = new ShankMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsShankMap" )) != kvp.end() )
        shankMap->fromString( it.value().toString() );
    else {
        // Only saved channels
        shankMap->fillDefaultNiSaved( niCumTypCnt[CniCfg::niTypeMN], chanIds );
    }

    return shankMap;
}


void DataFileNI::subclassParseMetaData()
{
// base class
    _vRange.rmin    = kvp["niAiRangeMin"].toDouble();
    _vRange.rmax    = kvp["niAiRangeMax"].toDouble();
    sRate           = kvp["niSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toUInt();

// subclass
    mnGain          = kvp["niMNGain"].toDouble();
    maGain          = kvp["niMAGain"].toDouble();

    parseChanCounts();
}


void DataFileNI::subclassStoreMetaData( const DAQ::Params &p )
{
    sRate   = p.ni.srate;

    kvp["typeThis"]             = "nidq";
    kvp["niAiRangeMin"]         = p.ni.range.rmin;
    kvp["niAiRangeMax"]         = p.ni.range.rmax;
    kvp["niSampRate"]           = sRate;
    kvp["niMNGain"]             = p.ni.mnGain;
    kvp["niMAGain"]             = p.ni.maGain;
    kvp["niDev1"]               = p.ni.dev1;
    kvp["niDev1ProductName"]    = CniCfg::getProductName( p.ni.dev1 );
    kvp["niClockSource"]        = p.ni.clockSource;
    kvp["niClockLine1"]         = p.ni.clockLine1;
    kvp["niMNChans1"]           = p.ni.uiMNStr1;
    kvp["niMAChans1"]           = p.ni.uiMAStr1;
    kvp["niXAChans1"]           = p.ni.uiXAStr1;
    kvp["niXDChans1"]           = p.ni.uiXDStr1;
    kvp["niXDBytes1"]           = p.ni.xdBytes1;
    kvp["niMuxFactor"]          = p.ni.muxFactor;
    kvp["niAiTermination"]      = CniCfg::termConfigToString( p.ni.termCfg );
    kvp["niStartEnable"]        = p.ni.startEnable;
    kvp["niStartLine"]          = p.ni.startLine;

    if( p.ni.isDualDevMode ) {

        kvp["niDev2"]               = p.ni.dev2;
        kvp["niDev2ProductName"]    = CniCfg::getProductName( p.ni.dev2 );
        kvp["niClockLine2"]         = p.ni.clockLine2;
        kvp["niMNChans2"]           = p.ni.uiMNStr2();
        kvp["niMAChans2"]           = p.ni.uiMAStr2();
        kvp["niXAChans2"]           = p.ni.uiXAStr2();
        kvp["niXDChans2"]           = p.ni.uiXDStr2();
        kvp["niXDBytes2"]           = p.ni.xdBytes2;
        kvp["niDualDevMode"]        = true;
    }

    kvp["syncNiThresh"]     = p.sync.niThresh;
    kvp["syncNiChanType"]   = p.sync.niChanType;
    kvp["syncNiChan"]       = p.sync.niChan;

    const int   *cum = p.ni.niCumTypCnt;

    kvp["acqMnMaXaDw"] =
        QString("%1,%2,%3,%4")
        .arg( cum[CniCfg::niTypeMN] )
        .arg( cum[CniCfg::niTypeMA] - cum[CniCfg::niTypeMN] )
        .arg( cum[CniCfg::niTypeXA] - cum[CniCfg::niTypeMA] )
        .arg( cum[CniCfg::niTypeXD] - cum[CniCfg::niTypeXA] );

    kvp["~snsShankMap"] =
        p.ni.sns.shankMap.toString( p.ni.sns.saveBits );

    kvp["~snsChanMap"] =
        p.ni.sns.chanMap.toString( p.ni.sns.saveBits );

    if( p.ni.sns.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = p.ni.sns.uiSaveChanStr;
        Subset::bits2Vec( chanIds, p.ni.sns.saveBits );
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
        nSaved = p.ni.sns.saveBits.count( true );

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


// Note: For FVW, map entries must match the saved chans.
//
void DataFileNI::subclassUpdateShankMap(
    const DataFile      &other,
    const QVector<uint> &idxOtherChans )
{
    const ShankMap  *A  = other.shankMap();
    const uint      n   = A->e.size();

    ShankMap    B( A->ns, A->nc, A->nr );

    foreach( uint i, idxOtherChans ) {

        if( i < n ) // Not aux chans!
            B.e.push_back( A->e[i] );
    }

    kvp["~snsShankMap"] = B.toString();
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileNI::subclassUpdateChanMap(
    const DataFile      &other,
    const QVector<uint> &idxOtherChans )
{
    const ChanMapNI *A = dynamic_cast<const ChanMapNI*>(other.chanMap());

    ChanMapNI   B( A->MN, A->MA, A->C, A->XA, A->XD );

    foreach( uint i, idxOtherChans )
        B.e.push_back( A->e[i] );

    kvp["~snsChanMap"] = B.toString();
}


// acqMnMaXaDw = acquired stream channel counts.
//
void DataFileNI::parseChanCounts()
{
    const QStringList   sl = kvp["acqMnMaXaDw"].toString().split(
                                QRegExp("^\\s+|\\s*,\\s*"),
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


