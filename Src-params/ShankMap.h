#ifndef SHANKMAP_H
#define SHANKMAP_H

#include <QBitArray>
#include <QMap>
#include <QString>
#include <QVector>

struct IMROTbl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ShankMapDesc
{
    quint16 s, c, r, u; // u = use

    ShankMapDesc() : s(0), c(0), r(0), u(1) {}
    ShankMapDesc( uint s, uint c, uint r, uint u )
        :   s(s), c(c), r(r), u(u)          {}
    bool operator==( const ShankMapDesc &rhs ) const
        {return s==rhs.s && c==rhs.c && r==rhs.r && u==rhs.u;}
    bool operator<( const ShankMapDesc &rhs ) const;
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
    uint                        ns, nc, nr;
    std::vector<ShankMapDesc>   e;

    ShankMap() : ns(0), nc(0), nr(0) {}
    ShankMap( uint ns, uint nc, uint nr ) : ns(ns), nc(nc), nr(nr) {}

    virtual ~ShankMap() {}

    void fillDefaultIm( const IMROTbl &T );
    void fillDefaultImSaved(
        const IMROTbl       &T,
        const QVector<uint> &saved );

    void fillDefaultNi( int nS, int nC, int nR, int nChan );
    void fillDefaultNiSaved( int nChan, const QVector<uint> &saved );

    void andOutImRefs( const IMROTbl &T );
    void andOutImStdby( const QBitArray &stdbyBits );

    void chanOrderFromMapIm( QString &s, int nLF ) const;
    void chanOrderFromMapNi( QString &s ) const;
    void revChanOrderFromMapIm( QString &s, int nLF ) const;
    void revChanOrderFromMapNi( QString &s ) const;
    void inverseMap( QMap<ShankMapDesc,uint> &inv ) const;

    int nSites() const  {return ns * nc * nr;}
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
    bool saveFile( QString &msg, const QString &path ) const;
};

#endif  // SHANKMAP_H


