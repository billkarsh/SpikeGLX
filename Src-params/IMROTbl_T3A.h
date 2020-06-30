#ifndef IMROTBL_T3A_H
#define IMROTBL_T3A_H

#include "IMROTbl.h"

#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc_T3A
{
    qint16  bank,
            refid,  // reference index
            apgn,   // gain, not index
            lfgn;   // gainm not index

    IMRODesc_T3A()
    :   bank(0), refid(0), apgn(500), lfgn(250)             {}
    IMRODesc_T3A( qint16 bank, qint16 refid, qint16 apgn, qint16 lfgn )
    :   bank(bank), refid(refid), apgn(apgn), lfgn(lfgn)    {}
    bool operator==( const IMRODesc_T3A &rhs ) const
        {return bank==rhs.bank && refid==rhs.refid
            &&  apgn==rhs.apgn &&  lfgn==rhs.lfgn;}
    QString toString( int chn ) const;
    static IMRODesc_T3A fromString( const QString &s );
};


struct IMROTbl_T3A : public IMROTbl
{
    enum imLims_T3A {
        imType3AType        = -3,

        imType3AOpt1Elec    = 384,
        imType3AOpt2Elec    = 384,
        imType3AOpt3Elec    = 960,
        imType3AOpt4Elec    = 966,

        imType3AOpt1Banks   = 1,
        imType3AOpt2Banks   = 1,
        imType3AOpt3Banks   = 3,
        imType3AOpt4Banks   = 4,

        imType3AOpt1Chan    = 384,
        imType3AOpt2Chan    = 384,
        imType3AOpt3Chan    = 384,
        imType3AOpt4Chan    = 276,

        imType3AOpt1Refs    = 11,
        imType3AOpt2Refs    = 11,
        imType3AOpt3Refs    = 11,
        imType3AOpt4Refs    = 8,

        imType3AGains       = 8
    };

    quint32                 pSN,
                            opt;
    QVector<IMRODesc_T3A>   e;

    IMROTbl_T3A()           {type=imType3AType;}
    virtual ~IMROTbl_T3A()  {}

    virtual void copyFrom( const IMROTbl *rhs )
    {
        type    = rhs->type;
        pSN     = ((const IMROTbl_T3A*)rhs)->pSN;
        opt     = ((const IMROTbl_T3A*)rhs)->opt;
        e       = ((const IMROTbl_T3A*)rhs)->e;
    }

    virtual void fillDefault();

    virtual int nBanks() const          {return (opt == 4 ? imType3AOpt4Banks :
                                                (opt == 3 ? imType3AOpt3Banks :
                                                            imType3AOpt1Banks));}
    virtual int nShank() const          {return 1;}
    virtual int nCol() const            {return 2;}
    virtual int nRow() const            {return nElec()/2;}
    virtual int nChan() const           {return e.size();}
    virtual int nAP() const             {return (opt == 4 ? imType3AOpt4Chan : imType3AOpt3Chan);}
    virtual int nLF() const             {return nAP();}
    virtual int nSY() const             {return 1;}
    virtual int nElec() const           {return (opt == 4 ? imType3AOpt4Elec :
                                        (opt == 3 ? imType3AOpt3Elec : imType3AOpt1Elec));}
    virtual int maxInt() const          {return 512;}
    virtual double maxVolts() const     {return 0.6;}
    virtual bool needADCCal() const     {return true;}
    virtual bool selectableGain() const {return true;}
    virtual bool setableHipass() const  {return false;}
    virtual bool isMultiSelect() const  {return false;}

    virtual bool operator==( const IMROTbl &rhs ) const
        {return opt==((const IMROTbl_T3A*)&rhs)->opt &&
         e == ((const IMROTbl_T3A*)&rhs)->e;}
    virtual bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    virtual bool isConnectedSame( const IMROTbl *rhs ) const;

    virtual QString toString() const;
    virtual void fromString( const QString &s );

    virtual bool loadFile( QString &msg, const QString &path );
    virtual bool saveFile( QString &msg, const QString &path ) const;

    virtual int shnk( int /* ch */ ) const          {return 0;}
    virtual int bank( int ch ) const                {return e[ch].bank;}
    virtual int elShankAndBank( int &bank, int ch ) const;
    virtual int elShankColRow( int &col, int &row, int ch ) const;
    virtual void eaChansOrder( QVector<int> &v ) const;
    virtual int refid( int ch ) const               {return e[ch].refid;}
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const;
    virtual int apGain( int ch ) const              {return e[ch].apgn;}
    virtual int lfGain( int ch ) const              {return e[ch].lfgn;}
    virtual int apFlt( int /* ch */ ) const         {return 0;}

    int chToEl( int ch ) const;

    static const int* optTo_r2c( int opt );

    static int optToNRef( int opt );
    static int elToCh384( int el );
    static int elToCh276( int el );
    static int chToEl384( int ch, int bank );
    static int chToEl276( int ch, int bank );
    static int chToRefid384( int ch );
    static int chToRefid276( int ch );
    static int elToRefid384( int el );
    static int elToRefid276( int el );

    virtual bool chIsRef( int ch ) const;
    virtual int idxToGain( int idx ) const;
    virtual int gainToIdx( int gain ) const;

    virtual double unityToVolts( double u ) const
        {return 1.2*u - 0.6;}

    virtual void muxTable( int &nADC, int &nChn, std::vector<int> &T ) const;
};

#endif  // IMROTBL_T3A_H



