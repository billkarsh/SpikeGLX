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
    int type;

    virtual ~IMROTbl()  {}

    virtual void copyFrom( const IMROTbl *rhs ) = 0;
    virtual void fillDefault() = 0;
    virtual void fillShankAndBank( int shank, int bank ) = 0;

    virtual int nElec() const = 0;
    virtual int nShank() const = 0;
    virtual int nElecPerShank() const = 0;
    virtual int nCol() const = 0;
    virtual int nRow() const = 0;
    virtual int nChan() const = 0;
    virtual int nAP() const = 0;
    virtual int nLF() const = 0;
    virtual int nSY() const = 0;
    virtual int nBanks() const = 0;
    virtual int nRefs() const = 0;
    virtual int maxInt() const = 0;
    virtual double maxVolts() const = 0;
    virtual bool needADCCal() const = 0;

    virtual bool operator==( const IMROTbl &rhs ) const = 0;
    virtual bool operator!=( const IMROTbl &rhs ) const = 0;

    virtual bool isConnectedSame( const IMROTbl *rhs ) const = 0;

    virtual QString toString() const = 0;
    virtual bool fromString( QString *msg, const QString &s ) = 0;

    virtual bool loadFile( QString &msg, const QString &path ) = 0;
    virtual bool saveFile( QString &msg, const QString &path ) const = 0;

    virtual int shnk( int ch ) const = 0;
    virtual int bank( int ch ) const = 0;
            int maxBank( int ch ) const {return (nElecPerShank()-ch-1)/nAP();}
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
    virtual void locFltRadii( int &rin, int &rout, int iflt ) const = 0;    // iflt = {1,2}

    virtual void muxTable( int &nADC, int &nGrp, std::vector<int> &T ) const = 0;

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
    virtual int selectRefs( int slot, int port, int dock ) const;
    virtual int selectGains( int slot, int port, int dock ) const;
    virtual int selectAPFlts( int slot, int port, int dock ) const;

    static bool pnToType( int &type, const QString &pn );
    static IMROTbl* alloc( int type );
    static QString default_imroLE( int type );
};

#endif  // IMROTBL_H


