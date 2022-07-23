
#include "IMROTbl_T0.h"
#include "IMROTbl_T1020.h"
#include "IMROTbl_T1030.h"
#include "IMROTbl_T1100.h"
#include "IMROTbl_T1110.h"
#include "IMROTbl_T1200.h"
#include "IMROTbl_T1300.h"
#include "IMROTbl_T21.h"
#include "IMROTbl_T24.h"
#include "IMROTbl_T3A.h"

#include "ShankMap.h"

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
/* IMROTbl -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int IMROTbl::maxBank( int ch, int shank ) const
{
    Q_UNUSED( shank );

    return (nElecPerShank() - ch - 1) / nAP();
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
    vR.push_back( IMRO_ROI( 0, 0, nAP() / nCol() ) );
    return 1;
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
    M.fillDefaultIm( *this );
    qSort( M.e );   // s->r->c

    int nC = nCol();

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

        IMRO_ROI    roi( B0.s, B0.r, B0.r + nrow, -1, -1 );

        // Note: c == nC kicks out last
        for( int c = 0; c <= nC; ++c ) {

            if( pat0 & (1 << c) ) {
                if( roi.c0 < 0 )
                    roi.c0 = c;
                roi.cLim = c + 1;
            }
            else if( roi.c0 >= 0 ) {
                vR.push_back( roi );
                roi.c0 = -1;
            }
        }
    }

    return vR.size();
}


void IMROTbl::edit_strike( tImroSites vS, tconstImroROIs vR ) const
{
    int ncol = nCol();

    for( int b = 0, nb = vR.size(); b < nb; ++b ) {

        const IMRO_ROI  &B = vR[b];

        for( int r = B.r0; r < B.rLim; ++r ) {

            for(
                int c = qMax( 0, B.c0 ),
                cLim  = (B.cLim < 0 ? ncol : B.cLim);
                c < cLim;
                ++c ) {

                edit_strike_1( vS, IMRO_Site( 0, c, r ) );
            }
        }
    }
}

/* ---------------------------------------------------------------- */
/* Allocate ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// How type value, per se, is consulted in the code:
// - CimCfg::ImProbeDat::setProbeType().
// - IMROTbl::alloc().
// - IMROTbl::default_imroLE().
// - IMROEditorLaunch().
// - ImAcqStream::ImAcqStream(), indirectly through maxInt().
//
// Type codes:
//    0:    NP 1.0 SS el 960
//              - PRB_1_4_0480_1 (Silicon cap)
//              - PRB_1_4_0480_1_C (Metal cap)
//              - Sapiens (NHP 10mm SOI 125  with Metal cap)
// 1020:    NHP phase 2 (active) 25 mm, SOI35 el 2496
// 1021:    NHP phase 2 (active) 25 mm, SOI60 el 2496
// 1030:    NHP phase 2 (active) 45 mm, SOI90 el 4416
// 1031:    NHP phase 2 (active) 45 mm, SOI125 el 4416
// 1100:    UHD phase 1 el 384
// 1110:    UHD phase 2 el 6144
// 1200:    NHP 128 channel analog 25mm (type also used for 45mm)
// 1210:    NHP 128 channel analog 45mm [NOT USED]
// 1300:    Opto
//   21:    NP 2.0 SS scrambled el 1280
//              - PRB2_1_2_0640_0   initial
//              - NP2000            later
//   24:    NP 2.0 MS el 1280
//              - PRB2_4_2_0640_0   initial
//              - NP2010            later
//
// Return true if supported.
//
bool IMROTbl::pnToType( int &type, const QString &pn )
{
    bool    supp = false;

    type = 0;           // NP 1.0

// Old codes ---------------------------------
    if( pn.startsWith( "PRB_1_4" ) ) {
        type = 0;       // NP 1.0
        supp = true;
    }
    else if( pn.startsWith( "PRB2_1" ) ) {
        type = 21;      // 2.0 SS
        supp = true;
    }
    else if( pn.startsWith( "PRB2_4" ) ) {
        type = 24;      // 2.0 MS
        supp = true;
    }
// New codes ---------------------------------
    else if( pn == "NP1000" || pn == "NP1001" || pn == "NP1010" ) {
        type = 0;       // NP 1.0, NHP 10mm
        supp = true;
    }
    else if( pn == "NP1020" || pn == "NP1021" ) {
        type = 1020;    // NHP 25mm
        supp = true;
    }
    else if( pn == "NP1030" || pn == "NP1031" ) {
        type = 1030;    // NHP 45mm
        supp = true;
    }
    else if( pn == "NP1100" ) {
        type = 1100;    // UHP 1
        supp = true;
    }
    else if( pn == "NP1110" ) {
        type = 1110;    // UHP 2
        supp = true;
    }
    else if( pn == "NP1200" || pn == "NP1210" ) {
        type = 1200;    // NHP 128 analog
        supp = true;
    }
    else if( pn == "NP1300" ) {
        type = 1300;    // Opto
        supp = true;
    }
    else if( pn == "NP2000" ) {
        type = 21;      // 2.0 SS
        supp = true;
    }
    else if( pn == "NP2010" ) {
        type = 24;      // 2.0 MS
        supp = true;
    }
    else {
        // likely early model 1.0
        supp = true;
    }

    return supp;
}


IMROTbl* IMROTbl::alloc( int type )
{
    switch( type ) {
        case 1020:  return new IMROTbl_T1020;
        case 1030:  return new IMROTbl_T1030;
        case 1100:  return new IMROTbl_T1100;
        case 1110:  return new IMROTbl_T1110;
        case 1200:  return new IMROTbl_T1200;
        case 1300:  return new IMROTbl_T1300;
        case 21:    return new IMROTbl_T21;
        case 24:    return new IMROTbl_T24;
        case -3:    return new IMROTbl_T3A;
        default:    return new IMROTbl_T0;
    }
}


QString IMROTbl::default_imroLE( int type )
{
    switch( type ) {
        case 21: return "*Default (bank 0, ref ext)";
        case 24: return "*Default (shnk 0, bank 0, ref ext)";
        case -3: return "*Default (bank 0, ref ext, gain 500/250)";
        default: return "*Default (bank 0, ref ext, gain 500/250, flt on)";
    }
}


