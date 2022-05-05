
#include "Util.h"
#include "CniCfg.h"
#include "Subset.h"
#include "SignalBlocker.h"

#ifdef HAVE_NIDAQmx
#include "NI/NIDAQmx.h"
#else
#pragma message("*** Message to self: Building simulated NI-DAQ version ***")
#endif

#include <QComboBox>
#include <QSettings>
#include <QThread>


/* ---------------------------------------------------------------- */
/* Param methods -------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CniCfg::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        if( ic < niCumTypCnt[niTypeMN] )
            g = mnGain;
        else if( ic < niCumTypCnt[niTypeMA] )
            g = maGain;

        if( g < 0.01 )
            g = 0.01;
    }

    return g;
}


// Given input fields:
// - uiMNStr1, uiMAStr1, uiXAStr1, uiXDStr1
// - uiMNStr2, uiMAStr2, uiXAStr2, uiXDStr2
// - muxFactor
// - isDualDevMode
//
// Derive:
// - xdBytes1
// - xdBytes2
// - niCumTypCnt[]
//
// Note:
// Nobody except the NI-DAQ card reader CniAcqDmx needs to distinguish
// counts of channels that separately come from dev1 and dev2.
//
void CniCfg::deriveChanCounts()
{
    QVector<uint>   vc;

// --------------------------------
// First count each type separately
// --------------------------------

    Subset::rngStr2Vec( vc, uiMNStr1 );
    niCumTypCnt[niTypeMN] = vc.size();
    Subset::rngStr2Vec( vc, uiMNStr2() );
    niCumTypCnt[niTypeMN] += vc.size();
    niCumTypCnt[niTypeMN] *= muxFactor;

    Subset::rngStr2Vec( vc, uiMAStr1 );
    niCumTypCnt[niTypeMA] = vc.size();
    Subset::rngStr2Vec( vc, uiMAStr2() );
    niCumTypCnt[niTypeMA] += vc.size();
    niCumTypCnt[niTypeMA] *= muxFactor;

    Subset::rngStr2Vec( vc, uiXAStr1 );
    niCumTypCnt[niTypeXA] = vc.size();
    Subset::rngStr2Vec( vc, uiXAStr2() );
    niCumTypCnt[niTypeXA] += vc.size();

    int nc;

    Subset::rngStr2Vec( vc, uiXDStr1 );
    if( (nc = vc.size()) )
        xdBytes1 = 1 + vc[nc-1]/8;
    else
        xdBytes1 = 0;

    Subset::rngStr2Vec( vc, uiXDStr2() );
    if( (nc = vc.size()) )
        xdBytes2 = 1 + vc[nc-1]/8;
    else
        xdBytes2 = 0;

    niCumTypCnt[niTypeXD] = (1 + xdBytes1+xdBytes2)/2;

// ---------
// Integrate
// ---------

    for( int i = 1; i < niNTypes; ++i )
        niCumTypCnt[i] += niCumTypCnt[i - 1];
}


int CniCfg::vToInt16( double v, int ic ) const
{
    return SHRT_MAX * v * chanGain( ic ) / range.rmax;
}


double CniCfg::int16ToV( int i16, int ic ) const
{
    return range.rmax * i16 / (SHRT_MAX * chanGain( ic ));
}


void CniCfg::fillSRateCB( QComboBox *CB, const QString &selKey ) const
{
    SignalBlocker   b(CB);

    CB->clear();

    foreach( const QString &key, nidev2srate.keys() )
        CB->addItem( key );

    CB->setCurrentIndex( CB->findText( selKey ) );
}


double CniCfg::key2SetRate( const QString &key ) const
{
    const QStringList   sl = key.split(
                                QRegExp("\\s*:\\s*"),
                                QString::SkipEmptyParts );

    if( sl.count() < 2 )
        return 100.0;   // min NI rate Bill allows

    return sl.at( 1 ).toDouble();
}


double CniCfg::getSRate( const QString &key ) const
{
    return nidev2srate.value( key, key2SetRate( key ) );
}


// Calibrated device sample rates.
// Maintained in _Calibration folder.
//
void CniCfg::loadSRateTable()
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_nidq" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedNIDevices" );

    nidev2srate.clear();

    foreach( const QString &key, settings.childKeys() ) {

        nidev2srate[key] =
            settings.value( key, key2SetRate( key ) ).toDouble();
    }
}


// Calibrated device sample rates.
// Maintained in _Calibration folder.
//
void CniCfg::saveSRateTable() const
{
    QSettings settings(
                calibPath( "calibrated_sample_rates_nidq" ),
                QSettings::IniFormat );
    settings.beginGroup( "CalibratedNIDevices" );

    QMap<QString,double>::const_iterator    it;

    for( it = nidev2srate.begin(); it != nidev2srate.end(); ++it )
        settings.setValue( it.key(), it.value() );
}


void CniCfg::loadSettings( QSettings &S )
{
    loadSRateTable();

    range.rmin =
    S.value( "niAiRangeMin", -2.0 ).toDouble();

    range.rmax =
    S.value( "niAiRangeMax", 2.0 ).toDouble();

    settle =
    S.value( "niSettle", 7.0 ).toDouble();

    mnGain =
    S.value( "niMNGain", 200.0 ).toDouble();

    maGain =
    S.value( "niMAGain", 1.0 ).toDouble();

    dev1 =
    S.value( "niDev1", "" ).toString();

    dev2 =
    S.value( "niDev2", "" ).toString();

    clockSource =
    S.value( "niClockSource", "" ).toString();

    clockLine1 =
    S.value( "niClockLine1", "Internal" ).toString();

    clockLine2 =
    S.value( "niClockLine2", "PFI2" ).toString();

    uiMNStr1 =
    S.value( "niMNChans1", "" ).toString();

    uiMAStr1 =
    S.value( "niMAChans1", "" ).toString();

    uiXAStr1 =
    S.value( "niXAChans1", "0:7" ).toString();

    uiXDStr1 =
    S.value( "niXDChans1", "0:7" ).toString();

    setUIMNStr2(
    S.value( "niMNChans2", "0:5" ).toString() );

    setUIMAStr2(
    S.value( "niMAChans2", "6,7" ).toString() );

    setUIXAStr2(
    S.value( "niXAChans2", "" ).toString() );

    setUIXDStr2(
    S.value( "niXDChans2", "" ).toString() );

    muxFactor =
    S.value( "niMuxFactor", 32 ).toUInt();

    termCfg = (TermConfig)
    S.value( "niAiTermConfig", int(Default) ).toInt();

    enabled =
    S.value( "niEnabled", false ).toBool();

    isDualDevMode =
    S.value( "niDualDevMode", false ).toBool();

    startEnable =
    S.value( "niStartEnable", false ).toBool();

    startLine =
    S.value( "niStartLine", "" ).toString();

    sns.shankMapFile =
    S.value( "niSnsShankMapFile", QString() ).toString();

    sns.chanMapFile =
    S.value( "niSnsChanMapFile", QString() ).toString();

    sns.uiSaveChanStr =
    S.value( "niSnsSaveChanSubset", "all" ).toString();
}


void CniCfg::saveSettings( QSettings &S ) const
{
    S.setValue( "niAiRangeMin", range.rmin );
    S.setValue( "niAiRangeMax", range.rmax );
    S.setValue( "niSettle", settle );
    S.setValue( "niMNGain", mnGain );
    S.setValue( "niMAGain", maGain );
    S.setValue( "niDev1", dev1 );
    S.setValue( "niDev2", dev2 );
    S.setValue( "niClockSource", clockSource );
    S.setValue( "niClockLine1", clockLine1 );
    S.setValue( "niClockLine2", clockLine2 );
    S.setValue( "niMNChans1", uiMNStr1 );
    S.setValue( "niMAChans1", uiMAStr1 );
    S.setValue( "niXAChans1", uiXAStr1 );
    S.setValue( "niXDChans1", uiXDStr1 );
    S.setValue( "niMNChans2", uiMNStr2Bare() );
    S.setValue( "niMAChans2", uiMAStr2Bare() );
    S.setValue( "niXAChans2", uiXAStr2Bare() );
    S.setValue( "niXDChans2", uiXDStr2Bare() );
    S.setValue( "niMuxFactor", muxFactor );
    S.setValue( "niAiTermConfig", int(termCfg) );
    S.setValue( "niEnabled", enabled );
    S.setValue( "niDualDevMode", isDualDevMode );
    S.setValue( "niStartEnable", startEnable );
    S.setValue( "niStartLine", startLine );
    S.setValue( "niSnsShankMapFile", sns.shankMapFile );
    S.setValue( "niSnsChanMapFile", sns.chanMapFile );
    S.setValue( "niSnsSaveChanSubset", sns.uiSaveChanStr );
}

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// ------
// Macros
// ------

#define SIMAIDEV    "Dev1"
#define SIMAODEV    "Dev3"

#define STRMATCH( s, target ) !(s).compare( target, Qt::CaseInsensitive )

#define DAQmxErrChk(functionCall)                           \
    do {                                                    \
    if( DAQmxFailed(dmxErrNum = (functionCall)) )           \
        {dmxFnName = STR(functionCall); goto Error_Out;}    \
    } while( 0 )

#define DAQmxErrChkNoJump(functionCall)                     \
    (DAQmxFailed(dmxErrNum = (functionCall)) &&             \
    (dmxFnName = STR(functionCall)))

// ----
// Data
// ----

#ifdef HAVE_NIDAQmx
static QVector<char>    dmxErrMsg;
static const char       *dmxFnName;
static int32            dmxErrNum;
#endif

static bool noDaqErrPrint = false;

QStringList             CniCfg::niDevNames;
CniCfg::DeviceRangeMap  CniCfg::aiDevRanges,
                        CniCfg::aoDevRanges;
CniCfg::DeviceChanCount CniCfg::aiDevChanCount,
                        CniCfg::aoDevChanCount,
                        CniCfg::diDevLineCount,
                        CniCfg::doDevLineCount;

// -------
// Methods
// -------

/* ---------------------------------------------------------------- */
/* clearDmxErrors ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
static void clearDmxErrors()
{
    dmxErrMsg.clear();
    dmxFnName   = "";
    dmxErrNum   = 0;
}
#endif

/* ---------------------------------------------------------------- */
/* lastDAQErrMsg -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Capture latest dmxErrNum as a descriptive C-string.
// Call as soon as possible after offending operation.
//
#ifdef HAVE_NIDAQmx
static void lastDAQErrMsg()
{
    const int msgbytes = 2048;
    dmxErrMsg.resize( msgbytes );
    dmxErrMsg[0] = 0;
    DAQmxGetExtendedErrorInfo( &dmxErrMsg[0], msgbytes );
}
#endif

/* ---------------------------------------------------------------- */
/* destroyTask ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
static void destroyTask( TaskHandle &taskHandle )
{
    if( taskHandle ) {
        DAQmxStopTask( taskHandle );
        DAQmxClearTask( taskHandle );
        taskHandle = 0;
    }
}
#endif

/* ---------------------------------------------------------------- */
/* getPhysChans --------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
typedef int32 (__CFUNC *QueryFunc_t)( const char [], char*, uInt32 );

static QStringList getPhysChans(
    const QString   &dev,
    QueryFunc_t     queryFunc,
    const QString   &fn = QString() )
{
    QString         funcName = fn;
    QVector<char>   buf( 65536 );

    if( !funcName.length() )
        funcName = "??";

    buf[0] = 0;

    clearDmxErrors();

    DAQmxErrChk( queryFunc( STR2CHR( dev ), &buf[0], buf.size() ) );

    // "\\s*,\\s*" encodes <optional wh spc><comma><optional wh spc>
    return QString( &buf[0] )
            .split( QRegExp("\\s*,\\s*"), QString::SkipEmptyParts );

Error_Out:
    if( DAQmxFailed( dmxErrNum ) ) {

        lastDAQErrMsg();

        if( !noDaqErrPrint ) {

            Error()
                << "DAQmx Error: Fun=<"
                << funcName
                << "> Bufsz=("
                << buf.size()
                << ") Err=<"
                << dmxErrNum
                << "> '" << &dmxErrMsg[0] << "'.";
        }
    }

    return QStringList();
}
#endif

/* ---------------------------------------------------------------- */
/* getAIChans ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/ai5"
//
#ifdef HAVE_NIDAQmx
static QStringList getAIChans( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevAIPhysicalChans,
            "DAQmxGetDevAIPhysicalChans" );
}
#else // !HAVE_NIDAQmx, emulated, 60 chans
static QStringList getAIChans( const QString &dev )
{
    QStringList L;

    if( dev == SIMAIDEV ) {

        for( int i = 0; i < 60; ++i ) {

            L.push_back(
                QString("%1/ai%2").arg( dev ).arg( i ) );
        }
    }

    return L;
}
#endif

/* ---------------------------------------------------------------- */
/* getAOChans ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/ao5"
//
#ifdef HAVE_NIDAQmx
static QStringList getAOChans( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevAOPhysicalChans,
            "DAQmxGetDevAOPhysicalChans" );
}
#else // !HAVE_NIDAQmx, emulated, 2 chans
static QStringList getAOChans( const QString &dev )
{
    QStringList L;

    if( dev == SIMAODEV ) {

        for( int i = 0; i < 2; ++i ) {

            L.push_back(
                QString("%1/ao%2").arg( dev ).arg( i ) );
        }
    }

    return L;
}
#endif

/* ---------------------------------------------------------------- */
/* getDILines ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/port0/line3"
//
#ifdef HAVE_NIDAQmx
static QStringList getDILines( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevDILines,
            "DAQmxGetDevDILines" );
}
#else
static QStringList getDILines( const QString &dev )
{
    if( dev == SIMAIDEV ) {
        return QStringList( QString("%1/port0/line0").arg( dev ) )
                << QString("%1/port0/line1").arg( dev );
    }
    else
        return QStringList();
}
#endif

/* ---------------------------------------------------------------- */
/* getDOLines ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/port0/line3"
//
#ifdef HAVE_NIDAQmx
static QStringList getDOLines( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevDOLines,
            "DAQmxGetDevDOLines" );
}
#else
static QStringList getDOLines( const QString &dev )
{
    return QStringList( QString("%1/port0/line0").arg( dev ) );
}
#endif

/* ---------------------------------------------------------------- */
/* probeAllAIRanges ----------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
static void probeAllAIRanges()
{
    CniCfg::aiDevRanges.clear();

    VRange  r;
    float64 dArr[512];

    foreach( const QString &D, CniCfg::niDevNames ) {

        memset( dArr, 0, sizeof(dArr) );

        if( !DAQmxFailed( DAQmxGetDevAIVoltageRngs(
                STR2CHR( D ),
                dArr,
                512 ) ) ) {

            for( int i = 0; i < 512; i +=2 ) {

                r.rmin = dArr[i];
                r.rmax = dArr[i+1];

                if( r.rmin == r.rmax )
                    break;

                CniCfg::aiDevRanges.insert( D, r );
            }
        }
    }
}
#else
static void probeAllAIRanges()
{
    CniCfg::aiDevRanges.clear();

    VRange  r;

    r.rmin = -2.5;
    r.rmax = 2.5;
    CniCfg::aiDevRanges.insert( SIMAIDEV, r );

    r.rmin = -5.0;
    r.rmax = 5.0;
    CniCfg::aiDevRanges.insert( SIMAIDEV, r );
}
#endif

/* ---------------------------------------------------------------- */
/* probeAllAORanges ----------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
static void probeAllAORanges()
{
    CniCfg::aoDevRanges.clear();

    VRange  r;
    float64 dArr[512];

    foreach( const QString &D, CniCfg::niDevNames ) {

        memset( dArr, 0, sizeof(dArr) );

        if( !DAQmxFailed( DAQmxGetDevAOVoltageRngs(
                STR2CHR( D ),
                dArr,
                512 ) ) ) {

            for( int i = 0; i < 512; i +=2 ) {

                r.rmin = dArr[i];
                r.rmax = dArr[i+1];

                if( r.rmin == r.rmax )
                    break;

                CniCfg::aoDevRanges.insert( D, r );
            }
        }
    }
}
#else
static void probeAllAORanges()
{
    CniCfg::aoDevRanges.clear();

    VRange  r;

    r.rmin = -2.5;
    r.rmax = 2.5;
    CniCfg::aoDevRanges.insert( SIMAODEV, r );

    r.rmin = -5.0;
    r.rmax = 5.0;
    CniCfg::aoDevRanges.insert( SIMAODEV, r );
}
#endif

/* ---------------------------------------------------------------- */
/* probeAllAIChannels --------------------------------------------- */
/* ---------------------------------------------------------------- */

static void probeAllAIChannels()
{
    CniCfg::aiDevChanCount.clear();

    bool    savedPrt = noDaqErrPrint;

    noDaqErrPrint = true;

    foreach( const QString &D, CniCfg::niDevNames ) {

        QStringList L = getAIChans( D );

        if( !L.empty() )
            CniCfg::aiDevChanCount[D] = L.count();
    }

    noDaqErrPrint = savedPrt;
}

/* ---------------------------------------------------------------- */
/* probeAllAOChannels --------------------------------------------- */
/* ---------------------------------------------------------------- */

static void probeAllAOChannels()
{
    CniCfg::aoDevChanCount.clear();

    bool    savedPrt = noDaqErrPrint;

    noDaqErrPrint = true;

    foreach( const QString &D, CniCfg::niDevNames ) {

        QStringList L = getAOChans( D );

        if( !L.empty() )
            CniCfg::aoDevChanCount[D] = L.count();
    }

    noDaqErrPrint = savedPrt;
}

/* ---------------------------------------------------------------- */
/* _sampleFreq1 --------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
static double _sampleFreq1( const QString &dev, const QString &pfi )
{
    TaskHandle      taskHandle  = 0;
    const QString   ctrStr      = QString("/%1/ctr0").arg( dev );
    float64         sampSecs    = 2.0,
                    freq        = 0;

    clearDmxErrors();

    DAQmxErrChk( DAQmxCreateTask( "", &taskHandle ) );
    DAQmxErrChk( DAQmxCreateCIFreqChan(
                    taskHandle,
                    STR2CHR( ctrStr ),
                    "",                     // N.A.
                    1e5,                    // min expected freq
                    2e6,                    // max expected freq
                    DAQmx_Val_Hz,           // units
                    DAQmx_Val_Rising,
                    DAQmx_Val_HighFreq2Ctr, // method
                    sampSecs,
                    4,                      // divisor (min=4)
                    "") );
    DAQmxErrChk( DAQmxSetCIFreqTerm(
                    taskHandle,
                    STR2CHR( ctrStr ),
                    STR2CHR( QString("/%1/%2").arg( dev ).arg( pfi ) ) ) );
    DAQmxErrChk( DAQmxStartTask( taskHandle ) );
    DAQmxErrChk( DAQmxReadCounterScalarF64(
                    taskHandle,
                    2*sampSecs,             // timeout secs
                    &freq,
                    NULL ) );

Error_Out:
    if( DAQmxFailed( dmxErrNum ) )
        lastDAQErrMsg();

    destroyTask( taskHandle );

    if( DAQmxFailed( dmxErrNum ) ) {

        QString e = QString("DAQmx Error:\nFun=<%1>\n").arg( dmxFnName );
        e += QString("ErrNum=<%1>\n").arg( dmxErrNum );
        e += QString("ErrMsg='%1'.").arg( &dmxErrMsg[0] );

        Error() << e;
    }

    return freq;
}
#else
static double _sampleFreq1( const QString &dev, const QString &pfi )
{
    Q_UNUSED( dev )
    Q_UNUSED( pfi )

    return 10000.0;
}
#endif

/* ---------------------------------------------------------------- */
/* stringToTermConfig --------------------------------------------- */
/* ---------------------------------------------------------------- */

CniCfg::TermConfig CniCfg::stringToTermConfig( const QString &txt )
{
    if( STRMATCH( txt, "RSE" ) )
        return RSE;
    else if( STRMATCH( txt, "NRSE" ) )
        return NRSE;
    else if( STRMATCH( txt, "Differential" )
             || STRMATCH( txt, "Diff" ) ) {

        return Diff;
    }
    else if( STRMATCH( txt, "PseudoDifferential" )
             || STRMATCH( txt, "PseudoDiff" ) ) {

        return PseudoDiff;
    }

    return Default;
}

/* ---------------------------------------------------------------- */
/* termConfigToString --------------------------------------------- */
/* ---------------------------------------------------------------- */

QString CniCfg::termConfigToString( TermConfig t )
{
    switch( t ) {
        case RSE:           return "RSE";
        case NRSE:          return "NRSE";
        case Diff:          return "Differential";
        case PseudoDiff:    return "PseudoDifferential";
        default:            return "Default";
    }
}

/* ---------------------------------------------------------------- */
/* isPFI2DI ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if given full pfi name can be connected
// as clock line to waveform DI task.
//
#ifdef HAVE_NIDAQmx
static bool isPFI2DI( const QString &dev, const QString &pfi )
{
    QString     s = QString("/%1/line0").arg( dev );
    TaskHandle  task;
    int         ret;
    bool        ok = false;

    DAQmxCreateTask( "", &task );

    ret = DAQmxCreateDIChan( task,
            STR2CHR( s ), "", DAQmx_Val_ChanForAllLines );

    if( DAQmxFailed( ret ) )
        goto exit;

    ret = DAQmxCfgSampClkTiming( task,
            STR2CHR( pfi ),
            100, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000 );

    if( DAQmxFailed( ret ) )
        goto exit;

    ret = DAQmxTaskControl( task, DAQmx_Val_Task_Commit );

    if( DAQmxFailed( ret ) && ret != DAQmxErrorTrigLineNotFoundSingleDevRoute_Routing )
        goto exit;

    ok = true;

exit:
    destroyTask( task );

    return ok;
}
#endif

/* ---------------------------------------------------------------- */
/* getPFIChans ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/PFI0"
//
#ifdef HAVE_NIDAQmx
QStringList CniCfg::getPFIChans( const QString &dev )
{
    QStringList  L = getPhysChans( dev,
                        DAQmxGetDevTerminals,
                        "DAQmxGetDevTerminals" )
                        .filter( "/PFI" );

    int n = L.size();

    for( int i = n - 1; i >= 0; --i ) {

        if( !isPFI2DI( dev, L[i] ) )
            L.removeAt( i );
    }

    return L;
}
#else // !HAVE_NIDAQmx, emulated, 16 chans
QStringList CniCfg::getPFIChans( const QString &dev )
{
    QStringList L;

    if( dev == SIMAIDEV || dev == SIMAODEV ) {

        for( int i = 0; i < 16; ++i ) {

            L.push_back(
                QString("%1/PFI%2").arg( dev ).arg( i ) );
        }
    }

    return L;
}
#endif

/* ---------------------------------------------------------------- */
/* getAllDOLines -------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
QStringList CniCfg::getAllDOLines()
{
    QStringList L;

    foreach( const QString &D, CniCfg::niDevNames )
        L += getDOLines( D );

    return L;
}
#else // !HAVE_NIDAQmx, emulated, dev1 only
QStringList CniCfg::getAllDOLines()
{
    return getDOLines( SIMAIDEV );
}
#endif

/* ---------------------------------------------------------------- */
/* parseDIStr ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Get device and line index from DI string
// with pattern "Dev6/port0/line3".
//
// Return error string (or empty).
//
QString CniCfg::parseDIStr(
    QString         &dev,
    int             &line,
    const QString   &lineStr )
{
    QString err;

    QRegExp re("^([^/]+)/");    // device name up to slash
    re.setCaseSensitivity( Qt::CaseInsensitive );

    if( lineStr.contains( re ) ) {

        dev = re.cap(1);

        QStringList L = getDILines( dev );

        line = L.indexOf( lineStr );

        if( line < 0 )
            err = "Line not supported on given device.";
    }
    else
        err = "Device name not found in DI string.";

    return err;
}

/* ---------------------------------------------------------------- */
/* isHardware ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// DAQmxGetSysDevNames returns string "Dev1, Dev2, Dev3...".
//
#ifdef HAVE_NIDAQmx
bool CniCfg::isHardware()
{
    char    data[2048] = {0};

    niDevNames.clear();

    if( DAQmxFailed( DAQmxGetSysDevNames( data, sizeof(data) ) ) )
        return false;

    niDevNames = QString(data).split(
                                QRegExp("\\s*,\\s*"),
                                QString::SkipEmptyParts );

    return !niDevNames.isEmpty();
}
#else
bool CniCfg::isHardware()
{
    niDevNames.clear();
    niDevNames << SIMAIDEV;

    return true;
}
#endif

/* ---------------------------------------------------------------- */
/* probeAIHardware ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CniCfg::probeAIHardware()
{
    probeAllAIRanges();
    probeAllAIChannels();
}

/* ---------------------------------------------------------------- */
/* probeAOHardware ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CniCfg::probeAOHardware()
{
    probeAllAORanges();
    probeAllAOChannels();
}

/* ---------------------------------------------------------------- */
/* probeAllDILines ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CniCfg::probeAllDILines()
{
    diDevLineCount.clear();

    bool    savedPrt = noDaqErrPrint;

    noDaqErrPrint = true;

    foreach( const QString &D, CniCfg::niDevNames )
        diDevLineCount[D] = nWaveformLines( D );

    noDaqErrPrint = savedPrt;
}

/* ---------------------------------------------------------------- */
/* probeAllDOLines ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CniCfg::probeAllDOLines()
{
    doDevLineCount.clear();

    bool    savedPrt = noDaqErrPrint;

    noDaqErrPrint = true;

    foreach( const QString &D, CniCfg::niDevNames ) {

        QStringList L = getDOLines( D );

        if( !L.empty() )
            doDevLineCount[D] = L.count();
    }

    noDaqErrPrint = savedPrt;
}

/* ---------------------------------------------------------------- */
/* supportsAISimultaneousSampling --------------------------------- */
/* ---------------------------------------------------------------- */

// Return true iff (device) supports AI simultaneous sampling.
//
#ifdef HAVE_NIDAQmx
bool CniCfg::supportsAISimultaneousSampling( const QString &dev )
{
    bool32 impl = false;

    if( DAQmxFailed(
        DAQmxGetDevAISimultaneousSamplingSupported(
        STR2CHR( dev ), &impl ) ) ) {

        Error()
            << "NI: Failed query whether AI dev "
            << dev << " supports simultaneous sampling.";
    }

    return impl;
}
#else
bool CniCfg::supportsAISimultaneousSampling( const QString & )
{
    return true;
}
#endif

/* ---------------------------------------------------------------- */
/* maxTimebase ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
double CniCfg::maxTimebase( const QString &dev )
{
    float64 val = 2e7;

    if( isDigitalDev( dev ) ) {

        QString     s = QString("/%1/line0").arg( dev );
        TaskHandle  task;
        int         ret;

        DAQmxCreateTask( "", &task );

        ret = DAQmxCreateDIChan( task,
                STR2CHR( s ), "", DAQmx_Val_ChanForAllLines );

        if( DAQmxFailed( ret ) )
            goto done;

        ret = DAQmxCfgSampClkTiming( task,
                "", maxDigitalRate( dev ),
                DAQmx_Val_Rising, DAQmx_Val_ContSamps, 100 );

        if( DAQmxFailed( ret ) )
            goto done;

        ret = DAQmxTaskControl( task, DAQmx_Val_Task_Commit );

        if( DAQmxFailed( ret ) )
            goto done;

        ret = DAQmxGetSampClkTimebaseRate( task, &val );

done:
        destroyTask( task );
    }
    else if( DAQmxFailed(
            DAQmxGetDevCOMaxTimebase( STR2CHR( dev ), &val ) ) ) {

        Error()
            << "NI: Failed query of dev "
            << dev << " COMaxTimebase.";
    }

    return val;
}
#else
double CniCfg::maxTimebase( const QString & )
{
    return 2e7;
}
#endif

/* ---------------------------------------------------------------- */
/* maxDigitalRate ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
double CniCfg::maxDigitalRate( const QString &dev )
{
    float64 val = 2e8;

    if( DAQmxFailed(
        DAQmxGetDevDIMaxRate( STR2CHR( dev ), &val ) ) ) {

        Error()
            << "NI: Failed query of dev "
            << dev << " DIMaxRate.";
    }

    return val;
}
#else
double CniCfg::maxDigitalRate( const QString & )
{
    return 2e8;
}
#endif

/* ---------------------------------------------------------------- */
/* maxSampleRate -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// If device is digital nChans is ignored.
// If nChans > 1 the max rate is divided by nChans.
// If nChans < 0 the max rate is returned unscaled.
//
#ifdef HAVE_NIDAQmx
double CniCfg::maxSampleRate( const QString &dev, int nChans )
{
    if( isDigitalDev( dev ) )
        return maxDigitalRate( dev );

    double  rate = SGLX_NI_MAXRATE;
    float64 val;
    int32   e;

    if( nChans == 0 )
        nChans = 1;

    if( nChans == 1 || nChans == -1 )
        e = DAQmxGetDevAIMaxSingleChanRate( STR2CHR( dev ), &val );
    else
        e = DAQmxGetDevAIMaxMultiChanRate( STR2CHR( dev ), &val );

    if( DAQmxFailed( e ) ) {
        Error()
            << "NI: Failed query of max sample rate for dev "
            << dev << ".";
    }
    else {

        rate = val;

        if( nChans > 1
            && !supportsAISimultaneousSampling( dev ) ) {

            rate = rate / nChans;
        }
    }

    return rate;
}
#else
double CniCfg::maxSampleRate( const QString &, int )
{
    return SGLX_NI_MAXRATE;
}
#endif

/* ---------------------------------------------------------------- */
/* minSampleRate -------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
double CniCfg::minSampleRate( const QString &dev )
{
    double  rate = 10.0;
    float64 val;

    if( DAQmxFailed(
        DAQmxGetDevAIMinRate( STR2CHR( dev ), &val ) ) ) {

        Error()
            << "NI: Failed query of min sample rate for dev "
            << dev << ".";
    }
    else
        rate = val;

    return rate;
}
#else
double CniCfg::minSampleRate( const QString & )
{
    return 10.0;
}
#endif

/* ---------------------------------------------------------------- */
/* nWaveformLines ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return number of lines supporting clocked (buffered) waveform
// input. Result will be one of {32, 24, 16, 8, 0}.
//
#ifdef HAVE_NIDAQmx
int CniCfg::nWaveformLines( const QString &dev )
{
    for( int i = 32; i >= 8; i -= 8  ) {

        QString     s = QString("/%1/line%2").arg( dev ).arg( i - 1 );
        TaskHandle  task;
        int         ret;
        bool        ok = false;

        DAQmxCreateTask( "", &task );

        ret = DAQmxCreateDIChan( task,
                STR2CHR( s ), "", DAQmx_Val_ChanForAllLines );

        if( DAQmxFailed( ret ) )
            goto next_line;

        ret = DAQmxCfgSampClkTiming( task,
                "", 100, DAQmx_Val_Rising, DAQmx_Val_ContSamps, 1000 );

        if( DAQmxFailed( ret ) )
            goto next_line;

        ret = DAQmxTaskControl( task, DAQmx_Val_Task_Commit );

        if( DAQmxFailed( ret ) && ret != DAQmxErrorExtSampClkSrcNotSpecified )
            goto next_line;

        ok = true;

next_line:
        destroyTask( task );

        if( ok )
            return i;
    }

    return 0;
}
#else
int CniCfg::nWaveformLines( const QString & )
{
    return 8;
}
#endif

/* ---------------------------------------------------------------- */
/* wrongTermConfig ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Note: Generally, the partner of differential input X is X+8.
// If a device has 32 SE and 16 diff inputs, the diffs will be
// 0:7, 16:23.
//
#ifdef HAVE_NIDAQmx
bool CniCfg::wrongTermConfig(
    QString             &err,
    const QString       &dev,
    const QVector<uint> &in,
    TermConfig          t )
{
    QString term;   // name for the error message
    int32   flag;   // used for bit test

    switch( t ) {
        case RSE:
            term = "RSE";
            flag = DAQmx_Val_Bit_TermCfg_RSE;
            break;
        case NRSE:
            term = "NRSE";
            flag = DAQmx_Val_Bit_TermCfg_NRSE;
            break;
        case Diff:
            term = "Differential";
            flag = DAQmx_Val_Bit_TermCfg_Diff;
            break;
        case PseudoDiff:
            term = "PseudoDifferential";
            flag = DAQmx_Val_Bit_TermCfg_PseudoDIFF;
            break;
        default:
            return false;
    }

    QVector<uint>   fail;   // vector of bad term types

    foreach( uint i, in ) {

        QString chan = QString("%1/ai%2").arg( dev ).arg( i );
        int32   data = 0;

        if( DAQmxFailed(
            DAQmxGetPhysicalChanAITermCfgs( STR2CHR(chan), &data ) ) ) {

            Error()
                << "NI: Failed query of supported termination for "
                << chan << ".";
        }

        if( !(data & flag) )
            fail.push_back( i );
    }

    if( fail.count() ) {

        err = QString("AI chans [%1] do not support %2 termination.")
                .arg( Subset::vec2RngStr( fail ) )
                .arg( term );
        return true;
    }

    return false;
}
#else
bool CniCfg::wrongTermConfig(
    QString             &,
    const QString       &,
    const QVector<uint> &,
    TermConfig )
{
    return false;
}
#endif

/* ---------------------------------------------------------------- */
/* getProductName ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
QString CniCfg::getProductName( const QString &dev )
{
    QVector<char>   buf( 65536 );
    strcpy( &buf[0], "Unknown" );

    if( DAQmxFailed(
        DAQmxGetDevProductType(
        STR2CHR( dev ), &buf[0], buf.size() ) ) ) {

        Error()
            << "NI: Failed query of product name for dev "
            << dev << ".";
    }

    return &buf[0];
}
#else
QString CniCfg::getProductName( const QString & )
{
    return "FakeDAQ";
}
#endif

/* ---------------------------------------------------------------- */
/* isDigitalDev --------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_NIDAQmx
bool CniCfg::isDigitalDev( const QString &dev )
{
    int32   data;
    bool    isDig = false;

    if( DAQmxFailed(
        DAQmxGetDevProductCategory( STR2CHR( dev ), &data ) ) ) {

        Error()
            << "NI: Failed query of product category for dev "
            << dev << ".";
    }
    else
        isDig = (data == DAQmx_Val_DigitalIO);

    return isDig;
}
#else
bool CniCfg::isDigitalDev( const QString & )
{
    return false;
}
#endif

/* ---------------------------------------------------------------- */
/* setDO ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return err string or null if success.
//
// Param (lines) can be a comma separated list of legal lines.
//
#ifdef HAVE_NIDAQmx
QString CniCfg::setDO( const QString &lines, bool onoff )
{
    TaskHandle  taskHandle  = 0;
    uInt32      w_data      = (onoff ? -1 : 0);

    clearDmxErrors();

    DAQmxErrChk( DAQmxCreateTask( "", &taskHandle ) );
    DAQmxErrChk( DAQmxCreateDOChan(
                    taskHandle,
                    STR2CHR( lines ),
                    "",
                    DAQmx_Val_ChanForAllLines ) );
    DAQmxErrChk( DAQmxWriteDigitalScalarU32(
                    taskHandle,
                    true,           // autostart
                    2.5,            // timeout secs
                    w_data,
                    NULL ) );

Error_Out:
    if( DAQmxFailed( dmxErrNum ) )
        lastDAQErrMsg();

    destroyTask( taskHandle );

    if( DAQmxFailed( dmxErrNum ) ) {

        QString e = QString("DAQmx Error:\nFun=<%1>\n").arg( dmxFnName );
        e += QString("ErrNum=<%1>\n").arg( dmxErrNum );
        e += QString("ErrMsg='%1'.").arg( &dmxErrMsg[0] );

        Error() << e;

        return e;
    }

    return QString();
}
#else
QString CniCfg::setDO( const QString &lines, bool onoff )
{
    Q_UNUSED( lines )
    Q_UNUSED( onoff )

    return QString();
}
#endif

/* ---------------------------------------------------------------- */
/* sampleFreqMode ------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CniCfg::sampleFreqMode(
    const QString   &dev,
    const QString   &pfi,
    const QString   &line )
{
    const int   nSamp = 10;

    double  freq = 0;

    if( !setDO( line, true ).isEmpty() )
        return 0.0;

// Wait turn on

    QThread::msleep( 100 );

// Collect samples
// Values come in just a few flavors; one value predominating.
// The most probable value is the best estimator.

    QMap<double,int>    mf; // value -> occurrences

    for( int i = 0; i < nSamp; ++i ) {

        double  f = _sampleFreq1( dev, pfi );

        if( !f ) {
            mf.clear();
            break;
        }

        mf[f] = mf.value( f, 0 ) + 1;
    }

// Turn off

    setDO( line, false );

// Return most probable

    if( mf.size() ) {

        QMap<double,int>::iterator  it  = mf.begin();
        int                         occ = it.value();

        freq = it.key();

        while( ++it != mf.end() ) {

            if( it.value() > occ ) {

                freq    = it.key();
                occ     = it.value();
            }
        }
    }

    return freq;
}

/* ---------------------------------------------------------------- */
/* sampleFreqAve -------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CniCfg::sampleFreqAve(
    const QString   &dev,
    const QString   &pfi,
    const QString   &line )
{
    const int   nSamp = 10;

    double  freq = 0;

    if( !setDO( line, true ).isEmpty() )
        return 0.0;

// Wait turn on

    QThread::msleep( 100 );

// Collect and average samples

    for( int i = 0; i < nSamp; ++i ) {

        double  f = _sampleFreq1( dev, pfi );

        if( !f ) {
            freq = 0;
            break;
        }

        freq += f;
    }

// Turn off

    setDO( line, false );

// Return average

    return freq / nSamp;
}


