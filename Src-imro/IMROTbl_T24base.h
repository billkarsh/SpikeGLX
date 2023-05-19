#ifndef IMROTBL_T24BASE_H
#define IMROTBL_T24BASE_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T24base
{
    qint16  shnk,
            bank,
            refid,  // reference index
            elec;   // calculated

    IMRODesc_T24base()
    :   shnk(0), bank(0), refid(0)              {}
    IMRODesc_T24base( int shnk, int bank, int refid )
    :   shnk(shnk), bank(bank), refid(refid)    {}
    void setElec( int ch )
        {elec = chToEl( ch, shnk, bank );}
    static int chToEl( int ch, int shank, int bank );
    bool operator==( const IMRODesc_T24base &rhs ) const
        {return shnk==rhs.shnk && bank==rhs.bank && refid==rhs.refid;}
    QString toString( int chn ) const;
    static IMRODesc_T24base fromString( const QString &s );
};


struct T24Key {
    int c, s, b;
    T24Key() : c(0), s(0), b(0)                         {}
    T24Key( int c, int s, int b ) : c(c), s(s), b(b)    {}
    bool operator<( const T24Key &rhs ) const;
};


struct IMROTbl_T24base : public IMROTbl
{
    enum imLims_T24base {
        imType24baseElPerShk    = 1280,
        imType24baseShanks      = 4,
        imType24baseElec        = imType24baseShanks * imType24baseElPerShk,
        imType24baseChan        = 384,
        imType24baseBanks       = 4
    };

    QVector<IMRODesc_T24base>       e;
    mutable QMap<T24Key,IMRO_Site>  k2s;
    mutable QMap<IMRO_Site,T24Key>  s2k;

    IMROTbl_T24base( const QString &pn, int type )
        :   IMROTbl(pn, type)   {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T24base*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const = 0;
    virtual int tip0refID() const = 0;

    virtual int nElec() const           {return imType24baseElec;}
    virtual int nShank() const          {return imType24baseShanks;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType24baseChan;}
    virtual int nLF() const             {return 0;}
    virtual int nBanks() const          {return imType24baseBanks;}
    virtual bool needADCCal() const     {return false;}

    // {0=NP1000, 1=NP2000, 2=NP2010, 3=NP1110}-like
    virtual int chanMapping() const     {return 2;}

    // {0=NP1000, 2=NP2000}-like
    virtual int apiFetchType() const    {return 2;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T24base*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual bool loadFile( QString &msg, const QString &path );
    virtual bool saveFile( QString &msg, const QString &path ) const;

    virtual int shnk( int ch ) const                {return e[ch].shnk;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
    virtual int maxBank( int ch, int shank = 0 );
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectGains( int, int, int ) const  {return 0;}
    virtual int selectAPFlts( int, int, int ) const {return 0;}

// Edit

    virtual bool edit_able() const  {return true;}
    virtual void edit_init() const;
    virtual IMRO_GUI edit_GUI() const;
    virtual IMRO_Attr edit_Attr_def() const;
    virtual IMRO_Attr edit_Attr_cur() const;
    virtual bool edit_Attr_canonical() const;
    virtual void edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTBL_T24BASE_H


