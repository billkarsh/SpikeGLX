
#include "IMROTbl_T1121.h"
#include "ShankMap.h"

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1121::toShankMap( ShankMap &S ) const
{
    IMROTbl::toShankMap( S );
    S.nc = imType1121Col_smap;
}


void IMROTbl_T1121::toShankMap_saved(
    ShankMap            &S,
    const QVector<uint> &saved,
    int                 offset ) const
{
    IMROTbl::toShankMap_saved( S, saved, offset );
    S.nc = imType1121Col_smap;
}


void IMROTbl_T1121::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 16; rout = 64; break;
        default:    rin = 8;  rout = 16; break;
    }
}


// Return ROI count.
//
int IMROTbl_T1121::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, nRow(), 0, nCol() ) );
    return 1;
}


