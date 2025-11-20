
#include "IMROTbl_T24.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */


static int refs[4] = {127,511,895,1279};


// refid [0]      ext, shank=s,    bank=0.
// refid [1..4]   tip, shank=id-1, bank=0.
// refid [5..8]   int, shank=0,    bank=id-5.
// refid [9..12]  int, shank=1,    bank=id-9.
// refid [13..16] int, shank=2,    bank=id-13.
// refid [17..20] int, shank=3,    bank=id-17.
//
int IMROTbl_T24::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    if( rid == 0 ) {
        shank   = e[ch].shnk;
        bank    = 0;
        return 0;
    }
    else if( rid <= imType24baseShanks ) {
        shank   = rid - 1;
        bank    = 0;
        return 1;
    }

    rid -= 5;

    shank   = rid / imType24baseBanks;
    bank    = rid - imType24baseBanks * shank;
    return 2;
}


