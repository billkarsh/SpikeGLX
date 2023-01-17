#ifndef IMROTBL_T1121_H
#define IMROTBL_T1121_H

#include "IMROTbl_T1100.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// UHD phase 3 (layout 1) 1x384 (3um pitch)
//
struct IMROTbl_T1121 : public IMROTbl_T1100
{
    enum imLims_T1121 {
        imType1121Type      = 1121,
        imType1121Col       = 1,
        imType1121Col_smap  = 21
    };

    IMROTbl_T1121() {type=imType1121Type;}

    virtual int typeConst() const       {return imType1121Type;}
    virtual int nCol() const            {return imType1121Col;}
    virtual int nCol_smap() const       {return imType1121Col_smap;}
    virtual int nRow() const            {return imType1100Elec/imType1121Col;}

    virtual void toShankMap( ShankMap &S ) const;
    virtual void toShankMap_saved(
        ShankMap            &S,
        const QVector<uint> &saved,
        int                 offset ) const;

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

// Edit

    virtual int edit_defaultROI( tImroROIs vR ) const;
};

#endif  // IMROTBL_T1121_H


