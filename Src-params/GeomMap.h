#ifndef GEOMMAP_H
#define GEOMMAP_H

#include <QBitArray>
#include <QMap>
#include <QString>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct GeomMapDesc
{
    float   x, z;   // (s x z u)
    qint16  s, u;   // u = use

    GeomMapDesc() : x(0), z(0), s(0), u(1)  {}
    GeomMapDesc( int s, float x, float z, int u )
        :   x(x), z(z), s(s), u(u)          {}
    bool operator==( const GeomMapDesc &rhs ) const
        {return s==rhs.s && x==rhs.x && z==rhs.z && u==rhs.u;}
    bool operator<( const GeomMapDesc &rhs ) const;
    QString toString() const;
    static GeomMapDesc fromString( const QString &s_in );
};


// Shank geometry class
//
// A GeomMap locates acq channels geometrically on probe.
//
struct GeomMap
{
    QString                     pn; // header (pn,ns,ds,wd)
    float                       ds,
                                wd;
    int                         ns;
    std::vector<GeomMapDesc>    e;

    GeomMap() : ds(0), wd(0), ns(0)         {}
    GeomMap( const QString &pn, int ns, float ds, float wd )
        :   pn(pn), ds(ds), wd(wd), ns(ns)  {}

    void andOutImStdby(
        const QBitArray     &stdbyBits,
        const QVector<uint> &saved,
        int                 offset );

    void chanOrderFromMapIm( QString &s, int nLF ) const;
    void revChanOrderFromMapIm( QString &s, int nLF ) const;
    void inverseMap( QMap<GeomMapDesc,uint> &inv ) const;

    bool equalHdr( const GeomMap &rhs ) const;

    bool operator==( const GeomMap &rhs ) const
        {return equalHdr(rhs) && e == rhs.e;}
    bool operator!=( const GeomMap &rhs ) const
        {return !(*this == rhs);}

    QString toString() const;
    QString toString( const QBitArray &onBits, int offset ) const;
    void fromString( const QString &s_in );
};

#endif  // GEOMMAP_H


