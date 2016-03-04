
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
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

        int nAP = imCumTypCnt[CimCfg::imTypeAP];

        if( ic < nAP )
            g = roTbl.e[ic].apgn;
        else if( ic < imCumTypCnt[CimCfg::imSumNeural] )
            g = roTbl.e[ic-nAP].lfgn;

        if( g < 50.0 )
            g = 50.0;
    }

    return g;
}


ChanMapIM DataFileIM::chanMap() const
{
    ChanMapIM chanMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsChanMap" )) != kvp.end() )
        chanMap.fromString( it.value().toString() );

    return chanMap;
}


void DataFileIM::subclassParseMetaData()
{
// base class
    sRate           = kvp["imSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toUInt();

// subclass
    imRange.rmin    = kvp["imAiRangeMin"].toDouble();
    imRange.rmax    = kvp["imAiRangeMax"].toDouble();

    parseChanCounts();
    roTbl.fromString( kvp["~imroTbl"].toString() );
}


void DataFileIM::subclassStoreMetaData( const DAQ::Params &p )
{
    sRate   = p.im.srate;

    kvp["typeThis"]     = "imec";
    kvp["imAiRangeMin"] = p.im.range.rmin;
    kvp["imAiRangeMax"] = p.im.range.rmax;
    kvp["imSampRate"]   = sRate;
    kvp["imhpFlt"]      = CimCfg::idxToFlt( p.im.hpFltIdx );
    kvp["imSoftStart"]  = p.im.softStart;
    kvp["~imroTbl"]     = p.im.roTbl.toString();

    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    kvp["imVersHwr"]    = imVers.hwr;
    kvp["imVersBs"]     = imVers.bas;
    kvp["imVersAPI"]    = imVers.api;
    kvp["imProbeSN"]    = imVers.pSN;
    kvp["imProbeOpt"]   = imVers.opt;

    const int   *cum = p.im.imCumTypCnt;

    kvp["acqApLfSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

    kvp["~snsChanMap"] =
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


// snsApLfSy = saved stream channel counts.
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


// acqApLfSy = acquired stream channel counts.
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


