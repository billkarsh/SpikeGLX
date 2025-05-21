#ifndef IMROTBL_T21BASE_H
#define IMROTBL_T21BASE_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T21base
{
    qint16  mbank,  // 1=B0 2=B1 4=B2 8=B3
            refid,  // reference index
            elec,   // lowest connected
            rsrv;   // padding

    IMRODesc_T21base()
    :   mbank(1), refid(0)          {}
    IMRODesc_T21base( int mbank, int refid )
    :   mbank(mbank), refid(refid)  {}
    void setElec( int ch )
        {elec = chToEl( ch, mbank );}
    static int lowBank( int mbank );
    static int chToEl( int ch, int mbank );
    bool operator==( const IMRODesc_T21base &rhs ) const
        {return mbank==rhs.mbank && refid==rhs.refid;}
    QString toString( int chn ) const;
    bool fromString( QString *msg, const QString &s );
};


struct T21Key {
    int c, m;
    T21Key() : c(0), m(0)               {}
    T21Key( int c, int m ) : c(c), m(m) {}
    bool operator<( const T21Key &rhs ) const;
};


struct IMROTbl_T21base : public IMROTbl
{
    enum imLims_T21base {
        imType21baseElec    = 1280,
        imType21baseChan    = 384,
        imType21baseBanks   = 4
    };

    QVector<IMRODesc_T21base>       e;
    mutable QMap<T21Key,IMRO_Site>  k2s;
    mutable QMap<IMRO_Site,T21Key>  s2k;

    IMROTbl_T21base( const QString &pn, int type )
        :   IMROTbl(pn, type)   {}
    virtual ~IMROTbl_T21base()  {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T21base*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const = 0;
    virtual int tip0refID() const = 0;

    virtual int nElec() const           {return imType21baseElec;}
    virtual int nShank() const          {return 1;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType21baseChan;}
    virtual int nLF() const             {return 0;}
    virtual int nBanks() const          {return imType21baseBanks;}
    virtual bool needADCCal() const     {return false;}

    // {0=NP1000, 1=NP2000, 2=NP2010, 3=NP1110}-like
    virtual int chanMapping() const     {return 1;}

    // {0=NP1000, 2=NP2000, 4=NP2020}-like
    virtual int apiFetchType() const    {return 2;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T21base*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int /* ch */ ) const          {return 0;}
    virtual int bank( int ch ) const                {return e[ch].mbank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
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

#endif  // IMROTBL_T21BASE_H


