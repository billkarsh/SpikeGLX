
#include "IMROTbl_T24base.h"
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
int IMRODesc_T24base::chToEl( int ch, int shank, int bank )
{
   int  block = ch / 48,
        subid = ch - 48 * block;

   return 384*bank + 48*blockMap[shank][block] + subid;
}


// Pattern: "(chn shnk bank refid elec)"
//
QString IMRODesc_T24base::toString( int chn ) const
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
IMRODesc_T24base IMRODesc_T24base::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc_T24base(
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

void IMROTbl_T24base::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T24base::fillDefault()
{
    e.clear();
    e.resize( imType24baseChan );
    setElecs();
}


void IMROTbl_T24base::fillShankAndBank( int shank, int bank )
{
    for( int i = 0, n = e.size(); i < n; ++i ) {
        IMRODesc_T24base    &E = e[i];
        E.shnk = shank;
        E.bank = qMin( bank, maxBank( i, shank ) );
    }

    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T24base::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T24base   *RHS    = (const IMROTbl_T24base*)rhs;
    int                 n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].shnk != RHS->e[i].shnk || e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
QString IMROTbl_T24base::toString() const
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
bool IMROTbl_T24base::fromString( QString *msg, const QString &s )
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
        if( msg )
            *msg = "Wrong imro header format [should be (type,nchan)]";
        return false;
    }

    int type = hl[0].toInt();

    if( type != typeConst() ) {
        if( msg ) {
            *msg = QString("Wrong imro type[%1] for probe type[%2]")
                    .arg( type ).arg( typeConst() );
        }
        return false;
    }

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( IMRODesc_T24base::fromString( sl[i] ) );

    if( e.size() != imType24baseChan ) {
        if( msg ) {
            *msg = QString("Wrong imro entry count [%1] (should be %2)")
                    .arg( e.size() ).arg( imType24baseChan );
        }
        return false;
    }

    setElecs();

    return true;
}


bool IMROTbl_T24base::loadFile( QString &msg, const QString &path )
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


bool IMROTbl_T24base::saveFile( QString &msg, const QString &path ) const
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


int IMROTbl_T24base::maxBank( int ch, int shank )
{
    int maxBank = imType24baseBanks - 1;

    if( IMRODesc_T24base::chToEl( ch, shank, maxBank ) >= imType24baseElPerShk )
        --maxBank;

    return maxBank;
}


int IMROTbl_T24base::elShankAndBank( int &bank, int ch ) const
{
    const IMRODesc_T24base  &E = e[ch];
    bank = E.bank;
    return E.shnk;
}


int IMROTbl_T24base::elShankColRow( int &col, int &row, int ch ) const
{
    const IMRODesc_T24base  &E = e[ch];
    int                     el = E.elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return E.shnk;
}


void IMROTbl_T24base::eaChansOrder( QVector<int> &v ) const
{
    int order = 0;

    v.resize( imType24baseChan + 1 );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < imType24baseShanks; ++sh ) {

        QMap<int,int>   el2Ch;

        for( int ic = 0; ic < imType24baseChan; ++ic ) {

            const IMRODesc_T24base  &E = e[ic];

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


void IMROTbl_T24base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T24base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
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

void IMROTbl_T24base::edit_init() const
{
// forward

    for( int c = 0; c < imType24baseChan; ++c ) {

        for( int s = 0; s < imType24baseShanks; ++s ) {

            for( int b = 0; b < imType24baseBanks; ++b ) {

                int e = IMRODesc_T24base::chToEl( c, s, b );

                if( e < imType24baseElPerShk )
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


IMRO_GUI IMROTbl_T24base::edit_GUI() const
{
    IMRO_GUI    G;

    if( tip0refID() == 2 )
        G.refs.push_back( "Ground" );

    for( int i = 0; i < imType24baseShanks; ++i )
        G.refs.push_back( QString("Tip %1").arg( i ) );
    G.refs.push_back( "Join Tips" );

    G.gains.push_back( apGain( 0 ) );
    G.grid = 8;     // prevents editing fragmentation
    return G;
}


IMRO_Attr IMROTbl_T24base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T24base::edit_Attr_cur() const
{
    IMRO_Attr   A( refid( 0 ), 0, 0, 0 );

    int tip0 = tip0refID();

    if( e[0].refid == tip0+0 && e[1].refid == tip0+1 &&
        e[2].refid == tip0+2 && e[3].refid == tip0+3 ) {

        A.refIdx = tip0+4;
    }

    return A;
}


bool IMROTbl_T24base::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType24baseChan )
        return false;

    const IMRODesc_T24base  &E = e[0];

    for( int ie = 4; ie < ne; ++ie ) {
        if( e[ie].refid != E.refid )
            return false;
    }

    return true;
}


void IMROTbl_T24base::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
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


void IMROTbl_T24base::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType24baseChan );

    int     refIdx      = A.refIdx,
            tip0        = tip0refID();
    bool    joinTips    = A.refIdx == tip0+4;

    if( joinTips )
        refIdx = tip0;

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = qMax( 0, B.c0 ),
            cL = (B.cLim >= 0 ? B.cLim : _ncolhwr);

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T24Key        &K = s2k[IMRO_Site( B.s, c, r )];
                IMRODesc_T24base    &E = e[K.c];

                E.shnk  = K.s;
                E.bank  = K.b;
                E.refid = refIdx;
            }
        }
    }

    if( joinTips ) {
        for( int c = 1; c < 4; ++c )
            e[c].refid = tip0 + c;
    }

    setElecs();
}


