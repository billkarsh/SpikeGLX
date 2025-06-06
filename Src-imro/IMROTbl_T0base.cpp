
#include "IMROTbl_T0base.h"

#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc_T0base ----------------------------------------- */
/* ---------------------------------------------------------------- */

int IMRODesc_T0base::chToEl( int ch, int bank )
{
    return (ch >= 0 ? ch + bank * 384 : 0);
}


// Pattern: "(chn bank refid apgn lfgn apflt)"
//
QString IMRODesc_T0base::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4 %5 %6)")
            .arg( chn )
            .arg( bank ).arg( refid )
            .arg( apgn ).arg( lfgn )
            .arg( apflt );
}


// Pattern: "chn bank refid apgn lfgn apflt"
//
// Note: The chn field is discarded.
//
bool IMRODesc_T0base::fromString( QString *msg, const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );
    bool                ok;

    if( sl.size() != 6 )
        goto fail;

    bank    = sl.at( 1 ).toInt( &ok ); if( !ok ) goto fail;
    refid   = sl.at( 2 ).toInt( &ok ); if( !ok ) goto fail;
    apgn    = sl.at( 3 ).toInt( &ok ); if( !ok ) goto fail;
    lfgn    = sl.at( 4 ).toInt( &ok ); if( !ok ) goto fail;
    apflt   = sl.at( 5 ).toInt( &ok ); if( !ok ) goto fail;

    return true;

fail:
    if( msg ) {
        *msg =
        QString("Bad IMRO element format (%1), expected (chn bank refid apgn lfgn apflt)")
        .arg( s );
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T0Key::operator<( const T0Key &rhs ) const
{
    if( c < rhs.c )
        return true;

    if( c > rhs.c )
        return false;

    return b < rhs.b;
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl_T0base ------------------------------------------ */
/* ---------------------------------------------------------------- */

void IMROTbl_T0base::fillDefault()
{
    e.clear();
    e.resize( nAP() );
}


void IMROTbl_T0base::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    for( int i = 0, n = e.size(); i < n; ++i )
        e[i].bank = qMin( bank, maxBank( i ) );
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T0base::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T0base    *RHS    = (const IMROTbl_T0base*)rhs;
    int                     n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn bank refid apgn lfgn apflt)()()...
//
QString IMROTbl_T0base::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << type << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString( i );

    return s;
}


// Pattern: (type,nchan)(chn bank refid apgn lfgn apflt)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T0base::fromString( QString *msg, const QString &s )
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

    for( int i = 1; i < n; ++i ) {
        IMRODesc_T0base D;
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

    return true;
}


int IMROTbl_T0base::elShankAndBank( int &bank, int ch ) const
{
    bank = e[ch].bank;
    return 0;
}


int IMROTbl_T0base::elShankColRow( int &col, int &row, int ch ) const
{
    int el = IMRODesc_T0base::chToEl( ch, e[ch].bank ),
        nc = _ncolhwr;

    row = el / nc;
    col = el - nc * row;

    return 0;
}


void IMROTbl_T0base::eaChansOrder( QVector<int> &v ) const
{
    QMap<int,int>   el2Ch;
    int             _nAP    = nAP(),
                    order   = 0;

    v.resize( 2 * _nAP + 1 );

// Order the AP set

    for( int ic = 0; ic < _nAP; ++ic )
        el2Ch[IMRODesc_T0base::chToEl( ic, e[ic].bank )] = ic;

    QMap<int,int>::iterator it;

    for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
        v[it.value()] = order++;

// The LF set have same order but offset by nAP

    for( it = el2Ch.begin(); it != el2Ch.end(); ++it )
        v[it.value() + _nAP] = order++;

// SY is last

    v[order] = order;
}


// refid [0]    ext, shank=0, bank=0.
// refid [1]    tip, shank=0, bank=0.
// refid [2..4] int, shank=0, bank=id-2.
//
int IMROTbl_T0base::refTypeAndFields( int &shank, int &bank, int ch ) const
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


static int i2gn[IMROTbl_T0base::imType0baseGains]
            = {50,125,250,500,1000,1500,2000,3000};


bool IMROTbl_T0base::chIsRef( int ch ) const
{
    return ch == 191;
}


int IMROTbl_T0base::idxToGain( int idx ) const
{
    return (idx >= 0 && idx < 8 ? i2gn[idx] : i2gn[3]);
}


int IMROTbl_T0base::gainToIdx( int gain ) const
{
    switch( gain ) {
        case 50:    return 0;
        case 125:   return 1;
        case 250:   return 2;
        case 500:   return 3;
        case 1000:  return 4;
        case 1500:  return 5;
        case 2000:  return 6;
        case 3000:  return 7;
        default:    return 3;
    }
}


void IMROTbl_T0base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T0base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 32;
    nGrp = 12;

    T.resize( imType0baseChan );

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

void IMROTbl_T0base::edit_init() const
{
// forward

    int ePerShank = nElecPerShank();

    for( int c = 0, nc = nAP(); c < nc; ++c ) {

        for( int b = 0, nb = nBanks(); b < nb; ++b ) {

            int e = IMRODesc_T0base::chToEl( c, b );

            if( e < ePerShank )
                k2s[T0Key( c, b )] = IMRO_Site( 0, e % _ncolhwr, e / _ncolhwr );
            else
                break;
        }
    }

// inverse

    QMap<T0Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T0base::edit_GUI() const
{
    IMRO_GUI    G;
    G.refs.push_back( "Tip" );
    G.gains.push_back( 50 );
    G.gains.push_back( 125 );
    G.gains.push_back( 250 );
    G.gains.push_back( 500 );
    G.gains.push_back( 1000 );
    G.gains.push_back( 1500 );
    G.gains.push_back( 2000 );
    G.gains.push_back( 3000 );
    G.apEnab = true;
    G.lfEnab = true;
    G.hpEnab = true;
    return G;
}


IMRO_Attr IMROTbl_T0base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 3, 2, 1 );
}


IMRO_Attr IMROTbl_T0base::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), gainToIdx( apGain( 0 ) ),
                    gainToIdx( lfGain( 0 ) ), apFlt( 0 ) );
}


bool IMROTbl_T0base::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != nAP() )
        return false;

    const IMRODesc_T0base   &E = e[0];

    if( E.refid > 1 )
        return false;

    for( int ie = 1; ie < ne; ++ie ) {
        const IMRODesc_T0base   &T = e[ie];
        if( T.apgn  != E.apgn  || T.lfgn  != E.lfgn ||
            T.refid != E.refid || T.apflt != E.apflt ) {

            return false;
        }
    }

    return true;
}


void IMROTbl_T0base::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T0Key   K = s2k[s];

    QMap<T0Key,IMRO_Site>::const_iterator
        it  = k2s.find( T0Key( K.c, 0 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T0Key &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.b != K.b )
            vX.push_back( k2s[ik] );
    }
}


int IMROTbl_T0base::edit_site2Chan( const IMRO_Site &s ) const
{
    return s2k[s].c;
}


void IMROTbl_T0base::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( nAP() );

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = B.c_0(),
            cL = B.c_lim( _ncolhwr );

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T0Key     &K = s2k[IMRO_Site( 0, c, r )];
                IMRODesc_T0base &E = e[K.c];

                E.bank  = K.b;
                E.apgn  = idxToGain( A.apgIdx );
                E.lfgn  = idxToGain( A.lfgIdx );
                E.refid = A.refIdx;
                E.apflt = A.hpfIdx;
            }
        }
    }
}


