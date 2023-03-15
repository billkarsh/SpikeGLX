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
        imType1120Type  = 1120
    };

    IMROTbl_T1120( const QString &pn )
        :   IMROTbl_T1100( pn ) {type=imType1120Type;}

    virtual int typeConst() const   {return imType1120Type;}

    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}
};

#endif  // IMROTBL_T1120_H


