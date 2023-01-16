#ifndef IMROTBL_T1122_H
#define IMROTBL_T1122_H

#include "IMROTbl_T1100.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// UHD phase 3 (layout 3) 16x24 (3um pitch)
//
struct IMROTbl_T1122 : public IMROTbl_T1100
{
    enum imLims_T1122 {
        imType1122Type      = 1122,
        imType1122Col       = 16,
        imType1122Col_smap  = 21
    };

    IMROTbl_T1122() {type=imType1122Type;}

    virtual int typeConst() const       {return imType1122Type;}
    virtual int nCol() const            {return imType1122Col;}
    virtual int nCol_smap() const       {return imType1122Col_smap;}
    virtual int nRow() const            {return imType1100Elec/imType1122Col;}

    virtual void toShankMap( ShankMap &S ) const;
    virtual void toShankMap_saved(
        ShankMap            &S,
        const QVector<uint> &saved,
        int                 offset ) const;

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

// Edit

    virtual int edit_defaultROI( tImroROIs vR ) const;
};

#endif  // IMROTBL_T1122_H


