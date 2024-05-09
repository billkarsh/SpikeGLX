
#include "IMROTbl_T21base.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#endif

#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

static char bF[4] = {1,7,5,3};  // multiplier per bank
static char bA[4] = {0,4,8,12}; // addend per bank


int IMRODesc_T21base::lowBank( int mbank )
{
    if( mbank & 1 )
        return 0;
    else if( mbank & 2 )
        return 1;
    else if( mbank & 4 )
        return 2;

    return 3;
}


int IMRODesc_T21base::chToEl( int ch, int mbank )
{
    int     bank,
            blkIdx,
            rem,
            irow;
    bool    bRight = ch & 1;    // RHS column

    blkIdx  = ch / 32;
    rem     = (ch - 32*blkIdx - bRight) / 2;

// mbank is multibank field, we take only lowest connected bank

    bank = lowBank( mbank );

// Find irow such that its 16-modulus is rem

    for( irow = 0; irow < 16; ++irow ) {

        if( rem == (irow*bF[bank] + bRight*bA[bank]) % 16 )
            break;
    }

// Precaution against bad file input

    if( irow > 15 )
        irow = 0;

    return 384*bank + 32*blkIdx + 2*irow + bRight;
}


// Pattern: "(chn mbank refid elec)"
//
QString IMRODesc_T21base::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4)")
            .arg( chn ).arg( mbank )
            .arg( refid ).arg( elec );
}


// Pattern: "chn mbank refid elec"
//
// Note: The chn field is discarded and elec is recalculated by caller.
//
IMRODesc_T21base IMRODesc_T21base::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc_T21base(
            sl.at( 1 ).toInt(), sl.at( 2 ).toInt() );
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T21Key::operator<( const T21Key &rhs ) const
{
    if( c < rhs.c )
        return true;

    if( c > rhs.c )
        return false;

    return m < rhs.m;
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T21base::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T21base::fillDefault()
{
    e.clear();
    e.resize( imType21baseChan );
    setElecs();
}


void IMROTbl_T21base::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    for( int i = 0, n = e.size(); i < n; ++i )
        e[i].mbank = (1 << qMin( bank, maxBank( i ) ));

    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T21base::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T21base   *RHS    = (const IMROTbl_T21base*)rhs;
    int                     n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].mbank != RHS->e[i].mbank )
            return false;
    }

    return true;
}


// Pattern: (type,nchan)(chn mbank refid elec)()()...
//
QString IMROTbl_T21base::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << type << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString( i );

    return s;
}


// Pattern: (type,nchan)(chn mbank refid elec)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T21base::fromString( QString *msg, const QString &s )
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
        e.push_back( IMRODesc_T21base::fromString( sl[i] ) );

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


int IMROTbl_T21base::elShankAndBank( int &bank, int ch ) const
{
    bank = e[ch].mbank;
    return 0;
}


int IMROTbl_T21base::elShankColRow( int &col, int &row, int ch ) const
{
    int el = e[ch].elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return 0;
}


void IMROTbl_T21base::eaChansOrder( QVector<int> &v ) const
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


void IMROTbl_T21base::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 2; rout = 8; break;
        default:    rin = 0; rout = 2; break;
    }
}


void IMROTbl_T21base::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 24;
    nGrp = 16;

    T.resize( imType21baseChan );

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
/* Hardware ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This method connects multiple electrodes per channel.
//
int IMROTbl_T21base::selectSites( int slot, int port, int dock, bool write ) const
{
#ifdef HAVE_IMEC
// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        if( chIsRef( ic ) )
            continue;

        int             shank, bank;
        NP_ErrorCode    err;

        shank = elShankAndBank( bank, ic );

        err = np_selectElectrodeMask( slot, port, dock, ic,
                shank, electrodebanks_t(bank) );

        if( err != SUCCESS )
            return err;
    }

    if( write )
        np_writeProbeConfiguration( slot, port, dock, true );
#endif

    return 0;
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T21base::edit_init() const
{
// forward

    for( int c = 0; c < imType21baseChan; ++c ) {

        for( int b = 0; b < imType21baseBanks; ++b ) {

            int e = IMRODesc_T21base::chToEl( c, 1 << b );

            if( e < imType21baseElec )
                k2s[T21Key( c, 1 << b )] = IMRO_Site( 0, e & 1, e >> 1 );
            else
                break;
        }
    }

// inverse

    QMap<T21Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T21base::edit_GUI() const
{
    IMRO_GUI    G;
    if( tip0refID() == 2 )
        G.refs.push_back( "Ground" );
    G.refs.push_back( "Tip" );
    G.gains.push_back( apGain( 0 ) );
    G.grid = 16;    // prevents editing fragmentation
    return G;
}


IMRO_Attr IMROTbl_T21base::edit_Attr_def() const
{
    return IMRO_Attr( 0, 0, 0, 0 );
}


IMRO_Attr IMROTbl_T21base::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), 0, 0, 0 );
}


bool IMROTbl_T21base::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType21baseChan )
        return false;

    int refid = e[0].refid;

    for( int ie = 1; ie < ne; ++ie ) {
        if( e[ie].refid != refid )
            return false;
    }

    return true;
}


void IMROTbl_T21base::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    T21Key  K = s2k[s];

    QMap<T21Key,IMRO_Site>::const_iterator
        it  = k2s.find( T21Key( K.c, 1 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T21Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.m != K.m )
            vX.push_back( k2s[ik] );
    }
}


void IMROTbl_T21base::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType21baseChan );

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = qMax( 0, B.c0 ),
            cL = (B.cLim >= 0 ? B.cLim : _ncolhwr);

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T21Key        &K = s2k[IMRO_Site( 0, c, r )];
                IMRODesc_T21base    &E = e[K.c];

                E.mbank = K.m;
                E.refid = A.refIdx;
            }
        }
    }

    setElecs();
}


