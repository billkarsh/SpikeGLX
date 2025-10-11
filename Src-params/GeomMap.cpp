
#include "GeomMap.h"

#include <QIODevice>
#include <QRegularExpression>
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
                                QRegularExpression("^\\s+|\\s*:\\s*"),
                                Qt::SkipEmptyParts );

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
void GeomMap::andOutImStdby(
    const QBitArray     &stdbyBits,
    const QVector<uint> &saved,
    int                 offset )
{
    for( int i = 0, nI = saved.size(), nS = stdbyBits.size(); i < nI; ++i ) {

        int ic = saved[i] - offset;

        if( ic < nS && stdbyBits.testBit( ic ) )
            e[i].u = 0;
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
    return  pn == rhs.pn && ns == rhs.ns && ds == rhs.ds && wd == rhs.wd;
}


// Pattern: (pn,ns,ds,wd)(s:x:z:u)()()...
//
QString GeomMap::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << pn << "," << ns << "," << ds << "," << wd << ")";

    for( int i = 0; i < n; ++i )
        ts << e[i].toString();

    return s;
}


// Pattern: (pn,ns,ds,wd)(s:x:z:u)()()...
//
QString GeomMap::toString( const QBitArray &onBits, int offset ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = qMin( int(e.size()), onBits.size() );

    ts << "(" << pn << "," << ns << "," << ds << "," << wd << ")";

    for( int i = 0; i < n; ++i ) {

        if( onBits.testBit( i + offset ) )
            ts << e[i].toString();
    }

    return s;
}


// Pattern: (pn,ns,ds,wd)(s:x:z:u)()()...
//
void GeomMap::fromString( const QString &s_in )
{
    QStringList sl = s_in.split(
                        QRegularExpression("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        Qt::SkipEmptyParts );
    int         n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegularExpression("^\\s+|\\s*,\\s*"),
                        Qt::SkipEmptyParts );

    if( hl.size() == 4 ) {
        pn = hl[0];
        ns = hl[1].toInt();
        ds = hl[2].toFloat();
        wd = hl[3].toFloat();
    }
    else {
        // originally, without ds field
        pn = hl[0];
        ns = hl[1].toInt();
        ds = 250.0f;
        wd = hl[2].toFloat();
    }

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( GeomMapDesc::fromString( sl[i] ) );
}


