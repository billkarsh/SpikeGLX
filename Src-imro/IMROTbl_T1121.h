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
        imType1121Type  = 1121
    };

    IMROTbl_T1121( const QString &pn )
        :   IMROTbl_T1100(pn)       {type=imType1121Type;}

    virtual int typeConst() const   {return imType1121Type;}

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}
};

#endif  // IMROTBL_T1121_H


