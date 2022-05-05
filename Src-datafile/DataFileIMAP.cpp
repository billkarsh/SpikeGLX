
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMAP.h"
#include "Subset.h"




// Type = {0=AP, 1=LF, 2=aux}.
//
int DataFileIMAP::origID2Type( int ic ) const
{
    if( ic >= imCumTypCnt[CimCfg::imTypeLF] )
        return 2;

    if( ic >= imCumTypCnt[CimCfg::imTypeAP] )
        return 1;

    return 0;
}


double DataFileIMAP::origID2Gain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[CimCfg::imTypeAP],
            nNu = imCumTypCnt[CimCfg::imTypeLF];

        if( ic < nAP )
            g = roTbl->apGain( ic );
        else if( ic < nNu || nNu == nAP )
            g = roTbl->lfGain( ic - nAP );
    }

    return g;
}


void DataFileIMAP::locFltRadii( int &rin, int &rout, int iflt ) const
{
    if( roTbl ) {
        roTbl->locFltRadii( rin, rout, iflt );
        return;
    }

    switch( iflt ) {
        case 2:     rin = 2, rout = 8; break;
        default:    rin = 0, rout = 2; break;
    }
}


// Note: For FVW, map entries must match the saved chans.
//
ChanMap* DataFileIMAP::chanMap() const
{
    ChanMapIM   *chanMap = new ChanMapIM;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsChanMap" )) != kvp.end() )
        chanMap->fromString( it.value().toString() );

    return chanMap;
}


// Note: For FVW, map entries must match the saved chans.
//
ShankMap* DataFileIMAP::shankMap() const
{
    ShankMap    *shankMap = new ShankMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsShankMap" )) != kvp.end() )
        shankMap->fromString( it.value().toString() );
    else {
        // Only saved channels
        shankMap->fillDefaultImSaved( *roTbl, chanIds );
    }

    return shankMap;
}


// - sRate is this substream.
// - nSavedChans is this substream.
//
void DataFileIMAP::subclassParseMetaData()
{
// base class
    _vRange.rmin    = kvp["imAiRangeMin"].toDouble();
    _vRange.rmax    = kvp["imAiRangeMax"].toDouble();
    sRate           = kvp["imSampRate"].toDouble();
    nSavedChans     = kvp["nSavedChans"].toInt();

// subclass
    parseChanCounts();

    int type = -3;
    if( kvp.contains( "imDatPrb_type" ) )
        type = kvp["imDatPrb_type"].toInt();

    roTbl = IMROTbl::alloc( type );
    roTbl->fromString( 0, kvp["~imroTbl"].toString() );
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
void DataFileIMAP::subclassStoreMetaData( const DAQ::Params &p )
{
    const CimCfg::ImProbeTable  &T  = mainApp()->cfgCtl()->prbTab;
    const CimCfg::ImProbeDat    &P  = T.get_iProbe( ip );
    const CimCfg::PrbEach       &E  = p.im.prbj[ip];

    sRate = E.srate;

    kvp["typeThis"]         = "imec";
    kvp["imSampRate"]       = sRate;
    kvp["imCalibrated"]     = (p.im.prbAll.calPolicy < 2) && (P.cal == 1);
    kvp["imSvySecPerBnk"]   = p.im.prbAll.svySecPerBnk;
    kvp["imTrgSource"]      = p.im.prbAll.trgSource;
    kvp["imTrgRising"]      = p.im.prbAll.trgRising;
    kvp["imRoFile"]         = E.imroFile;
    kvp["imStdby"]          = E.stdbyStr;
    kvp["imSvyMaxBnk"]      = E.svyMaxBnk;
    kvp["imLEDEnable"]      = E.LEDEnable;
    kvp["imAiRangeMin"]     = -E.roTbl->maxVolts();
    kvp["imAiRangeMax"]     = E.roTbl->maxVolts();
    kvp["imMaxInt"]         = E.roTbl->maxInt();
    kvp["~imroTbl"]         = E.roTbl->toString();

    kvp["imDatApi"]         = T.api;
    kvp["imDatBs_fw"]       = T.slot2Vers[P.slot].bsfw;
    kvp["imDatBsc_pn"]      = T.slot2Vers[P.slot].bscpn;
    kvp["imDatBsc_sn"]      = T.slot2Vers[P.slot].bscsn;
    kvp["imDatBsc_hw"]      = T.slot2Vers[P.slot].bschw;
    kvp["imDatBsc_fw"]      = T.slot2Vers[P.slot].bscfw;
    kvp["imDatHs_pn"]       = P.hspn;
    kvp["imDatHs_sn"]       = P.hssn;
    kvp["imDatHs_hw"]       = P.hshw;
    kvp["imDatFx_pn"]       = P.fxpn;
    kvp["imDatFx_sn"]       = P.fxsn;
    kvp["imDatFx_hw"]       = P.fxhw;
    kvp["imDatPrb_dock"]    = P.dock;
    kvp["imDatPrb_pn"]      = P.pn;
    kvp["imDatPrb_port"]    = P.port;
    kvp["imDatPrb_slot"]    = P.slot;
    kvp["imDatPrb_sn"]      = P.sn;
    kvp["imDatPrb_type"]    = P.type;

    kvp["syncImInputSlot"]  = p.sync.imPXIInputSlot;

    const int   *cum = E.imCumTypCnt;

    kvp["acqApLfSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

    QBitArray   apBits;

    E.apSaveBits( apBits );
    Subset::bits2Vec( chanIds, apBits );

    kvp["~snsShankMap"]         = E.sns.shankMap.toString( apBits );
    kvp["~snsChanMap"]          = E.sns.chanMap.toString( apBits );
    kvp["snsSaveChanSubset"]    = Subset::vec2RngStr( chanIds );

    subclassSetSNSChanCounts( &p, 0 );
}


// Currently this is whole stream chan count AP+LF+SY.
//
int DataFileIMAP::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.stream_nChans( 2, ip );
}


int DataFileIMAP::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.im.prbj[ip].apSaveChanCount();

    return nSaved;
}


// snsApLfSy = saved stream channel counts.
//
void DataFileIMAP::subclassSetSNSChanCounts(
    const DAQ::Params   *p,
    const DataFile      *dfSrc )
{
// ------------------------
// Sum each type separately
// ------------------------

    const uint  *cum;

    if( p )
        cum = reinterpret_cast<const uint*>(p->im.prbj[ip].imCumTypCnt);
    else
        cum = reinterpret_cast<const uint*>(((DataFileIMAP*)dfSrc)->imCumTypCnt);

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
void DataFileIMAP::subclassUpdateShankMap(
    const DataFile      &other,
    const QVector<uint> &idxOtherChans )
{
    const ShankMap  *A  = other.shankMap();
    const uint      n   = A->e.size();

    ShankMap    B( A->ns, A->nc, A->nr );

    foreach( uint i, idxOtherChans ) {

        if( i < n ) // Not Sync chan!
            B.e.push_back( A->e[i] );
    }

    kvp["~snsShankMap"] = B.toString();
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMAP::subclassUpdateChanMap(
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
void DataFileIMAP::parseChanCounts()
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


