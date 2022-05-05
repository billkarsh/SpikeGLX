#ifndef IMROTBL_T1100_H
#define IMROTBL_T1100_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// UHD phase 1
//
struct IMROTbl_T1100 : public IMROTbl_T0base
{
    enum imLims_T1100 {
        imType1100Type      = 1100,
        imType1100Elec      = 384,
        imType1100Col       = 8,
        imType1100Banks     = 1,
        imType1100Refids    = 2
    };

    IMROTbl_T1100() {type=imType1100Type;}

    virtual int typeConst() const       {return imType1100Type;}
    virtual int nElec() const           {return imType1100Elec;}
    virtual int nElecPerShank() const   {return imType1100Elec;}
    virtual int nCol() const            {return imType1100Col;}
    virtual int nRow() const            {return imType1100Elec/imType1100Col;}
    virtual int nBanks() const          {return imType1100Banks;}
    virtual int nRefs() const           {return imType1100Refids;}

    virtual bool chIsRef( int ) const   {return false;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual int selectSites( int, int, int, bool ) const    {return 0;}
};

#endif  // IMROTBL_T1100_H


