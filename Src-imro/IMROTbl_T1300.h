#ifndef IMROTBL_T1300_H
#define IMROTBL_T1300_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Optopix phase1
//
struct IMROTbl_T1300 : public IMROTbl_T0base
{
    enum imLims_T1300 {
        imType1300Type      = 1300,
        imType1300Elec      = 960,
        imType1300Banks     = 3,
        imType1300Refids    = 5
    };

    int curBlue,
        curRed;

    IMROTbl_T1300( const QString &pn )
        :   IMROTbl_T0base(pn, imType1300Type),
            curBlue(-1), curRed(-1)     {}

    virtual int typeConst() const       {return imType1300Type;}
    virtual int nElec() const           {return imType1300Elec;}
    virtual int nBanks() const          {return imType1300Banks;}
    virtual int nRefs() const           {return imType1300Refids;}
    virtual int nOptoSites() const      {return 14;}
    virtual int probeTech() const       {return t_tech_opto_p1;}

    virtual void optoSetCur( int color, int site );
    virtual int optoGetCur( std::vector<int> &vChan, int color ) const;

private:
    void neighbors( std::vector<int> &vChan, double zE ) const;
};

#endif  // IMROTBL_T1300_H


