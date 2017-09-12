
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMLF.h"
#include "Subset.h"




// Type = {0=AP, 1=LF, 2=aux}.
//
int DataFileIMLF::origID2Type( int ic ) const
{
    if( ic >= imCumTypCnt[CimCfg::imSumNeural] )
        return 2;

    if( ic >= imCumTypCnt[CimCfg::imSumAP] )
        return 1;

    return 0;
}


double DataFileIMLF::origID2Gain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[CimCfg::imTypeAP];

        if( ic < nAP )
            g = roTbl.e[ic].apgn;
        else if( ic < imCumTypCnt[CimCfg::imSumNeural] )
            g = roTbl.e[ic-nAP].lfgn;
    }

    return g;
}


// Note: For FVW, map entries must match the saved chans.
//
ChanMap* DataFileIMLF::chanMap() const
{
    ChanMapIM   *chanMap = new ChanMapIM;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsChanMap" )) != kvp.end() )
        chanMap->fromString( it.value().toString() );

    return chanMap;
}


// - sRate is this substream.
// - nSavedChans is this substream.
//
void DataFileIMLF::subclassParseMetaData()
{
// base class
    _vRange.rmin    = kvp["imAiRangeMin"].toDouble();
    _vRange.rmax    = kvp["imAiRangeMax"].toDouble();
    sRate           = kvp["imSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toUInt();

// subclass
    parseChanCounts();
    roTbl.fromString( kvp["~imroTbl"].toString() );
}


// Notes
// -----
// - imCumTypCnt[] is addressed by full chanIDs across all imec types.
// In AP and LF files these counts match and are original acq counts.
// - roTbl[] is addressed by full chanID so is whole table in both files.
// - chanMap is intersection of what's saved and this file's substream.
// AP file gets saved AP+SY, LF file gets saved LF+SY. We don't force SY
// to be included among the saveBits.
// - imSampRate is the substream rate.
// - chanIDs is subset in this substream.
// - snsSaveChanSubset is this substream.
//
void DataFileIMLF::subclassStoreMetaData( const DAQ::Params &p )
{
    sRate   = p.im.all.srate / 12;

    kvp["typeThis"]     = "imec";
    kvp["imAiRangeMin"] = p.im.all.range.rmin;
    kvp["imAiRangeMax"] = p.im.all.range.rmax;
    kvp["imSampRate"]   = sRate;
    kvp["imRoFile"]     = p.im.imroFile;
    kvp["imStdby"]      = p.im.stdbyStr;
    kvp["imHpFlt"]      = CimCfg::idxToFlt( p.im.hpFltIdx );
    kvp["imDoGainCor"]  = p.im.doGainCor;
    kvp["imNoLEDs"]     = p.im.noLEDs;
    kvp["imSoftStart"]  = p.im.all.softStart;
    kvp["~imroTbl"]     = p.im.roTbl.toString();

    const CimCfg::IMVers    &imVers = mainApp()->cfgCtl()->imVers;

    kvp["imVersHwr"]    = imVers.hwr;
    kvp["imVersBs"]     = imVers.bas;
    kvp["imVersAPI"]    = imVers.api;
    kvp["imProbeSN"]    = imVers.prb[iProbe].sn;
    kvp["imProbeOpt"]   = imVers.prb[iProbe].opt;

    const int   *cum = p.im.all.imCumTypCnt;

    kvp["acqApLfSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

    QBitArray   lfBits;

    p.lfSaveBits( lfBits );
    Subset::bits2Vec( chanIds, lfBits );

    kvp["~snsChanMap"] = p.sns.imChans.chanMap.toString( lfBits );
    kvp["snsSaveChanSubset"] = Subset::vec2RngStr( chanIds );

    subclassSetSNSChanCounts( &p, 0 );
}


// Currently this is whole stream chan count AP+LF+SY.
//
int DataFileIMLF::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.im.all.imCumTypCnt[CimCfg::imSumAll];
}


int DataFileIMLF::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.lfSaveChanCount();

    return nSaved;
}


// snsApLfSy = saved stream channel counts.
//
void DataFileIMLF::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *cum;

    if( p )
        cum = reinterpret_cast<const uint*>(p->im.all.imCumTypCnt);
    else
        cum = reinterpret_cast<const uint*>(((DataFileIMLF*)dfSrc)->imCumTypCnt);

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


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMLF::subclassUpdateChanMap(
    const DataFile      &other,
    const QVector<uint> &idxOtherChans )
{
    const ChanMapIM *A = dynamic_cast<const ChanMapIM*>(other.chanMap());

    ChanMapIM   B( A->AP, A->LF, A->SY );

    foreach( uint i, idxOtherChans )
        B.e.push_back( A->e[i] );

    kvp["~snsChanMap"] = B.toString();
}


// acqApLfSy = acquired stream channel counts.
//
void DataFileIMLF::parseChanCounts()
{
    const QStringList   sl = kvp["acqApLfSy"].toString().split(
                                QRegExp("^\\s+|\\s*,\\s*"),
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


