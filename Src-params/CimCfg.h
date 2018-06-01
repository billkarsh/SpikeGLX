#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"
#include "SnsMaps.h"

#include <QMap>

class QSettings;
class QTableWidget;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// MS: Does stdby still make sense?

struct IMRODesc
{
    qint16  bank,
            apgn,   // gain, not index
            lfgn;   // gain, not index
    qint8   refid,  // reference index
            apflt;  // bool

    IMRODesc()
    :   bank(0), apgn(500), lfgn(250), refid(0), apflt(1)               {}
    IMRODesc( int bank, int refid, int apgn, int lfgn, bool apflt )
    :   bank(bank), apgn(apgn), lfgn(lfgn), refid(refid), apflt(apflt)  {}
    bool operator==( const IMRODesc &rhs ) const
        {return bank==rhs.bank && apgn==rhs.apgn && lfgn==rhs.lfgn
            && refid==rhs.refid && apflt==rhs.apflt;}
    QString toString( int chn ) const;
    static IMRODesc fromString( const QString &s );
};


struct IMROTbl
{
    enum imLims {
        imType0Elec     = 960,

        imType0Banks    = 3,

        imType0Chan     = 384,

        imNRefids       = 5,

        imNGains        = 8
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

    static int typeToNElec( int type );
    static int chToEl384( int ch, int bank );
    static bool chIsRef( int ch );
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
        QString     hspn,       // detect
                    hsfw,       // detect
                    fxhw,       // detect
                    pn;         // detect
        quint64     hssn,       // detect   {UNSET64=unset}
                    sn;         // detect   {UNSET64=unset}
        quint16     type;       // detect   {-1=unset}
        bool        enab;       // ini
        quint16     ip;         // calc     {-1=unset}

        ImProbeDat( int slot, int port )
        :   slot(slot), port(port), enab(true)  {init();}
        ImProbeDat()
        :   enab(true)                          {init();}

        void init()
            {
                hspn.clear();
                hsfw.clear();
                fxhw.clear();
                pn.clear();
                hssn    = UNSET64;
                sn      = UNSET64;
                type    = -1;
                ip      = -1;
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
                    bscpn,  // char
                    bscsn,  // u64
                    bschw,  // maj.min
                    bscfw;  // maj.min
    };

    struct ImProbeTable {
private:
        int                     comIdx;     // 0=PXI
        QVector<ImProbeDat>     probes;
        QVector<int>            id2dat;     // probeID -> ImProbeDat
public:
        QVector<int>            slot;       // used slots
        QString                 api;        // maj.min
        QMap<int,ImSlotVers>    slot2Vers;

        void init();
        void clearProbes()          {probes.clear();}
        void setOneXilinx( QTableWidget *T );
        bool addSlot( QTableWidget *T, int slot );
        bool rmvSlot( QTableWidget *T, int slot );

        void set_comIdx( int idx )  {comIdx=idx;}
        int get_comIdx() const      {return comIdx;}

        int buildEnabIndexTables();
        int buildQualIndexTables();
        int nPhyProbes() const      {return probes.size();}
        int nLogProbes() const      {return id2dat.size();}

        ImProbeDat& mod_iProbe( int i )
            {return probes[id2dat[i]];}

        const ImProbeDat& get_kPhyProbe( int k ) const
            {return probes[k];}

        const ImProbeDat& get_iProbe( int i ) const
            {return probes[id2dat[i]];}

        int nQualPortsThisSlot( int slot ) const;

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
        int     trgSource;
        bool    trgRising;

        AttrAll() : range(VRange(-0.6,0.6)) {}
    };

    // --------------------------
    // Attributes for given probe
    // --------------------------

    // derived:
    // stdbyBits
    // imCumTypCnt[]

    struct AttrEach {
        double          srate;
        QString         imroFile,
                        stdbyStr;
        IMROTbl         roTbl;
        QBitArray       stdbyBits;
        int             imCumTypCnt[imNTypes];
        bool            skipCal,
                        LEDEnable;
        SnsChansImec    sns;

        AttrEach()
        :   srate(30000.0), skipCal(false), LEDEnable(false)    {}

        void deriveChanCounts( int type );
        bool deriveStdbyBits( QString &err, int nAP );

        void justAPBits(
            QBitArray       &apBits,
            const QBitArray &saveBits ) const;

        void justLFBits(
            QBitArray       &lfBits,
            const QBitArray &saveBits ) const;

        void apSaveBits( QBitArray &apBits ) const;
        void lfSaveBits( QBitArray &lfBits ) const;

        int apSaveChanCount() const;
        int lfSaveChanCount() const;

        double chanGain( int ic ) const;
    };

    // ------
    // Params
    // ------

private:
    int                 nProbes;
public:
    AttrAll             all;
    QVector<AttrEach>   each;
    bool                enabled;

    CimCfg() : nProbes(1), enabled(false)   {each.resize( 1 );}

    // -------------
    // Param methods
    // -------------

public:
    void set_nProbes( int np )
        {nProbes = (np > 0 ? np : 1); each.resize( nProbes );}
    int get_nProbes() const
        {return (enabled ? nProbes : 0);}

    int vToInt10( double v, int ip, int ic ) const;
    double int10ToV( int i10, int ip, int ic ) const;

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    // ------
    // Config
    // ------

    static bool detect( QStringList &sl, ImProbeTable &T );
};

#endif  // CIMCFG_H


