
#include "Util.h"
#include "CimCfg.h"

#ifdef HAVE_IMEC
#include "IMEC/Neuropix_basestation_api.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

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
// - probe option parameter
//
// Derive:
// - imCumTypCnt[]
//
void CimCfg::deriveChanCounts( int opt )
{
// --------------------------------
// First count each type separately
// --------------------------------

    imCumTypCnt[imTypeAP] = (opt == 4 ? 276 : 384);
    imCumTypCnt[imTypeLF] = imCumTypCnt[imTypeAP];
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
    S.value( "imEnabled", false ).toBool();

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

// ------
// Macros
// ------

#define STRMATCH( s, target ) !(s).compare( target, Qt::CaseInsensitive )

// ----
// Data
// ----

CimCfg::DeviceChanCount CimCfg::aiDevChanCount,
                        CimCfg::aoDevChanCount,
                        CimCfg::diDevLineCount;

// -------
// Methods
// -------

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
/* HAVE_IMEC ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC

static bool _open( QStringList &sl, Neuropix_basestation_api &IM )
{
    int err = IM.neuropix_open();

    if( err != OPEN_SUCCESS ) {
        sl.append( QString("IMEC open error %1.").arg( err ) );
        return false;
    }

    return true;
}


static bool _hwrVers(
    QStringList                 &sl,
    CimCfg::IMVers              &imVers,
    Neuropix_basestation_api    &IM )
{
    VersionNumber   vn;
    int             err = IM.neuropix_getHardwareVersion( &vn );

    if( err != SUCCESS ) {
        sl.append( QString("IMEC getHardwareVersion error %1.").arg( err ) );
        return false;
    }

    imVers.hwr = QString("%1.%2").arg( vn.major ).arg( vn.minor );
    sl.append( QString("Hardware version %1").arg( imVers.hwr ) );
    return true;
}


static bool _bsVers(
    QStringList                 &sl,
    CimCfg::IMVers              &imVers,
    Neuropix_basestation_api    &IM )
{
    uchar   bsVersMaj, bsVersMin;
    int     err = IM.neuropix_getBSVersion( bsVersMaj );

    if( err != CONFIG_SUCCESS ) {
        sl.append( QString("IMEC getBSVersion error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_getBSRevision( bsVersMin );

    if( err != CONFIG_SUCCESS ) {
        sl.append( QString("IMEC getBSRevision error %1.").arg( err ) );
        return false;
    }

    imVers.bs = QString("%1.%2").arg( bsVersMaj ).arg( bsVersMin );
    sl.append( QString("Basestation version %1").arg( imVers.bs ) );
    return true;
}


static bool _apiVers(
    QStringList                 &sl,
    CimCfg::IMVers              &imVers,
    Neuropix_basestation_api    &IM )
{
    VersionNumber   vn = IM.neuropix_getAPIVersion();

    imVers.api = QString("%1.%2").arg( vn.major ).arg( vn.minor );
    sl.append( QString("API version %1").arg( imVers.api ) );
    return true;
}


static bool _probeID(
    QStringList                 &sl,
    CimCfg::IMVers              &imVers,
    Neuropix_basestation_api    &IM )
{
    AsicID  asicID;
    int     err = IM.neuropix_readId( asicID );

    if( err != EEPROM_SUCCESS ) {
        sl.append( QString("IMEC readId error %1.").arg( err ) );
        return false;
    }

    imVers.pSN = QString("%1").arg( asicID.serialNumber, 0, 16 );
    imVers.opt = asicID.probeType;
    sl.append( QString("Probe serial# %1").arg( imVers.pSN ) );
    sl.append( QString("Probe option %1").arg( imVers.opt ) );
    return true;
}

#endif

/* ---------------------------------------------------------------- */
/* getVersions ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
bool CimCfg::getVersions( QStringList &sl, IMVers &imVers )
{
    bool    ok = false;

    sl.clear();
    imVers.opt = 0;

    Neuropix_basestation_api    IM;

    if( !_open( sl, IM ) )
        goto exit;

    if( !_hwrVers( sl, imVers, IM ) )
        goto exit;

    if( !_bsVers( sl, imVers, IM ) )
        goto exit;

    if( !_apiVers( sl, imVers, IM ) )
        goto exit;

//    if( !_probeID( sl, imVers, IM ) )
//        goto exit;

    sl.append( "Probe serial# 0 (simulated)" );
    sl.append( "Probe option 3 (simulated)" );
    imVers.pSN  = "0";
    imVers.opt  = 3;

    ok = true;

exit:
    IM.neuropix_close();
    return ok;
}
#else
bool CimCfg::getVersions( QStringList &sl, IMVers &imVers )
{
    sl.clear();
    sl.append( "Hardware version 0.0 (simulated)" );
    sl.append( "Basestation version 0.0 (simulated)" );
    sl.append( "API version 0.0 (simulated)" );
    sl.append( "Probe serial# 0 (simulated)" );
    sl.append( "Probe option 3 (simulated)" );
    imVers.hwr  = "0.0";
    imVers.bs   = "0.0";
    imVers.api  = "0.0";
    imVers.pSN  = "0";
    imVers.opt  = 3;
    return true;
}
#endif


