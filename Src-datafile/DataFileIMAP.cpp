
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMAP.h"
#include "Subset.h"




int DataFileIMAP::numNeuralChans() const
{
    QStringList sl = kvp["snsApLfSy"].toString().split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );
    return sl[0].toInt();
}


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
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


ShankMap* DataFileIMAP::shankMap_svy( int shank, int bank )
{
    ShankMap    *shankMap = new ShankMap;

    roTbl->fillShankAndBank( shank, bank );
    roTbl->toShankMap_vis( *shankMap );

    return shankMap;
}


// Note: For FVW, map entries must match the saved chans.
//
// There are only two clients for this:
// (1) forExport = true:
//          File based ShankMaps are obsolete. If already there
//          we will maintain it. If not there, do nothing.
// (1) forExport = false:
//          FVW wants a visual map. We will always generate this,
//          then graft use flags from whatever file map we have.
//
ShankMap* DataFileIMAP::shankMap( bool forExport ) const
{
    ShankMap    *shankMap = new ShankMap;

    KVParams::const_iterator    it;

    if( forExport ) {
        if( (it = kvp.find( "~snsShankMap" )) != kvp.end() )
            shankMap->fromString( it.value().toString() );
        else {
            delete shankMap;
            shankMap = 0;
        }
    }
    else {
        // generate

        roTbl->toShankMap_snsFileChans( *shankMap, snsFileChans, 0 );

        // graft use flags

        GeomMap    *G = geomMap( false );

        if( G ) {
            for( int ie = 0, ne = G->e.size(); ie < ne; ++ie )
                shankMap->e[ie].u = G->e[ie].u;
            delete G;
        }
        else {
            ShankMap    *S = new ShankMap;

            if( (it = kvp.find( "~snsShankMap" )) != kvp.end() ) {

                S->fromString( it.value().toString() );

                for( int ie = 0, ne = S->e.size(); ie < ne; ++ie )
                    shankMap->e[ie].u = S->e[ie].u;
            }

            delete S;
        }
    }

    return shankMap;
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

    QString pn( "Probe3A" );
    if( kvp.contains( "imDatPrb_pn" ) )
        pn = kvp["imDatPrb_pn"].toString();

    roTbl = IMROTbl::alloc( pn );
    roTbl->fromString( 0, kvp["~imroTbl"].toString() );
}


// Notes
// -----
// - imCumTypCnt[] enumerates full acq chan counts.
// In AP and LF files these counts match and are original acq counts.
// - roTbl[] is addressed by acq chan ID so is whole table in both files.
// - chanMap is intersection of what's saved and this file's substream.
// AP file gets saved AP+SY, LF file gets saved LF+SY. We don't force SY
// to be included among the saveBits.
// - imSampRate is the substream rate.
// - snsFileChans is subset in this substream.
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
    kvp["imTrgSource"]      = p.im.prbAll.trgSource;
    kvp["imTrgRising"]      = p.im.prbAll.trgRising;
    kvp["imSvySecPerBnk"]   = p.im.prbAll.svySecPerBnk;
    kvp["imIsSvyRun"]       = p.im.prbAll.isSvyRun;
    kvp["imroFile"]         = E.imroFile;
    kvp["imStdby"]          = E.stdbyStr;
    kvp["imSvyMaxBnk"]      = E.svyMaxBnk;
    kvp["imLEDEnable"]      = E.LEDEnable;
    kvp["imAiRangeMin"]     = -E.roTbl->maxVolts();
    kvp["imAiRangeMax"]     = E.roTbl->maxVolts();
    kvp["imMaxInt"]         = E.roTbl->maxInt();
    kvp["~imroTbl"]         = E.roTbl->toString();
    kvp["~muxTbl"]          = E.roTbl->muxTable_toString();

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
    Subset::bits2Vec( snsFileChans, apBits );

    kvp["~snsShankMap"]         = E.sns.shankMap.toString( apBits, 0 );
    kvp["~snsChanMap"]          = E.sns.chanMap.toString( apBits );
    kvp["snsSaveChanSubset"]    = Subset::vec2RngStr( snsFileChans );

    subclassSetSNSChanCounts( &p, 0 );
}


// Currently this is whole stream chan count AP+LF+SY.
//
int DataFileIMAP::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.stream_nChans( jsIM, ip );
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

    const int   *cum;

    if( p )
        cum = p->im.prbj[ip].imCumTypCnt;
    else
        cum = dfSrc->cumTypCnt();

    int imEachTypeCnt[CimCfg::imNTypes],
        i = 0,
        n = snsFileChans.size();

    memset( imEachTypeCnt, 0, CimCfg::imNTypes*sizeof(int) );

    while( i < n && snsFileChans[i] < cum[CimCfg::imTypeAP] ) {
        ++imEachTypeCnt[CimCfg::imTypeAP];
        ++i;
    }

    while( i < n && snsFileChans[i] < cum[CimCfg::imTypeLF] ) {
        ++imEachTypeCnt[CimCfg::imTypeLF];
        ++i;
    }

    while( i < n && snsFileChans[i] < cum[CimCfg::imTypeSY] ) {
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
// GeomMap is not used within SpikeGLX. We get it for only
// two reasons:
// (1) forExport = true:
//      Export needs to update an existing map's entry count.
//      If there isn't one, export will create one, and then
//      graft use flags from file's ShankMap.
// (2) forExport = false:
//      FVW is getting a visual ShankMap. Since this is generated,
//      the use flags are obtained either from an existing GeomMap
//      or ShankMap.
//
GeomMap* DataFileIMAP::geomMap( bool forExport ) const
{
    GeomMap *geomMap = new GeomMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsGeomMap" )) != kvp.end() )
        geomMap->fromString( it.value().toString() );
    else if( forExport ) {
//@OBX Need geom map generate from file data here
//        // generate
//        roTbl->toGeomMap_saved( *geomMap, snsFileChans, 0 );

//        // graft use flags
//        ShankMap    *S = shankMap( false );
//        if( S ) {
//            for( int ie = 0, ne = S->e.size(); ie < ne; ++ie )
//                geomMap->e[ie].u = S->e[ie].u;
//            delete S;
//        }
    }

    if( !geomMap->e.size() ) {
        delete geomMap;
        geomMap = 0;
    }

    return geomMap;
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMAP::subclassUpdateGeomMap(
    const DataFile      &dfSrc,
    const QVector<uint> &indicesOfSrcChans )
{
    const GeomMap   *A =
    (reinterpret_cast<const DataFileIMAP*>(&dfSrc))->geomMap( true );

    if( A ) {

        GeomMap B( A->pn, A->ns, A->wd );
        const uint  srcLim = qMin( int(A->e.size()), dfSrc.numNeuralChans() );

        foreach( uint i, indicesOfSrcChans ) {

            if( i < srcLim )
                B.e.push_back( A->e[i] );
        }

        kvp["~snsGeomMap"] = B.toString();
        delete A;
    }
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMAP::subclassUpdateShankMap(
    const DataFile      &dfSrc,
    const QVector<uint> &indicesOfSrcChans )
{
    const ShankMap  *A  = dfSrc.shankMap( true );

    if( A ) {

        ShankMap    B( A->ns, A->nc, A->nr );
        const uint  srcLim = qMin( int(A->e.size()), dfSrc.numNeuralChans() );

        foreach( uint i, indicesOfSrcChans ) {

            if( i < srcLim )
                B.e.push_back( A->e[i] );
        }

        kvp["~snsShankMap"] = B.toString();
        delete A;
    }
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMAP::subclassUpdateChanMap(
    const DataFile      &dfSrc,
    const QVector<uint> &indicesOfSrcChans )
{
    const ChanMapIM *A = dynamic_cast<const ChanMapIM*>(dfSrc.chanMap());

    ChanMapIM   B( A->AP, A->LF, A->SY );

    foreach( uint i, indicesOfSrcChans )
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


