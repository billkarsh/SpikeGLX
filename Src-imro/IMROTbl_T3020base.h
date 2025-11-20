#ifndef IMROTBL_T3020BASE_H
#define IMROTBL_T3020BASE_H

#include "IMROTbl.h"

#include <QMap>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T3020base
{
    qint16  chan;   // needed to sort into chan-order
    qint8   shnk,
            bank,
            refid;  // reference index
    qint16  elec;   // calculated

    IMRODesc_T3020base()
    :   chan(0), shnk(0), bank(0), refid(0)                 {}
    IMRODesc_T3020base( int chan, int shnk, int bank, int refid )
    :   chan(chan), shnk(shnk), bank(bank), refid(refid)    {}
    void setElec()
        {elec = chToEl( chan, shnk, bank );}
    static int chToEl( int ch, int shank, int bank );
    bool operator<( const IMRODesc_T3020base &rhs ) const
        {return chan < rhs.chan;}
    bool operator==( const IMRODesc_T3020base &rhs ) const
        {return chan==rhs.chan && shnk==rhs.shnk &&
                bank==rhs.bank && refid==rhs.refid;
        }
    QString toString( int chn ) const;
    int fromString( QString *msg, const QString &s );
};


struct T3020Key {
    int c, s, b;
    T3020Key() : c(0), s(0), b(0)                       {}
    T3020Key( int c, int s, int b ) : c(c), s(s), b(b)  {}
    bool operator<( const T3020Key &rhs ) const;
};


struct IMROTbl_T3020base : public IMROTbl
{
    enum imLims_T3020base {
        imType3020baseElPerShk  = 1280,
        imType3020baseShanks    = 4,
        imType3020baseElec      = imType3020baseShanks * imType3020baseElPerShk,
        imType3020baseChan      = 1536,
        imType3020baseChPerBnk  = 912,
        imType3020baseBanks     = 2,
        imType3020baseRefids    = 7,
        imType3020baseGain      = 100
    };

    QVector<IMRODesc_T3020base>         e;
    mutable QMap<T3020Key,IMRO_Site>    k2s;
    mutable QMap<IMRO_Site,T3020Key>    s2k;

    IMROTbl_T3020base( const QString &pn, int type )
        :   IMROTbl(pn, type)       {}
    virtual ~IMROTbl_T3020base()    {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T3020base*)rhs)->e;
    }

    virtual void fillDefault();
    virtual void fillShankAndBank( int shank, int bank );
    virtual int svy_minRow( int shank, int bank ) const;

    virtual int typeConst() const = 0;

    virtual int nElec() const           {return imType3020baseElec;}
    virtual int nShank() const          {return imType3020baseShanks;}
    virtual int nSvyShank() const       {return 1;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType3020baseChan;}
    virtual int nLF() const             {return 0;}
    virtual int nBanks() const          {return imType3020baseBanks;}
    virtual int nSvyBanks() const       {return 4;}
    virtual int nChanPerBank() const    {return imType3020baseChPerBnk;}
    virtual int nRefs() const           {return imType3020baseRefids;}
    virtual int maxInt() const          {return 2048;}
    virtual double maxVolts() const     {return 0.67;}
    virtual bool needADCCal() const     {return false;}
    virtual int probeTech() const       {return t_tech_nxt;}
    virtual int chanMapping() const     {return t_map_np2ms;}
    virtual int apiFetchType() const    {return t_fetch_np2;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type == rhs.type && e == ((const IMROTbl_T3020base*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual bool fromString( QString *msg, const QString &s );

    virtual int shnk( int ch ) const                {return e[ch].shnk;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
    virtual int maxBank( int ch, int shank = 0 ) const;
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int /* ch */ ) const        {return imType3020baseGain;}
    virtual int lfGain( int /* ch */ ) const        {return imType3020baseGain;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    virtual bool chIsRef( int /* ch */ ) const      {return false;}
    virtual int idxToGain( int /* idx */ ) const    {return imType3020baseGain;}
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
    virtual void edit_defaultROI( tImroROIs vR ) const;
};

#endif  // IMROTBL_T3020BASE_H


