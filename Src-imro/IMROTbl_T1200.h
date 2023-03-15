#ifndef IMROTBL_T1200_H
#define IMROTBL_T1200_H

#include "IMROTbl_T0base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NHP 128 channel analog
//
struct IMROTbl_T1200 : public IMROTbl_T0base
{
    enum imLims_T1200 {
        imType1200Type      = 1200,
        imType1200Elec      = 128,
        imType1200Chan      = 128,
        imType1200Banks     = 1,
        imType1200Refids    = 2
    };

    IMROTbl_T1200( const QString &pn )
        :   IMROTbl_T0base( pn, imType1200Type )    {}

    virtual int typeConst() const       {return imType1200Type;}
    virtual int nElec() const           {return imType1200Elec;}
    virtual int nAP() const             {return imType1200Chan;}
    virtual int nLF() const             {return imType1200Chan;}
    virtual int nBanks() const          {return imType1200Banks;}
    virtual int nRefs() const           {return imType1200Refids;}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectSites( int, int, int, bool ) const    {return 0;}
};

#endif  // IMROTBL_T1200_H


