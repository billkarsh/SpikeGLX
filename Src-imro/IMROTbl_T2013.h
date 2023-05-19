#ifndef IMROTBL_T2013_H
#define IMROTBL_T2013_H

#include "IMROTbl_T24base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Neuropixels 2.0 multishank probe
//
struct IMROTbl_T2013 : public IMROTbl_T24base
{
    enum imLims_T2013 {
        imType2013Type      = 2013,
        imType2013Refids    = 7,
        imType2013Gain      = 100
    };

    IMROTbl_T2013( const QString &pn )
        :   IMROTbl_T24base(pn, imType2013Type) {}

    virtual int typeConst() const   {return imType2013Type;}
    virtual int tip0refID() const   {return 2;}

    virtual int nRefs() const       {return imType2013Refids;}
    virtual int maxInt() const      {return 2048;}
    virtual double maxVolts() const {return 0.62;}

    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType2013Gain;}
    virtual int lfGain( int /* ch */ ) const        {return imType2013Gain;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType2013Gain;}
    virtual double unityToVolts( double u ) const   {return 1.24*u - 0.62;}
};

#endif  // IMROTBL_T2013_H


