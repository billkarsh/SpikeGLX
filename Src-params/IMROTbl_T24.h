#ifndef IMROTBL_T24_H
#define IMROTBL_T24_H

#include "IMROTbl.h"

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
    int chToEl( int ch ) const;
    bool operator==( const IMRODesc_T24 &rhs ) const
        {return shnk==rhs.shnk && bank==rhs.bank && refid==rhs.refid;}
    QString toString( int chn ) const;
    static IMRODesc_T24 fromString( const QString &s );
};


struct IMROTbl_T24 : public IMROTbl
{
    enum imLims_T24 {
        imType24Type    = 24,
        imType24Elec    = 5120, // 4 * 1280
        imType24Banks   = 4,
        imType24Chan    = 384,
        imType24Refids  = 18,
    };

    QVector<IMRODesc_T24>   e;

    IMROTbl_T24()           {type=imType24Type;}
    virtual ~IMROTbl_T24()  {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T24*)rhs)->e;
    }

    virtual void fillDefault();

    virtual int nShank() const          {return 4;}
    virtual int nCol() const            {return 2;}
    virtual int nRow() const            {return 1280/2;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType24Chan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 1;}
    virtual int nElec() const           {return imType24Elec;}
    virtual int maxInt() const          {return 8192;}
    virtual double maxVolts() const     {return 0.5;}
    virtual bool needADCCal() const     {return false;}
    virtual bool selectableGain() const {return false;}
    virtual bool setableHipass() const  {return false;}
    virtual bool isMultiSelect() const  {return false;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type==rhs.type && e == ((const IMROTbl_T24*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual void fromString( const QString &s );

    virtual bool loadFile( QString &msg, const QString &path );
    virtual bool saveFile( QString &msg, const QString &path ) const;

    virtual int shnk( int ch ) const                {return e[ch].shnk;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
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

    virtual double unityToVolts( double u ) const
        {return 1.0*u - 0.5;}

    virtual void muxTable( int &nADC, int &nChn, std::vector<int> &T ) const;
};

#endif  // IMROTbl_T24_H



