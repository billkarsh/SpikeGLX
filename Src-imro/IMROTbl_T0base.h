#ifndef IMROTBL_T0BASE_H
#define IMROTBL_T0BASE_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T0base
{
    qint16  bank,
            apgn,   // gain, not index
            lfgn;   // gain, not index
    qint8   refid,  // reference index
            apflt;  // bool

    IMRODesc_T0base()
    :   bank(0), apgn(500), lfgn(250), refid(0), apflt(1)               {}
    IMRODesc_T0base( int bank, int refid, int apgn, int lfgn, bool apflt )
    :   bank(bank), apgn(apgn), lfgn(lfgn), refid(refid), apflt(apflt)  {}
    static int chToEl( int ch, int bank );
    bool operator==( const IMRODesc_T0base &rhs ) const
        {return bank==rhs.bank   && apgn==rhs.apgn && lfgn==rhs.lfgn
            &&  refid==rhs.refid && apflt==rhs.apflt;}
    QString toString( int chn ) const;
    static IMRODesc_T0base fromString( const QString &s );
};


struct T0Key {
    int c, b;
    T0Key() : c(0), b(0)               {}
    T0Key( int c, int b ) : c(c), b(b) {}
    bool operator<( const T0Key &rhs ) const;
};


struct IMROTbl_T0base : public IMROTbl
{
    enum imLims_T0base {
        imType0baseCol      = 2,
        imType0baseChan     = 384,
        imType0baseGains    = 8
    };

    QVector<IMRODesc_T0base>        e;
    mutable QMap<T0Key,IMRO_Site>   k2s;
    mutable QMap<IMRO_Site,T0Key>   s2k;

    virtual ~IMROTbl_T0base()   {}

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T0base*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int typeConst() const = 0;
    virtual int nShank() const          {return 1;}
    virtual int nCol() const            {return imType0baseCol;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType0baseChan;}
    virtual int nLF() const             {return imType0baseChan;}
    virtual int nSY() const             {return 1;}
    virtual int maxInt() const          {return 512;}
    virtual double maxVolts() const     {return 0.6;}
    virtual bool needADCCal() const     {return true;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T0base*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual bool loadFile( QString &msg, const QString &path );
    virtual bool saveFile( QString &msg, const QString &path ) const;

    virtual int shnk( int /* ch */ ) const  {return 0;}
    virtual int bank( int ch ) const        {return e[ch].bank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const       {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int ch ) const      {return e[ch].apgn;}
    virtual int lfGain( int ch ) const      {return e[ch].lfgn;}
    virtual int apFlt( int ch ) const       {return e[ch].apflt;}

    virtual bool chIsRef( int ch ) const;
    virtual int idxToGain( int idx ) const;
    virtual int gainToIdx( int gain ) const;
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual double unityToVolts( double u ) const
        {return 1.2*u - 0.6;}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Edit

    virtual bool edit_able() const  {return true;}
    virtual void edit_init() const;
    virtual IMRO_GUI edit_GUI() const;
    virtual IMRO_Attr edit_Attr_def() const;
    virtual IMRO_Attr edit_Attr_cur() const;
    virtual void edit_exclude_1( tImroSites vS, const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTBL_T0BASE_H


