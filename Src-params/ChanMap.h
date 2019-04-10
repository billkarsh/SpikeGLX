#ifndef CHANMAP_H
#define CHANMAP_H

#include <QBitArray>
#include <QString>
#include <QVector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct ChanMapDesc
{
    QString name;
    uint    order;

    ChanMapDesc() : order(0)            {}
    ChanMapDesc( const QString &name, uint order )
        :   name(name), order(order)    {}
    bool operator==( const ChanMapDesc &rhs ) const
        {return name==rhs.name && order==rhs.order;}
    QString toString() const;
    QString toWhSpcSepString() const;
    static ChanMapDesc fromString( const QString &s_in );
    static ChanMapDesc fromWhSpcSepString( const QString &s_in );
};


// Channel mapping base class
//
// A ChanMap names and orders channels in graph views.
//
struct ChanMap
{
    std::vector<ChanMapDesc>    e;

    virtual ~ChanMap()  {}

    virtual void fillDefault() = 0;

    void defaultOrder( QVector<int> &v ) const;
    void userOrder( QVector<int> &v ) const;

    QString name( int ic, bool isTrigger = false ) const;

    virtual QString type() const = 0;
    virtual int i16Count() const = 0;
    virtual bool equalHdr( const ChanMap &rhs ) const = 0;

    bool operator==( const ChanMap &rhs ) const
        {return equalHdr(rhs) && e == rhs.e;}
    bool operator!=( const ChanMap &rhs ) const
        {return !(*this == rhs);}

    virtual QString hdrText() const = 0;
    virtual QString toString() const = 0;
    virtual QString toString( const QBitArray &onBits ) const = 0;
    virtual QString toWhSpcSepString() const = 0;
    virtual void fromString( const QString &s_in ) = 0;
    virtual void fromWhSpcSepString( const QString &s_in ) = 0;

    bool loadFile( QString &msg, const QString &path );
    bool saveFile( QString &msg, const QString &path ) const;
};


// NIDQ stream.
// Each timepoint's default ("acquired") channel ordering is:
// - (C) Muxed neural (MN0) channels
// - (C) Muxed neural (MN1) channels
// - etc.
// - (C) Muxed aux (MA0) channels
// - (C) Muxed aux (MA1) channels
// - etc.
// - (XA) Aux analog channels.
// - (XD) Aux digital channels.
//
struct ChanMapNI : public ChanMap
{
    uint    MN, MA, C, XA, XD;

    ChanMapNI() : MN(0), MA(0), C(0), XA(0), XD(0)  {}
    ChanMapNI( uint MN, uint MA, uint C, uint XA, uint XD )
        :   MN(MN), MA(MA), C(C), XA(XA), XD(XD)    {}

    virtual void fillDefault();

    virtual QString type() const    {return "nidq";}
    virtual int i16Count() const    {return(MN+MA)*C + XA + XD;}
    virtual bool equalHdr( const ChanMap &rhs ) const;

    virtual QString hdrText() const;
    virtual QString toString() const;
    virtual QString toString( const QBitArray &onBits ) const;
    virtual QString toWhSpcSepString() const;
    virtual void fromString( const QString &s_in );
    virtual void fromWhSpcSepString( const QString &s_in );
};


// IMEC stream.
// Each timepoint's default ("acquired") channel ordering is:
// - (AP) IMEC AP  channels
// - (LF) IMEC LFP channels
// - (SY) IMEC SYNC channels
//
struct ChanMapIM : public ChanMap
{
    uint    AP, LF, SY;

    ChanMapIM() : AP(0), LF(0), SY(0) {}
    ChanMapIM( uint AP, uint LF, uint SY ) : AP(AP), LF(LF), SY(SY) {}

    virtual void fillDefault();

    virtual QString type() const    {return "imec";}
    virtual int i16Count() const    {return AP + LF + SY;}
    virtual bool equalHdr( const ChanMap &rhs ) const;

    virtual QString hdrText() const;
    virtual QString toString() const;
    virtual QString toString( const QBitArray &onBits ) const;
    virtual QString toWhSpcSepString() const;
    virtual void fromString( const QString &s_in );
    virtual void fromWhSpcSepString( const QString &s_in );
};

#endif  // CHANMAP_H


