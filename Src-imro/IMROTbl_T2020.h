#ifndef IMROTBL_T2020_H
#define IMROTBL_T2020_H

#include "IMROTbl_T2020base.h"

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
    int fromString( QString *msg, const QString &s );
};


struct T2020Key {
    int c, s, b;
    T2020Key() : c(0), s(0), b(0)                       {}
    T2020Key( int c, int s, int b ) : c(c), s(s), b(b)  {}
    bool operator<( const T2020Key &rhs ) const;
};


struct IMROTbl_T2020 : public IMROTbl_T2020base
{
    enum imLims_T2020 {
        imType2020Type      = 2020,
    };

    QVector<IMRODesc_T2020>             e;
    mutable QMap<T2020Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T2020Key>    s2k;

    IMROTbl_T2020( const QString &pn )
        :   IMROTbl_T2020base(pn, imType2020Type)   {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T2020*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const       {return imType2020Type;}

    virtual int nChan() const           {return e.size();}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T2020*)&rhs)->e;}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int ch ) const    {return e[ch].shnk;}
    virtual int bank( int ch ) const    {return e[ch].bank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const   {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;

// Edit

    virtual void edit_init() const;
    virtual bool edit_Attr_canonical() const;
    virtual void edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const;
    virtual int edit_site2Chan( const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTBL_T2020_H


