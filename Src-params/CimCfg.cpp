
#include "Util.h"
#include "CimCfg.h"

#ifdef HAVE_IMEC
#include "IMEC/Neuropix_basestation_api.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QSettings>


/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Pattern: "chn bank refel apgn lfgn"
//
QString IMRODesc::toString( int chn ) const
{
    return QString("%1 %2 %3 %4 %5")
            .arg( chn )
            .arg( bank ).arg( refel )
            .arg( apgn ).arg( lfgn );
}


// Pattern: "chn bank refel apgn lfgn"
//
// Note: The chn field is discarded.
//
IMRODesc IMRODesc::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc(
            sl.at( 1 ).toShort(), sl.at( 2 ).toShort(),
            sl.at( 3 ).toShort(), sl.at( 4 ).toShort() );
}

/* ---------------------------------------------------------------- */
/* class IMROTbl -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl::fillDefault( quint32 pSN, int opt )
{
    this->pSN = pSN;
    this->opt = opt;

    e.clear();
    e.resize( opt == 4 ? imOpt4Chan : imOpt3Chan );
}


// Pattern: (pSN,opt,nchan)(chn bank refel apgn lfgn)()()...
//
QString IMROTbl::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << pSN << "," << opt << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString( i ) << ")";

    return s;
}


// Pattern: (pSN,opt,nchan)(chn bank refel apgn lfgn)()()...
//
void IMROTbl::fromString( const QString &s )
{
    QStringList	sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int			n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s*|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    pSN = hl[0].toUInt();
    opt = hl[1].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc::fromString( sl[i] ) );
}


//static int r2c384[IMROTbl::imOpt3Refs]   = {-1,36,75,112,151,188,227,264,303,340,379};
//static int r2c276[IMROTbl::imOpt4Refs]   = {-1,36,75,112,151,188,227,264};
static int i2gn[IMROTbl::imNGains]       = {50,125,250,500,1000,1500,2000,2500};


int IMROTbl::elToCh384( int el )
{
    return (el > 0 ? (el - 1) % 384 : -1);
}


int IMROTbl::elToCh276( int el )
{
    return (el > 0 ? (el - 1) % 276 : -1);
}


int IMROTbl::chToEl384( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*384 : 0);
}


int IMROTbl::chToEl276( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*276 : 0);
}


int IMROTbl::chToRefid384( int ch )
{
    switch( ch ) {
        case 36:
            return 1;
        case 75:
            return 2;
        case 112:
            return 3;
        case 151:
            return 4;
        case 188:
            return 5;
        case 227:
            return 6;
        case 264:
            return 7;
        case 303:
            return 8;
        case 340:
            return 9;
        case 379:
            return 10;
        default:
            break;
    }

    return 0;
}


int IMROTbl::chToRefid276( int ch )
{
    switch( ch ) {
        case 36:
            return 1;
        case 75:
            return 2;
        case 112:
            return 3;
        case 151:
            return 4;
        case 188:
            return 5;
        case 227:
            return 6;
        case 264:
            return 7;
        default:
            break;
    }

    return 0;
}


int IMROTbl::elToRefid384( int el )
{
    if( el > 0 )
        return chToRefid384( elToCh384( el ) );

    return 0;
}


int IMROTbl::elToRefid276( int el )
{
    if( el > 0 )
        return chToRefid276( elToCh276( el ) );

    return 0;
}


int IMROTbl::idxToGain( int idx )
{
    return (idx >= 0 && idx < 8 ? i2gn[idx] : i2gn[0]);
}


int IMROTbl::gainToIdx( int gain )
{
    switch( gain ) {
        case 50:
            return 0;
        case 125:
            return 1;
        case 250:
            return 2;
        case 500:
            return 3;
        case 1000:
            return 4;
        case 1500:
            return 5;
        case 2000:
            return 6;
        case 2500:
            return 7;
        default:
            break;
    }

    return 0;
}

/* ---------------------------------------------------------------- */
/* class CimCfg --------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CimCfg::chanGain( int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = imCumTypCnt[imTypeAP];

        if( ic < nAP )
            g = roTbl.e[ic].apgn;
        else if( ic < imCumTypCnt[imTypeLF] )
            g = roTbl.e[ic-nAP].lfgn;

        if( g < 50.0 )
            g = 50.0;
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
    range.rmin = -0.6;
//    S.value( "imAiRangeMin", -0.6 ).toDouble();

    range.rmax = 0.6;
//    S.value( "imAiRangeMax", 0.6 ).toDouble();

    srate = 30000.0;
//    S.value( "imSampRate", 30000.0 ).toDouble();

    hpFltIdx =
    S.value( "imHpFltIdx", 0 ).toInt();

    enabled =
    S.value( "imEnabled", true ).toBool();

    softStart =
    S.value( "imSoftStart", true ).toBool();
}


void CimCfg::saveSettings( QSettings &S ) const
{
    S.setValue( "imAiRangeMin", range.rmin );
    S.setValue( "imAiRangeMax", range.rmax );
    S.setValue( "imSampRate", srate );
    S.setValue( "imHpFltIdx", hpFltIdx );
    S.setValue( "imEnabled", enabled );
    S.setValue( "imSoftStart", softStart );
}


int CimCfg::idxToFlt( int idx )
{
    if( idx == 3 )
        return 1000;
    else if( idx == 1 )
        return 500;

    return 300;
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
    uchar   vMaj, vMin;
    int     err = IM.neuropix_getBSVersion( vMaj );

    if( err != CONFIG_SUCCESS ) {
        sl.append( QString("IMEC getBSVersion error %1.").arg( err ) );
        return false;
    }

    err = IM.neuropix_getBSRevision( vMin );

    if( err != CONFIG_SUCCESS ) {
        sl.append( QString("IMEC getBSRevision error %1.").arg( err ) );
        return false;
    }

    imVers.bas = QString("%1.%2").arg( vMaj ).arg( vMin );
    sl.append( QString("Basestation version %1").arg( imVers.bas ) );
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

    imVers.pSN = QString("%1").arg( asicID.serialNumber );
    imVers.opt = asicID.probeType + 1;
    sl.append( QString("Probe serial# %1").arg( imVers.pSN ) );
    sl.append( QString("Probe option  %1").arg( imVers.opt ) );
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

    imVers.clear();
    sl.clear();

    Neuropix_basestation_api    IM;

    if( !_open( sl, IM ) ) {

        sl.append( "Not reading data." );
        sl.append( "Check connections and power." );
        sl.append( "Your IP address must be 10.2.0.123." );
        sl.append( "Gateway 255.0.0.0." );
        goto exit;
    }

    sl.append( "Connected IMEC devices:" );
    sl.append( "-----------------------------------" );

    if( !_hwrVers( sl, imVers, IM ) )
        goto exit;

    if( !_bsVers( sl, imVers, IM ) )
        goto exit;

    if( !_apiVers( sl, imVers, IM ) )
        goto exit;

    if( imVers.api.compare( "3.0" ) >= 0 ) {

        if( !_probeID( sl, imVers, IM ) )
            goto exit;
    }
    else {
        imVers.pSN = "0";
        imVers.opt = 0;
        sl.append( QString("Probe serial# 0 (unknown)") );
        sl.append( QString("Probe option  0 (unknown)") );
    }

    ok = true;

exit:
    sl.append( "-- end --" );
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
    sl.append( "Probe option  3 (simulated)" );
    imVers.hwr  = "0.0";
    imVers.bas  = "0.0";
    imVers.api  = "0.0";
    imVers.pSN  = "0";
    imVers.opt  = 3;
    return true;
}
#endif


