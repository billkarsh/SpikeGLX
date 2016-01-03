#ifndef CHANMAP_H
#define CHANMAP_H

#include <QString>
#include <QVector>
#include <QBitArray>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ChanMapDesc
{
    QString name;
    uint    order;

    ChanMapDesc() : order(0) {}
    ChanMapDesc( const QString &name, uint order )
        : name(name), order(order) {}
    bool operator==( const ChanMapDesc &rhs ) const
        {return name==rhs.name && order==rhs.order;}
    QString toString() const;
    QString toWhSpcSepString() const;
    static ChanMapDesc fromString( const QString &s_in );
    static ChanMapDesc fromWhSpcSepString( const QString &s_in );
};


// There's always a ChanMap that names and orders the channels.
// Each timepoint's default ("acquired") channel ordering is:
// - (AP) IMEC AP  channels
// - (LF) IMEC LFP channels
// - (C) Muxed neural (MN0) channels
// - (C) Muxed neural (MN1) channels
// - etc.
// - (C) Muxed aux (MA0) channels
// - (C) Muxed aux (MA1) channels
// - etc.
// - (XA) Aux analog channels.
// - (XD) Aux digital channels.
//
struct ChanMap
{
    uint                    AP, LF, MN, MA, C, XA, XD;
    QVector<ChanMapDesc>    e;

    ChanMap() : AP(0), LF(0), MN(0), MA(0), C(0), XA(0), XD(0) {}
    ChanMap( uint AP, uint LF, uint MN, uint MA, uint C, uint XA, uint XD )
        : AP(AP), LF(LF), MN(MN), MA(MA), C(C), XA(XA), XD(XD) {}
    void fillDefault();

    void defaultOrder( QVector<int> &v );
    void userOrder( QVector<int> &v );

    int i16Count() const
        {return AP + LF + (MN+MA)*C + XA + XD;}
    bool equalHdr( const ChanMap &q ) const
        {return AP==q.AP && LF==q.LF && XA==q.XA && XD==q.XD
             && MN==q.MN && MA==q.MA && (!(MN+MA) || C==q.C);}
    bool operator==( const ChanMap &rhs ) const
        {return equalHdr(rhs) && e == rhs.e;}
    bool operator!=( const ChanMap &rhs ) const
        {return !(*this == rhs);}

    QString name( int ic, bool isTrigger = false ) const;

    QString toString() const;
    QString toString( const QBitArray &onBits ) const;
    QString toWhSpcSepString() const;
    void fromString( const QString &s_in );
    void fromWhSpcSepString( const QString &s_in );

    bool loadFile( QString &msg, const QString &path );
    bool saveFile( QString &msg, const QString &path );
};

#endif  // CHANMAP_H


