
#include "IMRO_Edit.h"
#include "IMROTbl.h"


/* ---------------------------------------------------------------- */
/* IMRO_Edit ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Return ROI count.
//
int IMRO_Edit::defaultROI( std::vector<IMRO_ROI> &R, const IMROTbl &T )
{
    R.clear();
    R.push_back( IMRO_ROI( 0, 0, T.nAP() / T.nCol() ) );
    return 1;
}


