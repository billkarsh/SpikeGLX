#ifndef IMROTBL_T24_H
#define IMROTBL_T24_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T24
{
    qint16  shnk,
            bank,
            refid,  // reference index
            elec;   // calculated

    IMRODesc_T24()
    :   shnk(0), bank(0), refid(0)              {}
    IMRODesc_T24( int shnk, int bank, int refid )
    :   shnk(shnk), bank(bank), refid(refid)    {}
    void setElec( int ch )
        {elec = chToEl( ch, shnk, bank );}
    static int chToEl( int ch, int shank, int bank );
    bool operator==( const IMRODesc_T24 &rhs ) const
        {return shnk==rhs.shnk && bank==rhs.bank && refid==rhs.refid;}
    QString toString( int chn ) const;
    static IMRODesc_T24 fromString( const QString &s );
};


struct T24Key {
    int c, s, b;
    T24Key() : c(0), s(0), b(0)                         {}
    T24Key( int c, int s, int b ) : c(c), s(s), b(b)    {}
    bool operator<( const T24Key &rhs ) const;
};


// NP 2.0 4-shank
//
struct IMROTbl_T24 : public IMROTbl
{
    enum imLims_T24 {
        imType24Type        = 24,
        imType24ElPerShk    = 1280,
        imType24Shanks      = 4,
        imType24Elec        = imType24Shanks * imType24ElPerShk,
        imType24Col         = 2,
        imType24Chan        = 384,
        imType24Banks       = 4,
        imType24Refids      = 18
    };

    QVector<IMRODesc_T24>           e;
    mutable QMap<T24Key,IMRO_Site>  k2s;
    mutable QMap<IMRO_Site,T24Key>  s2k;

    IMROTbl_T24()   {type=imType24Type;}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T24*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );

    virtual int nElec() const           {return imType24Elec;}
    virtual int nShank() const          {return imType24Shanks;}
    virtual int nElecPerShank() const   {return imType24ElPerShk;}
    virtual int nCol() const            {return imType24Col;}
    virtual int nRow() const            {return imType24ElPerShk/imType24Col;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType24Chan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 1;}
    virtual int nBanks() const          {return imType24Banks;}
    virtual int nRefs() const           {return imType24Refids;}
    virtual int maxInt() const          {return 8192;}
    virtual double maxVolts() const     {return 0.5;}
    virtual bool needADCCal() const     {return false;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T24*)&rhs)->e;}
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

    virtual int selectGains( int, int, int ) const  {return 0;}
    virtual int selectAPFlts( int, int, int ) const {return 0;}

// Edit

    virtual bool edit_able() const  {return true;}
    virtual void edit_init() const;
    virtual IMRO_GUI edit_GUI() const;
    virtual IMRO_Attr edit_Attr_def() const;
    virtual IMRO_Attr edit_Attr_cur() const;
    virtual void edit_exclude_1( tImroSites vS, const IMRO_Site &s ) const;
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A );
};

#endif  // IMROTbl_T24_H


