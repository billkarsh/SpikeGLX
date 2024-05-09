
#include "IMROTbl_T2013.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// refid [0]      ext, shank=0,    bank=0.
// refid [1]      gnd, shank=0,    bank=0.
// refid [2..5]   tip, shank=id-2, bank=0.
//
int IMROTbl_T2013::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    bank = 0;

    if( rid == 0 ) {
        shank = 0;
        return 0;
    }
    else if( rid == 1 ) {
        shank = 0;
        return 3;
    }

    shank = rid - 2;
    return 1;
}


