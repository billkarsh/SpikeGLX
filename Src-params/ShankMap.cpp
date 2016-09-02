
#include "ShankMap.h"
#include "CimCfg.h"
#include "Util.h"


/* ---------------------------------------------------------------- */
/* ShankMapDesc --------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool ShankMapDesc::operator<( const ShankMapDesc &rhs ) const
{
    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    if( r < rhs.r )
        return true;

    if( r > rhs.r )
        return false;

    return c < rhs.c;
}


// Pattern: "s:c:r:u"
//
QString ShankMapDesc::toString() const
{
    return QString("%1:%2:%3:%4").arg( s ).arg( c ).arg( r ).arg( u );
}


// Pattern: "s c r u"
//
QString ShankMapDesc::toWhSpcSepString() const
{
    return QString("%1 %2 %3 %4").arg( s ).arg( c ).arg( r ).arg( u );
}


// Pattern: "s:c:r:u"
//
ShankMapDesc ShankMapDesc::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("^\\s+|\\s*:\\s*"),
                                QString::SkipEmptyParts );

    return ShankMapDesc(
            sl.at( 0 ).toUInt(),
            sl.at( 1 ).toUInt(),
            sl.at( 2 ).toUInt(),
            sl.at( 3 ).toUInt() );
}


// Pattern: "s c r u"
//
ShankMapDesc ShankMapDesc::fromWhSpcSepString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return ShankMapDesc(
            sl.at( 0 ).toUInt(),
            sl.at( 1 ).toUInt(),
            sl.at( 2 ).toUInt(),
            sl.at( 3 ).toUInt() );
}

/* ---------------------------------------------------------------- */
/* ShankMap ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Assume single shank, two columns.
//
void ShankMap::fillDefaultIm( const IMROTbl &T )
{
    int nElec = T.nElec(),
        nChan = T.nChan();

    ns = 1;
    nc = 2;
    nr = nElec / 2;

    e.clear();

    if( T.opt <= 3 ) {

        for( int ic = 0; ic < nChan; ++ic ) {

            int el, cl, rw, u;

            el = T.chToEl384( ic, T.e[ic].bank ) - 1;
            rw = el / 2;
            cl = el - 2 * rw;
            u  = T.chToRefid384( ic ) == 0;

            e.push_back( ShankMapDesc( 0, cl, rw, u ) );
        }
    }
    else {

        for( int ic = 0; ic < nChan; ++ic ) {

            int el, cl, rw, u;

            el = T.chToEl276( ic, T.e[ic].bank ) - 1;
            rw = el / 2;
            cl = el - 2 * rw;
            u  = T.chToRefid276( ic ) == 0;

            e.push_back( ShankMapDesc( 0, cl, rw, u ) );
        }
    }
}


void ShankMap::fillDefaultImSaved(
    const IMROTbl       &T,
    const QVector<uint> &saved )
{
    int nElec   = T.nElec(),
        nChan   = T.nChan(),
        nI      = saved.size();

    ns = 1;
    nc = 2;
    nr = nElec / 2;

    e.clear();

    if( T.opt <= 3 ) {

        for( int i = 0; i < nI; ++i ) {

            int ic, el, cl, rw, u;

            ic = saved[i];

            if( ic >= nChan )
                break;

            el = T.chToEl384( ic, T.e[ic].bank ) - 1;
            rw = el / 2;
            cl = el - 2 * rw;
            u  = T.chToRefid384( ic ) == 0;

            e.push_back( ShankMapDesc( 0, cl, rw, u ) );
        }
    }
    else {

        for( int i = 0; i < nI; ++i ) {

            int ic, el, cl, rw, u;

            ic = saved[i];

            if( ic >= nChan )
                break;

            el = T.chToEl276( ic, T.e[ic].bank ) - 1;
            rw = el / 2;
            cl = el - 2 * rw;
            u  = T.chToRefid276( ic ) == 0;

            e.push_back( ShankMapDesc( 0, cl, rw, u ) );
        }
    }
}

// nChan must be <= nS*nC*nR.
//
// Scheme is to fill all shanks as evenly as possible, each
// from tip upward. If remainder, lower # shanks may get one
// more than others.
//
void ShankMap::fillDefaultNi( int nS, int nC, int nR, int nChan )
{
    ns = nS;
    nc = nC;
    nr = nR;

    e.clear();

    int nps = nChan / nS,
        rem = nChan - nS * nps;

    for( uint is = 0; is < ns; ++is ) {

        int nMore = nps + (rem-- > 0 ? 1 : 0);

        for( uint ir = 0; ir < nr; ++ir ) {

            for( uint ic = 0; ic < nc; ++ic ) {

                if( nMore-- > 0 )
                    e.push_back( ShankMapDesc( is, ic, ir, 1 ) );
                else
                    goto next_s;
            }
        }
next_s:;
    }
}


// Assume single shank, two columns.
//
void ShankMap::fillDefaultNiSaved( int nChan, const QVector<uint> &saved )
{
    ns = 1;
    nc = 2;
    nr = nChan / 2;

    e.clear();

    for( uint ir = 0; ir < nr; ++ir ) {

        for( uint ic = 0; ic < nc; ++ic ) {

            int chan = 2*ir + ic;

            if( chan < nChan && saved.contains( chan ) )
                e.push_back( ShankMapDesc( 0, ic, ir, 1 ) );
        }
    }
}


// Ensure imec refs are excluded (from user file, say).
//
void ShankMap::andOutImRefs( const IMROTbl &T )
{
    int n = e.size();

    if( T.opt <= 3 ) {

        for( int ic = 0; ic < n; ++ic ) {

            if( T.chToRefid384( ic ) != 0 )
                e[ic].u = 0;
        }
    }
    else {

        for( int ic = 0; ic < n; ++ic ) {

            if( T.chToRefid276( ic ) != 0 )
                e[ic].u = 0;
        }
    }
}


// Ensure imec stdbys are excluded (from user file, say).
//
void ShankMap::andOutImStdby( const QBitArray &stdbyBits )
{
    int n = e.size();

    for( int ic = 0; ic < n; ++ic ) {

        if( stdbyBits.testBit( ic ) )
            e[ic].u = 0;
    }
}


// In Im case, AP channels are ordered according to
// ShankMap as in Ni case (below), but the LF channels
// all follow the AP set and also have ShankMap order.
//
void ShankMap::chanOrderFromMapIm( QString &s ) const
{
    QMap<ShankMapDesc,uint> inv;

    inverseMap( inv );

    QMap<ShankMapDesc,uint>::iterator   it = inv.begin();

    if( it == inv.end() )
        return;

    QString     s2;
    QTextStream tsAP( &s,  QIODevice::WriteOnly );
    QTextStream tsLF( &s2, QIODevice::WriteOnly );
    int         nC = e.size(),
                v;

// first item

    v = it.value();
    tsAP << v;
    tsLF << v + nC;

// others

    for( ++it; it != inv.end(); ++it ) {

        v = it.value();
        tsAP << "," << v;
        tsLF << "," << v + nC;
    }

    tsAP << "," << s2;
}


void ShankMap::chanOrderFromMapNi( QString &s ) const
{
    QMap<ShankMapDesc,uint> inv;

    inverseMap( inv );

    QMap<ShankMapDesc,uint>::iterator   it = inv.begin();

    if( it == inv.end() )
        return;

    QTextStream ts( &s, QIODevice::WriteOnly );

// first item

    ts << it.value();

// others

    for( ++it; it != inv.end(); ++it )
        ts << "," << it.value();
}


void ShankMap::inverseMap( QMap<ShankMapDesc,uint> &inv ) const
{
    inv.clear();

    int n = e.size();

    for( int i = 0; i < n; ++i )
        inv[e[i]] = i;
}


bool ShankMap::equalHdr( const ShankMap &rhs ) const
{
    return  ns == rhs.ns && nc == rhs.nc && nr == rhs.nr;
}


QString ShankMap::hdrText() const
{
    return QString("ns,nc,nr = %1,%2,%3").arg( ns ).arg( nc ).arg( nr );
}


// Pattern: (ns,nc,nr)(s:c:r)()()...
//
QString ShankMap::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << ns << "," << nc << "," << nr << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString() << ")";

    return s;
}


// Pattern: (ns,nc,nr)(s:c:r)()()...
//
QString ShankMap::toString( const QBitArray &onBits ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << ns << "," << nc << "," << nr << ")";

    for( int i = 0; i < n; ++i ) {

        if( onBits.testBit( i ) )
            ts << "(" << e[i].toString() << ")";
    }

    return s;
}


// Pattern: ns,nc,nr\s1 c1 r1\s2 c2 r2\n...
//
QString ShankMap::toWhSpcSepString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << ns << "," << nc << "," << nr << "\n";

    for( int i = 0; i < n; ++i )
        ts << e[i].toWhSpcSepString() << "\n";

    return s;
}


// Pattern: (ns,nc,nr)(s:c:r)()()...
//
void ShankMap::fromString( const QString &s_in )
{
    QStringList sl = s_in.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    ns = hl[0].toUInt();
    nc = hl[1].toUInt();
    nr = hl[2].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( ShankMapDesc::fromString( sl[i] ) );
}


// Pattern: ns,nc,nr\s1 c1 r1\s2 c2 r2\n...
//
void ShankMap::fromWhSpcSepString( const QString &s_in )
{
    QStringList sl = s_in.split(
                        QRegExp("[\r\n]+"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    ns = hl[0].toUInt();
    nc = hl[1].toUInt();
    nr = hl[2].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( ShankMapDesc::fromWhSpcSepString( sl[i] ) );
}


bool ShankMap::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find [%1]").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromWhSpcSepString( f.readAll() );

        if( e.size() ) {

            msg = QString("Loaded [%1]").arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error reading [%1]").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening [%1]").arg( fi.fileName() );
        return false;
    }
}


bool ShankMap::saveFile( QString &msg, const QString &path ) const
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        int n = f.write( STR2CHR( toWhSpcSepString() ) );

        if( n > 0 ) {

            msg = QString("Saved [%1]").arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error writing [%1]").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening [%1]").arg( fi.fileName() );
        return false;
    }
}


