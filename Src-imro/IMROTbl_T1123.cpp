
#include "IMROTbl_T1123.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1123::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 10; rout = 42; break;
        default:    rin = 5;  rout = 10; break;
    }
}


