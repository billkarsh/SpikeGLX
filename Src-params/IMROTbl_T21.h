#ifndef IMROTBL_T21_H
#define IMROTBL_T21_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T21
{
    qint16  mbank,  // 1=B0 2=B1 4=B2 8=B3
            refid,  // reference index
            elec,   // lowest connected
            rsrv;   // padding

    IMRODesc_T21()
    :   mbank(1), refid(0)              {}
    IMRODesc_T21( int mbank, int refid )
    :   mbank(mbank), refid(refid)      {}
    void setElec( int ch )
        {elec = chToEl( ch, mbank );}
    static int lowBank( int mbank );
    static int chToEl( int ch, int mbank );
    bool operator==( const IMRODesc_T21 &rhs ) const
        {return mbank==rhs.mbank && refid==rhs.refid;}
    QString toString( int chn ) const;
    static IMRODesc_T21 fromString( const QString &s );
};


struct T21Key {
    int c, m;
    T21Key() : c(0), m(0)               {}
    T21Key( int c, int m ) : c(c), m(m) {}
    bool operator<( const T21Key &rhs ) const;
};


// NP 2.0 1-shank
//
struct IMROTbl_T21 : public IMROTbl
{
    enum imLims_T21 {
        imType21Type    = 21,
        imType21Elec    = 1280,
        imType21Col     = 2,
        imType21Chan    = 384,
        imType21Banks   = 4,
        imType21Refids  = 6
    };

    QVector<IMRODesc_T21>           e;
    mutable QMap<T21Key,IMRO_Site>  k2s;
    mutable QMap<IMRO_Site,T21Key>  s2k;

    IMROTbl_T21()   {type=imType21Type;}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T21*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int nElec() const           {return imType21Elec;}
    virtual int nShank() const          {return 1;}
    virtual int nElecPerShank() const   {return imType21Elec;}
    virtual int nCol() const            {return imType21Col;}
    virtual int nRow() const            {return imType21Elec/imType21Col;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType21Chan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 1;}
    virtual int nBanks() const          {return imType21Banks;}
    virtual int nRefs() const           {return imType21Refids;}
    virtual int maxInt() const          {return 8192;}
    virtual double maxVolts() const     {return 0.5;}
    virtual bool needADCCal() const     {return false;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T21*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual bool loadFile( QString &msg, const QString &path );
    virtual bool saveFile( QString &msg, const QString &path ) const;

    virtual int shnk( int /* ch */ ) const          {return 0;}
    virtual int bank( int ch ) const                {return e[ch].mbank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return 80;}
    virtual int lfGain( int /* ch */ ) const        {return 80;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual bool chIsRef( int ch ) const;
    virtual int idxToGain( int /* idx */ ) const    {return 80;}
    virtual int gainToIdx( int /* gain */ ) const   {return 0;}
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const;    // iflt = {1,2}

    virtual double unityToVolts( double u ) const
        {return 1.0*u - 0.5;}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const;

// Hardware

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
    virtual int selectGains( int, int, int ) const  {return 0;}
    virtual int selectAPFlts( int, int, int ) const {return 0;}

// Edit

    virtual bool edit_init() const;
    virtual int edit_gains( int &defLF, std::vector<int> &g ) const;
    virtual void edit_strike_1( tImroSites vS, const IMRO_Site &s ) const;
};

#endif  // IMROTBL_T21_H


