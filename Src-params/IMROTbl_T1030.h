#ifndef IMROTBL_T1030_H
#define IMROTBL_T1030_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMROTbl_T1030 : public IMROTbl_T0base
{
    enum imLims_T1030 {
        imType1030Type      = 1030,
        imType1030Elec      = 4416,
        imType1030Banks     = 12,
        imType1030Refids    = 14
    };

    IMROTbl_T1030()             {type=imType1030Type;}
    virtual ~IMROTbl_T1030()    {}

    virtual int typeConst() const   {return imType1030Type;}
    virtual int nBanks() const      {return imType1030Banks;}
    virtual int nRefs() const       {return imType1030Refids;}

    virtual int nRow() const        {return imType1030Elec/2;}
    virtual int nElec() const       {return imType1030Elec;}
};

#endif  // IMROTBL_T1030_H



