#ifndef IMROTBL_T2003_H
#define IMROTBL_T2003_H

#include "IMROTbl_T21base.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Neuropixels 2.0 single shank probe
//
struct IMROTbl_T2003 : public IMROTbl_T21base
{
    enum imLims_T2003 {
        imType2003Type      = 2003,
        imType2003Refids    = 3,
        imType2003Gain      = 100
    };

    IMROTbl_T2003( const QString &pn )
        :   IMROTbl_T21base(pn, imType2003Type) {}

    virtual int typeConst() const   {return imType2003Type;}
    virtual int tip0refID() const   {return 2;}

    virtual int nRefs() const       {return imType2003Refids;}
    virtual int maxInt() const      {return 2048;}
    virtual double maxVolts() const {return 0.62;}

    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType2003Gain;}
    virtual int lfGain( int /* ch */ ) const        {return imType2003Gain;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType2003Gain;}
    virtual double unityToVolts( double u ) const   {return 1.24*u - 0.62;}
};

#endif  // IMROTBL_T2003_H


