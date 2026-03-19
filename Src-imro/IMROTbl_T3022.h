#ifndef IMROTBL_T3022_H
#define IMROTBL_T3022_H

#include "IMROTbl_T3020base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Neuropixels NXT pre-alpha multishank silicon cap
//
struct IMROTbl_T3022 : public IMROTbl_T3020base
{
    enum imLims_T3022 {
        imType3022Type  = 3022
    };

    IMROTbl_T3022( const QString &pn )
        :   IMROTbl_T3020base(pn, imType3022Type)   {}

    virtual int typeConst() const   {return imType3022Type;}
    virtual int probeTech() const   {return t_tech_nxt_pa;}
};

#endif  // IMROTBL_T3022_H


