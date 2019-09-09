
#include "IMROTbl_T0.h"
#include "IMROTbl_T21.h"
#include "IMROTbl_T24.h"


IMROTbl* IMROTbl::alloc( int type )
{
    IMROTbl *T = 0;

    switch( type ) {
        case 21:  T = new IMROTbl_T21; break;
        case 24:  T = new IMROTbl_T24; break;
        default:  T = new IMROTbl_T0;  break;
    }

    return T;
}


QString IMROTbl::defaultString( int type )
{
    switch( type ) {
        case 21: return "*Default (bank 0, ref ext)"; break;
        case 24: return "*Default (shnk 0, bank 0, ref ext)"; break;
        default: return "*Default (bank 0, ref ext, gain 500/250, flt on)"; break;
    }
}


