
#include "IMROTbl_T2003.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// refid [0]    ext, shank=0, bank=0.
// refid [1]    gnd, shank=0, bank=0.
// refid [2]    tip, shank=0, bank=0.
//
int IMROTbl_T2003::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    shank = 0;
    bank  = 0;

    if( rid == 0 )
        return 0;
    else if( rid == 1 )
        return 3;

    return 1;
}


