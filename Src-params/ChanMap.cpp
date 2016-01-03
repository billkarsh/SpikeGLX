
#include "ChanMap.h"
#include "Util.h"

#include <QRegExp>
#include <QMap>


/* ---------------------------------------------------------------- */
/* ChanMapDesc ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Pattern: "name:order"
//
QString ChanMapDesc::toString() const
{
    return QString("%1:%2").arg( name ).arg( order );
}


// Pattern: "name order"
//
QString ChanMapDesc::toWhSpcSepString() const
{
    return QString("%1 %2").arg( name ).arg( order );
}


// Pattern: "name:order"
//
ChanMapDesc ChanMapDesc::fromString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("^\\s+|\\s*:\\s*"),
                                QString::SkipEmptyParts );

    return ChanMapDesc( sl.at( 0 ), sl.at( 1 ).toUInt() );
}


// Pattern: "name order"
//
ChanMapDesc ChanMapDesc::fromWhSpcSepString( const QString &s_in )
{
    const QStringList   sl = s_in.split(
                                QRegExp("\\s+"),
                                QString::SkipEmptyParts );

    return ChanMapDesc( sl.at( 0 ), sl.at( 1 ).toUInt() );
}

/* ---------------------------------------------------------------- */
/* ChanMap -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ChanMap::fillDefault()
{
    e.clear();

    int k = 0;

    for( uint i = 0; i < AP; ++i, ++k )
        e.push_back( ChanMapDesc( QString("AP%1;%2").arg( i ).arg( k ), k ) );

    for( uint i = 0; i < LF; ++i, ++k )
        e.push_back( ChanMapDesc( QString("LF%1;%2").arg( i ).arg( k ), k ) );

    for( uint i = 0; i < MN; ++i ) {

        for( uint j = 0; j < C; ++j, ++k ) {

            e.push_back( ChanMapDesc(
                QString("MN%1C%2;%3").arg( i ).arg( j ).arg( k ), k ) );
        }
    }

    for( uint i = 0; i < MA; ++i ) {

        for( uint j = 0; j < C; ++j, ++k ) {

            e.push_back( ChanMapDesc(
                QString("MA%1C%2;%3").arg( i ).arg( j ).arg( k ), k ) );
        }
    }

    for( uint i = 0; i < XA; ++i, ++k )
        e.push_back( ChanMapDesc( QString("XA%1;%2").arg( i ).arg( k ), k ) );

    for( uint i = 0; i < XD; ++i, ++k )
        e.push_back( ChanMapDesc( QString("XD%1;%2").arg( i ).arg( k ), k ) );
}


// Create mapping from order index in range [0,nEntries)
// to an entry index in range [0,nEntries). The default
// mapping is just the identity map.
//
void ChanMap::defaultOrder( QVector<int> &v )
{
    int n = e.size();

    v.resize( n );

    for( int i = 0; i < n; ++i )
        v[i] = i;
}


// Create mapping from order index in range [0,nEntries)
// to an entry index in range [0,nEntries). We need the
// intermediate order2entry device because the count of
// entries in this map may be smaller than order values.
//
void ChanMap::userOrder( QVector<int> &v )
{
    QMap<int,int>   order2entry;
    int             n = e.size(),
                    k = 0;

    for( int i = 0; i < n; ++i )
        order2entry[e[i].order] = i;

    v.resize( n );

    QMap<int,int>::iterator it;

    for( it = order2entry.begin(); it != order2entry.end(); ++it )
        v[k++] = it.value();
}


QString ChanMap::name( int ic, bool isTrigger ) const
{
    if( isTrigger )
        return QString("%1~TRG").arg( e[ic].name );
    else
        return e[ic].name;
}


// Pattern: (AP,LF,MN,MA,C,XA,XD)(name:order)()()...
//
QString ChanMap::toString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << AP << "," << LF << ","
              << MN << "," << MA << ","
              << C  << "," << XA << "," << XD << ")";

    for( int i = 0; i < n; ++i )
        ts << "(" << e[i].toString() << ")";

    return s;
}


// Pattern: (AP,LF,MN,MA,C,XA,XD)(name:order)()()...
//
QString ChanMap::toString( const QBitArray &onBits ) const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << "(" << AP << "," << LF << ","
              << MN << "," << MA << ","
              << C  << "," << XA << "," << XD << ")";

    for( int i = 0; i < n; ++i ) {

        if( onBits.testBit( i ) )
            ts << "(" << e[i].toString() << ")";
    }

    return s;
}


// Pattern: AP,LF,MN,MA,C,XA,XD\nname1 order1\nname2 order2\n...
//
QString ChanMap::toWhSpcSepString() const
{
    QString     s;
    QTextStream ts( &s, QIODevice::WriteOnly );
    int         n = e.size();

    ts << AP << "," << LF << ","
       << MN << "," << MA << ","
       << C  << "," << XA << "," << XD << "\n";

    for( int i = 0; i < n; ++i )
        ts << e[i].toWhSpcSepString() << "\n";

    return s;
}


// Pattern: (AP,LF,MN,MA,C,XA,XD)(name:order)()()...
//
void ChanMap::fromString( const QString &s_in )
{
    QStringList	sl = s_in.split(
                        QRegExp("^\\s*\\(|\\)\\s*\\(|\\)\\s*$"),
                        QString::SkipEmptyParts );
    int			n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s*|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    AP = hl[0].toUInt();
    LF = hl[1].toUInt();
    MN = hl[2].toUInt();
    MA = hl[3].toUInt();
    C  = hl[4].toUInt();
    XA = hl[5].toUInt();
    XD = hl[6].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( ChanMapDesc::fromString( sl[i] ) );
}


// Pattern: AP,LF,MN,MA,C,XA,XD\nname1 order1\nname2 order2\n...
//
void ChanMap::fromWhSpcSepString( const QString &s_in )
{
    QStringList sl = s_in.split(
                        QRegExp("[\r\n]+"),
                        QString::SkipEmptyParts );
    int			n  = sl.size();

// Header

    QStringList hl = sl[0].split(
                        QRegExp("^\\s*|\\s*,\\s*"),
                        QString::SkipEmptyParts );

    AP = hl[0].toUInt();
    LF = hl[1].toUInt();
    MN = hl[2].toUInt();
    MA = hl[3].toUInt();
    C  = hl[4].toUInt();
    XA = hl[5].toUInt();
    XD = hl[6].toUInt();

// Entries

    e.clear();
    e.reserve( n - 1 );

    for( int i = 1; i < n; ++i )
        e.push_back( ChanMapDesc::fromWhSpcSepString( sl[i] ) );
}


bool ChanMap::loadFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( !fi.exists() ) {

        msg = QString("Can't find [%1]")
                .arg( fi.fileName() );

        return false;
    }
    else if( f.open( QIODevice::ReadOnly | QIODevice::Text ) ) {

        ChanMap::fromWhSpcSepString( f.readAll() );

        if( e.size() ) {

            msg = QString("Loaded [%1]")
                    .arg( fi.fileName() );

            return true;
        }
        else {

            msg = QString("Error reading [%1]")
                    .arg( fi.fileName() );

            return false;
        }
    }
    else {

        msg = QString("Error opening [%1]")
                .arg( fi.fileName() );

        return false;
    }
}


bool ChanMap::saveFile( QString &msg, const QString &path )
{
    QFile       f( path );
    QFileInfo   fi( path );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        int n = f.write( STR2CHR( toWhSpcSepString() ) );

        if( n > 0 ) {

            msg = QString("Saved [%1]")
                    .arg( fi.fileName() );

            return true;
        }
        else {

            msg = QString("Error writing [%1]")
                    .arg( fi.fileName() );

            return false;
        }
    }
    else {

        msg = QString("Error opening [%1]")
                .arg( fi.fileName() );

        return false;
    }
}


