
#include "ShankMap.h"
#include "Util.h"


/* ---------------------------------------------------------------- */
/* ShankMapDesc --------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "s:c:r"
//
QString ShankMapDesc::toString() const
{
    return QString("%1:%2:%3").arg( s ).arg( c ).arg( r );
}


// Pattern: "s c r"
//
QString ShankMapDesc::toWhSpcSepString() const
{
    return QString("%1 %2 %3").arg( s ).arg( c ).arg( r );
}


// Pattern: "s:c:r"
//
ShankMapDesc ShankMapDesc::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("^\\s+|\\s*:\\s*"),
                                QString::SkipEmptyParts );

    return ShankMapDesc(
            sl.at( 0 ).toUInt(),
            sl.at( 1 ).toUInt(),
            sl.at( 2 ).toUInt() );
}


// Pattern: "s c r"
//
ShankMapDesc ShankMapDesc::fromWhSpcSepString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return ShankMapDesc(
            sl.at( 0 ).toUInt(),
            sl.at( 1 ).toUInt(),
            sl.at( 2 ).toUInt() );
}

/* ---------------------------------------------------------------- */
/* ShankMap ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankMap::fillDefault()
{
    e.clear();

    for( uint is = 0; is < ns; ++is ) {

        for( uint ir = 0; ir < nr; ++ir ) {

            for( uint ic = 0; ic < nc; ++ic )
                e.push_back( ShankMapDesc( is, ic, ir ) );
        }
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


bool ShankMap::saveFile( QString &msg, const QString &path )
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


