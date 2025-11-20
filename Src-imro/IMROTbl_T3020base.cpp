
#include "IMROTbl_T3020base.h"

#include <QIODevice>
#include <QRegularExpression>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

static char blockMap[4][32] = // 4 shanks X 32 blocks, 99 = forbidden
    {
       {0,16,1,17,2,18,3,99,4,99,5,99,6,99,7,99,8,99,9,99,10,99,11,99,12,99,13,99,14,99,15,99},
       {16,0,17,1,18,2,99,3,99,4,99,5,99,6,99,7,99,8,99,9,99,10,99,11,99,12,99,13,99,14,99,15},
       {8,99,9,99,10,99,11,99,12,99,13,99,14,99,15,99,0,16,1,17,2,18,3,99,4,99,5,99,6,99,7,99},
       {99,8,99,9,99,10,99,11,99,12,99,13,99,14,99,15,16,0,17,1,18,2,99,3,99,4,99,5,99,6,99,7}
    };


// Each channel lives in only one block within a bank.
// Each electrode also lives in one block in a bank.
// Blocks contain 48 consecutively numbered channels.
// Each shank has its own order of blocks.
//
int IMRODesc_T3020base::chToEl( int ch, int shank, int bank )
{
   int  block = ch / 48,
        subid = ch - 48 * block;

   return 912*bank + 48*blockMap[shank][block] + subid;
}


// Pattern: "(chn shnk bank refid elec)"
//
QString IMRODesc_T3020base::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4 %5)")
            .arg( chn ).arg( shnk )
            .arg( bank ).arg( refid )
            .arg( elec );
}


// Pattern: "chn shnk bank refid elec"
//
// Note: chan (or -1) is returned; elec is recalculated by caller.
//
int IMRODesc_T3020base::fromString( QString *msg, const QString &s )
{
    const QStringList   sl = s.split(
                                QRegularExpression("\\s+"),
                                Qt::SkipEmptyParts );
    bool                ok;

    if( sl.size() != 5 )
        goto fail;

    chan    = sl.at( 0 ).toInt( &ok ); if( !ok ) goto fail;
    shnk    = sl.at( 1 ).toInt( &ok ); if( !ok ) goto fail;
    bank    = sl.at( 2 ).toInt( &ok ); if( !ok ) goto fail;
    refid   = sl.at( 3 ).toInt( &ok ); if( !ok ) goto fail;

    return chan;

fail:
    if( msg ) {
        *msg =
        QString("Bad IMRO element format (%1), expected (chn shnk bank refid elec)")
        .arg( s );
    }

    return -1;
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T3020Key::operator<( const T3020Key &rhs ) const
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

void IMROTbl_T3020base::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec();

    std::sort( e.begin(), e.end() );
}


// 1536 readout channels -> 768 on two shanks -> 16 blocks per shank.
// Set lowest 16 blocks on shank {0,1}.
//
void IMROTbl_T3020base::fillDefault()
{
    e.clear();

    for( int is = 0; is < 2; ++is ) {

        const char *shblks = blockMap[is];

        for( int ib = 0; ib < 32; ++ib ) {

            if( shblks[ib] < 16 ) {
                for( int ic = 0; ic < 48; ++ic )
                    e.push_back( IMRODesc_T3020base( 48*ib+ic, is, 0, 0 ) );
            }
        }
    }

    setElecs();
}


void IMROTbl_T3020base::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )
    std::vector<IMRO_ROI>   vR;
    IMRO_Attr               A = (e.size() ? edit_Attr_cur() : edit_Attr_def());
    edit_init();

    switch( bank ) {
        case 0:
            // full lowest 192 rows
            vR.push_back( IMRO_ROI( 0, 0, 192 ) );
            vR.push_back( IMRO_ROI( 1, 0, 192 ) );
            vR.push_back( IMRO_ROI( 2, 0, 192 ) );
            vR.push_back( IMRO_ROI( 3, 0, 192 ) );
            break;
        case 1:
            // full next 192 rows
            vR.push_back( IMRO_ROI( 0, 192, 384 ) );
            vR.push_back( IMRO_ROI( 1, 192, 384 ) );
            vR.push_back( IMRO_ROI( 2, 192, 384 ) );
            vR.push_back( IMRO_ROI( 3, 192, 384 ) );
            break;
        case 2:
            // overlapped next 192 rows (keep >= 384)
            vR.push_back( IMRO_ROI( 0, 264, 456 ) );
            vR.push_back( IMRO_ROI( 1, 264, 456 ) );
            vR.push_back( IMRO_ROI( 2, 264, 456 ) );
            vR.push_back( IMRO_ROI( 3, 264, 456 ) );
            break;
        case 3:
            // top 184 rows (keep >= 456)
            vR.push_back( IMRO_ROI( 0, 456, 640 ) );
            vR.push_back( IMRO_ROI( 1, 456, 640 ) );
            vR.push_back( IMRO_ROI( 2, 456, 640 ) );
            vR.push_back( IMRO_ROI( 3, 456, 640 ) );
            // 8 mid rows (discard)
            vR.push_back( IMRO_ROI( 0, 376, 384 ) );
            vR.push_back( IMRO_ROI( 1, 376, 384 ) );
            vR.push_back( IMRO_ROI( 2, 376, 384 ) );
            vR.push_back( IMRO_ROI( 3, 376, 384 ) );
            break;
    }

    edit_ROI2tbl( vR, A );
}


// Return min row to graft into world-map, or,
// return -1 to keep all.
//
int IMROTbl_T3020base::svy_minRow( int shank, int bank ) const
{
    Q_UNUSED( shank )

    if( bank == 2 )
        return 384;
    else if( bank == 3 )
        return 456;

    return -1;
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T3020base::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T3020base *RHS    = (const IMROTbl_T3020base*)rhs;
    int                     n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].shnk != RHS->e[i].shnk || e[i].bank != RHS->e[i].bank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn shnk bank refid elec)()()...
//
QString IMROTbl_T3020base::toString() const
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
bool IMROTbl_T3020base::fromString( QString *msg, const QString &s )
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

    int nC = 0,
        nN = nAP();

    e.clear();
    e.resize( nN );

    for( int i = 1; i < n; ++i ) {

        IMRODesc_T3020base  D;
        int                 C = D.fromString( msg, sl[i] );
        if( C >= nN ) {
            if( msg ) {
                *msg = QString("Channel index <%1> exceeds %2")
                        .arg( C ).arg( nN - 1 );
            }
            return false;
        }
        else if( C >= 0 ) {
            if( blockMap[D.shnk][C/48] == 99 ) {
                if( msg ) {
                    *msg = QString("Illegal 3020 mapping for channel [%1]")
                            .arg( C );
                }
                return false;
            }
            e[C] = D;
            ++nC;
        }
        else
            return false;
    }

    if( nC != nN ) {
        if( msg ) {
            *msg = QString("Wrong imro entry count [%1] (should be %2)")
                    .arg( nC ).arg( nN );
        }
        return false;
    }

    setElecs();

    return true;
}


int IMROTbl_T3020base::maxBank( int ch, int shank ) const
{
    int maxBank = imType3020baseBanks - 1;

    if( IMRODesc_T3020base::chToEl( ch, shank, maxBank ) >= imType3020baseElPerShk )
        --maxBank;

    return maxBank;
}


int IMROTbl_T3020base::elShankAndBank( int &bank, int ch ) const
{
    const IMRODesc_T3020base    &E = e[ch];
    bank = E.bank;
    return E.shnk;
}


int IMROTbl_T3020base::elShankColRow( int &col, int &row, int ch ) const
{
    const IMRODesc_T3020base    &E = e[ch];
    int                         el = E.elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return E.shnk;
}


void IMROTbl_T3020base::eaChansOrder( QVector<int> &v ) const
{
    int _nAP    = nAP(),
        order   = 0;

    v.resize( _nAP + 1 );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < imType3020baseShanks; ++sh ) {

        QMap<int,int>   el2Ch;

        for( int ic = 0; ic < _nAP; ++ic ) {

            const IMRODesc_T3020base    &E = e[ic];

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


// refid [0]      ext, shank=0,    bank=0.
// refid [1]      gnd, shank=0,    bank=0.
// refid [2..5]   tip, shank=id-2, bank=0.
//
int IMROTbl_T3020base::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    int rid = e[ch].refid;

    bank = 0;

    if( rid == 0 ) {
        shank = (ch < 4 ? ch : 0);
        return 0;
    }
    else if( rid == 1 ) {
        shank = 0;
        return 3;
    }

    shank = rid - 2;
    return 1;
}


void IMROTbl_T3020base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T3020base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 96;
    nGrp = 16;

    T.resize( imType3020baseChan );

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

void IMROTbl_T3020base::edit_init() const
{
// forward

    for( int c = 0; c < imType3020baseChan; ++c ) {

        for( int s = 0; s < imType3020baseShanks; ++s ) {

            for( int b = 0; b < imType3020baseBanks; ++b ) {

                int e = IMRODesc_T3020base::chToEl( c, s, b );

                if( e < imType3020baseElPerShk )
                    k2s[T3020Key( c, s, b )] = IMRO_Site( s, e & 1, e >> 1 );
                else
                    break;
            }
        }
    }

// inverse

    QMap<T3020Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T3020base::edit_GUI() const
{
    IMRO_GUI    G;
    G.refs.push_back( "Ground" );
    for( int i = 0; i < imType3020baseShanks; ++i )
        G.refs.push_back( QString("Tip %1").arg( i ) );
    G.refs.push_back( "Join Tips" );
    G.gains.push_back( apGain( 0 ) );
    return G;
}


IMRO_Attr IMROTbl_T3020base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T3020base::edit_Attr_cur() const
{
    IMRO_Attr   A( refid( 0 ), 0, 0, 0 );

    if( e[0].refid == 2 && e[1].refid == 3 &&
        e[2].refid == 4 && e[3].refid == 5 ) {

        A.refIdx = 6;   // join tips
    }

    return A;
}


bool IMROTbl_T3020base::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType3020baseChan )
        return false;

    int refid = e[0].refid;

    for( int ie = 4; ie < ne; ++ie ) {
        if( e[ie].refid != refid )
            return false;
    }

    return true;
}


void IMROTbl_T3020base::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T3020Key    K = s2k[s];

    QMap<T3020Key,IMRO_Site>::const_iterator
        it  = k2s.find( T3020Key( K.c, 0, 0 ) ),
        end = k2s.end();

// For this mapping:
// If there is no (shank,bank)=(0,0) entry, then there is also no (0,1).
// Each chan maps to at least three shanks, so the next lowest is (1,0).

    if( it == end )
        it = k2s.find( T3020Key( K.c, 1, 0 ) );

    for( ; it != end; ++it ) {
        const T3020Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.s != K.s || ik.b != K.b )
            vX.push_back( k2s[ik] );
    }
}


int IMROTbl_T3020base::edit_site2Chan( const IMRO_Site &s ) const
{
    return s2k[s].c;
}


void IMROTbl_T3020base::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType3020baseChan );

    int     refIdx      = A.refIdx;
    bool    joinTips    = A.refIdx == 6;

    if( joinTips )
        refIdx = 2;

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = B.c_0(),
            cL = B.c_lim( _ncolhwr );

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T3020Key      &K = s2k[IMRO_Site( B.s, c, r )];
                IMRODesc_T3020base  &E = e[K.c];

                E.chan  = K.c;
                E.shnk  = K.s;
                E.bank  = K.b;
                E.refid = refIdx;
            }
        }
    }

    setElecs();

    if( joinTips ) {
        for( int c = 1; c < 4; ++c )
            e[c].refid = 2 + c;
    }
}


// 1536 readout channels -> 768 on two shanks.
// Set lowest 384 row on shank {0,1}.
//
void IMROTbl_T3020base::edit_defaultROI( tImroROIs vR ) const
{
    vR.clear();
    vR.push_back( IMRO_ROI( 0, 0, 384 ) );
    vR.push_back( IMRO_ROI( 1, 0, 384 ) );
}


