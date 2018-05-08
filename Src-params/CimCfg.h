#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"

#include <QBitArray>
#include <QMap>
#include <QString>
#include <QVector>

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRODesc
{
    qint16  bank,
            refid,  // reference index
            apgn,   // gain, not index
            lfgn;   // gainm not index

    IMRODesc()
    :   bank(0), refid(0), apgn(500), lfgn(250)             {}
    IMRODesc( qint16 bank, qint16 refid, qint16 apgn, qint16 lfgn )
    :   bank(bank), refid(refid), apgn(apgn), lfgn(lfgn)    {}
    bool operator==( const IMRODesc &rhs ) const
        {return bank==rhs.bank && refid==rhs.refid
            &&  apgn==rhs.apgn &&  lfgn==rhs.lfgn;}
    QString toString( int chn ) const;
    static IMRODesc fromString( const QString &s );
};


struct IMROTbl
{
    enum imLims {
        imOpt1Elec  = 384,
        imOpt2Elec  = 384,
        imOpt3Elec  = 960,
        imOpt4Elec  = 966,

        imOpt1Banks = 1,
        imOpt2Banks = 1,
        imOpt3Banks = 3,
        imOpt4Banks = 4,

        imOpt1Chan  = 384,
        imOpt2Chan  = 384,
        imOpt3Chan  = 384,
        imOpt4Chan  = 276,

        imOpt1Refs  = 11,
        imOpt2Refs  = 11,
        imOpt3Refs  = 11,
        imOpt4Refs  = 8,

        imNGains    = 8
    };

    quint32             pSN,
                        opt;
    QVector<IMRODesc>   e;

    void fillDefault( quint32 pSN, int opt );

    int nChan() const   {return e.size();}
    int nElec() const   {return optToNElec( opt );}

    bool operator==( const IMROTbl &rhs ) const
        {return opt==rhs.opt && e == rhs.e;}
    bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    bool banksSame( const IMROTbl &rhs ) const;

    QString toString() const;
    void fromString( const QString &s );

    bool loadFile( QString &msg, const QString &path );
    bool saveFile( QString &msg, const QString &path, quint32 pSN );

    static const int* optTo_r2c( int opt );

    static int optToNElec( int opt );
    static int optToNRef( int opt );
    static int elToCh384( int el );
    static int elToCh276( int el );
    static int chToEl384( int ch, int bank );
    static int chToEl276( int ch, int bank );
    static int chToRefid384( int ch );
    static int chToRefid276( int ch );
    static int elToRefid384( int el );
    static int elToRefid276( int el );
    static int idxToGain( int idx );
    static int gainToIdx( int gain );
};


// Base class for IMEC configuration
//
class CimCfg
{
    // ---------
    // Constants
    // ---------

public:
    enum imTypeId {
        imTypeAP    = 0,
        imTypeLF    = 1,
        imTypeSY    = 2,
        imSumAP     = 0,
        imSumNeural = 1,
        imSumAll    = 2,
        imNTypes    = 3
    };

    // -----
    // Types
    // -----

    struct IMVers {
        QString hwr,
                bas,
                api,
                pLB,
                pSN;
        int     opt;    // [1,4]
        bool    force,
                skipADC;
        void clear()
            {
                hwr.clear(), bas.clear(), api.clear(),
                pLB.clear(), pSN.clear(), opt = 0,
                force = false, skipADC = false;
            }
    };

    // ------
    // Params
    // ------

    // derived:
    // imCumTypCnt[]

public:
    VRange      range;
    double      srate;
    QString     imroFile,
                stdbyStr;
    IMROTbl     roTbl;
    QBitArray   stdbyBits;
    int         imCumTypCnt[imNTypes];
    int         hpFltIdx;
    bool        enabled,
                doGainCor,
                noLEDs,
                softStart;

    CimCfg() : range(VRange(-0.6,0.6))  {}

    // -------------
    // Param methods
    // -------------

public:
    double chanGain( int ic ) const;

    void deriveChanCounts( int opt );
    bool deriveStdbyBits( QString &err, int nAP );

    int vToInt10( double v, int ic ) const;
    double int10ToV( int i10, int ic ) const;

    void justAPBits( QBitArray &apBits, const QBitArray &saveBits ) const;
    void justLFBits( QBitArray &apBits, const QBitArray &saveBits ) const;

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    static int idxToFlt( int idx );

    // ------
    // Config
    // ------

    static bool getVersions( QStringList &sl, IMVers &imVers );
};

#endif  // CIMCFG_H


