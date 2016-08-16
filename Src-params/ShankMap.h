#ifndef SHANKMAP_H
#define SHANKMAP_H

#include <QString>
#include <QVector>
#include <QBitArray>
#include <QMap>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ShankMapDesc
{
    uint    s, c, r;

    ShankMapDesc() : s(0), c(0), r(0)   {}
    ShankMapDesc( uint s, uint c, uint r ) : s(s), c(c), r(r)   {}
    bool operator==( const ShankMapDesc &rhs ) const
        {return s==rhs.s && c==rhs.c && r==rhs.r;}
    bool operator<( const ShankMapDesc &rhs ) const
        {
            if( s < rhs.s )
                return true;
            if( r < rhs.r )
                return true;
            return c < rhs.c;
        }
    QString toString() const;
    QString toWhSpcSepString() const;
    static ShankMapDesc fromString( const QString &s_in );
    static ShankMapDesc fromWhSpcSepString( const QString &s_in );
};


// Shank mapping class
//
// A ShankMap locates acq channels geometrically on probe.
//
struct ShankMap
{
    uint                    ns, nc, nr;
    QVector<ShankMapDesc>   e;

    ShankMap() : ns(0), nc(0), nr(0) {}
    ShankMap( uint ns, uint nc, uint nr ) : ns(ns), nc(nc), nr(nr) {}

    virtual ~ShankMap() {}

    void fillDefault();

    void inverseMap( QMap<ShankMapDesc,uint> &inv ) const;

    int count() const   {return ns * nc * nr;}
    bool equalHdr( const ShankMap &rhs ) const;

    bool operator==( const ShankMap &rhs ) const
        {return equalHdr(rhs) && e == rhs.e;}
    bool operator!=( const ShankMap &rhs ) const
        {return !(*this == rhs);}

    QString hdrText() const;
    QString toString() const;
    QString toString( const QBitArray &onBits ) const;
    QString toWhSpcSepString() const;
    void fromString( const QString &s_in );
    void fromWhSpcSepString( const QString &s_in );

    bool loadFile( QString &msg, const QString &path );
    bool saveFile( QString &msg, const QString &path );
};

#endif  // SHANKMAP_H


