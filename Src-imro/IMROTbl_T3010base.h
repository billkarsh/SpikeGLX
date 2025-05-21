#ifndef IMROTBL_T3010BASE_H
#define IMROTBL_T3010BASE_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T3010base
{
    qint8   bank,
            refid;  // reference index
    qint16  elec;   // calculated

    IMRODesc_T3010base()
    :   bank(0), refid(0)           {}
    IMRODesc_T3010base( int bank, int refid )
    :   bank(bank), refid(refid)    {}
    void setElec( int ch )
        {elec = chToEl( ch, bank );}
    static int chToEl( int ch, int bank );
    bool operator==( const IMRODesc_T3010base &rhs ) const
        {return bank==rhs.bank && refid==rhs.refid;}
    QString toString( int chn ) const;
    bool fromString( QString *msg, const QString &s );
};


struct T3010Key {
    int c, b;
    T3010Key() : c(0), b(0)                 {}
    T3010Key( int c, int m ) : c(c), b(m)   {}
    bool operator<( const T3010Key &rhs ) const;
};


struct IMROTbl_T3010base : public IMROTbl
{
    enum imLims_T3010base {
        imType3010baseElec      = 1280,
        imType3010baseChan      = 912,
        imType3010baseBanks     = 2,
        imType3010baseRefids    = 3,
        imType3010baseGain      = 100
    };

    QVector<IMRODesc_T3010base>         e;
    mutable QMap<T3010Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T3010Key>    s2k;

    IMROTbl_T3010base( const QString &pn, int type )
        :   IMROTbl(pn, type)       {}
    virtual ~IMROTbl_T3010base()    {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T3010base*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const = 0;

    virtual int nElec() const           {return imType3010baseElec;}
    virtual int nShank() const          {return 1;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType3010baseChan;}
    virtual int nLF() const             {return 0;}
    virtual int nBanks() const          {return imType3010baseBanks;}
    virtual int nRefs() const           {return imType3010baseRefids;}
    virtual int maxInt() const          {return 2048;}
    virtual double maxVolts() const     {return 0.67;}
    virtual bool needADCCal() const     {return false;}

    // {0=NP1000, 2=NP2000, 4=NP2020}-like
    virtual int apiFetchType() const    {return 2;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T3010base*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int /* ch */ ) const          {return 0;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
    virtual int maxBank( int ch, int shank = 0 ) const;
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType3010baseGain;}
    virtual int lfGain( int /* ch */ ) const        {return imType3010baseGain;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType3010baseGain;}
    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual double unityToVolts( double u ) const   {return 1.34*u - 0.67;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectGains( int, int, int ) const  {return 0;}
    virtual int selectAPFlts( int, int, int ) const {return 0;}

// Edit

    virtual void edit_init() const;
    virtual IMRO_GUI edit_GUI() const;
    virtual IMRO_Attr edit_Attr_def() const;
    virtual IMRO_Attr edit_Attr_cur() const;
    virtual bool edit_Attr_canonical() const;
    virtual void edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const;
    virtual int edit_site2Chan( const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTBL_T3010BASE_H


