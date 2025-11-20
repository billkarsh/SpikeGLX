
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "DataFileIMLF.h"
#include "Subset.h"

#include <QRegularExpression>




int DataFileIMLF::numNeuralChans() const
{
    QStringList sl = kvp["snsApLfSy"].toString().split(
                        QRegularExpression("^\\s+|\\s*,\\s*"),
                        Qt::SkipEmptyParts );
    int lf = sl[1].toInt();

    return (lf ? lf : sl[0].toInt());
}


// Type = {0=AP, 1=LF, 2=aux}.
//
int DataFileIMLF::origID2Type( int ic ) const
{
    if( ic >= imCumTypCnt[CimCfg::imSumNeural] )
        return 2;

    return 1;
}


double DataFileIMLF::origID2Gain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[CimCfg::imTypeAP],
            nNu = imCumTypCnt[CimCfg::imTypeLF];

        if( ic < nAP )
            g = roTbl->apGain( ic );
        else if( ic < nNu )
            g = roTbl->lfGain( ic - nAP );
    }

    return g;
}


void DataFileIMLF::locFltRadii( int &rin, int &rout, int iflt ) const
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


int DataFileIMLF::svySettleSecs() const
{
    KVParams::const_iterator    it;
    int                         secs = 0;

    if( (it = kvp.find( "imSvySettleSec" )) != kvp.end() )
        secs = it.value().toInt();

    return qMax( secs, 2 );
}


ShankMap* DataFileIMLF::shankMap_svy( int shank, int bank )
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
ShankMap* DataFileIMLF::shankMap( bool forExport ) const
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

        int offset = 0; // AP or LF numbered chans?
        if( snsFileChans[0] >= imCumTypCnt[CimCfg::imTypeAP] )
            offset = imCumTypCnt[CimCfg::imTypeAP];

        roTbl->toShankMap_snsFileChans( *shankMap, snsFileChans, offset );

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
bool DataFileIMLF::subclassParseMetaData( QString *error )
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

    return roTbl->fromString( error, kvp["~imroTbl"].toString() );
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
void DataFileIMLF::subclassStoreMetaData( const DAQ::Params &p )
{
    const CimCfg::ImProbeTable  &T  = mainApp()->cfgCtl()->prbTab;
    const CimCfg::ImProbeDat    &P  = T.get_iProbe( ip );
    const CimCfg::PrbEach       &E  = p.im.prbj[ip];

    sRate = E.srate / 12;

    kvp["typeThis"]         = "imec";
    kvp["imSampRate"]       = sRate;
    kvp["imCalibrated"]     = (p.im.prbAll.calPolicy < 2) && (P.cal == 1);
    kvp["imSRAtDetect"]     = p.im.prbAll.srAtDetect;
    kvp["imLowLatency"]     = p.im.prbAll.lowLatency;
    kvp["imTrgSource"]      = p.im.prbAll.trgSource;
    kvp["imTrgRising"]      = p.im.prbAll.trgRising;
    kvp["imSvySettleSec"]   = p.im.prbAll.svySettleSec;
    kvp["imSvySecPerBnk"]   = p.im.prbAll.svySecPerBnk;
    kvp["imIsSvyRun"]       = p.im.prbAll.isSvyRun;
    kvp["imroFile"]         = E.imroFile;
    kvp["imStdby"]          = E.stdbyStr;
    kvp["imSvyMaxBnk"]      = E.svyMaxBnk;
    kvp["imLEDEnable"]      = E.LEDEnable;
    kvp["imSvyNShanks"]     = E.roTbl->nSvyShank();
    kvp["imAiRangeMin"]     = -E.roTbl->maxVolts();
    kvp["imAiRangeMax"]     = E.roTbl->maxVolts();
    kvp["imMaxInt"]         = E.roTbl->maxInt();
    kvp["imChan0lfGain"]    = E.roTbl->lfGain( 0 );
    kvp["imChan0Ref"]       = E.roTbl->chan0Ref();
    kvp["imTipLength"]      = E.roTbl->tipLength();
    kvp["imColsPerShank"]   = E.roTbl->nCol_hwr();
    kvp["imRowsPerShank"]   = E.roTbl->nRow();
    kvp["imXPitch"]         = E.roTbl->xPitch();
    kvp["imZPitch"]         = E.roTbl->zPitch();
    kvp["imX0EvenRow"]      = E.roTbl->x0EvenRow();
    kvp["imX0OddRow"]       = E.roTbl->x0OddRow();
    kvp["~imroTbl"]         = E.roTbl->toString();
    kvp["~muxTbl"]          = E.roTbl->muxTable_toString();

    kvp["imDatApi"]         = T.api;
    kvp["imDatBs_fw"]       = T.slot2Vers[P.slot].bsfw;
    kvp["imDatBsc_pn"]      = T.slot2Vers[P.slot].bscpn;
    kvp["imDatBsc_sn"]      = T.slot2Vers[P.slot].bscsn;
    kvp["imDatBsc_hw"]      = T.slot2Vers[P.slot].bschw;
    kvp["imDatBsc_fw"]      = T.slot2Vers[P.slot].bscfw;
    kvp["imDatBsc_tech"]    = IMROTbl::strTech( T.slot2Vers[P.slot].bsctech );
    kvp["imDatHs_pn"]       = P.hspn;
    kvp["imDatHs_sn"]       = P.hssn;
    kvp["imDatHs_hw"]       = P.hshw;
    kvp["imDatHs_fw"]       = P.hsfw;
    kvp["imDatFx_pn"]       = P.fxpn;
    kvp["imDatFx_sn"]       = P.fxsn;
    kvp["imDatFx_hw"]       = P.fxhw;
    kvp["imDatPrb_dock"]    = P.dock;
    kvp["imDatPrb_pn"]      = P.pn;
    kvp["imDatPrb_port"]    = P.port;
    kvp["imDatPrb_slot"]    = P.slot;
    kvp["imDatPrb_sn"]      = P.sn;
    kvp["imDatPrb_sr_nok"]  = P.sr_nok;
    kvp["imDatPrb_sr_mask"] = P.sr_mask;
    kvp["imDatPrb_tech"]    = IMROTbl::strTech( P.prbtech );
    kvp["imDatPrb_type"]    = E.roTbl->type;

    kvp["syncImInputSlot"]  = p.sync.imPXIInputSlot;

    const int   *cum = E.imCumTypCnt;

    kvp["acqApLfSy"] =
        QString("%1,%2,%3")
        .arg( cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeLF] - cum[CimCfg::imTypeAP] )
        .arg( cum[CimCfg::imTypeSY] - cum[CimCfg::imTypeLF] );

    GeomMap     G;
    QBitArray   lfBits;

    E.lfSaveBits( lfBits );
    Subset::bits2Vec( snsFileChans, lfBits );

    E.roTbl->toGeomMap_snsFileChans( G, snsFileChans, cum[CimCfg::imTypeAP] );
    G.andOutImStdby( E.stdbyBits, snsFileChans, cum[CimCfg::imTypeAP] );

    kvp["~snsGeomMap"]          = G.toString();
    kvp["~snsChanMap"]          = E.sns.chanMap.toString( lfBits );
    kvp["snsSaveChanSubset"]    = Subset::vec2RngStr( snsFileChans );

    subclassSetSNSChanCounts( &p, 0 );
}


// Currently this is whole stream chan count AP+LF+SY.
//
int DataFileIMLF::subclassGetAcqChanCount( const DAQ::Params &p )
{
    return p.stream_nChans( jsIM, ip );
}


int DataFileIMLF::subclassGetSavChanCount( const DAQ::Params &p )
{
    int nSaved = 0;

    if( subclassGetAcqChanCount( p ) )
        nSaved = p.im.prbj[ip].lfSaveChanCount();

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
GeomMap* DataFileIMLF::geomMap( bool forExport ) const
{
    GeomMap *geomMap = new GeomMap;

    KVParams::const_iterator    it;

    if( (it = kvp.find( "~snsGeomMap" )) != kvp.end() )
        geomMap->fromString( it.value().toString() );
    else if( forExport ) {

        // generate

        roTbl->toGeomMap_snsFileChans(
                *geomMap, snsFileChans, imCumTypCnt[CimCfg::imTypeAP] );

        // graft use flags

        ShankMap    *S = shankMap( false );
        if( S ) {
            for( int ie = 0, ne = S->e.size(); ie < ne; ++ie )
                geomMap->e[ie].u = S->e[ie].u;
            delete S;
        }
    }

    if( !geomMap->e.size() ) {
        delete geomMap;
        geomMap = 0;
    }

    return geomMap;
}


// Note: For FVW, map entries must match the saved chans.
//
void DataFileIMLF::subclassUpdateGeomMap(
    const DataFile      &dfSrc,
    const QVector<uint> &indicesOfSrcChans )
{
    const GeomMap   *A =
    (reinterpret_cast<const DataFileIMLF*>(&dfSrc))->geomMap( true );

    if( A ) {

        GeomMap B( A->pn, A->ns, A->ds, A->wd );
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
void DataFileIMLF::subclassUpdateShankMap(
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
void DataFileIMLF::subclassUpdateChanMap(
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
void DataFileIMLF::parseChanCounts()
{
    const QStringList   sl = kvp["acqApLfSy"].toString().split(
                                QRegularExpression("^\\s+|\\s*,\\s*"),
                                Qt::SkipEmptyParts );

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


