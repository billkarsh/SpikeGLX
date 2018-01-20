
#include "Subset.h"

#include <QStringList>
#include <QTextStream>


/* ---------------------------------------------------------------- */
/* bits2Vec ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Resulting vector has canonical form:
// - ascending order,
// - unique elements.
//
void Subset::bits2Vec( QVector<uint> &v, const QBitArray &b )
{
    int nb = b.size();

    v.clear();

    for( int i = 0; i < nb; ++i ) {

        if( b.testBit( i ) )
            v.push_back( i );
    }
}

/* ---------------------------------------------------------------- */
/* vec2Bits ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Input vector need not have canonical form.
//
void Subset::vec2Bits( QBitArray &b, const QVector<uint> &v )
{
    int sz = v.size();

    b.clear();

    if( !sz )
        return;

    sz = v[sz - 1] + 1;

    b.resize( sz );

    foreach( int i, v ) {

        if( i >= sz )
            b.resize( sz = i + 1 );

        b.setBit( i );
    }
}

/* ---------------------------------------------------------------- */
/* canonVec ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Resulting vector has canonical form:
// - ascending order,
// - unique elements.
//
void Subset::canonVec( QVector<uint> &vo, const QVector<uint> &vi )
{
    QBitArray   b;

    vec2Bits( b, vi );
    bits2Vec( vo, b );
}

/* ---------------------------------------------------------------- */
/* defaultBits ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Resulting bits name all (nChan) channels.
//
void Subset::defaultBits( QBitArray &b, int nChans )
{
    b.fill( true, nChans );
}

/* ---------------------------------------------------------------- */
/* defaultVec ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Resulting vector names all (nChan) channels.
//
void Subset::defaultVec( QVector<uint> &v, int nChans )
{
    v.resize( nChans );

    for( int i = 0; i < nChans; ++i )
        v[i] = i;
}

/* ---------------------------------------------------------------- */
/* isAllChansStr -------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if trimmed string is some variant of
// the "all channels" string, that is:
// - is empty
// - is "*"
// - is "all" (any case)
//
bool Subset::isAllChansStr( const QString &s )
{
    QString t = s.trimmed();

    return  t.isEmpty()
            || t == "*"
            || !t.compare( "all", Qt::CaseInsensitive );
}

/* ---------------------------------------------------------------- */
/* cmdStr2Bits ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// If isAllChansStr( s ) test returns true, then chanBits
// is set identically to parameter allBits. In this case,
// params {s, nTotalBits} are not used.
//
// (Note that allBits pattern is up to the caller. It might
// represent all acquired channels or just saved channels).
//
// Alternatively, use CmdServer string (s), with pattern:
// "id1#id2#..." to set bits in array of size (nTotalBits).
//
// Return format error message, or null string.
//
QString Subset::cmdStr2Bits(
    QBitArray       &chanBits,
    const QBitArray &allBits,
    const QString   &s,
    int             nTotalBits )
{
    if( isAllChansStr( s ) )
        chanBits = allBits;
    else {

        chanBits.fill( false, nTotalBits );

        QStringList CL = s.split( '#', QString::SkipEmptyParts );

        if( !CL.size() )
            return "Bad channel string format.";

        foreach( const QString &cs, CL ) {

            int c = cs.toInt();

            if( c < 0 || c >= nTotalBits ) {
                return QString("Channel %1 out of range [0..%2].")
                        .arg( c )
                        .arg( nTotalBits - 1 );
            }

            chanBits.setBit( c );
        }
    }

    return QString::null;
}

/* ---------------------------------------------------------------- */
/* bits2Str ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Format: (count) 1 1 1 0 0 1 ...
//
QString Subset::bits2Str( const QBitArray &b )
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         nb = b.size();

    ts << "(" << nb << ")";

    for( int ib = 0; ib < nb; ++ib )
        ts << (b.testBit( ib ) ? " 1" : " 0");

    return s;
}

/* ---------------------------------------------------------------- */
/* bits2RngStr ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given standard QBitArray, return a compact string
// representation wherein contiguous runs are encoded
// as follows: "0:1,3:21,30:60".
//
QString Subset::bits2RngStr( const QBitArray &b )
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         nb = b.size(),
                ib;
    bool        rn = false;

    for( ib = 0; ib < nb; ++ib ) {

        if( b.testBit( ib ) )
            goto got_first;
    }

    return "";

got_first:
    ts << ib;

    for( ++ib; ib < nb; ++ib ) {

        if( !b.testBit( ib ) ) {

            if( rn ) {
                ts << ":" << ib-1;
                rn = false;
            }
        }
        else {

            if( !b.testBit( ib-1 ) )
                ts << "," << ib;
            else
                rn = true;
        }
    }

    if( rn )
        ts << ":" << nb-1;

    return s;
}

/* ---------------------------------------------------------------- */
/* vec2RngStr ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given a vector of channels, return a compact string
// representation wherein contiguous runs are encoded
// as follows: "0:1,3:21,30:60".
//
QString Subset::vec2RngStr( const QVector<uint> &v )
{
    QBitArray   b;

    vec2Bits( b, v );
    return bits2RngStr( b );
}

/* ---------------------------------------------------------------- */
/* rngStr2Bits ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Parse range string, nominally of form "0:1,3:21,30:60",
// producing QBitArray equivalent.
//
// Return true if interpretable, false otherwise.
//
bool Subset::rngStr2Bits( QBitArray &b, const QString &s )
{
    b.clear();

    int         sz = 0;
    QStringList terms = s.split(
                            QRegExp("[,;]"),
                            QString::SkipEmptyParts );

    foreach( const QString &t, terms ) {

        QStringList rng = t.split(
                            QRegExp("[:-]"),
                            QString::SkipEmptyParts );
        int         n   = rng.count(),
                    r1, r2, sw;
        bool        ok1, ok2;

        if( n > 2 )
            return false;

        if( n == 2 ) {

            r1  = rng[0].toUInt( &ok1 ),
            r2  = rng[1].toUInt( &ok2 );

            if( !ok1 || !ok2 )
                return false;

            if( r2 < r1 ) {
                sw = r1;
                r1 = r2;
                r2 = sw;
            }

            if( r2 >= sz )
                b.resize( sz = r2 + 1 );

            for( int i = r1; i <= r2; ++i )
                b.setBit( i );
        }
        else if( n == 1 ) {

            int r1 = rng[0].toUInt( &ok1 );

            if( !ok1 )
                return false;

            if( r1 >= sz )
                b.resize( sz = r1 + 1 );

            b.setBit( r1 );
        }
    }

    return true;
}

/* ---------------------------------------------------------------- */
/* rngStr2Vec ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Parse range string, nominally of form "0:1,3:21,30:60",
// producing canonical vector equivalent.
//
// Return true if interpretable, false otherwise.
//
bool Subset::rngStr2Vec( QVector<uint> &v, const QString &s )
{
    v.clear();

    QBitArray   b;

    if( rngStr2Bits( b, s ) ) {

        bits2Vec( v, b );
        return true;
    }

    return false;
}

/* ---------------------------------------------------------------- */
/* subset --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given (nchans) src channels per timepoint, create
// dst vector keeping only listed indices (iKeep[]).
//
// Caller must set iKeep[] appropriately.
//
// In-place operation (dst == src) is allowed.
//
void Subset::subset(
    vec_i16             &dst,
    vec_i16             &src,
    const QVector<uint> &iKeep,
    int                 nchans )
{
    int nk = iKeep.size();

    if( nk >= nchans ) {

        if( &dst != &src )
            dst = src;

        return;
    }

    int ntpts = (int)src.size() / nchans;

    if( &dst != &src )
        dst.resize( ntpts * nk );

    const uint  *K = &iKeep[0];
    qint16      *D = &dst[0],
                *S = &src[0];

    for( int it = 0; it < ntpts; ++it, S += nchans ) {

        for( int ik = 0; ik < nk; ++ik )
            *D++ = S[K[ik]];
    }

    if( &dst == &src )
        dst.resize( ntpts * nk );
}

/* ---------------------------------------------------------------- */
/* subsetBlock ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Given (nchans) src channels per timepoint, create
// dst vector keeping only contiguous subset [c0,cLim).
//
// In-place operation (dst == src) is allowed.
//
void Subset::subsetBlock(
    vec_i16             &dst,
    vec_i16             &src,
    int                 c0,
    int                 cLim,
    int                 nchans )
{
    int nk = cLim - c0;

    if( nk >= nchans ) {

        if( &dst != &src )
            dst = src;

        return;
    }

    int ntpts   = (int)src.size() / nchans,
        ncpy    = nk * sizeof(qint16);

    if( &dst != &src )
        dst.resize( ntpts * nk );

    qint16  *D = &dst[0],
            *S = &src[c0];

    for( int it = 0; it < ntpts; ++it, D += nk, S += nchans )
        memcpy( D, S, ncpy );

    if( &dst == &src )
        dst.resize( ntpts * nk );
}

/* ---------------------------------------------------------------- */
/* downsample ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

// All src channels are downsampled/averaged.
//
// In-place operation (dst == src) is allowed.
//
// Return count of resulting dst timepoints.
//
uint Subset::downsample(
    vec_i16         &dst,
    vec_i16         &src,
    int             nchans,
    int             dnsmp )
{
    int ntpts = (int)src.size() / nchans;

    if( dnsmp <= 1 ) {

        if( &dst != &src )
            dst = src;

        return ntpts;
    }

    uint    dtpts = (ntpts + dnsmp - 1) / dnsmp;

    if( &dst != &src )
        dst.resize( dtpts * nchans );

    qint16              *D = &dst[0],
                        *S = &src[0];
    std::vector<double> sum( nchans );

    for( int it = 0; it < ntpts; it += dnsmp, D += nchans ) {

        int ns = std::min( ntpts - it, dnsmp );

        memset( &sum[0], 0, nchans*sizeof(double) );

        for( int is = 0; is < ns; ++is, S += nchans ) {

            for( int ic = 0; ic < nchans; ++ic )
                sum[ic] += S[ic];
        }

        for( int ic = 0; ic < nchans; ++ic )
            D[ic] = qint16(sum[ic] / ns);
    }

    if( &dst == &src )
        dst.resize( dtpts * nchans );

    return dtpts;
}

/* ---------------------------------------------------------------- */
/* downsampleNeural ----------------------------------------------- */
/* ---------------------------------------------------------------- */

// All src channels are downsampled by taking largest amplitude
// value from each bin.
//
// In-place operation (dst == src) is allowed.
//
// Return count of resulting dst timepoints.
//
uint Subset::downsampleNeural(
    vec_i16         &dst,
    vec_i16         &src,
    int             nchans,
    int             dnsmp )
{
    int ntpts = (int)src.size() / nchans;

    if( dnsmp <= 1 ) {

        if( &dst != &src )
            dst = src;

        return ntpts;
    }

    uint    dtpts = (ntpts + dnsmp - 1) / dnsmp;

    if( &dst != &src )
        dst.resize( dtpts * nchans );

    qint16  *D = &dst[0],
            *S = &src[0];
    vec_i16 bMin( nchans ),
            bMax( nchans );

    for( int it = 0; it < ntpts; it += dnsmp, D += nchans ) {

        int ns = std::min( ntpts - it, dnsmp );

        memcpy( &bMin[0], S, nchans*sizeof(qint16) );
        memcpy( &bMax[0], S, nchans*sizeof(qint16) );
        S += nchans;

        for( int is = 1; is < ns; ++is, S += nchans ) {

            for( int ic = 0; ic < nchans; ++ic ) {

                int val = S[ic];

                if( val <= bMin[ic] )
                    bMin[ic] = val;
                else if( val > bMax[ic] )
                    bMax[ic] = val;
            }
        }

        for( int ic = 0; ic < nchans; ++ic ) {

            if( abs( bMax[ic] ) >= abs( bMin[ic] ) )
                D[ic] = bMax[ic];
            else
                D[ic] = bMin[ic];
        }
    }

    if( &dst == &src )
        dst.resize( dtpts * nchans );

    return dtpts;
}


