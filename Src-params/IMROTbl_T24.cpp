
#include "IMROTbl_T24.h"
#include "Util.h"

#include <QFileInfo>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

static char blockMap[4][8] = // 4 shanks X 8 orders
    {
       {0,6,1,7,2,4,3,5},
       {6,0,7,1,4,2,5,3},
       {2,4,3,5,0,6,1,7},
       {4,2,5,3,6,0,7,1}
    };


// Each channel lives in only one block within a bank.
// Each electrode also lives in one block in a bank.
// Blocks contain 48 consecutively numbered channels.
// Each shank has its own order of blocks.
//
int IMRODesc_T24::chToEl( int ch ) const
{
   int  block = ch / 48,
        subid = ch - 48 * block;

   return 384*bank + 48*blockMap[shnk][block] + subid;
}


// Pattern: "chn shnk bank refid elec"
//
QString IMRODesc_T24::toString( int chn ) const
{
    return QString("%1 %2 %3 %4 %5")
            .arg( chn ).arg( shnk )
            .arg( bank ).arg( refid )
            .arg( elec );
}


// Pattern: "chn shnk bank refid elec"
//
// Note: The chn field is discarded and elec is recalculated by caller.
//
IMRODesc_T24 IMRODesc_T24::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc_T24(
            sl.at( 1 ).toInt(), sl.at( 2 ).toInt(),
            sl.at( 3 ).toInt() );
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T24::setElecs()
{
    int n = nChan();

    for( int i = 0; i < n; ++i )
        e[i].elec = e[i].chToEl( i );
}


void IMROTbl_T24::fillDefault()
{
    type = imType24Type;

    e.clear();
    e.resize( imType24Chan );
    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T24::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T24   *RHS    = (const IMROTbl_T24*)rhs;
    int                 n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].shnk != RHS->e[i].shnk || e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
QString IMROTbl_T24::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << type << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString( i ) << ")";

    return s;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
void IMROTbl_T24::fromString( const QString &s )
{
    QStringList sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    type = hl[0].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc_T24::fromString( sl[i] ) );

    setElecs();
}


bool IMROTbl_T24::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find '%1'").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromString( f.readAll() );

        if( type == imType24Type && nChan() == imType24Chan ) {

            msg = QString("Loaded (type=%1) file '%2'")
                    .arg( type )
                    .arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString(
                    "Error: Wrong type [%1] or chan count [%2] in file '%3'")
                    .arg( type ).arg( nChan() ).arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
        return false;
    }
}


bool IMROTbl_T24::saveFile( QString &msg, const QString &path ) const
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        int n = f.write( STR2CHR( toString() ) );

        if( n > 0 ) {

            msg = QString("Saved (type=%1) file '%2'")
                    .arg( type )
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


int IMROTbl_T24::elShankAndBank( int &bank, int ch ) const
{
    const IMRODesc_T24  &E = e[ch];
    bank = E.bank;
    return E.shnk;
}


int IMROTbl_T24::elShankColRow( int &col, int &row, int ch ) const
{
    const IMRODesc_T24  &E = e[ch];
    int                 el = E.elec;

    row = el / 2;
    col = el - 2 * row;

    return E.shnk;
}


void IMROTbl_T24::eaChansOrder( QVector<int> &v ) const
{
    int order = 0;

    v.resize( imType24Chan + 1 );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < 4; ++sh ) {

        QMap<int,int>   el2Ch;

        for( int ic = 0; ic < imType24Chan; ++ic ) {

            const IMRODesc_T24  &E = e[ic];

            if( E.shnk == sh )
                el2Ch[E.elec] = ic;
        }

        QMap<int,int>::iterator it;

        for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
            v[it.value()] = order++;
    }

// SY is last

    v[order] = order;
}


static int refs[4] = {127,511,895,1279};


// refid [0]      ext, shank=0,    bank=0.
// refid [1..4]   tip, shank=id-1, bank=0.
// refid [5..8]   int, shank=0,    bank=id-5.
// refid [9..12]  int, shank=1,    bank=id-9.
// refid [13..16] int, shank=2,    bank=id-13.
// refid [17..20] int, shank=3,    bank=id-17.
//
int IMROTbl_T24::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    const IMRODesc_T24  &E  = e[ch];
    int                 rid = E.refid;

    if( rid == 0 ) {
        shank   = 0;
        bank    = 0;
        return 0;
    }
    else if( rid <= 4 ) {
        shank   = rid - 1;
        bank    = 0;
        return 1;
    }

    rid -= 5;

    shank   = rid / 4;
    bank    = rid - 4 * shank;
    return 2;
}


bool IMROTbl_T24::chIsRef( int ch ) const
{
    return ch == 127;
}


void IMROTbl_T24::muxTable( int &nADC, int &nChn, std::vector<int> &T ) const
{
    nADC = 24;
    nChn = 16;

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


