
#include "DataFileIM.h"
#include "Subset.h"




// Type = {0=AP, 1=LF, 2=aux}.
//
int DataFileIM::origID2Type( int ic ) const
{
    if( ic >= imCumTypCnt[CimCfg::imSumNeural] )
        return 2;

    if( ic >= imCumTypCnt[CimCfg::imSumAP] )
        return 1;

    return 0;
}


double DataFileIM::origID2Gain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        if( ic < imCumTypCnt[CimCfg::imTypeAP] )
            g = 1.0;
        else if( ic < imCumTypCnt[CimCfg::imSumNeural] )
            g = 1.0;

        if( g < 0.01 )
            g = 0.01;
    }

    return g;
}


ChanMapIM DataFileIM::chanMap() const
{
    ChanMapIM chanMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "snsChanMap" )) != kvp.end() )
        chanMap.fromString( it.value().toString() );

    return chanMap;
}


void DataFileIM::subclassParseMetaData()
{
// base class
    sRate           = kvp["imSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toUInt();

// subclass
    parseChanCounts();
}


// To check completeness, here is full list of daq.ini settings.
// No other ini file contains experiment parameters:
//
//    imSampRate=30000
//    imSoftStart=false
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
void DataFileIM::subclassStoreMetaData( const DAQ::Params &p )
{
    sRate   = p.im.srate;

    kvp["imSampRate"]   = sRate;
    kvp["imSoftStart"]  = p.im.softStart;

    const int   *cum = p.im.imCumTypCnt;

    kvp["acqApLfSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

    kvp["snsChanMap"] =
        p.sns.imChans.chanMap.toString( p.sns.imChans.saveBits );

    if( p.sns.imChans.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = p.sns.imChans.uiSaveChanStr;
        Subset::bits2Vec( chanIds, p.sns.imChans.saveBits );
    }
    else {

        kvp["snsSaveChanSubset"] = "all";
        Subset::defaultVec( chanIds, nSavedChans );
    }

    subclassSetSNSChanCounts( &p, 0 );
}


int DataFileIM::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.im.imCumTypCnt[CimCfg::imSumAll];
}


int DataFileIM::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.sns.imChans.saveBits.count( true );

    return nSaved;
}


// Note: The snsChanMap tag stores the original acquired channel set.
// It is independent of snsSaveChanSubset. On the other hand, this
// tag records counts of saved channels in each category, thus the
// binary stream format.
//
void DataFileIM::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *cum;

    if( p )
        cum = reinterpret_cast<const uint*>(p->im.imCumTypCnt);
    else
        cum = reinterpret_cast<const uint*>(((DataFileIM*)dfSrc)->imCumTypCnt);

    int imEachTypeCnt[CimCfg::imNTypes],
        i = 0,
        n = chanIds.size();

    memset( imEachTypeCnt, 0, CimCfg::imNTypes*sizeof(int) );

    while( i < n && chanIds[i] < cum[CimCfg::imTypeAP] ) {
        ++imEachTypeCnt[CimCfg::imTypeAP];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CimCfg::imTypeLF] ) {
        ++imEachTypeCnt[CimCfg::imTypeLF];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CimCfg::imTypeSY] ) {
        ++imEachTypeCnt[CimCfg::imTypeSY];
        ++i;
    }

    kvp["snsApLfSy"] =
        QString("%1,%2,%3")
        .arg( imEachTypeCnt[CimCfg::imTypeAP] )
        .arg( imEachTypeCnt[CimCfg::imTypeLF] )
        .arg( imEachTypeCnt[CimCfg::imTypeSY] );
}


void DataFileIM::subclassListSavChans(
    QVector<uint>       &v,
    const DAQ::Params   &p )
{
    Subset::bits2Vec( v, p.sns.imChans.saveBits );
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
void DataFileIM::parseChanCounts()
{
    const QStringList   sl = kvp["acqApLfSy"].toString().split(
                                QRegExp("^\\s*|\\s*,\\s*"),
                                QString::SkipEmptyParts );

// --------------------------------
// First count each type separately
// --------------------------------

    imCumTypCnt[CimCfg::imTypeAP] = sl[0].toInt();
    imCumTypCnt[CimCfg::imTypeLF] = sl[1].toInt();
    imCumTypCnt[CimCfg::imTypeSY] = sl[2].toInt();

// ---------
// Integrate
// ---------

    for( int i = 1; i < CimCfg::imNTypes; ++i )
        imCumTypCnt[i] += imCumTypCnt[i - 1];
}


