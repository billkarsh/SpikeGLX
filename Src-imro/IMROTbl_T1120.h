#ifndef IMROTBL_T1120_H
#define IMROTBL_T1120_H

#include "IMROTbl_T1100.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// UHD phase 3 (layout 2) 2x192 (4.5um pitch)
//
struct IMROTbl_T1120 : public IMROTbl_T1100
{
    enum imLims_T1120 {
        imType1120Type      = 1120,
        imType1120Col       = 2,
        imType1120Col_smap  = 14
    };

    IMROTbl_T1120() {type=imType1120Type;}

    virtual int typeConst() const       {return imType1120Type;}
    virtual int nCol() const            {return imType1120Col;}
    virtual int nCol_smap() const       {return imType1120Col_smap;}
    virtual int nRow() const            {return imType1100Elec/imType1120Col;}

    virtual void toShankMap( ShankMap &S ) const;
    virtual void toShankMap_saved(
        ShankMap            &S,
        const QVector<uint> &saved,
        int                 offset ) const;

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

// Edit

    virtual int edit_defaultROI( tImroROIs vR ) const;
};

#endif  // IMROTBL_T1120_H


