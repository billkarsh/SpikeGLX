
#include "IMROTbl_T2022.h"
#include "Util.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#endif

#include <QIODevice>
#include <QRegularExpression>
#include <QTextStream>
#include <QThread>

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

static char bF[4] = {1,7,5,3};  // multiplier per bank
static char bA[4] = {0,4,8,12}; // addend per bank


int IMRODesc_T2022::lowBank( int mbank )
{
    if( mbank & 1 )
        return 0;
    else if( mbank & 2 )
        return 1;
    else if( mbank & 4 )
        return 2;

    return 3;
}


int IMRODesc_T2022::chToEl( int ch, int mbank )
{
    ch %= 384;

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


// Pattern: "(chn shank mbank refid elec)"
//
QString IMRODesc_T2022::toString( int chn ) const
{
    return QString("(%1 %2 %3 %4 %5)")
            .arg( chn ).arg( shnk )
            .arg( mbank ).arg( refid )
            .arg( elec );
}


// Pattern: "chn shnk mbank refid elec"
//
// Note: chn (or -1) is returned; elec is recalculated by caller.
//
int IMRODesc_T2022::fromString( QString *msg, const QString &s )
{
    const QStringList   sl = s.split(
                                QRegularExpression("\\s+"),
                                Qt::SkipEmptyParts );
    int                 chn;
    bool                ok;

    if( sl.size() != 5 )
        goto fail;

    chn     = sl.at( 0 ).toInt( &ok ); if( !ok ) goto fail;
    shnk    = sl.at( 1 ).toInt( &ok ); if( !ok ) goto fail;
    mbank   = sl.at( 2 ).toInt( &ok ); if( !ok ) goto fail;
    refid   = sl.at( 3 ).toInt( &ok ); if( !ok ) goto fail;

    return chn;

fail:
    if( msg ) {
        *msg =
        QString("Bad IMRO element format (%1), expected (chn shnk mbank refid elec)")
        .arg( s );
    }

    return -1;
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T2022Key::operator<( const T2022Key &rhs ) const
{
    if( c < rhs.c )
        return true;
    if( c > rhs.c )
        return false;
    if( s < rhs.s )
        return true;
    if( s > rhs.s )
        return false;
    return m < rhs.m;
}

/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T2022::setElecs()
{
    for( int i = 0, n = nChan(); i < n; ++i )
        e[i].setElec( i );
}


void IMROTbl_T2022::fillDefault()
{
    e.clear();
    e.resize( imType2020baseChan );

// Set shnk fields

    for( int is = 1; is < imType2020baseShanks; ++is ) {

        int ic0 = is * imType2020baseChPerShk;

        for( int ic = 0; ic < imType2020baseChPerShk; ++ic )
            e[ic0 + ic].shnk = is;
    }

    setElecs();
}


// All shanks reflect given bank
//
void IMROTbl_T2022::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    for( int i = 0, n = e.size(); i < n; ++i ) {
        IMRODesc_T2022  &E = e[i];
        E.mbank = (1 << qMin( bank, maxBank( i ) ));
    }

    setElecs();
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T2022::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T2022 *RHS    = (const IMROTbl_T2022*)rhs;
    int                 n       = nChan();

    for( int i = 0; i < n; ++i ) {

        if( e[i].shnk != RHS->e[i].shnk || e[i].mbank != RHS->e[i].mbank )
            return false;
    }

    return true;
}


// Pattern: (pn,nchan)(chn shnk mbank refid elec)()()...
//
QString IMROTbl_T2022::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = nChan();

    ts << "(" << pn << "," << n << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString( i );

    return s;
}


// Pattern: (pn,nchan)(chn shnk mbank refid elec)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T2022::fromString( QString *msg, const QString &s )
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
            *msg = "Wrong imro header format [should be (pn,nchan)]";
        return false;
    }

    bool    type_ok;

    if( hl[0].toInt( &type_ok ) == typeConst() && type_ok )
        ;
    else {
        int type;
        type_ok = pnToType( type, hl[0].trimmed() );

        if( !type_ok || type != typeConst() ) {
            if( msg ) {
                *msg =
                QString("Wrong imro header id[%1] for probe pn[%2]")
                .arg( hl[0].trimmed() ).arg( pn );
            }
            return false;
        }
    }

// Entries

    int nC = 0,
        nN = nAP();

    e.clear();
    e.resize( nN );

    for( int i = 1; i < n; ++i ) {
        IMRODesc_T2022  D;
        int             C = D.fromString( msg, sl[i] );
        if( C >= nN ) {
            if( msg ) {
                *msg = QString("Channel index <%1> exceeds %2")
                        .arg( C ).arg( nN - 1 );
            }
            return false;
        }
        else if( C >= 0 ) {
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


int IMROTbl_T2022::elShankAndBank( int &bank, int ch ) const
{
    const IMRODesc_T2022    &E = e[ch];
    bank = E.mbank;
    return E.shnk;
}


int IMROTbl_T2022::elShankColRow( int &col, int &row, int ch ) const
{
    const IMRODesc_T2022    &E = e[ch];
    int                     el = E.elec;

    row = el / _ncolhwr;
    col = el - _ncolhwr * row;

    return E.shnk;
}


void IMROTbl_T2022::eaChansOrder( QVector<int> &v ) const
{
    int _nAP    = nAP(),
        _nSY    = nSY(),
        order   = 0;

    v.resize( _nAP + _nSY );

// Order the AP set

// Major order is by shank.
// Minor order is by electrode.

    for( int sh = 0; sh < imType2020baseShanks; ++sh ) {

        QMap<int,int>   el2Ch;

        for( int ic = 0; ic < _nAP; ++ic ) {

            const IMRODesc_T2022    &E = e[ic];

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


// refid [0]    ext, shank=s, bank=0.
// refid [1]    gnd, shank=s, bank=0.
// refid [2]    tip, shank=s, bank=0.
//
int IMROTbl_T2022::refTypeAndFields( int &shank, int &bank, int ch ) const
{
    const IMRODesc_T2022    &E = e[ch];

    shank = E.shnk;
    bank  = 0;

    if( E.refid == 0 )
        return 0;
    else if( E.refid == 1 )
        return 3;

    return 1;
}

/* ---------------------------------------------------------------- */
/* Hardware ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This method connects multiple electrodes per channel.
//
int IMROTbl_T2022::selectSites4( const PAddr& adr, bool write, bool check ) const
{
#ifdef HAVE_IMEC
// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    NP_ErrorCode    err;

    for( int ic = 0, nC = nChan(); ic < nC; ++ic ) {

        int bank, shank = elShankAndBank( bank, ic );

        err = np_selectElectrodeMask( adr.slot, adr.port, adr.dock, ic,
                shank, electrodebanks_t(bank) );

        if( err != SUCCESS )
            return err;
    }

    if( write ) {

        for( int itry = 1; itry <= 10; ++itry ) {

            err = np_writeProbeConfiguration(
                    adr.slot, adr.port, adr.dock, check );

            if( err == SUCCESS ) {
                if( itry > 1 ) {
                    Warning() <<
                    QString("Probe(%1): writeConfig() took %2 tries.")
                    .arg( adr.tx_spd() ).arg( itry );
                }
                break;
            }

            QThread::msleep( 100 );
        }

        return err;
    }
#else
    Q_UNUSED( adr )
    Q_UNUSED( write )
    Q_UNUSED( check )
#endif

    return 0;
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T2022::edit_init() const
{
// forward

    for( int c = 0; c < imType2020baseChan; ++c ) {

        int s = c / imType2020baseChPerShk;

        for( int b = 0; b < imType2020baseBanks; ++b ) {

            int e = IMRODesc_T2022::chToEl( c, 1 << b );

            if( e < imType2020baseElPerShk )
                k2s[T2022Key( c, s, 1 << b )] = IMRO_Site( s, e & 1, e >> 1 );
            else
                break;
        }
    }

// inverse

    QMap<T2022Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


bool IMROTbl_T2022::edit_Attr_canonical() const
{
    int ne = e.size();

    if( ne != imType2020baseChan )
        return false;

    int refid = e[0].refid;

    for( int ie = 1; ie < ne; ++ie ) {
        if( e[ie].refid != refid )
            return false;
    }

    return true;
}


void IMROTbl_T2022::edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
{
    const T2022Key  K = s2k[s];

    QMap<T2022Key,IMRO_Site>::const_iterator
        it  = k2s.constFind( T2022Key( K.c, K.s, 1 ) ),
        end = k2s.constEnd();

    for( ; it != end; ++it ) {
        const T2022Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.m != K.m )
            vX.push_back( k2s[ik] );
    }
}


int IMROTbl_T2022::edit_site2Chan( const IMRO_Site &s ) const
{
    return s2k[s].c;
}


void IMROTbl_T2022::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    e.clear();
    e.resize( imType2020baseChan );

    for( int ib = 0, nb = (int)vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        int c0 = B.c_0(),
            cL = B.c_lim( _ncolhwr );

        for( int r = B.r0; r < B.rLim; ++r ) {

            for( int c = c0; c < cL; ++c ) {

                const T2022Key  &K = s2k[IMRO_Site( B.s, c, r )];
                IMRODesc_T2022  &E = e[K.c];

                E.shnk  = K.s;
                E.mbank = K.m;
                E.refid = A.refIdx;
            }
        }
    }

    setElecs();
}


