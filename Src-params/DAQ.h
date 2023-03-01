#ifndef DAQ_H
#define DAQ_H

#include "CimCfg.h"
#include "CniCfg.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

enum jsType {
    jsNI    = 0,
    jsOB    = 1,
    jsIM    = 2
};

namespace DAQ
{
/* --------- */
/* Constants */
/* --------- */

enum SyncSource {
    eSyncSourceNone = 0,
    eSyncSourceExt  = 1,
    eSyncSourceNI   = 2,
    eSyncSourceIM   = 3
};

enum GateMode {
    eGateImmed  = 0,
    eGateTCP,
    N_gateModes
};

enum TrigMode {
    eTrigImmed  = 0,
    eTrigTimed,
    eTrigTTL,
    eTrigSpike,
    eTrigTCP,
    N_trigModes
};

enum TrgTTLMode {
    TrgTTLLatch     = 0,
    TrgTTLTimed     = 1,
    TrgTTLFollowV   = 2
};

/* ----- */
/* Types */
/* ----- */

struct SyncParams {
    double      sourcePeriod;
    SyncSource  sourceIdx;
    double      niThresh;
    int         imPXIInputSlot,
                niChanType,     // {0=digital, 1=analog}
                niChan,
                calMins;
    bool        isCalRun;
};

struct DOParams {
// BK: Future programmable digital out

    void deriveDOParams();
};

struct TrgTimParams {
    double          tL0,
                    tH,
                    tL;
    uint            nH;
    bool            isHInf,
                    isNInf;
};

struct TrgTTLParams {
    double          T,
                    marginSecs,
                    refractSecs,
                    tH;
    QString         stream;
    int             mode,
                    chan,
                    bit;
    uint            inarow,
                    nH;
    bool            isAnalog,
                    isNInf;
};

struct TrgSpikeParams {
    double          T,
                    periEvtSecs,
                    refractSecs;
    QString         stream;
    int             aiChan;
    uint            inarow,
                    nS;
    bool            isNInf;
};

struct ModeParams {
    GateMode        mGate;
    TrigMode        mTrig;
    int             initG,  // (-1,-1) or continuation indices
                    initT;
    bool            manOvShowBut,
                    manOvInitOff,
                    manOvConfirm;
    ModeParams() : initG(-1), initT(-1) {}
};

struct SeeNSave {
    QString         notes,
                    runName;
    int             reqMins;
    bool            pairChk,
                    fldPerPrb;
};

struct Params {
    CimCfg          im;
    CniCfg          ni;
    SyncParams      sync;
    DOParams        DO;
    TrgTimParams    trgTim;
    TrgTTLParams    trgTTL;
    TrgSpikeParams  trgSpike;
    ModeParams      mode;
    SeeNSave        sns;

    static inline bool stream_isNI( const QString &stream )
        {return stream.startsWith( "n" );}
    static inline bool stream_isOB( const QString &stream )
        {return stream.startsWith( "o" );}
    static inline bool stream_isIM( const QString &stream )
        {return stream.startsWith( "i" );}

    inline int stream_nNI() const   {return ni.enabled;}
    inline int stream_nOB() const   {return im.get_nOneBox();}
    inline int stream_nIM() const   {return im.get_nProbes();}
    inline int stream_nq() const    {return stream_nNI() + stream_nOB() + stream_nIM();}

    static int stream2js( const QString &stream );
    static int stream2ip( const QString &stream );
    int stream2jsip( int &ip, const QString &stream ) const;
    int stream2iq( const QString &stream ) const;

    static QString jsip2stream( int js, int ip );
    QString iq2stream( int iq ) const;
    int iq2jsip( int &ip, int iq ) const;

    double stream_rate( int js, int ip ) const;
    int stream_nChans( int js, int ip ) const;

    void streamCB_fillRuntime( QComboBox *CB ) const;
    bool streamCB_selItem( QComboBox *CB, QString stream, bool autosel ) const;

    QString trigStream() const;
    int trigThreshAsInt() const;
    int trigChan() const;
    bool trig_isChan( int js, int ip, int chan ) const
        {return jsip2stream( js, ip ) == trigStream() && chan == trigChan();}
    bool trigStream_isNI( const QString &stream ) const;
    bool trigStream_isOB( const QString &stream ) const;
    bool trigStream_isIM( const QString &stream ) const;

    void loadSettings( bool remote = false );
    void saveSettings( bool remote = false ) const;

    static QString remoteGetDAQParams();
    static void remoteSetDAQParams( const QString &str );
};

/* ------- */
/* Methods */
/* ------- */

const QString& gateModeToString( GateMode gateMode );
GateMode stringToGateMode( const QString &str );

const QString& trigModeToString( TrigMode trigMode );
TrigMode stringToTrigMode( const QString &str );

}   // namespace DAQ

#endif  // DAQ_H


