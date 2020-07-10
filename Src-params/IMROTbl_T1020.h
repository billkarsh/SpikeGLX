#ifndef IMROTBL_T1020_H
#define IMROTBL_T1020_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMROTbl_T1020 : public IMROTbl_T0base
{
    enum imLims_T1020 {
        imType1020Type      = 1020,
        imType1020Elec      = 2496,
        imType1020Banks     = 7,
        imType1020Refids    = 9
    };

    IMROTbl_T1020()             {type=imType1020Type;}
    virtual ~IMROTbl_T1020()    {}

    virtual int typeConst() const   {return imType1020Type;}
    virtual int nBanks() const      {return imType1020Banks;}
    virtual int nRefs() const       {return imType1020Refids;}
    virtual int nRow() const        {return imType1020Elec/2;}
    virtual int nElec() const       {return imType1020Elec;}
};

#endif  // IMROTBL_T1020_H



