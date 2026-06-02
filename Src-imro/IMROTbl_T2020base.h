#ifndef IMROTBL_T2020BASE_H
#define IMROTBL_T2020BASE_H

#include "IMROTbl.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// 2.0 quad base (Ph 2C)
//
struct IMROTbl_T2020base : public IMROTbl
{
    enum imLims_T2020base {
        imType2020baseElPerShk  = 1280,
        imType2020baseChPerShk  = 384,
        imType2020baseShanks    = 4,
        imType2020baseElec      = imType2020baseShanks * imType2020baseElPerShk,
        imType2020baseChan      = imType2020baseShanks * imType2020baseChPerShk,
        imType2020baseBanks     = 4,
        imType2020baseRefids    = 3,
        imType2020baseGain      = 100
    };

    IMROTbl_T2020base( const QString &pn, int type )
        :   IMROTbl(pn, type)       {}
    virtual ~IMROTbl_T2020base()    {}

    virtual int nElec() const           {return imType2020baseElec;}
    virtual int nShank() const          {return imType2020baseShanks;}
    virtual int nSvyShank() const       {return 1;}
    virtual int nAP() const             {return imType2020baseChan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 4;}
    virtual int nBanks() const          {return imType2020baseBanks;}
    virtual int nChanPerBank() const    {return imType2020baseChPerShk;}
    virtual int nRefs() const           {return imType2020baseRefids;}
    virtual int maxInt() const          {return 2048;}
    virtual double maxVolts() const     {return 0.62;}
    virtual bool needADCCal() const     {return false;}
    virtual int probeTech() const       {return t_tech_qb;}
    virtual int apiFetchType() const    {return t_fetch_qb;}

    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual int apGain( int /* ch */ ) const        {return imType2020baseGain;}
    virtual int lfGain( int /* ch */ ) const        {return imType2020baseGain;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType2020baseGain;}
    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual double unityToVolts( double u ) const   {return 1.24*u - 0.62;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectGains4( const PAddr& ) const  {return 0;}
    virtual int selectAPFlts4( const PAddr& ) const {return 0;}

// Edit

    virtual IMRO_GUI edit_GUI() const;
    virtual IMRO_Attr edit_Attr_def() const;
    virtual IMRO_Attr edit_Attr_cur() const;
    virtual void edit_defaultROI( tImroROIs vR ) const;
    virtual bool edit_isCanonical( tImroROIs vR ) const;
};

#endif  // IMROTBL_T2020BASE_H


