
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileOB.h"
#include "Subset.h"




// Type = {1=AN, 2=DG}.
//
int DataFileOB::origID2Type( int ic ) const
{
    if( ic >= obCumTypCnt[CimCfg::obTypeXA] )
        return 2;

    return 1;
}


void DataFileOB::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2, rout = 8; break;
        default:    rin = 0, rout = 2; break;
    }
}


// Note: For FVW, map entries must match the saved chans.
//
ChanMap* DataFileOB::chanMap() const
{
    ChanMapOB   *chanMap = new ChanMapOB;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsChanMap" )) != kvp.end() )
        chanMap->fromString( it.value().toString() );

    return chanMap;
}


// - sRate is this substream.
// - nSavedChans is this substream.
//
void DataFileOB::subclassParseMetaData()
{
// base class
    _vRange.rmin    = kvp["obAiRangeMin"].toDouble();
    _vRange.rmax    = kvp["obAiRangeMax"].toDouble();
    sRate           = kvp["obSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toInt();

// subclass
    parseChanCounts();
}


void DataFileOB::subclassStoreMetaData( const DAQ::Params &p )
{
    const CimCfg::ImProbeTable  &T  = mainApp()->cfgCtl()->prbTab;
    const CimCfg::ImProbeDat    &P  = T.get_iOnebox( ip );
    const CimCfg::ObxEach       &E  = p.im.obxj[ip];

    sRate = E.srate;

    kvp["typeThis"]     = "obx";
    kvp["obSampRate"]   = sRate;
    kvp["obMaxInt"]     = 32768;
    kvp["imTrgSource"]  = p.im.prbAll.trgSource;
    kvp["imTrgRising"]  = p.im.prbAll.trgRising;
    kvp["obAiRangeMin"] = E.range.rmin;
    kvp["obAiRangeMax"] = E.range.rmax;

    kvp["imDatApi"]         = T.api;
    kvp["imDatBs_fw"]       = T.slot2Vers[P.slot].bsfw;
    kvp["imDatBsc_pn"]      = T.slot2Vers[P.slot].bscpn;
    kvp["imDatBsc_sn"]      = T.slot2Vers[P.slot].bscsn;
    kvp["imDatBsc_hw"]      = T.slot2Vers[P.slot].bschw;
    kvp["imDatBsc_fw"]      = T.slot2Vers[P.slot].bscfw;
    kvp["imDatObx_slot"]    = P.slot;

    const int   *cum = E.obCumTypCnt;

    kvp["acqXaDwSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::obTypeXA] )
        .arg( cum[CimCfg::obTypeXD] - cum[CimCfg::obTypeXA] )
        .arg( cum[CimCfg::obTypeSY] - cum[CimCfg::obTypeXD] );

    kvp["~snsChanMap"] = E.sns.chanMap.toString( E.sns.saveBits );

    if( E.sns.saveBits.count( false ) ) {

        kvp["snsSaveChanSubset"] = E.sns.uiSaveChanStr;
        Subset::bits2Vec( chanIds, E.sns.saveBits );
    }
    else {

        kvp["snsSaveChanSubset"] = "all";
        Subset::defaultVec( chanIds, nSavedChans );
    }

    subclassSetSNSChanCounts( &p, 0 );
}


int DataFileOB::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.stream_nChans( jsOB, ip );
}


int DataFileOB::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.im.obxj[ip].sns.saveBits.count( true );

    return nSaved;
}


// snsXaDwSy = saved stream channel counts.
//
void DataFileOB::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const int   *cum;

    if( p )
        cum = p->im.obxj[ip].obCumTypCnt;
    else
        cum = dfSrc->cumTypCnt();

    int obEachTypeCnt[CimCfg::obNTypes],
        i = 0,
        n = chanIds.size();

    memset( obEachTypeCnt, 0, CimCfg::obNTypes*sizeof(int) );

    while( i < n && chanIds[i] < cum[CimCfg::obTypeXA] ) {
        ++obEachTypeCnt[CimCfg::obTypeXA];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CimCfg::obTypeXD] ) {
        ++obEachTypeCnt[CimCfg::obTypeXD];
        ++i;
    }

    while( i < n && chanIds[i] < cum[CimCfg::obTypeSY] ) {
        ++obEachTypeCnt[CimCfg::obTypeSY];
        ++i;
    }

    kvp["snsXaDwSy"] =
        QString("%1,%2,%3")
        .arg( obEachTypeCnt[CimCfg::obTypeXA] )
        .arg( obEachTypeCnt[CimCfg::obTypeXD] )
        .arg( obEachTypeCnt[CimCfg::obTypeSY] );
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileOB::subclassUpdateChanMap(
    const DataFile      &other,
    const QVector<uint> &idxOtherChans )
{
    const ChanMapOB *A = dynamic_cast<const ChanMapOB*>(other.chanMap());

    ChanMapOB   B( A->XA, A->XD, A->SY );

    foreach( uint i, idxOtherChans )
        B.e.push_back( A->e[i] );

    kvp["~snsChanMap"] = B.toString();
}


// acqXaDwSy = acquired stream channel counts.
//
void DataFileOB::parseChanCounts()
{
    const QStringList   sl = kvp["acqXaDwSy"].toString().split(
                                QRegExp("^\\s+|\\s*,\\s*"),
                                QString::SkipEmptyParts );

// --------------------------------
// First count each type separately
// --------------------------------

    obCumTypCnt[CimCfg::obTypeXA] = sl[0].toInt();
    obCumTypCnt[CimCfg::obTypeXD] = sl[1].toInt();
    obCumTypCnt[CimCfg::obTypeSY] = sl[2].toInt();

// ---------
// Integrate
// ---------

    for( int i = 1; i < CimCfg::obNTypes; ++i )
        obCumTypCnt[i] += obCumTypCnt[i - 1];
}


