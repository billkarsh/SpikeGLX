#ifndef IMROTBL_T21_H
#define IMROTBL_T21_H

#include "IMROTbl_T21base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NP 2.0 1-shank
//
struct IMROTbl_T21 : public IMROTbl_T21base
{
    enum imLims_T21 {
        imType21Type    = 21,
        imType21Refids  = 6,
        imType21Gain    = 80
    };

    IMROTbl_T21( const QString &pn )
        :   IMROTbl_T21base(pn, imType21Type)   {}

    virtual int typeConst() const   {return imType21Type;}
    virtual int tip0refID() const   {return 1;}

    virtual int nRefs() const       {return imType21Refids;}
    virtual int maxInt() const      {return 8192;}
    virtual double maxVolts() const {return 0.5;}

    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType21Gain;}
    virtual int lfGain( int /* ch */ ) const        {return imType21Gain;}

    virtual bool chIsRef( int ch ) const            {return ch == 127;}
    virtual int idxToGain( int /* idx */ ) const    {return imType21Gain;}
    virtual double unityToVolts( double u ) const   {return 1.0*u - 0.5;}
};

#endif  // IMROTBL_T21_H


