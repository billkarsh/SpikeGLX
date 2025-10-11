
#include "IMROTbl_T3010base.h"

#include <QIODevice>
#include <QRegularExpression>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

int IMRODesc_T3010base::chToEl( int ch, int bank )
{
    return (ch >= 0 ? ch + bank * 912 : 0);
}


// Pattern: "(chn bank refid elec)"
//
QString IMRODesc_T3010base::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4)")
            .arg( chn ).arg( bank )
            .arg( refid ).arg( elec );
}


// Pattern: "chn bank refid elec"
//
// Note: The chn field is discarded and elec is recalculated by caller.
//
bool IMRODesc_T3010base::fromString( QString *msg, const QString &s )
{
    const QStringList   sl = s.split(
                                QRegularExpression("\\s+"),
                                Qt::SkipEmptyParts );
    bool                ok;

    if( sl.size() != 4 )
        goto fail;

    bank    = sl.at( 1 ).toInt( &ok ); if( !ok ) goto fail;
    refid   = sl.at( 2 ).toInt( &ok ); if( !ok ) goto fail;

    return true;

fail:
    if( msg ) {
        *msg =
        QString("Bad IMRO element format (%1), expected (chn bank refid elec)")
        .arg( s );
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T3010Key::operator<( const T3010Key &rhs ) const
{
    if( c < rhs.c )
        return true;
    if( c > rhs.c )
        return false;
    return b < rhs.b;
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T3010base::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T3010base::fillDefault()
{
    e.clear();
    e.resize( imType3010baseChan );
    setElecs();
}


void IMROTbl_T3010base::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    for( int i = 0, n = e.size(); i < n; ++i )
        e[i].bank = qMin( bank, maxBank( i ) );

    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T3010base::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T3010base *RHS    = (const IMROTbl_T3010base*)rhs;
    int                     n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn bank refid elec)()()...
//
QString IMROTbl_T3010base::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << type << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString( i );

    return s;
}


// Pattern: (type,nchan)(chn bank refid elec)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T3010base::fromString( QString *msg, const QString &s )
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
        IMRODesc_T3010base    D;
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


int IMROTbl_T3010base::maxBank( int ch, int shank ) const
{
    Q_UNUSED( shank )
    return int(ch < 368);
}


int IMROTbl_T3010base::elShankAndBank( int &bank, int ch ) const
{
    bank = e[ch].bank;
    return 0;
}


int IMROTbl_T3010base::elShankColRow( int &col, int &row, int ch ) const
{
    int el = e[ch].elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return 0;
}


void IMROTbl_T3010base::eaChansOrder( QVector<int> &v ) const
{
    QMap<int,int>   el2Ch;
    int             _nAP    = nAP(),
                    order   = 0;

    v.resize( _nAP + 1 );

// Order the AP set

    for( int ic = 0; ic < _nAP; ++ic )
        el2Ch[e[ic].elec] = ic;

    QMap<int,int>::iterator it;

    for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
        v[it.value()] = order++;

// SY is last

    v[order] = order;
}


// refid [0]    ext, shank=0, bank=0.
// refid [1]    gnd, shank=0, bank=0.
// refid [2]    tip, shank=0, bank=0.
//
int IMROTbl_T3010base::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    shank = 0;
    bank  = 0;

    if( rid == 0 )
        return 0;
    else if( rid == 1 )
        return 3;

    return 1;
}


void IMROTbl_T3010base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T3010base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 58;
    nGrp = 16;

    T.resize( nADC * nGrp );

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

void IMROTbl_T3010base::edit_init() const
{
// forward

    for( int c = 0; c < imType3010baseChan; ++c ) {

        for( int b = 0; b < imType3010baseBanks; ++b ) {

            int e = IMRODesc_T3010base::chToEl( c, b );

            if( e < imType3010baseElec )
                k2s[T3010Key( c, b )] = IMRO_Site( 0, e & 1, e >> 1 );
            else
                break;
        }
    }

// inverse

    QMap<T3010Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T3010base::edit_GUI() const
{
    IMRO_GUI    G;
    G.refs.push_back( "Ground" );
    G.refs.push_back( "Tip" );
    G.gains.push_back( apGain( 0 ) );
    return G;
}


IMRO_Attr IMROTbl_T3010base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T3010base::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), 0, 0, 0 );
}


bool IMROTbl_T3010base::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType3010baseChan )
        return false;

    int refid = e[0].refid;

    for( int ie = 1; ie < ne; ++ie ) {
        if( e[ie].refid != refid )
            return false;
    }

    return true;
}


void IMROTbl_T3010base::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T3010Key    K = s2k[s];

    QMap<T3010Key,IMRO_Site>::const_iterator
        it  = k2s.find( T3010Key( K.c, 0 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T3010Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.b != K.b )
            vX.push_back( k2s[ik] );
    }
}


int IMROTbl_T3010base::edit_site2Chan( const IMRO_Site &s ) const
{
    return s2k[s].c;
}


void IMROTbl_T3010base::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType3010baseChan );

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = B.c_0(),
            cL = B.c_lim( _ncolhwr );

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T3010Key      &K = s2k[IMRO_Site( 0, c, r )];
                IMRODesc_T3010base  &E = e[K.c];

                E.bank  = K.b;
                E.refid = A.refIdx;
            }
        }
    }

    setElecs();
}


