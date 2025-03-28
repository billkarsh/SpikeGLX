#ifndef IMROTBL_T3010_H
#define IMROTBL_T3010_H

#include "IMROTbl_T3010base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NXT single shank (Ph 1B)
//
struct IMROTbl_T3010 : public IMROTbl_T3010base
{
    enum imLims_T3010 {
        imType3010Type  = 3010
    };

    IMROTbl_T3010( const QString &pn )
        :   IMROTbl_T3010base(pn, imType3010Type)   {}

    virtual int typeConst() const   {return imType3010Type;}
};

#endif  // IMROTBL_T3010_H


