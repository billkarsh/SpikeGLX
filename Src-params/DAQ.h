#ifndef DAQ_H
#define DAQ_H

#include "CimCfg.h"
#include "CniCfg.h"
#include "ChanMap.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

namespace DAQ
{
/* --------- */
/* Constants */
/* --------- */

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
    TrgTTLFollowAI  = 2
};

/* ----- */
/* Types */
/* ----- */

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
                    aiChan;
    uint            inarow,
                    nH;
    bool            isNInf;
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
    bool            manOvShowBut,
                    manOvInitOff;
};

struct SnsChansBase {
//
// derived:
// chanMap, (ConfigCtl::validChanMap)
// saveBits
//
    QString         chanMapFile,
                    uiSaveChanStr;
    QBitArray       saveBits;
    virtual QString type() = 0;
    bool deriveSaveBits( QString &err, int n16BitChans );
};

struct SnsChansImec : public SnsChansBase {
    ChanMapIM       chanMap;
    virtual QString type()  {return "imec";}
};

struct SnsChansNidq : public SnsChansBase {
    ChanMapNI       chanMap;
    virtual QString type()  {return "nidq";}
};

struct SeeNSave {
//
// derived:
// chanMap, (ConfigCtl::validChanMap)
// saveBits
//
    SnsChansImec    imChans;
    SnsChansNidq    niChans;
    QString         runName;
};

struct Params {
    CimCfg          im;
    CniCfg          ni;
    DOParams        DO;
    TrgTimParams    trgTim;
    TrgTTLParams    trgTTL;
    TrgSpikeParams  trgSpike;
    ModeParams      mode;
    SeeNSave        sns;

    QString trigStream() const;
    int trigChan() const;
    bool isTrigChan( QString stream, int chan ) const
        {return stream == trigStream() && chan == trigChan();}

    void loadSettings( bool remote = false );
    void saveSettings( bool remote = false ) const;

    static QString settings2Str();
    static void str2RemoteSettings( const QString &str );
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


