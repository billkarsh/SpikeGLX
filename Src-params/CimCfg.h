#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"

#include <QBitArray>
#include <QMap>
#include <QString>
#include <QVector>

#include <limits>

class QSettings;
class QTableWidget;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// MS: Revisit all logic based upon probe option
// MS: Update user help texts
// MS: Does stdby still make sense?

struct IMRODesc
{
    qint16  bank,
            refid,  // reference index
            apgn,   // gain, not index
            lfgn;   // gainm not index

    IMRODesc()
    : bank(0), refid(0), apgn(500), lfgn(250) {}
    IMRODesc( qint16 bank, qint16 refid, qint16 apgn, qint16 lfgn )
    : bank(bank), refid(refid), apgn(apgn), lfgn(lfgn) {}
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

    quint32             type;
    QVector<IMRODesc>   e;

    void fillDefault( int type );

    int nChan() const   {return e.size();}
    int nElec() const   {return typeToNElec( type );}

    bool operator==( const IMROTbl &rhs ) const
        {return type==rhs.type && e == rhs.e;}
    bool operator!=( const IMROTbl &rhs ) const
        {return !(*this == rhs);}

    bool banksSame( const IMROTbl &rhs ) const;

    QString toString() const;
    void fromString( const QString &s );

    bool loadFile( QString &msg, const QString &path );
    bool saveFile( QString &msg, const QString &path );

    static const int* typeTo_r2c( int type );

    static int typeToNElec( int type );
    static int typeToNRef( int type );
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

    struct ImProbeDat {
        quint16     slot,       // ini
                    port;       // ini
        quint32     hssn;       // detect
        QString     hsfw;       // detect
        quint64     sn;         // detect
        quint16     type;       // detect
        bool        enab;       // ini
        quint16     id;         // calc

        ImProbeDat( int slot, int port )
        :   slot(slot), port(port), enab(true)  {init();}
        ImProbeDat()
        :   enab(true)                          {init();}

        void init()
            {
                hssn    = -1;
                hsfw.clear();
                sn      = std::numeric_limits<qlonglong>::max();
                type    = -1;
                id      = -1;
            }

        bool operator< ( const ImProbeDat &rhs ) const
            {
                if( slot < rhs.slot )
                    return true;
                else if( slot == rhs.slot )
                    return port < rhs.port;
                else
                    return false;
            }

        void load( QSettings &S, int i );
        void save( QSettings &S, int i ) const;
    };

    struct ImSlotVers {
        QString     bsfw,   // maj.min
                    bscsn,
                    bschw,  // maj.min
                    bscfw;  // maj.min
    };

    struct ImProbeTable {
        int                     comIdx;     // 0=PXI
        QVector<ImProbeDat>     probes;
        QVector<int>            id2dat;     // probeID -> ImProbeDat
        QVector<int>            slot;       // used slots
        QString                 api;        // maj.min
        QMap<int,ImSlotVers>    slot2Vers;

        void init();

        void loadSettings();
        void saveSettings() const;

        void toGUI( QTableWidget *T ) const;
        void fromGUI( QTableWidget *T );
    };

    // -------------------------------
    // Attributes common to all probes
    // -------------------------------

    struct AttrAll {
        VRange  range;
        double  srate;
        bool    softStart;

        AttrAll() : range(VRange(-0.6,0.6)), srate(3e4)  {}
    };

    // --------------------------
    // Attributes for given probe
    // --------------------------

    struct AttrEach {
        QString     imroFile,
                    stdbyStr;
        IMROTbl     roTbl;
        QBitArray   stdbyBits;
        int         imCumTypCnt[imNTypes];
        int         hpFltIdx;
        bool        doGainCor,
                    LEDEnable;

        AttrEach() : hpFltIdx(0), doGainCor(false), LEDEnable(false)    {}

        void deriveChanCounts( int type );
        bool deriveStdbyBits( QString &err, int nAP );

        void justAPBits(
            QBitArray       &apBits,
            const QBitArray &saveBits ) const;

        void justLFBits(
            QBitArray       &lfBits,
            const QBitArray &saveBits ) const;

        double chanGain( int ic ) const;
    };

    // ------
    // Params
    // ------

    // derived:
    // stdbyBits
    // each.imCumTypCnt[]

public:
    AttrAll             all;
    QVector<AttrEach>   each;
    int                 nProbes;
    bool                enabled;

    CimCfg() : nProbes(1), enabled(false)   {each.resize( 1 );}

    // -------------
    // Param methods
    // -------------

public:
    int vToInt10( double v, int ip, int ic ) const;
    double int10ToV( int i10, int ip, int ic ) const;

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    static int idxToFlt( int idx );

    // ------
    // Config
    // ------

    static bool detect( QStringList &sl, ImProbeTable &T );
};

#endif  // CIMCFG_H


