#ifndef IMROTBL_T0_H
#define IMROTBL_T0_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMROTbl_T0 : public IMROTbl_T0base
{
    enum imLims_T0 {
        imType0Type     = 0,
        imType0Elec     = 960,
        imType0Banks    = 3,
        imType0Refids   = 5
    };

    IMROTbl_T0()            {type=imType0Type;}
    virtual ~IMROTbl_T0()   {}

    virtual int typeConst() const   {return imType0Type;}
    virtual int nBanks() const      {return imType0Banks;}
    virtual int nRefs() const       {return imType0Refids;}
    virtual int nRow() const        {return imType0Elec/2;}
    virtual int nElec() const       {return imType0Elec;}
};

#endif  // IMROTBL_T0_H



