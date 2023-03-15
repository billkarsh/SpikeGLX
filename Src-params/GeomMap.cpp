
#include "GeomMap.h"

#include <QTextStream>


/* ---------------------------------------------------------------- */
/* GeomMapDesc ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool GeomMapDesc::operator<( const GeomMapDesc &rhs ) const
{
    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    if( z < rhs.z )
        return true;

    if( z > rhs.z )
        return false;

    if( x < rhs.x )
        return true;

    if( x > rhs.x )
        return false;

    return u < rhs.u;
}


// Pattern: "(s:x:z:u)"
//
QString GeomMapDesc::toString() const
{
    return QString("(%1:%2:%3:%4)").arg( s ).arg( x ).arg( z ).arg( u );
}


// Pattern: "s:x:z:u"
//
GeomMapDesc GeomMapDesc::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("^\\s+|\\s*:\\s*"),
                                QString::SkipEmptyParts );

    return GeomMapDesc(
            sl.at( 0 ).toInt(),
            sl.at( 1 ).toFloat(),
            sl.at( 2 ).toFloat(),
            sl.at( 3 ).toInt() );
}

/* ---------------------------------------------------------------- */
/* GeomMap -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Ensure imec stdbys are excluded (from user file, say).
//
void GeomMap::andOutImStdby( const QBitArray &stdbyBits )
{
    int n = qMin( int(e.size()), stdbyBits.size() );

    for( int ic = 0; ic < n; ++ic ) {

        if( stdbyBits.testBit( ic ) )
            e[ic].u = 0;
    }
}


// AP channels are ordered according to GeomMap.
// The LF channels all follow the AP set and also
// have GeomMap order.
//
void GeomMap::chanOrderFromMapIm( QString &s, int nLF ) const
{
    QMap<GeomMapDesc,uint> inv;

    inverseMap( inv );

    QMap<GeomMapDesc,uint>::iterator   it = inv.begin();

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


// AP channels are rev ordered according to GeomMap.
// The LF channels all follow the AP set and also
// have rev GeomMap order.
//
void GeomMap::revChanOrderFromMapIm( QString &s, int nLF ) const
{
    QMap<GeomMapDesc,uint>  inv;

    inverseMap( inv );

    QMapIterator<GeomMapDesc,uint>  it( inv );

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


void GeomMap::inverseMap( QMap<GeomMapDesc,uint> &inv ) const
{
    inv.clear();

    int n = e.size();

    for( int i = 0; i < n; ++i )
        inv[e[i]] = i;
}


bool GeomMap::equalHdr( const GeomMap &rhs ) const
{
    return  pn == rhs.pn && ns == rhs.ns && wd == rhs.wd;
}


// Pattern: (pn,ns,wd)(s:x:z:u)()()...
//
QString GeomMap::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << pn << "," << ns << "," << wd << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString();

    return s;
}


// Pattern: (pn,ns,wd)(s:x:z:u)()()...
//
QString GeomMap::toString( const QBitArray &onBits, int offset ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = qMin( int(e.size()), onBits.size() );

    ts << "(" << pn << "," << ns << "," << wd << ")";

    for( int i = 0; i < n; ++i ) {

        if( onBits.testBit( i + offset ) )
            ts << e[i].toString();
    }

    return s;
}


// Pattern: (pn,ns,wd)(s:x:z:u)()()...
//
void GeomMap::fromString( const QString &s_in )
{
    QStringList sl = s_in.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s+|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    pn = hl[0];
    ns = hl[1].toInt();
    wd = hl[2].toFloat();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( GeomMapDesc::fromString( sl[i] ) );
}


