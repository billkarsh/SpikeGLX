
#include "IMROTbl_T1122.h"
#include "ShankMap.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1122::toShankMap( ShankMap &S ) const
{
    IMROTbl::toShankMap( S );
    S.nc = imType1122Col_smap;
}


void IMROTbl_T1122::toShankMap_saved(
    ShankMap            &S,
    const QVector<uint> &saved,
    int                 offset ) const
{
    IMROTbl::toShankMap_saved( S, saved, offset );
    S.nc = imType1122Col_smap;
}


void IMROTbl_T1122::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 16, rout = 64; break;
        default:    rin = 8,  rout = 16; break;
    }
}


// Return ROI count.
//
int IMROTbl_T1122::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, nRow(), 0, nCol() ) );
    return 1;
}


