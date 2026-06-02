#ifndef IMROTBL_T2022_H
#define IMROTBL_T2022_H

#include "IMROTbl_T2020base.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// NP2022 2.0 quad base NHP (Ph 2C)
//
struct IMRODesc_T2022
{
    qint16  shnk,
            mbank,  // 1=B0 2=B1 4=B2 8=B3
            refid,  // reference index
            elec;   // lowest connected

    IMRODesc_T2022()
    :   shnk(0), mbank(1), refid(0)             {}
    IMRODesc_T2022( int shnk, int mbank, int refid )
    :   shnk(shnk), mbank(mbank), refid(refid)  {}
    void setElec( int ch )  {elec = chToEl( ch, mbank );}
    static int lowBank( int mbank );
    static int chToEl( int ch, int mbank );
    bool operator==( const IMRODesc_T2022 &rhs ) const
        {return shnk==rhs.shnk && mbank==rhs.mbank && refid==rhs.refid;}
    QString toString( int chn ) const;
    int fromString( QString *msg, const QString &s );
};


struct T2022Key {
    int c, s, m;
    T2022Key() : c(0), s(0), m(0)                       {}
    T2022Key( int c, int s, int m ) : c(c), s(s), m(m)  {}
    bool operator<( const T2022Key &rhs ) const;
};


struct IMROTbl_T2022 : public IMROTbl_T2020base
{
    enum imLims_T2022 {
        imType2022Type      = 2022,
    };

    QVector<IMRODesc_T2022>             e;
    mutable QMap<T2022Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T2022Key>    s2k;

    IMROTbl_T2022( const QString &pn )
        :   IMROTbl_T2020base(pn, imType2022Type)   {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T2022*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const       {return imType2022Type;}

    virtual int nChan() const           {return e.size();}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T2022*)&rhs)->e;}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int ch ) const    {return e[ch].shnk;}
    virtual int bank( int ch ) const    {return e[ch].mbank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const   {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;

// Hardware

    virtual int selectSites4( const PAddr& adr, bool write, bool check ) const;

// Edit

    virtual void edit_init() const;
    virtual bool edit_Attr_canonical() const;
    virtual void edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const;
    virtual int edit_site2Chan( const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTBL_T2022_H


