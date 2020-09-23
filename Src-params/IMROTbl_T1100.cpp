
#include "IMROTbl_T1100.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1100::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 8, rout = 32; break;
        default:    rin = 4, rout = 8;  break;
    }
}


