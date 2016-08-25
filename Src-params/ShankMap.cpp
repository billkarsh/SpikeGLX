
#include "ShankMap.h"
#include "CimCfg.h"
#include "Util.h"


/* ---------------------------------------------------------------- */
/* ShankMapDesc --------------------------------------------------- */
/* ---------------------------------------------------------------- */

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

void ShankMap::fillDefaultNi( int nS, int nC, int nR )
{
    ns = nS;
    nc = nC;
    nr = nR;

    e.clear();

    for( uint is = 0; is < ns; ++is ) {

        for( uint ir = 0; ir < nr; ++ir ) {

            for( uint ic = 0; ic < nc; ++ic )
                e.push_back( ShankMapDesc( is, ic, ir, 1 ) );
        }
    }
}


void ShankMap::fillDefaultNiSaved(
    int                 nS,
    int                 nC,
    int                 nR,
    const QVector<uint> &saved )
{
    ns = nS;
    nc = nC;
    nr = nR;

    e.clear();

    for( uint is = 0; is < ns; ++is ) {

        for( uint ir = 0; ir < nr; ++ir ) {

            for( uint ic = 0; ic < nc; ++ic ) {

                if( saved.contains( 2*ir + ic ) )
                    e.push_back( ShankMapDesc( is, ic, ir, 1 ) );
            }
        }
    }
}


// Assume single shank, 2 columns.
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

            if( saved.contains( ic ) )
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

            if( saved.contains( ic ) )
                e.push_back( ShankMapDesc( 0, cl, rw, u ) );
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


