
#include "IMROTbl_T1123.h"
#include "ShankMap.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1123::toShankMap( ShankMap &S ) const
{
    IMROTbl::toShankMap( S );
    S.nc = imType1123Col_smap;
}


void IMROTbl_T1123::toShankMap_saved(
    ShankMap            &S,
    const QVector<uint> &saved,
    int                 offset ) const
{
    IMROTbl::toShankMap_saved( S, saved, offset );
    S.nc = imType1123Col_smap;
}


void IMROTbl_T1123::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 10, rout = 42; break;
        default:    rin = 5,  rout = 10; break;
    }
}


// Return ROI count.
//
int IMROTbl_T1123::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, nRow(), 0, nCol() ) );
    return 1;
}


