#ifndef IMROTBL_H
#define IMROTBL_H

#include <qglobal.h>

#include <vector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// virtual base class
//
struct IMROTbl
{
    quint32 type;

    virtual ~IMROTbl()  {}

    virtual void copyFrom( const IMROTbl *rhs ) = 0;
    virtual void fillDefault() = 0;

    virtual int nShank() const = 0;
    virtual int nCol() const = 0;
    virtual int nRow() const = 0;
    virtual int nChan() const = 0;
    virtual int nAP() const = 0;
    virtual int nLF() const = 0;
    virtual int nSY() const = 0;
    virtual int nElec() const = 0;
    virtual int maxInt() const = 0;
    virtual double maxVolts() const = 0;
    virtual bool needADCCal() const = 0;
    virtual bool selectableGain() const = 0;
    virtual bool setableHipass() const = 0;
    virtual bool isMultiSelect() const = 0;

    virtual bool operator==( const IMROTbl &rhs ) const = 0;
    virtual bool operator!=( const IMROTbl &rhs ) const = 0;

    virtual bool isConnectedSame( const IMROTbl *rhs ) const = 0;

    virtual QString toString() const = 0;
    virtual void fromString( const QString &s ) = 0;

    virtual bool loadFile( QString &msg, const QString &path ) = 0;
    virtual bool saveFile( QString &msg, const QString &path ) const = 0;

    virtual int shnk( int ch ) const = 0;
    virtual int bank( int ch ) const = 0;
    virtual int elShankAndBank( int &bank, int ch ) const = 0;
    virtual int elShankColRow( int &col, int &row, int ch ) const = 0;
    virtual void eaChansOrder( QVector<int> &v ) const = 0;
    virtual int refid( int ch ) const = 0;
    virtual int refTypeAndFields( int &shank, int &bank, int ch ) const = 0;
    virtual int apGain( int ch ) const = 0;
    virtual int lfGain( int ch ) const = 0;
    virtual int apFlt( int ch ) const = 0;

    virtual bool chIsRef( int ch ) const = 0;
    virtual int idxToGain( int idx ) const = 0;
    virtual int gainToIdx( int gain ) const = 0;
    virtual double unityToVolts( double u ) const = 0;

    virtual void muxTable( int &nADC, int &nChn, std::vector<int> &T ) const = 0;

    static IMROTbl* alloc( int type );
    static QString defaultString( int type );
};

#endif  // IMROTBL_H


