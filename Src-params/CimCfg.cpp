
#include "CimCfg.h"

#include <QSettings>

/* ---------------------------------------------------------------- */
/* Param methods -------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CimCfg::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        if( ic < imCumTypCnt[imTypeAP] )
            g = apGain;
        else if( ic < imCumTypCnt[imTypeLF] )
            g = lfGain;

        if( g < 0.01 )
            g = 0.01;
    }

    return g;
}


// Given input fields:
// -
// -
// -
// -
//
// Derive:
// - imCumTypCnt[]
//
void CimCfg::deriveChanCounts()
{
// --------------------------------
// First count each type separately
// --------------------------------

// BK: Need to flesh this out by probe option as determined
// on first config page.

    imCumTypCnt[imTypeAP] = 384;
    imCumTypCnt[imTypeLF] = 384;
    imCumTypCnt[imTypeSY] = 1;

// ---------
// Integrate
// ---------

    for( int i = 1; i < imNTypes; ++i )
        imCumTypCnt[i] += imCumTypCnt[i - 1];
}


void CimCfg::loadSettings( QSettings &S )
{
    range.rmin =
    S.value( "imAiRangeMin", -0.6 ).toDouble();

    range.rmax =
    S.value( "imAiRangeMax", 0.6 ).toDouble();

    srate =
    S.value( "imSampRate", 30000.0 ).toDouble();

    apGain =
    S.value( "imAPGain", 1.0 ).toDouble();

    lfGain =
    S.value( "imLFGain", 1.0 ).toDouble();

    dev1 =
    S.value( "imDev1", "" ).toString();

    dev2 =
    S.value( "imDev2", "" ).toString();

    uiMNStr1 =
    S.value( "imMNChans1", "0:5" ).toString();

    uiMAStr1 =
    S.value( "imMAChans1", "6,7" ).toString();

    uiXAStr1 =
    S.value( "imXAChans1", "" ).toString();

    uiXDStr1 =
    S.value( "imXDChans1", "" ).toString();

    enabled =
    S.value( "imEnabled", true ).toBool();

    softStart =
    S.value( "imSoftStart", true ).toBool();
}


void CimCfg::saveSettings( QSettings &S ) const
{
// BK: Don't create metadata until realistic

//    S.setValue( "imAiRangeMin", range.rmin );
//    S.setValue( "imAiRangeMax", range.rmax );
//    S.setValue( "imSampRate", srate );
//    S.setValue( "imAPGain", mnGain );
//    S.setValue( "imLFGain", maGain );
//    S.setValue( "imDev1", dev1 );
//    S.setValue( "imDev2", dev2 );
//    S.setValue( "imMNChans1", uiMNStr1 );
//    S.setValue( "imMAChans1", uiMAStr1 );
//    S.setValue( "imXAChans1", uiXAStr1 );
//    S.setValue( "imXDChans1", uiXDStr1 );
//    S.setValue( "imEnabled", enabled );
//    S.setValue( "imSoftStart", softStart );
}

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#include "Util.h"

#ifdef HAVE_IMEC
#include "IMEC/Neuropix_basestation_api.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QRegExp>

// ------
// Macros
// ------

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

#ifdef HAVE_IMEC
static QVector<char>    dmxErrMsg;
static const char       *dmxFnName;
static int32            dmxErrNum;
#endif

static bool noDaqErrPrint = false;

CimCfg::DeviceChanCount CimCfg::aiDevChanCount,
                        CimCfg::aoDevChanCount,
                        CimCfg::diDevLineCount;

// -------
// Methods
// -------

/* ---------------------------------------------------------------- */
/* clearDmxErrors ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
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
#ifdef HAVE_IMEC
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

#ifdef HAVE_IMEC
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

#if HAVE_IMEC
typedef int32 (__CFUNC *QueryFunc_t)( const char [], char*, uInt32 );

static QStringList getPhysChans(
    const QString   &dev,
    QueryFunc_t     queryFunc,
    const QString   &fn = QString::null )
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
                << "> [" << &dmxErrMsg[0] << "].";
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
#ifdef HAVE_IMEC
static QStringList getAIChans( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevAIPhysicalChans,
            "DAQmxGetDevAIPhysicalChans" );
}
#else // !HAVE_IMEC, emulated, 60 chans
static QStringList getAIChans( const QString &dev )
{
    QStringList L;

    if( dev == "Dev1" ) {

        for( int i = 0; i < 60; ++i ) {

            L.push_back(
                QString( "%1/ai%2" ).arg( dev ).arg( i ) );
        }
    }

    return L;
}
#endif

/* ---------------------------------------------------------------- */
/* probeAllAIChannels --------------------------------------------- */
/* ---------------------------------------------------------------- */

static void probeAllAIChannels()
{
    CimCfg::aiDevChanCount.clear();

    bool    savedPrt = noDaqErrPrint;

    noDaqErrPrint = true;

    for( int idev = 0; idev <= 16; ++idev ) {

        QString     dev = QString( "Dev%1" ).arg( idev );
        QStringList L   = getAIChans( dev );

        if( !L.empty() )
            CimCfg::aiDevChanCount[dev] = L.count();
    }

    noDaqErrPrint = savedPrt;
}

/* ---------------------------------------------------------------- */
/* stringToTermConfig --------------------------------------------- */
/* ---------------------------------------------------------------- */

CimCfg::TermConfig CimCfg::stringToTermConfig( const QString &txt )
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

QString CimCfg::termConfigToString( TermConfig t )
{
    switch( t ) {

        case RSE:
            return "RSE";
        case NRSE:
            return "NRSE";
        case Diff:
            return "Differential";
        case PseudoDiff:
            return "PseudoDifferential";
        default:
            break;
    }

    return "Default";
}

/* ---------------------------------------------------------------- */
/* getPFIChans ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Gets entries of type "Dev6/PFI0"
//
#ifdef HAVE_IMEC
QStringList CimCfg::getPFIChans( const QString &dev )
{
    return getPhysChans( dev,
            DAQmxGetDevTerminals,
            "DAQmxGetDevTerminals" )
            .filter( "/PFI" );
}
#else // !HAVE_IMEC, emulated, 16 chans
QStringList CimCfg::getPFIChans( const QString &dev )
{
    QStringList L;

    if( dev == "Dev1" ) {

        for( int i = 0; i < 16; ++i ) {

            L.push_back(
                QString( "%1/PFI%2" ).arg( dev ).arg( i ) );
        }
    }

    return L;
}
#endif

/* ---------------------------------------------------------------- */
/* isHardware ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
bool CimCfg::isHardware()
{
    char    data[2048] = {0};

    if( DAQmxFailed( DAQmxGetSysDevNames( data, sizeof(data) ) ) )
        return false;

    return data[0] != 0;
}
#else
bool CimCfg::isHardware()
{
    return true;
}
#endif

/* ---------------------------------------------------------------- */
/* probeAIHardware ------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CimCfg::probeAIHardware()
{
    probeAllAIChannels();
}

/* ---------------------------------------------------------------- */
/* getProductName ------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
QString CimCfg::getProductName( const QString &dev )
{
    QVector<char>   buf( 65536 );
    strcpy( &buf[0], "Unknown" );

    if( DAQmxFailed(
        DAQmxGetDevProductType(
        STR2CHR( dev ), &buf[0], buf.size() ) ) ) {

        Error()
            << "Failed during query of product name for dev "
            << dev << ".";
    }

    return &buf[0];
}
#else
QString CimCfg::getProductName( const QString & )
{
    return "FakeImec";
}
#endif



