#ifndef IMROTBL_T3020_H
#define IMROTBL_T3020_H

#include "IMROTbl_T3020base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NXT multishank (Ph 1B)
//
struct IMROTbl_T3020 : public IMROTbl_T3020base
{
    enum imLims_T3020 {
        imType3020Type  = 3020
    };

    IMROTbl_T3020( const QString &pn )
        :   IMROTbl_T3020base(pn, imType3020Type)   {}

    virtual int typeConst() const   {return imType3020Type;}
};

#endif  // IMROTBL_T3020_H


