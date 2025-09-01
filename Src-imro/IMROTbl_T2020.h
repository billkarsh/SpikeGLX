#ifndef IMROTBL_T2020_H
#define IMROTBL_T2020_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NP2020 2.0 quad base (Ph 2C)
//
struct IMRODesc_T2020
{
    qint8   shnk,
            bank,
            refid;  // reference index
    qint16  elec;   // calculated

    IMRODesc_T2020()
    :   shnk(0), bank(0), refid(0)              {}
    IMRODesc_T2020( int shnk, int bank, int refid )
    :   shnk(shnk), bank(bank), refid(refid)    {}
    void setElec( int ch )  {elec = chToEl( ch, bank );}
    static int chToEl( int ch, int bank );
    bool operator==( const IMRODesc_T2020 &rhs ) const
        {return shnk==rhs.shnk && bank==rhs.bank && refid==rhs.refid;}
    QString toString( int chn ) const;
    bool fromString( QString *msg, const QString &s );
};


struct T2020Key {
    int c, s, b;
    T2020Key() : c(0), s(0), b(0)                       {}
    T2020Key( int c, int s, int b ) : c(c), s(s), b(b)  {}
    bool operator<( const T2020Key &rhs ) const;
};


struct IMROTbl_T2020 : public IMROTbl
{
    enum imLims_T2020 {
        imType2020Type      = 2020,
        imType2020ElPerShk  = 1280,
        imType2020ChPerShk  = 384,
        imType2020Shanks    = 4,
        imType2020Elec      = imType2020Shanks * imType2020ElPerShk,
        imType2020Chan      = imType2020Shanks * imType2020ChPerShk,
        imType2020Banks     = 4,
        imType2020Refids    = 3,
        imType2020Gain      = 100
    };

    QVector<IMRODesc_T2020>             e;
    mutable QMap<T2020Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T2020Key>    s2k;

    IMROTbl_T2020( const QString &pn )
        :   IMROTbl(pn, imType2020Type) {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T2020*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const       {return imType2020Type;}

    virtual int nElec() const           {return imType2020Elec;}
    virtual int nShank() const          {return imType2020Shanks;}
    virtual int nSvyShank() const       {return 1;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType2020Chan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 4;}
    virtual int nBanks() const          {return imType2020Banks;}
    virtual int nChanPerBank() const    {return imType2020ChPerShk;}
    virtual int nRefs() const           {return imType2020Refids;}
    virtual int maxInt() const          {return 2048;}
    virtual double maxVolts() const     {return 0.62;}
    virtual bool needADCCal() const     {return false;}
    virtual int apiFetchType() const    {return t_fetch_qb;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T2020*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int ch ) const                {return e[ch].shnk;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType2020Gain;}
    virtual int lfGain( int /* ch */ ) const        {return imType2020Gain;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType2020Gain;}
    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual double unityToVolts( double u ) const   {return 1.24*u - 0.62;}
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
    virtual void edit_defaultROI( tImroROIs vR ) const;
    virtual bool edit_isCanonical( tImroROIs vR ) const;
};

#endif  // IMROTBL_T2020_H


