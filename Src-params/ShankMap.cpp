
#include "ShankMap.h"
#include "IMROTbl.h"
#include "Util.h"

#include <QFileInfo>


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


// Pattern: "(s:c:r:u)"
//
QString ShankMapDesc::toString() const
{
    return QString("(%1:%2:%3:%4)").arg( s ).arg( c ).arg( r ).arg( u );
}


// Pattern: "s c r u\n"
//
QString ShankMapDesc::toWhSpcSepString() const
{
    return QString("%1 %2 %3 %4\n").arg( s ).arg( c ).arg( r ).arg( u );
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

void ShankMap::fillDefaultIm( const IMROTbl &T )
{
    int nChan = T.nChan();

    ns = T.nShank();
    nc = T.nCol();
    nr = T.nRow();

    e.clear();

    for( int ic = 0; ic < nChan; ++ic ) {

        int sh, cl, rw, u;

        sh = T.elShankColRow( cl, rw, ic );
        u  = !T.chIsRef( ic );

        e.push_back( ShankMapDesc( sh, cl, rw, u ) );
    }
}


void ShankMap::fillDefaultImSaved(
    const IMROTbl       &T,
    const QVector<uint> &saved,
    int                 offset )
{
    int nChan   = T.nChan() + offset,
        nI      = saved.size();

    ns = T.nShank();
    nc = T.nCol();
    nr = T.nRow();

    e.clear();

    for( int i = 0; i < nI; ++i ) {

        int ic, sh, cl, rw, u;

        ic = saved[i];

        if( ic >= nChan )
            break;

        sh = T.elShankColRow( cl, rw, ic );
        u  = !T.chIsRef( ic );

        e.push_back( ShankMapDesc( sh, cl, rw, u ) );
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

    for( int ic = 0; ic < n; ++ic ) {

        if( T.chIsRef( ic ) )
            e[ic].u = 0;
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
void ShankMap::chanOrderFromMapIm( QString &s, int nLF ) const
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

    if( nLF )
        tsLF << v + nC;

// others

    for( ++it; it != inv.end(); ++it ) {

        v = it.value();
        tsAP << "," << v;

        if( nLF )
            tsLF << "," << v + nC;
    }

    if( nLF )
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


// In Im case, AP channels are rev ordered according to
// ShankMap as in Ni case (below), but the LF channels
// all follow the AP set and also have rev ShankMap order.
//
void ShankMap::revChanOrderFromMapIm( QString &s, int nLF ) const
{
    QMap<ShankMapDesc,uint> inv;

    inverseMap( inv );

    QMapIterator<ShankMapDesc,uint> it( inv );

    it.toBack();

    if( !it.hasPrevious() )
        return;

    QString     s2;
    QTextStream tsAP( &s,  QIODevice::WriteOnly );
    QTextStream tsLF( &s2, QIODevice::WriteOnly );
    int         nC = e.size(),
                v;

// first item

    it.previous();
    v = it.value();
    tsAP << v;

    if( nLF )
        tsLF << v + nC;

// others

    while( it.hasPrevious() ) {

        it.previous();
        v = it.value();
        tsAP << "," << v;

        if( nLF )
            tsLF << "," << v + nC;
    }

    if( nLF )
        tsAP << "," << s2;
}


void ShankMap::revChanOrderFromMapNi( QString &s ) const
{
    QMap<ShankMapDesc,uint> inv;

    inverseMap( inv );

    QMapIterator<ShankMapDesc,uint> it( inv );

    it.toBack();

    if( !it.hasPrevious() )
        return;

    QTextStream ts( &s, QIODevice::WriteOnly );

// first item

    it.previous();
    ts << it.value();

// others

    while( it.hasPrevious() ) {

        it.previous();
        ts << "," << it.value();
    }
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


// Pattern: (ns,nc,nr)(s:c:r:u)()()...
//
QString ShankMap::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << ns << "," << nc << "," << nr << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString();

    return s;
}


// Pattern: (ns,nc,nr)(s:c:r:u)()()...
//
QString ShankMap::toString( const QBitArray &onBits , int offset ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << ns << "," << nc << "," << nr << ")";

    for( int i = 0; i < n; ++i ) {

        if( onBits.testBit( i + offset ) )
            ts << e[i].toString();
    }

    return s;
}


// Pattern: ns,nc,nr\ns1 c1 r1 u1\ns2 c2 r2 u2\n...
//
QString ShankMap::toWhSpcSepString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << ns << "," << nc << "," << nr << "\n";

    for( int i = 0; i < n; ++i )
        ts << e[i].toWhSpcSepString();

    return s;
}


// Pattern: (ns,nc,nr)(s:c:r:u)()()...
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


// Pattern: ns,nc,nr\ns1 c1 r1 u1\ns2 c2 r2 u2\n...
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

        msg = QString("Can't find '%1'").arg( fi.fileName() );
        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        fromWhSpcSepString( f.readAll() );

        if( e.size() ) {

            msg = QString("Loaded '%1'").arg( fi.fileName() );
            return true;
        }
        else {
            msg = QString("Error reading '%1'").arg( fi.fileName() );
            return false;
        }
    }
    else {
        msg = QString("Error opening '%1'").arg( fi.fileName() );
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

            msg = QString("Saved '%1'").arg( fi.fileName() );
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


