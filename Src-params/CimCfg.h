#ifndef CIMCFG_H
#define CIMCFG_H

#include "SGLTypes.h"
#include "SnsMaps.h"
#include "IMROTbl.h"
#include "SimProbes.h"

#include <QMap>
#include <QSet>

class QSettings;
class QTableWidget;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define imOBX_SRATE 30303.0
#define imOBX_NCHN  12

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
        imNTypes    = 3,

        obTypeXA    = 0,
        obTypeXD    = 1,
        obTypeSY    = 2,
        obSumAnalog = 0,
        obSumData   = 1,
        obSumAll    = 2,
        obNTypes    = 3
    };

    enum imSlot {
        imSlotPXIMin    = 2,
        imSlotPXILim    = 19,
        imSlotNone      = 19,
        imSlotUSBMin    = 20,
        imSlotUSBLim    = 32,
        imSlotSIMMin    = 40,
        imSlotSIMLim    = 42,
        imSlotMin       = imSlotPXIMin,
        imSlotPhyLim    = imSlotUSBLim,
        imSlotLogLim    = imSlotSIMLim
    };

    // -----
    // Types
    // -----

    struct CfgSlot {
        int     slot,
                ID;         // 0=PXI/SIM, or OneBox ID
        qint16  show;       // 0=not, 1=1-dock, 2=2-dock
        bool    detected;

        CfgSlot()
        :   slot(0), ID(0), show(0), detected(false)            {}
        CfgSlot( int slot, int ID, short show, bool detected )
        :   slot(slot), ID(ID), show(show), detected(detected)  {}

        bool operator<( const CfgSlot &rhs ) const
            {
                if( slot == imSlotNone ) {
                    if( rhs.slot != imSlotNone )
                        return false;
                    else
                        return ID < rhs.ID;
                }
                else if( rhs.slot == imSlotNone )
                    return true;
                else if( slot < rhs.slot )
                    return true;
                else if( slot == rhs.slot )
                    return ID < rhs.ID;
                else
                    return false;
            }
    };

    // One per probe (port != 9) or OneBox (port == 9)
    //
    struct ImProbeDat {
        quint16     slot,       // ini
                    port;       // ini
        QString     hspn,       // detect
                    hshw,       // detect
                    hsfw,       // detect
                    fxpn,       // detect
                    fxsn,       // detect
                    fxhw,       // detect
                    pn;         // detect   {pn or obx}
        quint64     hssn,       // detect   {UNSET64=unset}
                    sn;         // detect   {UNSET64=unset}
        int         obsn,       // detect   {-1=unset}
                    type,       // detect   {-1=unset}
                    tech;       // detect   {-1=unset}
        quint16     dock,       // ini
                    cal,        // detect   {-1=unset,0=N,1=Y}
                    ip;         // calc     {-1=unset}
        bool        enab;       // ini

        ImProbeDat( int slot, int port, int dock, bool enab )
        :   slot(slot), port(port),
            dock(dock), enab(enab)  {init();}
        ImProbeDat()
        :   enab(false)             {init();}

        void init()
            {
                hspn.clear();
                hshw.clear();
                hsfw.clear();
                fxpn.clear();
                fxsn.clear();
                fxhw.clear();
                pn.clear();
                hssn    = UNSET64;
                sn      = UNSET64;
                obsn    = -1;
                type    = -1;
                tech    = -1;
                cal     = -1;
                ip      = -1;
            }

        bool operator<( const ImProbeDat &rhs ) const
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

        bool isProbe() const    {return port != 9;}
        bool isOneBox() const   {return port == 9;}
        bool setProbeType();
        int nHSDocks() const;
        quint64 calSN() const   {return (type == 1200 ? hssn : sn);}

        void loadSettings( QSettings &S, int i );
        void saveSettings( QSettings &S, int i ) const;
    };

    struct ImSlotVers {
        QString     bsfw,       // maj.min
                    bscpn,      // char
                    bscsn,      // u64
                    bschw,      // maj.min
                    bscfw;      // maj.min
        int         bsctech;    // T_probe_tech
    };

    struct ProbeAddr {
        QSet<quint16>   addr;

        void clear()
            {addr.clear();}
        void store( int slot, int port, int dock )
            {addr.insert( (slot << 8) + (port << 4) + dock );}
        bool has( int slot, int port, int dock ) const
            {return addr.contains( (slot << 8) + (port << 4) + dock );}
    };

    // Probes (port != 9) and OneBoxes (port == 9)
    //
    struct ImProbeTable {
private:
        QVector<ImProbeDat>     probes;
        QVector<int>            iprb2dat;   // sel probeID -> ImProbeDat
        QVector<int>            iobx2dat;   // sel obxID   -> ImProbeDat
        QVector<int>            slotsUsed;  // used slots
        QMap<int,int>           slot2zIdx;  // slot -> zero-based order idx
        QMap<int,int>           slot2type;  // slot -> {0=PXI,1=OneBox}
        QMap<int,int>           onebx2slot; // Obx ID -> slot
        QMap<quint64,double>    hssn2srate; // hssn -> srate
        QMap<int,double>        obsn2srate; // obsn -> srate
        ProbeAddr               setEnabled; // which probes are enabled
public:
        SimProbes               simprb;
        QString                 api;        // maj.min
        QMap<int,ImSlotVers>    slot2Vers;

        void init();
        int buildEnabIndexTables();
        int buildQualIndexTables();
        bool haveQualCalFiles() const;
        int nTblEntries() const     {return probes.size();}
        int nSelSlots() const       {return slotsUsed.size();}
        int nSelProbes() const      {return iprb2dat.size();}
        int nSelOneBox() const      {return iobx2dat.size();}

        void setCfgSlots( const QVector<CfgSlot> &vCS );
        void getCfgSlots( QVector<CfgSlot> &vCS );
        bool scanCfgSlots( QVector<CfgSlot> &vCS, QString &msg ) const;

        bool mapObxSlots( QStringList &slVers );

        bool isSlotUsed( int slot ) const
            {return slotsUsed.contains( slot );}

        int getEnumSlot( int i ) const
            {return slotsUsed[i];}

        void setSlotType( int slot, int bstype )
            {slot2type[slot] = bstype;}

        int getSlotType( int slot ) const
            {return slot2type[slot];}

        bool anySlotPXIType() const;
        bool anySlotUSBType() const;

        bool isSlotPXIType( int slot ) const;
        bool isSlotUSBType( int slot ) const;

        int getTypedSlots( QVector<int> &vslot, int bstype ) const;

        ImProbeDat& mod_iProbe( int i )
            {return probes[iprb2dat[i]];}

        ImProbeDat& mod_iOneBox( int isel )
            {return probes[iobx2dat[isel]];}

        const ImProbeDat& get_kTblEntry( int k ) const
            {return probes[k];}

        const ImProbeDat& get_iProbe( int i ) const
            {return probes[iprb2dat[i]];}

        const ImProbeDat& get_iOneBox( int isel ) const
            {return probes[iobx2dat[isel]];}

        int nQualStreamsThisSlot( int slot ) const;

        double get_iProbe_SRate( int i ) const;
        void set_iProbe_SRate( int i, double srate )
            {hssn2srate[probes[iprb2dat[i]].hssn] = srate;}
        void set_hssn_SRate( quint64 hssn, double srate )
            {hssn2srate[hssn] = srate;}

        double get_iOneBox_SRate( int isel ) const;
        void set_iOneBox_SRate( int isel, double srate )
            {obsn2srate[probes[iobx2dat[isel]].obsn] = srate;}
        void set_obsn_SRate( int obsn, double srate )
            {obsn2srate[obsn] = srate;}

        void loadProbeSRates();
        void saveProbeSRates() const;

        void loadOneBoxSRates();
        void saveOneBoxSRates() const;

        void loadSlotTable();
        void saveSlotTable() const;

        void loadProbeTable();
        void saveProbeTable() const;

        void toGUI( QTableWidget *T ) const;
        void fromGUI( QTableWidget *T );

        void toggleAll(
            QTableWidget    *T,
            int             row,
            int             subset ) const;
        QString whosChecked( QTableWidget *T, bool b_hssn ) const;
    };

    // -------------------------------
    // Attributes common to all probes
    // -------------------------------

    struct PrbAll {
        QString qf_secsStr,
                qf_loCutStr,
                qf_hiCutStr;
        int     calPolicy,  // {0=required,1=avail,2=never}
                trgSource,  // {0=software,1=SMA}
                svySettleSec,
                svySecPerBnk;
        bool    lowLatency,
                trgRising,
                bistAtDetect,
                isSvyRun,
                qf_on;

        PrbAll()
        :   qf_secsStr( ".5" ), qf_loCutStr( "300" ), qf_hiCutStr( "9000" ),
            calPolicy(0), trgSource(0), svySettleSec(2), svySecPerBnk(35),
            lowLatency(false), trgRising(true), bistAtDetect(true),
            isSvyRun(false), qf_on(true)    {}

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;

        static QString remoteGetPrbAll();
        static void remoteSetPrbAll( const QString &str );
    };

    // --------------------------
    // Attributes for given probe
    // --------------------------

    // derived:
    // stdbyBits
    // imCumTypCnt[]

    struct PrbEach {
        double          srate;
        QString         when,       // last set by user
                        imroFile,
                        stdbyStr;
        IMROTbl         *roTbl;
        QBitArray       stdbyBits;
        int             imCumTypCnt[imNTypes],
                        svyMaxBnk;  // -1=all
        bool            LEDEnable;
        SnsChansImec    sns;

        PrbEach()
        :   srate(30000.0), roTbl(0),
            svyMaxBnk(-1), LEDEnable(false) {}
        PrbEach( const PrbEach &rhs )
        {
            srate       = rhs.srate;
            when        = rhs.when;
            imroFile    = rhs.imroFile;
            stdbyStr    = rhs.stdbyStr;
            stdbyBits   = rhs.stdbyBits;
            svyMaxBnk   = rhs.svyMaxBnk;
            LEDEnable   = rhs.LEDEnable;
            sns         = rhs.sns;

            if( rhs.roTbl ) {
                roTbl = IMROTbl::alloc( rhs.roTbl->pn );
                roTbl->copyFrom( rhs.roTbl );
            }
            else
                roTbl = 0;

            for( int i = 0; i < imNTypes; ++i )
                imCumTypCnt[i] = rhs.imCumTypCnt[i];
        }
        PrbEach& operator=( const PrbEach &rhs )
        {
            srate       = rhs.srate;
            when        = rhs.when;
            imroFile    = rhs.imroFile;
            stdbyStr    = rhs.stdbyStr;
            stdbyBits   = rhs.stdbyBits;
            svyMaxBnk   = rhs.svyMaxBnk;
            LEDEnable   = rhs.LEDEnable;
            sns         = rhs.sns;

            if( roTbl ) {
                delete roTbl;
                roTbl = 0;
            }

            if( rhs.roTbl ) {
                roTbl = IMROTbl::alloc( rhs.roTbl->pn );
                roTbl->copyFrom( rhs.roTbl );
            }
            else
                roTbl = 0;

            for( int i = 0; i < imNTypes; ++i )
                imCumTypCnt[i] = rhs.imCumTypCnt[i];

            return *this;
        }
        virtual ~PrbEach()  {if( roTbl ) {delete roTbl; roTbl = 0;}}

        void deriveChanCounts();
        bool deriveStdbyBits( QString &err, int nAP, int ip );

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

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;

        QString remoteGetGeomMap() const;
        QString remoteGetPrbEach() const;
    };

    // ---------------------------
    // Attributes for given OneBox
    // ---------------------------

    // derived:
    // obCumTypCnt[]

    struct ObxEach {
        VRange          range;
        double          srate;
        QString         when,       // last set by user
                        uiXAStr,
                        uiAOStr;
        int             obCumTypCnt[obNTypes];
        bool            isXD;
        SnsChansObx     sns;

        ObxEach()
        :   range(-5,5), srate(imOBX_SRATE),
            uiXAStr("0:11"), isXD(true) {}

        void deriveChanCounts();

        bool isStream() const   {return isXD || !uiXAStr.isEmpty();}

        int vToInt16( double v ) const;
        double int16ToV( int i16 ) const;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;

        QString remoteGetObxEach() const;
    };

    // ------
    // Params
    // ------

private:
    QVector<ObxEach>    obxj;       // selected params
    QVector<int>        istr2isel;  // stream-ip -> selected-ip
    QMap<int,int>       slot2istr;  // slot -> stream-ip
    int                 nObxStr;    // num recording
public:
    PrbAll              prbAll;
    QVector<PrbEach>    prbj;       // selected params
    bool                enabled;

    CimCfg() : enabled(false)   {set_ini_nprb_nobx(0, 0);}

    // -------------
    // Param methods
    // -------------

public:
    void set_ini_nprb_nobx( int nprb, int nobx );
    void set_cfg_def_no_streams( const CimCfg &RHS );
    void set_cfg_nprb( const QVector<PrbEach> &each, int nprb );
    void set_cfg_nobx( const QVector<ObxEach> &each, int nobx );
    void set_cfg_obxj_istr_data( const ImProbeTable &T );
    int get_nProbes() const             {return (enabled ? prbj.size() : 0);}
    int get_nOneBox() const             {return (enabled ? obxj.size() : 0);}
    int get_nObxStr() const             {return (enabled ? nObxStr : 0);}
    int obx_istr2isel( int istr ) const {return istr2isel[istr];}
    int obx_slot2istr( int slot ) const;

    ObxEach& mod_iSelOneBox( int isel )             {return obxj[isel];}
    ObxEach& mod_iStrOneBox( int istr )             {return obxj[istr2isel[istr]];}
    const ObxEach& get_iSelOneBox( int isel ) const {return obxj[isel];}
    const ObxEach& get_iStrOneBox( int istr ) const {return obxj[istr2isel[istr]];}

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    // ------
    // Config
    // ------

    static bool isBSSupported( int slot );
    static void closeAllBS( bool report = true );
    static bool detect(
        QStringList             &slVers,
        QStringList             &slBIST,
        QVector<int>            &vHSpsv,
        QVector<int>            &vHS20,
        ImProbeTable            &T,
        bool                    doBIST );
    static void detect_API(
        QStringList             &slVers,
        ImProbeTable            &T );
    static bool detect_slots(
        QStringList             &slVers,
        ImProbeTable            &T );
    static bool detect_slot_type(
        QStringList             &slVers,
        ImProbeTable            &T,
        int                     slot );
    static bool detect_slot_openBS(
        QStringList             &slVers,
        int                     slot );
    static bool detect_slot_BSFW(
        QStringList             &slVers,
        ImSlotVers              &V,
        int                     slot );
    static bool detect_slot_BSC_hID(
        QStringList             &slVers,
        ImSlotVers              &V,
        int                     slot );
    static bool detect_slot_BSCFW(
        QStringList             &slVers,
        ImProbeTable            &T,
        ImSlotVers              &V,
        int                     slot );
    static void detect_simSlot(
        QStringList             &slVers,
        ImProbeTable            &T,
        int                     slot );
    static bool detect_headstages(
        QStringList             &slVers,
        QMap<int,QString>       &qbMap,
        QVector<int>            &vHSpsv,
        QVector<int>            &vHS20,
        ImProbeTable            &T );
    static bool detect_probes(
        QStringList             &slVers,
        QStringList             &slBIST,
        const QMap<int,QString> &qbMap,
        const QVector<int>      &vHSpsv,
        ImProbeTable            &T,
        bool                    doBIST );
    static bool detect_simProbe(
        QStringList             &slVers,
        ImProbeTable            &T,
        ImProbeDat              &P );
    static void detect_OneBoxes( ImProbeTable &T );
    static bool testFixCalPath( quint64 sn );
    static bool ftdiCheck( QString &msg, bool usbInTbl );
    static void forceProbeData(
        int             slot,
        int             port,
        int             dock,
        const QString   &sn,
        const QString   &pn );
};

#endif  // CIMCFG_H


