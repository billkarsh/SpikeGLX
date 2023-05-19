#ifndef IMROTBL_T24_H
#define IMROTBL_T24_H

#include "IMROTbl_T24base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NP 2.0 4-shank
//
struct IMROTbl_T24 : public IMROTbl_T24base
{
    enum imLims_T24 {
        imType24Type    = 24,
        imType24Refids  = 18,
        imType24Gain    = 80
    };

    IMROTbl_T24( const QString &pn )
        :   IMROTbl_T24base(pn, imType24Type)   {}

    virtual int typeConst() const   {return imType24Type;}
    virtual int tip0refID() const   {return 1;}

    virtual int nRefs() const       {return imType24Refids;}
    virtual int maxInt() const      {return 8192;}
    virtual double maxVolts() const {return 0.5;}

    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType24Gain;}
    virtual int lfGain( int /* ch */ ) const        {return imType24Gain;}

    virtual bool chIsRef( int ch ) const            {return ch == 127;}
    virtual int idxToGain( int /* idx */ ) const    {return imType24Gain;}
    virtual double unityToVolts( double u ) const   {return 1.0*u - 0.5;}
};

#endif  // IMROTBL_T24_H


