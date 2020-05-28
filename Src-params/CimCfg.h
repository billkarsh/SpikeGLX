#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"
#include "SnsMaps.h"
#include "IMROTbl.h"

#include <QMap>

class QSettings;
class QTableWidget;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

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
                    fxpn,       // detect
                    fxhw,       // detect
                    pn;         // detect
        quint64     hssn,       // detect   {UNSET64=unset}
                    sn;         // detect   {UNSET64=unset}
        quint16     dock,       // ini
                    type;       // detect   {-1=unset}
        bool        enab;       // ini
        quint16     cal,        // detect   {-1=unset,0=N,1=Y}
                    ip;         // calc     {-1=unset}

        ImProbeDat( int slot, int port, int dock )
        :   slot(slot), port(port),
            dock(dock), enab(true)  {init();}
        ImProbeDat()
        :   enab(true)              {init();}

        void init()
            {
                hspn.clear();
                hsfw.clear();
                fxpn.clear();
                fxhw.clear();
                pn.clear();
                hssn    = UNSET64;
                sn      = UNSET64;
                type    = -1;
                cal     = -1;
                ip      = -1;
            }

        bool operator< ( const ImProbeDat &rhs ) const
            {
                if( slot < rhs.slot )
                    return true;
                else if( slot == rhs.slot ) {

                    if( port < rhs.port )
                        return true;
                    else if( port == rhs.port )
                        return dock < rhs.dock;
                    else
                        return false;
                }
                else
                    return false;
            }

        bool setProbeType();

        void loadSettings( QSettings &S, int i );
        void saveSettings( QSettings &S, int i ) const;
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
        QVector<ImProbeDat>     probes;
        QVector<int>            id2dat;     // probeID -> ImProbeDat
        QVector<int>            slotsUsed;  // used slots
        QMap<int,int>           slot2zIdx;  // slot -> zero-based order idx
        QMap<quint64,double>    srateTable; // hssn -> srate
public:
        QString                 api;        // maj.min
        QMap<int,ImSlotVers>    slot2Vers;

        void init();
        void clearProbes()          {probes.clear();}
        bool addSlot( QTableWidget *T, int slot );
        bool rmvSlot( QTableWidget *T, int slot );

        int buildEnabIndexTables();
        int buildQualIndexTables();
        bool haveQualCalFiles() const;
        int nLogSlots() const       {return slotsUsed.size();}
        int nPhyProbes() const      {return probes.size();}
        int nLogProbes() const      {return id2dat.size();}

        bool isSlotUsed( int slot ) const
            {return slotsUsed.contains( slot );}

        int getEnumSlot( int i ) const
            {return slotsUsed[i];}

        ImProbeDat& mod_iProbe( int i )
            {return probes[id2dat[i]];}

        const ImProbeDat& get_kPhyProbe( int k ) const
            {return probes[k];}

        const ImProbeDat& get_iProbe( int i ) const
            {return probes[id2dat[i]];}

        int nQualDocksThisSlot( int slot ) const;

        double getSRate( int i ) const;
        void setSRate( int i, double srate )
            {srateTable[probes[id2dat[i]].hssn] = srate;}

        void loadSRateTable();
        void saveSRateTable() const;

        void loadSettings();
        void saveSettings() const;

        void toGUI( QTableWidget *T ) const;
        void fromGUI( QTableWidget *T );

        void toggleAll(
            QTableWidget    *T,
            int             row,
            int             subset ) const;
        void whosChecked( QString &s, QTableWidget *T ) const;
    };

    // -------------------------------
    // Attributes common to all probes
    // -------------------------------

    struct AttrAll {
        int     calPolicy,  // {0=required,1=avail,2=never}
                trgSource;  // {0=software,1=SMA}
        bool    trgRising,
                bistAtDetect;

        AttrAll()
        :   calPolicy(0),
            trgSource(0), trgRising(true),
            bistAtDetect(true)  {}

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
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
        IMROTbl         *roTbl;
        QBitArray       stdbyBits;
        int             imCumTypCnt[imNTypes];
        bool            LEDEnable;
        SnsChansImec    sns;

        AttrEach()
        :   srate(30000.0), roTbl(0), LEDEnable(false)  {}
        AttrEach( const AttrEach &rhs )
        {
            srate       = rhs.srate;
            imroFile    = rhs.imroFile;
            stdbyStr    = rhs.stdbyStr;
            stdbyBits   = rhs.stdbyBits;
            LEDEnable   = rhs.LEDEnable;
            sns         = rhs.sns;

            if( rhs.roTbl ) {
                roTbl = IMROTbl::alloc( rhs.roTbl->type );
                roTbl->copyFrom( rhs.roTbl );
            }
            else
                roTbl = 0;

            for( int i = 0; i < imNTypes; ++i )
                imCumTypCnt[i] = rhs.imCumTypCnt[i];
        }
        AttrEach& operator=( const AttrEach &rhs )
        {
            srate       = rhs.srate;
            imroFile    = rhs.imroFile;
            stdbyStr    = rhs.stdbyStr;
            stdbyBits   = rhs.stdbyBits;
            LEDEnable   = rhs.LEDEnable;
            sns         = rhs.sns;

            if( roTbl ) {
                delete roTbl;
                roTbl = 0;
            }

            if( rhs.roTbl ) {
                roTbl = IMROTbl::alloc( rhs.roTbl->type );
                roTbl->copyFrom( rhs.roTbl );
            }
            else
                roTbl = 0;

            for( int i = 0; i < imNTypes; ++i )
                imCumTypCnt[i] = rhs.imCumTypCnt[i];

            return *this;
        }
        virtual ~AttrEach() {if( roTbl ) {delete roTbl, roTbl = 0;}}

        void loadSettings( QSettings &S, int ip );
        void saveSettings( QSettings &S, int ip ) const;

        void deriveChanCounts();
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

        bool lfIsSaving() const;

        double chanGain( int ic ) const;

        int vToInt( double v, int ic ) const;
        double intToV( int i, int ic ) const;
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

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    // ------
    // Config
    // ------

    static void closeAllBS();
    static bool detect(
        QStringList     &slVers,
        QStringList     &slBIST,
        QVector<int>    &vHS20,
        ImProbeTable    &T,
        bool            doBIST );
    static void forceProbeData(
        int             slot,
        int             port,
        int             dock,
        const QString   &sn,
        const QString   &pn );
};

#endif  // CIMCFG_H


