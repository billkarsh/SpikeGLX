
#include "IMROTbl_T1122.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1122::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 16; rout = 64; break;
        default:    rin = 8;  rout = 16; break;
    }
}


