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
        imType1122Type  = 1122
    };

    IMROTbl_T1122( const QString &pn )
        :   IMROTbl_T1100( pn ) {type=imType1122Type;}

    virtual int typeConst() const   {return imType1122Type;}

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}
};

#endif  // IMROTBL_T1122_H


