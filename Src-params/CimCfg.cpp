
#include "Util.h"
#include "CimCfg.h"
#include "Subset.h"

#ifdef HAVE_IMEC
#include "IMEC/Neuropix_basestation_api.h"
#else
#pragma message("*** Message to self: Building simulated IMEC version ***")
#endif

#include <QBitArray>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Pattern: "chn bank refid apgn lfgn"
//
QString IMRODesc::toString( int chn ) const
{
    return QString("%1 %2 %3 %4 %5")
            .arg( chn )
            .arg( bank ).arg( refid )
            .arg( apgn ).arg( lfgn );
}


// Pattern: "chn bank refid apgn lfgn"
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
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl::fillDefault( quint32 pSN, int opt )
{
    this->pSN = pSN;
    this->opt = opt;

    e.clear();
    e.resize( opt == 4 ? imOpt4Chan : imOpt3Chan );
}


// Return true if two tables are same w.r.t banks.
//
bool IMROTbl::banksSame( const IMROTbl &rhs ) const
{
    int n = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].bank != rhs.e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (pSN,opt,nchan)(chn bank refid apgn lfgn)()()...
//
QString IMROTbl::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << pSN << "," << opt << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString( i ) << ")";

    return s;
}


// Pattern: (pSN,opt,nchan)(chn bank refid apgn lfgn)()()...
//
void IMROTbl::fromString( const QString &s )
{
    QStringList sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    pSN = hl[0].toUInt();
    opt = hl[1].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc::fromString( sl[i] ) );
}


bool IMROTbl::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find [%1]").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromString( f.readAll() );

        if( (opt <= 3 && nChan() == imOpt3Chan)
            || (opt == 4 && nChan() == imOpt4Chan) ) {

            msg = QString("Loaded (SN,opt)=(%1,%2) file [%3]")
                    .arg( pSN )
                    .arg( opt )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error reading [%1]").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening [%1]").arg( fi.fileName() );
        return false;
    }
}


bool IMROTbl::saveFile( QString &msg, const QString &path, quint32 pSN )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        this->pSN = pSN;

        int n = f.write( STR2CHR( toString() ) );

        if( n > 0 ) {

            msg = QString("Saved (SN,opt)=(%1,%2) file [%3]")
                    .arg( pSN )
                    .arg( opt )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error writing [%1]").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening [%1]").arg( fi.fileName() );
        return false;
    }
}

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

static int _r2c384[IMROTbl::imOpt3Refs] = {-1,36,75,112,151,188,227,264,303,340,379};
static int _r2c276[IMROTbl::imOpt4Refs] = {-1,36,75,112,151,188,227,264};
static int i2gn[IMROTbl::imNGains]      = {50,125,250,500,1000,1500,2000,2500};


const int* IMROTbl::optTo_r2c( int opt )
{
    if( opt <= 3 )
        return _r2c384;

    return _r2c276;
}


int IMROTbl::optToNElec( int opt )
{
    switch( opt ) {

        case 1:
        case 2:
            return imOpt1Elec;
        case 3:
            return imOpt3Elec;
    }

    return imOpt4Elec;
}


int IMROTbl::optToNRef( int opt )
{
    if( opt <= 3 )
        return imOpt3Refs;

    return imOpt4Refs;
}


int IMROTbl::elToCh384( int el )
{
    return (el > 0 ? (el - 1) % imOpt3Chan : -1);
}


int IMROTbl::elToCh276( int el )
{
    return (el > 0 ? (el - 1) % imOpt4Chan : -1);
}


int IMROTbl::chToEl384( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*imOpt3Chan : 0);
}


int IMROTbl::chToEl276( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*imOpt4Chan : 0);
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
/* struct AttrEach ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Given input fields:
// - probe option parameter
//
// Derive:
// - imCumTypCnt[]
//
void CimCfg::AttrEach::deriveChanCounts( int opt )
{
// --------------------------------
// First count each type separately
// --------------------------------

    imCumTypCnt[imTypeAP] = (opt == 4 ? IMROTbl::imOpt4Chan : IMROTbl::imOpt3Chan);
    imCumTypCnt[imTypeLF] = imCumTypCnt[imTypeAP];
    imCumTypCnt[imTypeSY] = 1;

// ---------
// Integrate
// ---------

    for( int i = 1; i < imNTypes; ++i )
        imCumTypCnt[i] += imCumTypCnt[i - 1];
}


void CimCfg::AttrEach::justAPBits(
    QBitArray       &apBits,
    const QBitArray &saveBits ) const
{
    apBits = saveBits;
    apBits.fill( 0, imCumTypCnt[imSumAP], imCumTypCnt[imSumNeural] );
}


void CimCfg::AttrEach::justLFBits(
    QBitArray       &lfBits,
    const QBitArray &saveBits ) const
{
    lfBits = saveBits;
    lfBits.fill( 0, 0, imCumTypCnt[imSumAP] );
}

/* ---------------------------------------------------------------- */
/* class CimCfg --------------------------------------------------- */
/* ---------------------------------------------------------------- */

double CimCfg::chanGain( int ip, int ic ) const
{
    double  g = 1.0;

    if( ic > -1 ) {

        int nAP = each[ip].imCumTypCnt[imTypeAP];

        if( ic < nAP )
            g = roTbl.e[ic].apgn;
        else if( ic < each[ip].imCumTypCnt[imTypeLF] )
            g = roTbl.e[ic-nAP].lfgn;
        else
            return 1.0;

// MS: Revisit this base gain value
        if( g < 50.0 )
            g = 50.0;
    }

    return g;
}


// Given input fields:
// - stdbyStr (trimmed)
// - nAP channels (parameter)
//
// Derive:
// - stdbyBits
//
// Return true if stdbyStr format OK.
//
bool CimCfg::deriveStdbyBits( QString &err, int nAP )
{
    err.clear();

    if( stdbyStr.isEmpty() )
        stdbyBits.fill( false, nAP );
    else if( Subset::isAllChansStr( stdbyStr ) ) {

        stdbyStr = "all";
        Subset::defaultBits( stdbyBits, nAP );
    }
    else if( Subset::rngStr2Bits( stdbyBits, stdbyStr ) ) {

        stdbyStr = Subset::bits2RngStr( stdbyBits );

        if( stdbyBits.size() > nAP ) {
            err = QString(
                    "Imec off-channel string includes channels"
                    " higher than maximum [%1].")
                    .arg( nAP - 1 );
            return false;
        }

        // in case smaller
        stdbyBits.resize( nAP );
    }
    else {
        err = "Imec off-channel string has incorrect format.";
        return false;
    }

    return true;
}


int CimCfg::vToInt10( double v, int ip, int ic ) const
{
    return 1023 * all.range.voltsToUnity( v * chanGain( ip, ic ) ) - 512;
}


double CimCfg::int10ToV( int i10, int ip, int ic ) const
{
    return all.range.unityToVolts( (i10 + 512) / 1024.0 )
            / chanGain( ip, ic );
}


void CimCfg::loadSettings( QSettings &S )
{
//    all.range.rmin =
//    S.value( "imAiRangeMin", -0.6 ).toDouble();

//    all.range.rmax =
//    S.value( "imAiRangeMax", 0.6 ).toDouble();

//    all.srate =
//    S.value( "imSampRate", 30000.0 ).toDouble();

    all.softStart =
    S.value( "imSoftStart", true ).toBool();

    imroFile =
    S.value( "imRoFile", QString() ).toString();

    stdbyStr =
    S.value( "imStdby", QString() ).toString();

    hpFltIdx =
    S.value( "imHpFltIdx", 0 ).toInt();

    enabled =
    S.value( "imEnabled", false ).toBool();

    doGainCor =
    S.value( "imDoGainCor", false ).toBool();

    noLEDs =
    S.value( "imNoLEDs", false ).toBool();
}


void CimCfg::saveSettings( QSettings &S ) const
{
    S.setValue( "imAiRangeMin", all.range.rmin );
    S.setValue( "imAiRangeMax", all.range.rmax );
    S.setValue( "imSampRate", all.srate );
    S.setValue( "imSoftStart", all.softStart );

    S.setValue( "imRoFile", imroFile );
    S.setValue( "imStdby", stdbyStr );
    S.setValue( "imHpFltIdx", hpFltIdx );
    S.setValue( "imEnabled", enabled );
    S.setValue( "imDoGainCor", doGainCor );
    S.setValue( "imNoLEDs", noLEDs );
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
    Neuropix_basestation_api    &IM,
    int                         nProbes )
{
// Favorite broken test probe ----
#if 0
for( int ip = 0; ip < nProbes; ++ip ) {
    QString sn  = "513180531";
    int     opt = 1;
    imVers.prb.push_back( CimCfg::IMProbeRec( sn, 1, true ) );
    sl.append( QString("Probe serial# %1, option %2")
        .arg( sn )
        .arg( opt ) );
}
return true;
#endif
//--------------------------------

// MS: Generalize, currently read one probe and copy it
    AsicID  asicID;
    int     err = IM.neuropix_readId( asicID );

    if( err != EEPROM_SUCCESS ) {
        sl.append( QString("IMEC readId error %1.").arg( err ) );
        return false;
    }

    QString sn  = QString::number( asicID.serialNumber );
    int     opt = asicID.probeType + 1;

    for( int ip = 0; ip < nProbes; ++ip ) {
        imVers.prb.push_back( CimCfg::IMProbeRec( sn, opt ) );
        sl.append( QString("Probe serial# %1, option %2")
            .arg( sn )
            .arg( opt ) );
    }
    return true;
}

#endif

/* ---------------------------------------------------------------- */
/* getVersions ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef HAVE_IMEC
bool CimCfg::getVersions(
    QStringList &sl,
    IMVers      &imVers,
    int         nProbes )
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

        if( !_probeID( sl, imVers, IM, nProbes ) )
            goto exit;
    }
    else {

        for( int ip = 0; ip < nProbes; ++ip ) {

            imVers.prb.push_back( CimCfg::IMProbeRec( "0", 0 ) );
            sl.append( QString("Probe serial# 0, option 0") );
        }
    }

    ok = true;

exit:
    sl.append( "-- end --" );
    IM.neuropix_close();
    return ok;
}
#else
bool CimCfg::getVersions(
    QStringList &sl,
    IMVers      &imVers,
    int         nProbes )
{
    imVers.clear();
    sl.clear();

    imVers.hwr  = "0.0";
    imVers.bas  = "0.0";
    imVers.api  = "0.0";

    sl.append( "Hardware version 0.0 (simulated)" );
    sl.append( "Basestation version 0.0 (simulated)" );
    sl.append( "API version 0.0 (simulated)" );

    for( int ip = 0; ip < nProbes; ++ip ) {
        imVers.prb.push_back( CimCfg::IMProbeRec( "0", 3 ) );
        sl.append( QString("Probe serial# 0, option 3") );
    }
    return true;
}
#endif


