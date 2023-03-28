
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
int IMRODesc_T24::chToEl( int ch, int shank, int bank )
{
   int  block = ch / 48,
        subid = ch - 48 * block;

   return 384*bank + 48*blockMap[shank][block] + subid;
}


// Pattern: "(chn shnk bank refid elec)"
//
QString IMRODesc_T24::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4 %5)")
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
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T24Key::operator<( const T24Key &rhs ) const
{
    if( c < rhs.c )
        return true;

    if( c > rhs.c )
        return false;

    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    return b < rhs.b;
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

IMROTbl_T24::IMROTbl_T24( const QString &pn ) : IMROTbl(pn, imType24Type)
{
    if( pn.startsWith( "P" ) ) {
        _vRng   = 1.0;
        _vMax   = 0.5;
        _maxInt = 8192;
        _gain   = 80;
        _refChn = 127;
    }
    else {
        switch( pn.mid( 2 ).toInt() ) {
            case 2010:
                _vRng   = 1.0;
                _vMax   = 0.5;
                _maxInt = 8192;
                _gain   = 80;
                _refChn = 127;
                break;
            default:    // 2013, 2014
                _vRng   = 1.24;
                _vMax   = 0.62;
                _maxInt = 2048;
                _gain   = 100;
                _refChn = -1;
                break;
        }
    }
}


void IMROTbl_T24::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T24::fillDefault()
{
    type = imType24Type;
    e.clear();
    e.resize( imType24Chan );
    setElecs();
}


void IMROTbl_T24::fillShankAndBank( int shank, int bank )
{
    for( int i = 0, n = e.size(); i < n; ++i ) {
        IMRODesc_T24    &E = e[i];
        E.shnk = shank;
        E.bank = qMin( bank, maxBank( i, shank ) );
    }

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
        ts << e[i].toString( i );

    return s;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T24::fromString( QString *msg, const QString &s )
{
    QStringList sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    if( hl.size() != 2 ) {
        type = -3;      // 3A type
        if( msg )
            *msg = "Wrong imro header format [should be (type,nchan)]";
        return false;
    }

    type = hl[0].toInt();

    if( type != imType24Type ) {
        if( msg ) {
            *msg = QString("Wrong imro type[%1] for probe type[%2]")
                    .arg( type ).arg( imType24Type );
        }
        return false;
    }

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc_T24::fromString( sl[i] ) );

    if( e.size() != imType24Chan ) {
        if( msg ) {
            *msg = QString("Wrong imro entry count [%1] (should be %2)")
                    .arg( e.size() ).arg( imType24Type );
        }
        return false;
    }

    setElecs();

    return true;
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

        QString reason;

        if( fromString( &reason, f.readAll() ) ) {

            msg = QString("Loaded (type=%1) file '%2'")
                    .arg( type ).arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error: %1 in file '%2'")
                    .arg( reason ).arg( fi.fileName() );
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


int IMROTbl_T24::maxBank( int ch, int shank )
{
    int maxBank = imType24Banks - 1;

    if( IMRODesc_T24::chToEl( ch, shank, maxBank ) >= imType24ElPerShk )
        --maxBank;

    return maxBank;
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

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return E.shnk;
}


void IMROTbl_T24::eaChansOrder( QVector<int> &v ) const
{
    int order = 0;

    v.resize( imType24Chan + 1 );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < imType24Shanks; ++sh ) {

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
    else if( rid <= imType24Shanks ) {
        shank   = rid - 1;
        bank    = 0;
        return 1;
    }

    rid -= 5;

    shank   = rid / imType24Banks;
    bank    = rid - imType24Banks * shank;
    return 2;
}


bool IMROTbl_T24::chIsRef( int ch ) const
{
    return ch == _refChn;
}


void IMROTbl_T24::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T24::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 24;
    nGrp = 16;

    T.resize( 384 );

// Generate by pairs of columns

    int ch = 0;

    for( int icol = 0; icol < nADC; icol += 2 ) {

        for( int irow = 0; irow < nGrp; ++irow ) {
            T[nADC*irow + icol]     = ch++;
            T[nADC*irow + icol + 1] = ch++;
        }
    }
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T24::edit_init() const
{
// forward

    for( int c = 0; c < imType24Chan; ++c ) {

        for( int s = 0; s < imType24Shanks; ++s ) {

            for( int b = 0; b < imType24Banks; ++b ) {

                int e = IMRODesc_T24::chToEl( c, s, b );

                if( e < imType24ElPerShk )
                    k2s[T24Key( c, s, b )] = IMRO_Site( s, e & 1, e >> 1 );
                else
                    break;
            }
        }
    }

// inverse

    QMap<T24Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T24::edit_GUI() const
{
    IMRO_GUI    G;
    G.gains.push_back( _gain );
    G.grid = 8;     // prevents editing fragmentation
    return G;
}


IMRO_Attr IMROTbl_T24::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T24::edit_Attr_cur() const
{
    IMRO_Attr   A( refid( 0 ), 0, 0, 0 );

    if( e[0].refid == 1 && e[1].refid == 2 &&
        e[2].refid == 3 && e[3].refid == 4 ) {

        A.refIdx = 5;
    }

    return A;
}


bool IMROTbl_T24::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType24Chan )
        return false;

    const IMRODesc_T24  &E = e[0];

    for( int ie = 4; ie < ne; ++ie ) {
        if( e[ie].refid != E.refid )
            return false;
    }

    return true;
}


void IMROTbl_T24::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T24Key  K = s2k[s];

    QMap<T24Key,IMRO_Site>::const_iterator
        it  = k2s.find( T24Key( K.c, 0, 0 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T24Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.s != K.s || ik.b != K.b )
            vX.push_back( k2s[ik] );
    }
}


void IMROTbl_T24::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType24Chan );

    int     refIdx      = A.refIdx;
    bool    joinTips    = A.refIdx == 5;

    if( joinTips )
        refIdx = 1;

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = qMax( 0, B.c0 ),
            cL = (B.cLim >= 0 ? B.cLim : _ncolhwr);

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T24Key    &K = s2k[IMRO_Site( B.s, c, r )];
                IMRODesc_T24    &E = e[K.c];

                E.shnk  = K.s;
                E.bank  = K.b;
                E.refid = refIdx;
            }
        }
    }

    if( joinTips ) {
        for( int c = 1; c < 4; ++c )
            e[c].refid = 1 + c;
    }

    setElecs();
}


