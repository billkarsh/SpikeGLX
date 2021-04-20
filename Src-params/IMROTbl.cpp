
#include "IMROTbl_T0.h"
#include "IMROTbl_T1020.h"
#include "IMROTbl_T1030.h"
#include "IMROTbl_T1100.h"
#include "IMROTbl_T1200.h"
#include "IMROTbl_T21.h"
#include "IMROTbl_T24.h"
#include "IMROTbl_T3A.h"


// How type value, per se, is consulted in the code:
// - CimCfg::ImProbeDat::setProbeType().
// - IMROTbl::alloc().
// - IMROTbl::default_imroLE().
// - IMROEditorLaunch().
// - ImAcqProbe::ImAcqProbe(), indirectly through maxInt().
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
// 1110:    UHD phase 2 el 6000
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
    }
    else if( pn == "NP1200" || pn == "NP1210" ) {
        type = 1200;    // NHP 128 analog
        supp = true;
    }
    else if( pn == "NP1300" ) {
        type = 1300;    // Opto
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
    IMROTbl *T = 0;

    switch( type ) {
        case 1020:  T = new IMROTbl_T1020;  break;
        case 1030:  T = new IMROTbl_T1030;  break;
        case 1100:  T = new IMROTbl_T1100;  break;
        case 1200:  T = new IMROTbl_T1200;  break;
        case 21:    T = new IMROTbl_T21;    break;
        case 24:    T = new IMROTbl_T24;    break;
        case -3:    T = new IMROTbl_T3A;    break;
        default:    T = new IMROTbl_T0;     break;
    }

    return T;
}


QString IMROTbl::default_imroLE( int type )
{
    switch( type ) {
        case 21: return "*Default (bank 0, ref ext)"; break;
        case 24: return "*Default (shnk 0, bank 0, ref ext)"; break;
        case -3: return "*Default (bank 0, ref ext, gain 500/250)"; break;
        default: return "*Default (bank 0, ref ext, gain 500/250, flt on)"; break;
    }
}


