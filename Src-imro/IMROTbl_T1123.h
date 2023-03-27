#ifndef IMROTBL_T1123_H
#define IMROTBL_T1123_H

#include "IMROTbl_T1100.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// UHD phase 3 (layout 4) 12x32 (4.5um pitch)
//
struct IMROTbl_T1123 : public IMROTbl_T1100
{
    enum imLims_T1123 {
        imType1123Type  = 1123
    };

    IMROTbl_T1123( const QString &pn )
        :   IMROTbl_T1100(pn)       {type=imType1123Type;}

    virtual int typeConst() const   {return imType1123Type;}

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}
};

#endif  // IMROTBL_T1123_H


