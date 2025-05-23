#ifndef IMROTBL_T1110_H
#define IMROTBL_T1110_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMROHdr_T1110
{
    qint16  apgn,       // gain, not index
            lfgn;       // gain, not index
    qint8   colmode,    // {0,1,2} = {INNER,OUTER,ALL}
            refid,      // reference index
            apflt,      // bool
            rsrv;
    IMROHdr_T1110()
    :   apgn(500), lfgn(250), colmode(2), refid(0), apflt(1), rsrv(0)   {}
    IMROHdr_T1110( int colmode, int refid, int apgn, int lfgn, int apflt )
    :   apgn(apgn), lfgn(lfgn),
        colmode(colmode), refid(refid), apflt(apflt), rsrv(0)           {}
    bool operator==( const IMROHdr_T1110 &rhs ) const
        {return apgn==rhs.apgn       && lfgn==rhs.lfgn
            &&  colmode==rhs.colmode && refid==rhs.refid
            &&  apflt==rhs.apflt;}
    QString toString( int type ) const;
};


struct IMRODesc_T1110
{
    qint8   bankA,
            bankB;
    IMRODesc_T1110()
    :   bankA(0), bankB(0)          {}
    IMRODesc_T1110( int bankA, int bankB )
    :   bankA(bankA), bankB(bankB)  {}
    bool operator==( const IMRODesc_T1110 &rhs ) const
        {return bankA==rhs.bankA && bankB==rhs.bankB;}
    QString toString( int grp ) const;
    static IMRODesc_T1110 fromString( const QString &s );
};


// Only colMode=ALL supported
//
struct T1110Key {
    int c, b;
    T1110Key() : c(0), b(0)                 {}
    T1110Key( int c, int b ) : c(c), b(b)   {}
    bool operator<( const T1110Key &rhs ) const;
};


// UHD phase 2 el 6144
//
struct IMROTbl_T1110 : public IMROTbl
{
    enum imLims_T1110 {
        imType1110Type      = 1110,
        imType1110Elec      = 6144,
        imType1110Chan      = 384,
        imType1110Groups    = 24,
        imType1110Banks     = 16,
        imType1110Refids    = 2,
        imType1110Gains     = 8
    };

    IMROHdr_T1110                       ehdr;
    QVector<IMRODesc_T1110>             e;
    mutable QMap<T1110Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T1110Key>    s2k;

    IMROTbl_T1110( const QString &pn )
        :   IMROTbl(pn, imType1110Type) {}

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        ehdr    = ((const IMROTbl_T1110*)rhs)->ehdr;
        e       = ((const IMROTbl_T1110*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int nElec() const           {return imType1110Elec;}
    virtual int nShank() const          {return 1;}
    virtual int nChan() const           {return imType1110Chan;}
    virtual int nAP() const             {return imType1110Chan;}
    virtual int nLF() const             {return imType1110Chan;}
    virtual int nBanks() const          {return imType1110Banks;}
    virtual int nRefs() const           {return imType1110Refids;}
    virtual int maxInt() const          {return 512;}
    virtual double maxVolts() const     {return 0.6;}
    virtual bool needADCCal() const     {return true;}

    // {0=NP1000, 1=NP2000, 2=NP2010, 3=NP1110}-like
    virtual int chanMapping() const     {return 3;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type
            &&  ehdr == ((const IMROTbl_T1110*)&rhs)->ehdr
            &&  e    == ((const IMROTbl_T1110*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int /* ch */ ) const          {return 0;}
    virtual int bank( int ch ) const;
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int /* ch */ ) const         {return ehdr.refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int /* ch */ ) const;
    virtual int apGain( int /* ch */ ) const        {return ehdr.apgn;}
    virtual int lfGain( int /* ch */ ) const        {return ehdr.lfgn;}
    virtual int apFlt( int /* ch */ ) const         {return ehdr.apflt;}

    int grpIdx( int ch ) const;
    int col( int ch, int bank ) const;
    int row( int ch, int bank ) const;
    int chToEl( int ch ) const;
    int chToEl( int ch, int bank ) const;

    virtual bool chIsRef( int ) const               {return false;}
    virtual int idxToGain( int idx ) const;
    virtual int gainToIdx( int gain ) const;
    virtual double unityToVolts( double u ) const   {return 1.2*u - 0.6;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
    virtual int selectRefs( int slot, int port, int dock ) const;
    virtual int selectGains( int slot, int port, int dock ) const;
    virtual int selectAPFlts( int slot, int port, int dock ) const;

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

#endif  // IMROTBL_T1110_H


