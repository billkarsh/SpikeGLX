
#include "IMROTbl_T2020.h"

#include <QIODevice>
#include <QRegularExpression>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

int IMRODesc_T2020::chToEl( int ch, int bank )
{
    ch %= 384;

    return (ch >= 0 ? ch + bank * 384 : 0);
}


// Pattern: "(chn shank bank refid elec)"
//
QString IMRODesc_T2020::toString( int chn ) const
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
bool IMRODesc_T2020::fromString( QString *msg, const QString &s )
{
    const QStringList   sl = s.split(
                                QRegularExpression("\\s+"),
                                Qt::SkipEmptyParts );
    bool                ok;

    if( sl.size() != 5 )
        goto fail;

    shnk    = sl.at( 1 ).toInt( &ok ); if( !ok ) goto fail;
    bank    = sl.at( 2 ).toInt( &ok ); if( !ok ) goto fail;
    refid   = sl.at( 3 ).toInt( &ok ); if( !ok ) goto fail;

    return true;

fail:
    if( msg ) {
        *msg =
        QString("Bad IMRO element format (%1), expected (chn shnk bank refid elec)")
        .arg( s );
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T2020Key::operator<( const T2020Key &rhs ) const
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

void IMROTbl_T2020::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T2020::fillDefault()
{
    e.clear();
    e.resize( imType2020Chan );

// Set shnk fields

    for( int is = 1; is < imType2020Shanks; ++is ) {

        int ic0 = is * imType2020ChPerShk;

        for( int ic = 0; ic < imType2020ChPerShk; ++ic )
            e[ic0 + ic].shnk = is;
    }

    setElecs();
}


// All shanks reflect given bank
//
void IMROTbl_T2020::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    for( int i = 0, n = e.size(); i < n; ++i ) {
        IMRODesc_T2020  &E = e[i];
        E.bank = qMin( bank, maxBank( i, E.shnk ) );
    }

    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T2020::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T2020 *RHS    = (const IMROTbl_T2020*)rhs;
    int                 n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].shnk != RHS->e[i].shnk || e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
QString IMROTbl_T2020::toString() const
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
bool IMROTbl_T2020::fromString( QString *msg, const QString &s )
{
    QStringList sl = s.split(
                        QRegularExpression("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        Qt::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegularExpression("^\\s+|\\s*,\\s*"),
                        Qt::SkipEmptyParts );

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

    for( int i = 1; i < n; ++i ) {
        IMRODesc_T2020  D;
        if( D.fromString( msg, sl[i] ) )
            e.push_back( D );
        else
            return false;
    }

    if( e.size() != nAP() ) {
        if( msg ) {
            *msg = QString("Wrong imro entry count [%1] (should be %2)")
                    .arg( e.size() ).arg( nAP() );
        }
        return false;
    }

    setElecs();

    return true;
}


int IMROTbl_T2020::elShankAndBank( int &bank, int ch ) const
{
    const IMRODesc_T2020    &E = e[ch];
    bank = E.bank;
    return E.shnk;
}


int IMROTbl_T2020::elShankColRow( int &col, int &row, int ch ) const
{
    const IMRODesc_T2020    &E = e[ch];
    int                     el = E.elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return E.shnk;
}


void IMROTbl_T2020::eaChansOrder( QVector<int> &v ) const
{
    int _nAP    = nAP(),
        _nSY    = nSY(),
        order   = 0;

    v.resize( _nAP + _nSY );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < imType2020Shanks; ++sh ) {

        QMap<int,int>   el2Ch;

        for( int ic = 0; ic < _nAP; ++ic ) {

            const IMRODesc_T2020    &E = e[ic];

            if( E.shnk == sh )
                el2Ch[E.elec] = ic;
        }

        QMap<int,int>::iterator it;

        for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
            v[it.value()] = order++;
    }

// SY are last

    for( int ic = 0; ic < _nSY; ++ic )
        v[_nAP + ic] = order++;
}


// refid [0]    ext, shank=s, bank=b.
// refid [1]    gnd, shank=s, bank=b.
// refid [2]    tip, shank=s, bank=b.
//
int IMROTbl_T2020::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    const IMRODesc_T2020    &E = e[ch];

    shank = E.shnk;
    bank  = E.bank;

    if( E.refid == 0 )
        return 0;
    else if( E.refid == 1 )
        return 3;

    return 1;
}


void IMROTbl_T2020::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T2020::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 24 * imType2020Shanks;
    nGrp = 16;

    T.resize( imType2020Chan );

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

void IMROTbl_T2020::edit_init() const
{
// forward

    for( int c = 0; c < imType2020Chan; ++c ) {

        int s = c / imType2020ChPerShk;

        for( int b = 0; b < imType2020Banks; ++b ) {

            int e = IMRODesc_T2020::chToEl( c, b );

            if( e < imType2020ElPerShk )
                k2s[T2020Key( c, s, b )] = IMRO_Site( s, e & 1, e >> 1 );
            else
                break;
        }
    }

// inverse

    QMap<T2020Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T2020::edit_GUI() const
{
    IMRO_GUI    G;
    G.refs.push_back( "Ground" );
    G.refs.push_back( "Tip" );
    G.gains.push_back( apGain( 0 ) );
    G.nBase = 4;
    return G;
}


IMRO_Attr IMROTbl_T2020::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T2020::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), 0, 0, 0 );
}


bool IMROTbl_T2020::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType2020Chan )
        return false;

    int refid = e[0].refid;

    for( int ie = 1; ie < ne; ++ie ) {
        if( e[ie].refid != refid )
            return false;
    }

    return true;
}


void IMROTbl_T2020::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T2020Key    K = s2k[s];

    QMap<T2020Key,IMRO_Site>::const_iterator
        it  = k2s.find( T2020Key( K.c, K.s, 0 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T2020Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.b != K.b )
            vX.push_back( k2s[ik] );
    }
}


int IMROTbl_T2020::edit_site2Chan( const IMRO_Site &s ) const
{
    return s2k[s].c;
}


void IMROTbl_T2020::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType2020Chan );

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = B.c_0(),
            cL = B.c_lim( _ncolhwr );

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T2020Key  &K = s2k[IMRO_Site( B.s, c, r )];
                IMRODesc_T2020  &E = e[K.c];

                E.shnk  = K.s;
                E.bank  = K.b;
                E.refid = A.refIdx;
            }
        }
    }

    setElecs();
}


void IMROTbl_T2020::edit_defaultROI( tImroROIs vR ) const
{
    int rowsPerBank = nChanPerBank() / _ncolhwr;

    vR.clear();

    for( int is = 0; is < imType2020Shanks; ++is )
        vR.push_back( IMRO_ROI( is, 0, rowsPerBank ) );
}


// - Boxes are whole- or half-shank width.
// - Boxes enclose all channels per shank.
// - Canonical attributes all channels.
//
bool IMROTbl_T2020::edit_isCanonical( tImroROIs vR ) const
{
// Assess boxes per shank

    int ne[4] = {0,0,0,0};

    for( int ib = 0; ib < vR.size(); ++ib ) {

        const IMRO_ROI  &B = vR[ib];
        int             w  = B.width( _ncolhwr );

        if( !(w == _ncolhwr || w == _ncolhwr/2) )
            return false;

        ne[B.s] += B.area( _ncolhwr );
    }

    for( int is = 0; is < imType2020Shanks; ++is ) {
        if( ne[is] != imType2020ChPerShk )
            return false;
    }

    return edit_Attr_canonical();
}


