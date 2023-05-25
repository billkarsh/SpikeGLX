
#include "IMROTbl_T0.h"
#include "IMROTbl_T1020.h"
#include "IMROTbl_T1030.h"
#include "IMROTbl_T1100.h"
#include "IMROTbl_T1110.h"
#include "IMROTbl_T1120.h"
#include "IMROTbl_T1121.h"
#include "IMROTbl_T1122.h"
#include "IMROTbl_T1123.h"
#include "IMROTbl_T1200.h"
#include "IMROTbl_T1300.h"
#include "IMROTbl_T21.h"
#include "IMROTbl_T24.h"
#include "IMROTbl_T2003.h"
#include "IMROTbl_T2013.h"
#include "IMROTbl_T3A.h"
#include "GeomMap.h"
#include "ShankMap.h"

#include <QSet>

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#endif


/* ---------------------------------------------------------------- */
/* IMRO_Site ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool IMRO_Site::operator<( const IMRO_Site &rhs ) const
{
    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    if( r < rhs.r )
        return true;

    if( r > rhs.r )
        return false;

    return c < rhs.c;
}

/* ---------------------------------------------------------------- */
/* IMRO_ROI ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMRO_ROI::operator<( const IMRO_ROI &rhs ) const
{
    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    if( r0 < rhs.r0 )
        return true;

    if( r0 > rhs.r0 )
        return false;

    if( rLim < rhs.rLim )
        return true;

    if( rLim > rhs.rLim )
        return false;

    int cme = (c0 >= 0 ? c0 : 0);
    int crh = (rhs.c0 >= 0 ? rhs.c0 : 0);

    if( cme < crh )
        return true;

    if( cme > crh )
        return false;

    cme = (cLim >= 0 ? cLim : INT_MAX);
    crh = (rhs.cLim >= 0 ? rhs.cLim : INT_MAX);

    return cme < crh;
}

/* ---------------------------------------------------------------- */
/* IMROTbl -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMROTbl::IMROTbl( const QString &pn, int type ) : pn(pn), type(type)
{
// Old codes ---------------------------------
    if( pn.startsWith( "PRB_1_4" ) ) {
        // PRB_1_4_0480_1 (Silicon cap)
        // PRB_1_4_0480_1_C (Metal cap)
        _ncolhwr    = 2;
        _ncolvis    = 4;
        col2vis_ev  = {1,3};
        col2vis_od  = {0,2};
        _shankpitch = 0;
        _shankwid   = 70;
        _tiplength  = 209;
        _x0_ev      = 27;
        _x0_od      = 11;
        _xpitch     = 32;
        _zpitch     = 20;
    }
    else if( pn.startsWith( "PRB2_1" ) ) {
        // PRB2_1_2_0640_0
        // NP 2.0 SS scrambled el 1280
        _ncolhwr    = 2;
        _ncolvis    = 2;
        col2vis_ev  = {0,1};
        col2vis_od  = {0,1};
        _shankpitch = 0;
        _shankwid   = 70;
        _tiplength  = 206;
        _x0_ev      = 27;
        _x0_od      = 27;
        _xpitch     = 32;
        _zpitch     = 15;
    }
    else if( pn.startsWith( "PRB2_4" ) ) {
        // PRB2_4_2_0640_0
        // NP 2.0 MS el 1280
        _ncolhwr    = 2;
        _ncolvis    = 2;
        col2vis_ev  = {0,1};
        col2vis_od  = {0,1};
        _shankpitch = 250;
        _shankwid   = 70;
        _tiplength  = 206;
        _x0_ev      = 27;
        _x0_od      = 27;
        _xpitch     = 32;
        _zpitch     = 15;
    }
// New codes ---------------------------------
    else if( pn.startsWith( "NP" ) ) {

        switch( pn.mid( 2 ).toInt() ) {
            case 1000:  // PRB_1_4_0480_1 (Silicon cap)
            case 1001:  // PRB_1_4_0480_1_C (Metal cap)
            case 1010:  // Sapiens (NHP 10mm SOI 125 with Metal cap)
            case 1011:  // 1.0 NHP short wired (with GND wire and tip sharpened)
            case 1012:  // 1.0 NHP short biocompatible packaging (with parylene coating)
            case 1013:  // Neuropixels 1.0 NHP short biocompatible packaging with cap + Neuropixels 1.0 head stage sterilized
                _ncolhwr    = 2;
                _ncolvis    = 4;
                col2vis_ev  = {1,3};
                col2vis_od  = {0,2};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 209;
                _x0_ev      = 27;
                _x0_od      = 11;
                _xpitch     = 32;
                _zpitch     = 20;
                break;
            case 1014:  // [[ unassigned ]]
            case 1015:  // 1.0 NHP short linear
            case 1016:  // Neuropixels 1.0 NHP short linear biocompatible packaging unsterilized with cap
            case 1017:  // Neuropixels 1.0 NHP short linear biocompatible packaging sterilized with cap
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 209;
                _x0_ev      = 27;
                _x0_od      = 27;
                _xpitch     = 32;
                _zpitch     = 20;
                break;
            case 1020:  // NHP phase 2 (active) 25 mm, SOI35 el 2496
            case 1021:  // NHP phase 2 (active) 25 mm, SOI60 el 2496
                _ncolhwr    = 2;
                _ncolvis    = 4;
                col2vis_ev  = {1,3};
                col2vis_od  = {0,2};
                _shankpitch = 0;
                _shankwid   = 125;
                _tiplength  = 373;
                _x0_ev      = 27;
                _x0_od      = 11;
                _xpitch     = 87;
                _zpitch     = 20;
                break;
            case 1022:  // NHP phase 2 (active) 25 mm, SOI115 linear
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 125;
                _tiplength  = 373;
                _x0_ev      = 11;
                _x0_od      = 11;
                _xpitch     = 103;
                _zpitch     = 20;
                break;
            case 1030:  // NHP phase 2 (active) 45 mm, SOI90 el 4416
            case 1031:  // NHP phase 2 (active) 45 mm, SOI125 el 4416
                _ncolhwr    = 2;
                _ncolvis    = 4;
                col2vis_ev  = {1,3};
                col2vis_od  = {0,2};
                _shankpitch = 0;
                _shankwid   = 125;
                _tiplength  = 373;
                _x0_ev      = 27;
                _x0_od      = 11;
                _xpitch     = 87;
                _zpitch     = 20;
                break;
            case 1032:  // NHP phase 2 (active) 45 mm, SOI115 / 125 linear
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 125;
                _tiplength  = 373;
                _x0_ev      = 11;
                _x0_od      = 11;
                _xpitch     = 103;
                _zpitch     = 20;
                break;
            case 1100:  // UHD phase 1 el 384
                _ncolhwr    = 8;
                _ncolvis    = 8;
                col2vis_ev  = {0,1,2,3,4,5,6,7};
                col2vis_od  = {0,1,2,3,4,5,6,7};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 206.5;
                _x0_ev      = 14;
                _x0_od      = 14;
                _xpitch     = 6;
                _zpitch     = 6;
                break;
            case 1110:  // UHD phase 2 el 6144
                _ncolhwr    = 8;
                _ncolvis    = 8;
                col2vis_ev  = {0,1,2,3,4,5,6,7};
                col2vis_od  = {0,1,2,3,4,5,6,7};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 203.5;
                _x0_ev      = 14;
                _x0_od      = 14;
                _xpitch     = 6;
                _zpitch     = 6;
                break;
            case 1120:  // UHD phase 3 (layout 2) 2x192 (4.5um pitch)
                _ncolhwr    = 2;
                _ncolvis    = 14;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 205.75;
                _x0_ev      = 6.75f;
                _x0_od      = 6.75f;
                _xpitch     = 4.5f;
                _zpitch     = 4.5f;
                break;
            case 1121:  // UHD phase 3 (layout 1) 1x384 (3um pitch)
                _ncolhwr    = 1;
                _ncolvis    = 21;
                col2vis_ev  = {0};
                col2vis_od  = {0};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 205.25;
                _x0_ev      = 6.25f;
                _x0_od      = 6.25f;
                _xpitch     = 3;
                _zpitch     = 3;
                break;
            case 1122:  // UHD phase 3 (layout 3) 16x24 (3um pitch)
                _ncolhwr    = 16;
                _ncolvis    = 16;
                col2vis_ev  = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
                col2vis_od  = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 205.25;
                _x0_ev      = 12.5f;
                _x0_od      = 12.5f;
                _xpitch     = 3;
                _zpitch     = 3;
                break;
            case 1123:  // UHD phase 3 (layout 4) 12x32 (4.5um pitch)
                _ncolhwr    = 12;
                _ncolvis    = 12;
                col2vis_ev  = {0,1,2,3,4,5,6,7,8,9,10,11};
                col2vis_od  = {0,1,2,3,4,5,6,7,8,9,10,11};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 205.75;
                _x0_ev      = 10.25f;
                _x0_od      = 10.25f;
                _xpitch     = 4.5f;
                _zpitch     = 4.5f;
                break;
            case 1200:  // NHP 128 channel analog 25mm
            case 1210:  // NHP 128 channel analog 45mm
//@OBX Need true NP1200 xoffsets.
                _ncolhwr    = 2;
                _ncolvis    = 4;
                col2vis_ev  = {1,3};
                col2vis_od  = {0,2};
                _shankpitch = 0;
                _shankwid   = 125;
                _tiplength  = 372;
                _x0_ev      = 54.5;
                _x0_od      = 38.5;
                _xpitch     = 32;
                _zpitch     = 20;
                break;
            case 1300:  // Opto
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 209;
                _x0_ev      = 11;
                _x0_od      = 11;
                _xpitch     = 48;
                _zpitch     = 20;
                break;
            case 2000:  // NP 2.0 SS scrambled el 1280
            case 2003:  // Neuropixels 2.0 single shank probe
            case 2004:  // Neuropixels 2.0 single shank probe with cap
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 206;
                _x0_ev      = 27;
                _x0_od      = 27;
                _xpitch     = 32;
                _zpitch     = 15;
                break;
            case 2010:  // NP 2.0 MS el 1280
            case 2013:  // Neuropixels 2.0 multishank probe
            case 2014:  // Neuropixels 2.0 multishank probe with cap
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 250;
                _shankwid   = 70;
                _tiplength  = 206;
                _x0_ev      = 27;
                _x0_od      = 27;
                _xpitch     = 32;
                _zpitch     = 15;
                break;
            case 2020:  // 2.0 multi shank (Ph 2C)
                _ncolhwr    = 2;
                _ncolvis    = 2;
                col2vis_ev  = {0,1};
                col2vis_od  = {0,1};
                _shankpitch = 250;
                _shankwid   = 70;
                _tiplength  = 206;
                _x0_ev      = 27;
                _x0_od      = 27;
                _xpitch     = 32;
                _zpitch     = 15;
                break;
            case 3000:  // Passive NXT probe
                _ncolhwr    = 1;
                _ncolvis    = 4;
                col2vis_ev  = {3};
                col2vis_od  = {3};
                _shankpitch = 0;
                _shankwid   = 70;
//@OBX Need true NP3000 tiplength.
                _tiplength  = 206;
                _x0_ev      = 53;
                _x0_od      = 53;
                _xpitch     = 15;
                _zpitch     = 15;
                break;
            default:
                // likely early model 1.0
                _ncolhwr    = 2;
                _ncolvis    = 4;
                col2vis_ev  = {1,3};
                col2vis_od  = {0,2};
                _shankpitch = 0;
                _shankwid   = 70;
                _tiplength  = 209;
                _x0_ev      = 27;
                _x0_od      = 11;
                _xpitch     = 32;
                _zpitch     = 20;
                break;
        }
    }
    else if( pn == "Probe3A" ) {
        _ncolhwr    = 2;
        _ncolvis    = 4;
        col2vis_ev  = {1,3};
        col2vis_od  = {0,2};
        _shankpitch = 0;
        _shankwid   = 70;
        _tiplength  = 209;
        _x0_ev      = 27;
        _x0_od      = 11;
        _xpitch     = 32;
        _zpitch     = 20;
    }
    else {
        // likely early model 1.0
        _ncolhwr    = 2;
        _ncolvis    = 4;
        col2vis_ev  = {1,3};
        col2vis_od  = {0,2};
        _shankpitch = 0;
        _shankwid   = 70;
        _tiplength  = 209;
        _x0_ev      = 27;
        _x0_od      = 11;
        _xpitch     = 32;
        _zpitch     = 20;
    }
}


void IMROTbl::toShankMap_hwr( ShankMap &S ) const
{
    S.ns = nShank();
    S.nc = _ncolhwr;
    S.nr = nRow();
    S.e.clear();

    for( int ic = 0, nC = nAP(); ic < nC; ++ic ) {

        int sh, cl, rw, u;

        sh = elShankColRow( cl, rw, ic );
        u  = !chIsRef( ic );

        S.e.push_back( ShankMapDesc( sh, cl, rw, u ) );
    }
}


void IMROTbl::toShankMap_vis( ShankMap &S ) const
{
    S.ns = nShank();
    S.nc = _ncolvis;
    S.nr = nRow();
    S.e.clear();

    for( int ic = 0, nC = nAP(); ic < nC; ++ic ) {

        int sh, cl, rw, u;

        sh = elShankColRow( cl, rw, ic );
        u  = !chIsRef( ic );
        cl = (rw & 1 ? col2vis_od[cl] : col2vis_ev[cl]);

        S.e.push_back( ShankMapDesc( sh, cl, rw, u ) );
    }
}


void IMROTbl::toShankMap_snsFileChans(
    ShankMap            &S,
    const QVector<uint> &saved,
    int                 offset ) const
{
    S.ns = nShank();
    S.nc = _ncolvis;
    S.nr = nRow();
    S.e.clear();

    int nC  = nAP(),
        nI  = qMin( saved.size(), nC );

    for( int i = 0; i < nI; ++i ) {

        int ic, sh, cl, rw, u;

        ic = saved[i] - offset;

        if( ic >= nC )
            break;

        sh = elShankColRow( cl, rw, ic );
        u  = !chIsRef( ic );
        cl = (rw & 1 ? col2vis_od[cl] : col2vis_ev[cl]);

        S.e.push_back( ShankMapDesc( sh, cl, rw, u ) );
    }
}


void IMROTbl::toGeomMap_snsFileChans(
    GeomMap             &G,
    const QVector<uint> &saved,
    int                 offset ) const
{
    G.pn = pn;
    G.ns = nShank();
    G.ds = _shankpitch;
    G.wd = _shankwid;
    G.e.clear();

    int nC  = nAP(),
        nI  = qMin( saved.size(), nC );

    for( int i = 0; i < nI; ++i ) {

        float   x, z;
        int     ic, sh, cl, rw, u;

        ic = saved[i] - offset;

        if( ic >= nC )
            break;

        sh = elShankColRow( cl, rw, ic );
        u  = !chIsRef( ic );
        x  = (rw & 1 ? _x0_od : _x0_ev) + cl * _xpitch;
        z  = rw * _zpitch;

        G.e.push_back( GeomMapDesc( sh, x, z, u ) );
    }
}


int IMROTbl::maxBank( int ch, int shank ) const
{
    Q_UNUSED( shank )
    return (nElecPerShank() - ch - 1) / nAP();
}


bool IMROTbl::anyChanFullBand() const
{
    if( !nLF() )
        return true;

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {
        if( !apFlt( ic ) )
            return true;
    }

    return false;
}


QString IMROTbl::muxTable_toString() const
{
    std::vector<int>    Tbl;
    int                 *T;
    int                 nADC, nGrp;

    muxTable( nADC, nGrp, Tbl );
    T = &Tbl[0];

    QString s = QString("(%1,%2)").arg( nADC ).arg( nGrp );

    for( int irow = 0; irow < nGrp; ++irow ) {

        s += QString("(%1").arg( *T++ );

        for( int icol = 1; icol < nADC; ++icol )
            s += QString(" %1").arg( *T++ );

        s += ")";
    }

    return s;
}

/* ---------------------------------------------------------------- */
/* Hardware ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This method connects one electrode per channel.
//
int IMROTbl::selectSites( int slot, int port, int dock, bool write ) const
{
#ifdef HAVE_IMEC
// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        if( chIsRef( ic ) )
            continue;

        int             shank, bank;
        NP_ErrorCode    err;

        shank = elShankAndBank( bank, ic );

        err = np_selectElectrode( slot, port, dock, ic, shank, bank );

        if( err != SUCCESS )
            return err;
    }

    if( write )
        np_writeProbeConfiguration( slot, port, dock, true );
#endif

    return 0;
}


int IMROTbl::selectRefs( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// ---------------------------------------
// Connect all according to table ref data
// ---------------------------------------

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        int             type, shank, bank;
        NP_ErrorCode    err;

        type = refTypeAndFields( shank, bank, ic );

        err = np_setReference( slot, port, dock, ic,
                shank, channelreference_t(type), bank );

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}


int IMROTbl::selectGains( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// --------------------------------
// Set all according to table gains
// --------------------------------

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = np_setGain( slot, port, dock, ic,
                gainToIdx( apGain( ic ) ),
                gainToIdx( lfGain( ic ) ) );

//---------------------------------------------------------
// Experiment to visualize LF scambling on shankviewer by
// setting every nth gain high and others low.
#if 0
        int apidx, lfidx;

        if( !(ic % 10) ) {
            apidx = R->gainToIdx( 3000 );
            lfidx = R->gainToIdx( 3000 );
        }
        else {
            apidx = R->gainToIdx( 50 );
            lfidx = R->gainToIdx( 50 );
        }

        err = np_setGain( P.slot, P.port, P.dock, ic,
                apidx,
                lfidx );
#endif
//---------------------------------------------------------

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}


int IMROTbl::selectAPFlts( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// ----------------------------------
// Set all according to table filters
// ----------------------------------

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        NP_ErrorCode    err;

        err = np_setAPCornerFrequency( slot, port, dock, ic, !apFlt( ic ) );

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return ROI count.
//
int IMROTbl::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, nAP() / _ncolhwr ) );
    return 1;
}


// - Box count: {1,2,4,...,IMRO_ROI_MAX}.
// - Boxes span shanks.
// - Boxes enclose all AP channels.
// - Canonical attributes all channels.
//
bool IMROTbl::edit_isCanonical( tconstImroROIs vR ) const
{
// Calculate Boxes menu entries

    QSet<int>   boxesMenu = {1};
    IMRO_GUI    G = edit_GUI();
    int         rows_per_box = nRow() / _ncolvis;

    for( int nb = 2; nb <= IMRO_ROI_MAX; nb *= 2 ) {

        if( (rows_per_box / nb) % G.grid == 0 )
            boxesMenu.insert( nb );
        else
            break;
    }

// Check box count

    int nb = vR.size();

    if( !boxesMenu.contains( nb ) )
        return false;

// Assess boxes

    int nr = 0;

    for( int ib = 0; ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = qMax( 0, B.c0 ),
            cL = (B.cLim >= 0 ? B.cLim : _ncolhwr);

        if( c0 != 0 || cL != _ncolhwr )
            return false;

        nr += B.rLim - B.r0;
    }

    return nr * _ncolhwr == nAP() && edit_Attr_canonical();
}


// Scan up for ranges of consecutive like rows.
// Within range, scan across for contiguous 1s.
// That defines ROI boxes.
//
// Return ROI count.
//
int IMROTbl::edit_tbl2ROI( tImroROIs vR ) const
{
    vR.clear();

    ShankMap    M;
    toShankMap_hwr( M );
    qSort( M.e );   // s->r->c

    for( int ie = 0, ne = M.e.size(); ie < ne; ) {

        // Start pat0 and box row-range
        ShankMapDesc    &B0     = M.e[ie++];
        quint32         pat0    = 1 << B0.c;
        int             nrow    = 1;

        // Complete pat0
        while( ie < ne ) {
            ShankMapDesc    E = M.e[ie];
            if( E.s == B0.s && E.r == B0.r ) {
                pat0 |= 1 << E.c;
                ++ie;
            }
            else
                break;
        }

        // ie not yet included

        // Extend consecutive row-range?

        while( ie < ne ) {

            // Consecutive row?
            ShankMapDesc    X0 = M.e[ie];
            int             ix = ie + 1;
            if( X0.s == B0.s && X0.r == B0.r + nrow ) {

                // Start new patX and extension row
                quint32 patX = 1 << X0.c;

                // Complete patX
                while( ix < ne ) {
                    ShankMapDesc    X = M.e[ix];
                    if( X.s == X0.s && X.r == X0.r ) {
                        patX |= 1 << X.c;
                        ++ix;
                    }
                    else
                        break;
                }

                // Successful extension?
                if( patX == pat0 ) {
                    ++nrow;
                    ie = ix;
                }
                else
                    break;
            }
            else
                break;
        }

        // End range extension

        // Split into boxes

        IMRO_ROI    roi( B0.s, B0.r, B0.r + nrow );

        // Note: c == nC kicks out last
        for( int c = 0; c <= _ncolhwr; ++c ) {

            if( pat0 & (1 << c) ) {
                if( roi.c0 < 0 )
                    roi.c0 = c;
                roi.cLim = c;
            }
            else if( roi.c0 >= 0 ) {
                ++roi.cLim;
                vR.push_back( roi );
                roi.c0 = -1;
            }
        }
    }

    return vR.size();
}


void IMROTbl::edit_exclude( tImroSites vX, tconstImroROIs vR ) const
{
    vX.clear();

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = qMax( 0, B.c0 ),
            cL = (B.cLim >= 0 ? B.cLim : _ncolhwr);

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c )
                edit_exclude_1( vX, IMRO_Site( B.s, c, r ) );
        }
    }

    qSort( vX.begin(), vX.end() );
}


// vS sorted s -> r -> c.
//
bool IMROTbl::edit_isAllowed( tconstImroSites vX, const IMRO_ROI &B ) const
{
    for( int ix = 0, nx = vX.size(); ix < nx; ++ix ) {

        const IMRO_Site &X = vX[ix];

        if( X.s == B.s ) {

            if( X.r >= B.r0 )
                return X.r >= B.rLim;
        }
        else if( X.s > B.s )
            break;
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* Allocate ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// How type value, per se, is consulted in the code:
// - CimCfg::ImProbeDat::setProbeType().
// - CimCfg::ImProbeDat::nHSDocks().
// - IMROTbl::default_imroLE().
// - CmdWorker::getStreamSN().
// - CimAcqImec::opto_XXX().
// - CimAcqImec::_xx_calibrateGain().
// - CimAcqImec::_xx_calibrateOpto().
// - IMROEditorLaunch().
// - Dev tab: Display in probe table.
// - IM tab: Copy probe.
//
// Return true if supported.
//
bool IMROTbl::pnToType( int &type, const QString &pn )
{
    bool    supp = false;

    type = 0;       // NP 1.0 SS el 960

// Old codes ---------------------------------
    if( pn.startsWith( "PRB_1_4" ) ) {
        // PRB_1_4_0480_1 (Silicon cap)
        // PRB_1_4_0480_1_C (Metal cap)
        type = 0;
        supp = true;
    }
    else if( pn.startsWith( "PRB2_1" ) ) {
        // PRB2_1_2_0640_0
        // NP 2.0 SS scrambled el 1280
        type = 21;
        supp = true;
    }
    else if( pn.startsWith( "PRB2_4" ) ) {
        // PRB2_4_2_0640_0
        // NP 2.0 MS el 1280
        type = 24;  // 2.0 MS
        supp = true;
    }
// New codes ---------------------------------
    else if( pn.startsWith( "NP" ) ) {

        switch( pn.mid( 2 ).toInt() ) {
            case 1000:  // PRB_1_4_0480_1 (Silicon cap)
            case 1001:  // PRB_1_4_0480_1_C (Metal cap)
            case 1010:  // Sapiens (NHP 10mm SOI 125 with Metal cap)
            case 1011:  // 1.0 NHP short wired (with GND wire and tip sharpened)
            case 1012:  // 1.0 NHP short biocompatible packaging (with parylene coating)
            case 1013:  // Neuropixels 1.0 NHP short biocompatible packaging with cap + Neuropixels 1.0 head stage sterilized
            case 1014:  // [[ unassigned ]]
            case 1015:  // 1.0 NHP short linear
            case 1016:  // Neuropixels 1.0 NHP short linear biocompatible packaging unsterilized with cap
            case 1017:  // Neuropixels 1.0 NHP short linear biocompatible packaging sterilized with cap
                type = 0;
                supp = true;
                break;
            case 1020:  // NHP phase 2 (active) 25 mm, SOI35 el 2496
            case 1021:  // NHP phase 2 (active) 25 mm, SOI60 el 2496
            case 1022:  // NHP phase 2 (active) 25 mm, SOI115 linear
                type = 1020;
                supp = true;
                break;
            case 1030:  // NHP phase 2 (active) 45 mm, SOI90 el 4416
            case 1031:  // NHP phase 2 (active) 45 mm, SOI125 el 4416
            case 1032:  // NHP phase 2 (active) 45 mm, SOI115 / 125 linear
                type = 1030;
                supp = true;
                break;
            case 1100:  // UHD phase 1 el 384
                type = 1100;
                supp = true;
                break;
            case 1110:  // UHD phase 2 el 6144
                type = 1110;
                supp = true;
                break;
            case 1120:  // UHD phase 3 (layout 2) 2x192 (4.5um pitch)
                type = 1120;
                supp = true;
                break;
            case 1121:  // UHD phase 3 (layout 1) 1x384 (3um pitch)
                type = 1121;
                supp = true;
                break;
            case 1122:  // UHD phase 3 (layout 3) 16x24 (3um pitch)
                type = 1122;
                supp = true;
                break;
            case 1123:  // UHD phase 3 (layout 4) 12x32 (4.5um pitch)
                type = 1123;
                supp = true;
                break;
            case 1200:  // NHP 128 channel analog 25mm
            case 1210:  // NHP 128 channel analog 45mm
                type = 1200;
                supp = true;
                break;
            case 1300:  // Opto
                type = 1300;
                supp = true;
                break;
            case 2000:  // NP 2.0 SS scrambled el 1280
                type = 21;
                supp = true;
                break;
            case 2003:  // Neuropixels 2.0 single shank probe
            case 2004:  // Neuropixels 2.0 single shank probe with cap
                type = 2003;
                supp = true;
                break;
            case 2010:  // NP 2.0 MS el 1280
                type = 24;
                supp = true;
                break;
            case 2013:  // Neuropixels 2.0 multishank probe
            case 2014:  // Neuropixels 2.0 multishank probe with cap
                type = 2013;
                supp = true;
                break;
            case 3000:  // Passive NXT probe
                type = 1200;
                supp = true;
                break;
            default:    // likely early model 1.0
                supp = true;
                break;
        }
    }
    else {
        // likely early model 1.0
        supp = true;
    }

    return supp;
}


IMROTbl* IMROTbl::alloc( const QString &pn )
{
// Old codes ---------------------------------
    if( pn.startsWith( "PRB_1_4" ) ) {
        // PRB_1_4_0480_1 (Silicon cap)
        // PRB_1_4_0480_1_C (Metal cap)
        return new IMROTbl_T0( pn );
    }
    else if( pn.startsWith( "PRB2_1" ) ) {
        // PRB2_1_2_0640_0
        // NP 2.0 SS scrambled el 1280
        return new IMROTbl_T21( pn );
    }
    else if( pn.startsWith( "PRB2_4" ) ) {
        // PRB2_4_2_0640_0
        // NP 2.0 MS el 1280
        return new IMROTbl_T24( pn );
    }
// New codes ---------------------------------
    else if( pn.startsWith( "NP" ) ) {

        switch( pn.mid( 2 ).toInt() ) {
            case 1000:  // PRB_1_4_0480_1 (Silicon cap)
            case 1001:  // PRB_1_4_0480_1_C (Metal cap)
            case 1010:  // Sapiens (NHP 10mm SOI 125 with Metal cap)
            case 1011:  // 1.0 NHP short wired (with GND wire and tip sharpened)
            case 1012:  // 1.0 NHP short biocompatible packaging (with parylene coating)
            case 1013:  // Neuropixels 1.0 NHP short biocompatible packaging with cap + Neuropixels 1.0 head stage sterilized
            case 1014:  // [[ unassigned ]]
            case 1015:  // 1.0 NHP short linear
            case 1016:  // Neuropixels 1.0 NHP short linear biocompatible packaging unsterilized with cap
            case 1017:  // Neuropixels 1.0 NHP short linear biocompatible packaging sterilized with cap
                return new IMROTbl_T0( pn );
            case 1020:  // NHP phase 2 (active) 25 mm, SOI35 el 2496
            case 1021:  // NHP phase 2 (active) 25 mm, SOI60 el 2496
            case 1022:  // NHP phase 2 (active) 25 mm, SOI115 linear
                return new IMROTbl_T1020( pn );
            case 1030:  // NHP phase 2 (active) 45 mm, SOI90 el 4416
            case 1031:  // NHP phase 2 (active) 45 mm, SOI125 el 4416
            case 1032:  // NHP phase 2 (active) 45 mm, SOI115 / 125 linear
                return new IMROTbl_T1030( pn );
            case 1100:  // UHD phase 1 el 384
                return new IMROTbl_T1100( pn );
            case 1110:  // UHD phase 2 el 6144
                return new IMROTbl_T1110( pn );
            case 1120:  // UHD phase 3 (layout 2) 2x192 (4.5um pitch)
                return new IMROTbl_T1120( pn );
            case 1121:  // UHD phase 3 (layout 1) 1x384 (3um pitch)
                return new IMROTbl_T1121( pn );
            case 1122:  // UHD phase 3 (layout 3) 16x24 (3um pitch)
                return new IMROTbl_T1122( pn );
            case 1123:  // UHD phase 3 (layout 4) 12x32 (4.5um pitch)
                return new IMROTbl_T1123( pn );
            case 1200:  // NHP 128 channel analog 25mm
            case 1210:  // NHP 128 channel analog 45mm
            case 3000:  // Passive NXT probe
                return new IMROTbl_T1200( pn );
            case 1300:  // Opto
                return new IMROTbl_T1300( pn );
            case 2000:  // NP 2.0 SS scrambled el 1280
                return new IMROTbl_T21( pn );
            case 2003:  // Neuropixels 2.0 single shank probe
            case 2004:  // Neuropixels 2.0 single shank probe with cap
                return new IMROTbl_T2003( pn );
            case 2010:  // NP 2.0 MS el 1280
                return new IMROTbl_T24( pn );
            case 2013:  // Neuropixels 2.0 multishank probe
            case 2014:  // Neuropixels 2.0 multishank probe with cap
                return new IMROTbl_T2013( pn );
            default:
                // likely early model 1.0
                return new IMROTbl_T0( pn );
        }
    }
    else if( pn == "Probe3A" )
        return new IMROTbl_T3A( pn );

// likely early model 1.0
    return new IMROTbl_T0( pn );
}


QString IMROTbl::default_imroLE( int type )
{
    switch( type ) {
        case 21:
        case 2003: return "*Default (bank 0, ref ext)";
        case 24:
        case 2013: return "*Default (shnk 0, bank 0, ref ext)";
        case -3: return "*Default (bank 0, ref ext, gain 500/250)";
        default: return "*Default (bank 0, ref ext, gain 500/250, flt on)";
    }
}


