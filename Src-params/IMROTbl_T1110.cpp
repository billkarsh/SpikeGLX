
#include "IMROTbl_T1110.h"
#include "Util.h"

#ifdef HAVE_IMEC
#include "IMEC/NeuropixAPI.h"
using namespace Neuropixels;
#endif

#include <QFileInfo>
#include <QStringList>
#include <QRegExp>
#include <QTextStream>

/* ---------------------------------------------------------------- */
/* struct IMROHdr ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "(type,colmode,refid,apgn,lfgn,apflt)"
//
QString IMROHdr_T1110::toString( int type ) const
{
    return QString("(%1,%2,%3,%4,%5,%6)")
            .arg( type ).arg( colmode ).arg( refid )
            .arg( apgn ).arg( lfgn ).arg( apflt );
}

/* ---------------------------------------------------------------- */
/* struct IMRODesc ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Pattern: "(grp bankA bankB)"
//
QString IMRODesc_T1110::toString( int grp ) const
{
    return QString("(%1 %2 %3)").arg( grp ).arg( bankA ).arg( bankB );
}


// Pattern: "grp bankA bankB"
//
// Note: The grp field is discarded.
//
IMRODesc_T1110 IMRODesc_T1110::fromString( const QString &s )
{
    const QStringList   sl = s.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return IMRODesc_T1110( sl.at( 1 ).toInt(), sl.at( 2 ).toInt() );
}

/* ---------------------------------------------------------------- */
/* struct Key ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool T1110Key::operator<( const T1110Key &rhs ) const
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

void IMROTbl_T1110::fillDefault()
{
    type = imType1110Type;
    ehdr = IMROHdr_T1110();
    e.clear();
    e.resize( imType1110Groups );
}


void IMROTbl_T1110::fillShankAndBank( int shank, int bank )
{
    Q_UNUSED( shank )

    ehdr.colmode = 2;

    for( int i = 0, n = e.size(); i < n; ++i ) {
        IMRODesc_T1110  &E = e[i];
        E.bankA = bank;
        E.bankB = bank;
    }
}


// Return true if two tables are same w.r.t connectivity.
//
bool IMROTbl_T1110::isConnectedSame( const IMROTbl *rhs ) const
{
    const IMROTbl_T1110 *RHS    = (const IMROTbl_T1110*)rhs;

    return ehdr.colmode == RHS->ehdr.colmode && e == RHS->e;
}


// Pattern: (type,colmode,refid,apgn,lfgn,apflt)(grp bankA bankB)()()...
//
QString IMROTbl_T1110::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );

    ts << ehdr.toString( type );

    for( int ig = 0; ig < imType1110Groups; ++ig )
        ts << e[ig].toString( ig );

    return s;
}


// Pattern: (type,colmode,refid,apgn,lfgn,apflt)(grp bankA bankB)()()...
//
// Return true if file type compatible.
//
bool IMROTbl_T1110::fromString( QString *msg, const QString &s )
{
    QStringList sl = s.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    if( hl.size() != 6 ) {
        type = -3;      // 3A type
        if( msg )
            *msg = "Wrong imro header size (should be 6)";
        return false;
    }

    type = hl[0].toInt();

    if( type != imType1110Type ) {
        if( msg ) {
            *msg = QString("Wrong imro type[%1] for probe type[%2]")
                    .arg( type ).arg( imType1110Type );
        }
        return false;
    }

    ehdr = IMROHdr_T1110(
            hl[1].toInt(), hl[2].toInt(), hl[3].toInt(),
            hl[4].toInt(), hl[5].toInt() );

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i ) {

        IMRODesc_T1110  E = IMRODesc_T1110::fromString( sl[i] );

        if( ehdr.colmode == 2 ) {
            if( E.bankA != E.bankB ) {
                if( msg )
                    *msg = "In 'ALL' col mode bankA must equal bankB.";
                return false;
            }
        }
        else {
            bool aColCrossed = (E.bankA/4) & 1,
                 bColCrossed = (E.bankB/4) & 1;

            if( aColCrossed == bColCrossed ) {
                if( msg ) {
                    *msg =  "In 'INNER' (or 'OUTER') col mode, one bank"
                            " must be col-crossed and the other not.";
                }
                return false;
            }
        }

        e.push_back( E );
    }

    if( e.size() != imType1110Groups ) {
        if( msg ) {
            *msg = QString("Wrong imro entry count [%1] (should be %2)")
                    .arg( e.size() ).arg( imType1110Groups );
        }
        return false;
    }

    return true;
}


bool IMROTbl_T1110::loadFile( QString &msg, const QString &path )
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


bool IMROTbl_T1110::saveFile( QString &msg, const QString &path ) const
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


// In ALL mode bankA and bankB are the same.
// In OUTER mode upper cols are even and lower cols are odd.
// In INNER mode upper cols are odd  and lower cols are even.
//
int IMROTbl_T1110::bank( int ch ) const
{
    const IMRODesc_T1110    &E = e[grpIdx( ch )];

    if( ehdr.colmode == 2 )         // ALL
        return E.bankA;
    else {

        int col = this->col( ch, E.bankA );

        if( ehdr.colmode == 1 ) {  // OUTER

            if( col <= 3 ) {
                if( !(col & 1) )
                    return E.bankA;
            }
            else if( col & 1 )
                return E.bankA;
        }
        else {                      // INNER

            if( col <= 3 ) {
                if( col & 1 )
                    return E.bankA;
            }
            else if( !(col & 1) )
                return E.bankA;
        }
    }

    return E.bankB;
}


int IMROTbl_T1110::elShankAndBank( int &bank, int ch ) const
{
    bank = this->bank( ch );
    return 0;
}


int IMROTbl_T1110::elShankColRow( int &col, int &row, int ch ) const
{
    int bank = this->bank( ch );

    col = this->col( ch, bank );
    row = this->row( ch, bank );

    return 0;
}


void IMROTbl_T1110::eaChansOrder( QVector<int> &v ) const
{
    QMap<int,int>   el2Ch;
    int             order   = 0,
                    _nAP    = nAP();

    v.resize( 2 * _nAP + 1 );

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


// refid [0]    ext, shank=0, bank=0.
// refid [1]    tip, shank=0, bank=0.
//
int IMROTbl_T1110::refTypeAndFields( int &shank, int &bank, int /* ch */ ) const
{
    shank   = 0;
    bank    = 0;
    return ehdr.refid;
}


// Depends only upon channel, not crossings, not switches.
// Pattern repeats each bank of 384 channels.
// Each pair of groups (0,1) (2,3) ... contains 32 contiguous channels.
// All channels in {even/odd} groups are {even/odd}.
//
int IMROTbl_T1110::grpIdx( int ch ) const
{
    ch %= 384;

    return 2 * (ch / 32) + (ch & 1);
}


int IMROTbl_T1110::col( int ch, int bank ) const
{
    int col_tbl[8]  = {0,3,1,2,  1,2,0,3},
        grpIdx      = this->grpIdx( ch ),
        grp_col     = col_tbl[4*(bank & 1) + (grpIdx % 4)],
        crossed     = (bank / 4) & 1,
        ingrp_col   = ((((ch % 64) % 32) / 2) & 1) ^ crossed;   // ch-even

    return 2*grp_col + (ch & 1 ? 1 - ingrp_col : ingrp_col);
}


int IMROTbl_T1110::row( int ch, int bank ) const
{
// Row within bank-0:

    int grpIdx      = this->grpIdx( ch ),
        grp_row     = grpIdx / 4,
        ingrp_row   = ((ch % 64) % 32) / 4, // ch-even
        b0_row      = 8*grp_row + (ch & 1 ? 7 - ingrp_row : ingrp_row);

// Rows per bank = 384 / 8 = 48

    return 48*bank + b0_row;
}


// Our idea of electrode index runs consecutively, and
// faster across row. Bottom row has electrodes {0..7}.
// Our electrodes have simple row and column ordering.
// That differs somewhat from convention in imec docs,
// but is easier to understand and the only purpose is
// sorting chans tip to base. Electrode index is a
// virtual intermediary between channel and (row,col).
//
int IMROTbl_T1110::chToEl( int ch ) const
{
    int bank = this->bank( ch );

    return 8*row( ch, bank ) + col( ch, bank );
}


int IMROTbl_T1110::chToEl( int ch, int bank ) const
{
    return 8*row( ch, bank ) + col( ch, bank );
}


static int i2gn[IMROTbl_T1110::imType1110Gains]
            = {50,125,250,500,1000,1500,2000,3000};


int IMROTbl_T1110::idxToGain( int idx ) const
{
    return (idx >= 0 && idx < 8 ? i2gn[idx] : i2gn[3]);
}


int IMROTbl_T1110::gainToIdx( int gain ) const
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


void IMROTbl_T1110::locFltRadii( int &rin, int &rout, int iflt ) const
{
    switch( iflt ) {
        case 2:     rin = 8, rout = 32; break;
        default:    rin = 4, rout = 8;  break;
    }
}


void IMROTbl_T1110::muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const
{
    nADC = 32;
    nGrp = 12;

    T.resize( imType1110Chan );

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

int IMROTbl_T1110::selectSites( int slot, int port, int dock, bool write ) const
{
#ifdef HAVE_IMEC

    NP_ErrorCode    err;

// ---------------
// Set column mode
// ---------------

    err = np_selectColumnPattern( slot, port, dock,
            columnpattern_t(ehdr.colmode) );

    if( err != SUCCESS )
        return err;

// ------------------------------------
// Connect all according to table banks
// ------------------------------------

    for( int ig = 0; ig < imType1110Groups; ++ig ) {

        const IMRODesc_T1110    E = e[ig];

        if( ehdr.colmode == ALL )
            err = np_selectElectrodeGroup( slot, port, dock, ig, E.bankA );
        else {
            err = np_selectElectrodeGroupMask( slot, port, dock, ig,
                    electrodebanks_t((1<<E.bankA) + (1<<E.bankB)) );
        }

        if( err != SUCCESS )
            return err;
    }

    if( write )
        np_writeProbeConfiguration( slot, port, dock, true );
#endif

    return 0;
}


int IMROTbl_T1110::selectRefs( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// -----------------------------
// Connect all according to ehdr
// -----------------------------

    for( int ic = 0; ic < imType1110Chan; ++ic ) {

        NP_ErrorCode    err;

        err = np_setReference( slot, port, dock, ic,
                0, channelreference_t(ehdr.refid), 0 );

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}

#if 1
// True gain setter
int IMROTbl_T1110::selectGains( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// -------------------------
// Set all according to ehdr
// -------------------------

    for( int ic = 0; ic < imType1110Chan; ++ic ) {

        NP_ErrorCode    err;

        err = np_setGain( slot, port, dock, ic,
                gainToIdx( ehdr.apgn ),
                gainToIdx( ehdr.lfgn ) );

//---------------------------------------------------------
// Experiment to visualize LF scambling on shankviewer by
// setting every nth gain high and others low.
#if 0
        int apidx, lfidx;

        if( !(ic % 10) ) {
            apidx = R->gainToIdx( 3000 );
            lfidx = R->gainToIdx( 3000 );
        }
        else {
            apidx = R->gainToIdx( 50 );
            lfidx = R->gainToIdx( 50 );
        }

        err = np_setGain( P.slot, P.port, P.dock, ic,
                apidx,
                lfidx );
#endif
//---------------------------------------------------------

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}
#endif


#if 0
// Experiment setting gain by row or col
int IMROTbl_T1110::selectGains( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC

    for( int ic = 0; ic < imType1110Chan; ++ic ) {

        NP_ErrorCode    err;

        int idx = this->col( ic, 0 );

        err = np_setGain( slot, port, dock, ic, idx, idx );

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}
#endif


int IMROTbl_T1110::selectAPFlts( int slot, int port, int dock ) const
{
#ifdef HAVE_IMEC
// -------------------------
// Set all according to ehdr
// -------------------------

    for( int ic = 0; ic < imType1110Chan; ++ic ) {

        NP_ErrorCode    err;

        err = np_setAPCornerFrequency( slot, port, dock, ic, !ehdr.apflt );

        if( err != SUCCESS )
            return err;
    }
#endif

    return 0;
}

/* ---------------------------------------------------------------- */
/* Edit ----------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROTbl_T1110::edit_init() const
{
// forward

    for( int c = 0; c < imType1110Chan; ++c ) {

        for( int b = 0; b < imType1110Banks; ++b ) {

            int e = chToEl( c, b );

            k2s[T1110Key( c, b )] =
                IMRO_Site( 0, e % imType1110Col, e / imType1110Col );
        }
    }

// inverse

    QMap<T1110Key,IMRO_Site>::iterator
        it  = k2s.begin(),
        end = k2s.end();

    for( ; it != end; ++it )
        s2k[it.value()] = it.key();
}


IMRO_GUI IMROTbl_T1110::edit_GUI() const
{
    IMRO_GUI    G;
    G.gains.push_back( 50 );
    G.gains.push_back( 125 );
    G.gains.push_back( 250 );
    G.gains.push_back( 500 );
    G.gains.push_back( 1000 );
    G.gains.push_back( 1500 );
    G.gains.push_back( 2000 );
    G.gains.push_back( 3000 );
    G.grid      = 8;
    G.apEnab    = true;
    G.lfEnab    = true;
    G.hpEnab    = true;
    return G;
}


IMRO_Attr IMROTbl_T1110::edit_Attr_def() const
{
    return IMRO_Attr( 0, 3, 2, 1 );
}


IMRO_Attr IMROTbl_T1110::edit_Attr_cur() const
{
    return IMRO_Attr( refid( 0 ), gainToIdx( apGain( 0 ) ),
                    gainToIdx( lfGain( 0 ) ), apFlt( 0 ) );
}


void IMROTbl_T1110::edit_exclude_1( tImroSites vS, const IMRO_Site &s ) const
{
    T1110Key    K = s2k[s];

    QMap<T1110Key,IMRO_Site>::const_iterator
        it  = k2s.find( T1110Key( K.c, 0 ) ),
        end = k2s.end();

    for( ; it != end; ++it ) {
        const T1110Key  &ik = it.key();
        if( ik.c != K.c )
            break;
        if( ik.b != K.b )
            vS.push_back( k2s[ik] );
    }
}


void IMROTbl_T1110::edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
{
    ehdr.apgn       = idxToGain( A.apgIdx );
    ehdr.lfgn       = idxToGain( A.lfgIdx );
    ehdr.colmode    = 2;
    ehdr.refid      = A.refIdx;
    ehdr.apflt      = A.hpfIdx;

    e.clear();
    e.resize( imType1110Groups );

    for( int ib = 0, nb = vR.size(); ib < nb; ++ib ) {

        const IMRO_ROI  &B = vR[ib];

        for( int r = B.r0; r < B.rLim; ++r ) {

            for(
                int c = qMax( 0, B.c0 ),
                cLim  = (B.cLim < 0 ? imType1110Col : B.cLim);
                c < cLim;
                ++c ) {

                const T1110Key  &K = s2k[IMRO_Site( 0, c, r )];
                IMRODesc_T1110  &E = e[grpIdx( K.c )];

                E.bankA = K.b;
                E.bankB = K.b;
            }
        }
    }
}


