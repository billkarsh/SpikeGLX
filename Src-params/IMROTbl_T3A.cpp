
#include "IMROTbl_T3A.h"
#include "Util.h"

#include <QFileInfo>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc_T3A -------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "chn bank refid apgn lfgn"
//
QString IMRODesc_T3A::toString( int chn ) const
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
IMRODesc_T3A IMRODesc_T3A::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc_T3A(
            sl.at( 1 ).toShort(), sl.at( 2 ).toShort(),
            sl.at( 3 ).toShort(), sl.at( 4 ).toShort() );
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl_T3A --------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T3A::fillDefault()
{
    type        = imType3AType;
    this->pSN   = 0;
    this->opt   = 3;

    e.clear();
    e.resize( opt == 4 ? imType3AOpt4Chan : imType3AOpt3Chan );
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T3A::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T3A   *RHS    = (const IMROTbl_T3A*)rhs;
    int                 n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (pSN,opt,nchan)(chn bank refid apgn lfgn)()()...
//
QString IMROTbl_T3A::toString() const
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
void IMROTbl_T3A::fromString( const QString &s )
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
        e.push_back( IMRODesc_T3A::fromString( sl[i] ) );
}


bool IMROTbl_T3A::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find '%1'").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromString( f.readAll() );

        if( (opt <= 3 && nChan() == imType3AOpt3Chan)
            || (opt == 4 && nChan() == imType3AOpt4Chan) ) {

            msg = QString("Loaded (SN,opt)=(%1,%2) file '%3'")
                    .arg( pSN )
                    .arg( opt )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString(
                    "Error: Wrong option [%1] or chan count [%2] in file '%3'")
                    .arg( opt ).arg( nChan() ).arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
        return false;
    }
}


bool IMROTbl_T3A::saveFile( QString &msg, const QString &path ) const
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        int n = f.write( STR2CHR( toString() ) );

        if( n > 0 ) {

            msg = QString("Saved (SN,opt)=(%1,%2) file '%3'")
                    .arg( pSN )
                    .arg( opt )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error writing '%1'").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
        return false;
    }
}


int IMROTbl_T3A::elShankAndBank( int &bank, int ch ) const
{
    bank = e[ch].bank;
    return 0;
}


int IMROTbl_T3A::elShankColRow( int &col, int &row, int ch ) const
{
    int el = chToEl( ch ) - 1;

    row = el / 2;
    col = el - 2 * row;

    return 0;
}


void IMROTbl_T3A::eaChansOrder( QVector<int> &v ) const
{
    QMap<int,int>   el2Ch;
    int             order   = 0,
                    _nAP    = nAP();

    v.resize( 2*_nAP + 1 );

// Order the AP set

    for( int ic = 0; ic < _nAP; ++ic )
        el2Ch[chToEl( ic )] = ic;

    QMap<int,int>::iterator it;

    for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
        v[it.value()] = order++;

// The LF set have same order but offset by nAP

    for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
        v[it.value() + _nAP] = order++;

// SY is last

    v[order] = order;
}


static int _r2c384[IMROTbl_T3A::imType3AOpt3Refs] = {-1,36,75,112,151,188,227,264,303,340,379};
static int _r2c276[IMROTbl_T3A::imType3AOpt4Refs] = {-1,36,75,112,151,188,227,264};
static int i2gn[IMROTbl_T3A::imType3AGains]       = {50,125,250,500,1000,1500,2000,2500};


// Copied from T0, not really right for 3A, but not needed offline.
//
// refid [0]    ext, shank=0, bank=0.
// refid [1]    tip, shank=0, bank=0.
// refid [2..4] int, shank=0, bank=id-2.
//
int IMROTbl_T3A::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    shank = 0;

    if( rid == 0 ) {
        bank = 0;
        return 0;
    }
    else if( rid == 1 ) {
        bank = 0;
        return 1;
    }

    bank = rid - 2;
    return 2;
}


int IMROTbl_T3A::chToEl( int ch ) const
{
    return (opt == 4 ?
            chToEl276( ch, e[ch].bank ) :
            chToEl384( ch, e[ch].bank ));
}


const int* IMROTbl_T3A::optTo_r2c( int opt )
{
    if( opt <= 3 )
        return _r2c384;

    return _r2c276;
}


int IMROTbl_T3A::optToNRef( int opt )
{
    if( opt <= 3 )
        return imType3AOpt3Refs;

    return imType3AOpt4Refs;
}


int IMROTbl_T3A::elToCh384( int el )
{
    return (el > 0 ? (el - 1) % imType3AOpt3Chan : -1);
}


int IMROTbl_T3A::elToCh276( int el )
{
    return (el > 0 ? (el - 1) % imType3AOpt4Chan : -1);
}


int IMROTbl_T3A::chToEl384( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*imType3AOpt3Chan : 0);
}


int IMROTbl_T3A::chToEl276( int ch, int bank )
{
    return (ch >= 0 ? (ch + 1) + bank*imType3AOpt4Chan : 0);
}


int IMROTbl_T3A::chToRefid384( int ch )
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


int IMROTbl_T3A::chToRefid276( int ch )
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


int IMROTbl_T3A::elToRefid384( int el )
{
    if( el > 0 )
        return chToRefid384( elToCh384( el ) );

    return 0;
}


int IMROTbl_T3A::elToRefid276( int el )
{
    if( el > 0 )
        return chToRefid276( elToCh276( el ) );

    return 0;
}


bool IMROTbl_T3A::chIsRef( int ch ) const
{
    if( opt == 4 )
        return chToRefid276( ch ) == 0;

    return chToRefid384( ch ) == 0;
}


int IMROTbl_T3A::idxToGain( int idx ) const
{
    return (idx >= 0 && idx < 8 ? i2gn[idx] : i2gn[3]);
}


int IMROTbl_T3A::gainToIdx( int gain ) const
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
        case 3000:
            return 7;
        default:
            break;
    }

    return 3;
}


void IMROTbl_T3A::muxTable( int &nADC, int &nChn, std::vector<int> &T ) const
{
    nADC = 32;
    nChn = 12;

    T.resize( 384 );

// Generate by pairs of columns

    int ch = 0;

    for( int icol = 0; icol < nADC; icol += 2 ) {

        for( int irow = 0; irow < nChn; ++irow ) {
            T[nADC*irow + icol]     = ch++;
            T[nADC*irow + icol + 1] = ch++;
        }
    }
}


