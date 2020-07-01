
#include "IMROTbl_T0.h"
#include "IMROTbl_T1020.h"
#include "IMROTbl_T1030.h"
#include "IMROTbl_T1100.h"
#include "IMROTbl_T1200.h"
#include "IMROTbl_T3A.h"


IMROTbl* IMROTbl::alloc( int type )
{
    IMROTbl *T = 0;

    switch( type ) {
        case 1020:  T = new IMROTbl_T1020;  break;
        case 1030:  T = new IMROTbl_T1030;  break;
        case 1100:  T = new IMROTbl_T1100;  break;
        case 1200:  T = new IMROTbl_T1200;  break;
        case -3:    T = new IMROTbl_T3A;    break;
        default:    T = new IMROTbl_T0;     break;
    }

    return T;
}


QString IMROTbl::default_imroLE( int type )
{
    switch( type ) {
        case -3: return "*Default (bank 0, ref ext, gain 500/250)"; break;
        default: return "*Default (bank 0, ref ext, gain 500/250, flt on)"; break;
    }
}


