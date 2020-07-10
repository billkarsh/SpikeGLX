#ifndef IMROTBL_T21_H
#define IMROTBL_T21_H

#include "IMROTbl.h"

#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T21
{
    qint16  mbank,  // 1=B0 2=B1 4=B2 8=B3
            refid,  // reference index
            elec,
            rsrv;   // padding

    IMRODesc_T21()
    :   mbank(1), refid(0)              {}
    IMRODesc_T21( int mbank, int refid )
    :   mbank(mbank), refid(refid)      {}
    int lowBank() const;
    int chToEl( int ch ) const;
    bool operator==( const IMRODesc_T21 &rhs ) const
        {return mbank==rhs.mbank && refid==rhs.refid;}
    QString toString( int chn ) const;
    static IMRODesc_T21 fromString( const QString &s );
};


struct IMROTbl_T21 : public IMROTbl
{
    enum imLims_T21 {
        imType21Type    = 21,
        imType21Elec    = 1280,
        imType21Banks   = 4,
        imType21Chan    = 384,
        imType21Refids  = 6
    };

    QVector<IMRODesc_T21>   e;

    IMROTbl_T21()           {type=imType21Type;}
    virtual ~IMROTbl_T21()  {}

    void setElecs();

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        e       = ((const IMROTbl_T21*)rhs)->e;
    }

    virtual void fillDefault();

    virtual int nBanks() const          {return imType21Banks;}
    virtual int nRefs() const           {return imType21Refids;}
    virtual int nShank() const          {return 1;}
    virtual int nCol() const            {return 2;}
    virtual int nRow() const            {return imType21Elec/2;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return imType21Chan;}
    virtual int nLF() const             {return 0;}
    virtual int nSY() const             {return 1;}
    virtual int nElec() const           {return imType21Elec;}
    virtual int maxInt() const          {return 8192;}
    virtual double maxVolts() const     {return 0.5;}
    virtual bool needADCCal() const     {return false;}
    virtual bool selectableGain() const {return false;}
    virtual bool setableHipass() const  {return false;}
    virtual bool isMultiSelect() const  {return true;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return type==rhs.type && e == ((const IMROTbl_T21*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual void fromString( const QString &s );

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

    virtual void muxTable( int &nADC, int &nChn, std::vector<int> &T ) const;
};

#endif  // IMROTBL_T21_H



