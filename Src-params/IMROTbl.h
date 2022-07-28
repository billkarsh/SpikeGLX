#ifndef IMROTBL_H
#define IMROTBL_H

#include <qglobal.h>

#include <vector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Editing helper
//
struct IMRO_Site {
    int s, c, r;
    IMRO_Site() : s(0), c(0), r(0)                        {}
    IMRO_Site( int s, int c, int r ) : s(s), c(c), r(r)   {}
    bool operator<( const IMRO_Site &rhs ) const;
};

// Editing helper
//
struct IMRO_ROI {
// if c0   <= 0  full left  included
// if cLim == -1 full right included
    int s, r0, rLim, c0, cLim;
    IMRO_ROI() : s(0), r0(0), rLim(0), c0(-1), cLim(-1)     {}
    IMRO_ROI( int s, int r0, int rLim, int c0 = -1, int cLim = -1 )
        :   s(s), r0(r0), rLim(rLim), c0(c0), cLim(cLim)    {}
};

// Editing helper
//
struct IMRO_GUI {
    std::vector<int>    gains;
    bool                apEnab,
                        lfEnab,
                        hpEnab;
    IMRO_GUI() : apEnab(false), lfEnab(false), hpEnab(false)    {}
};

// Editing helper
//
struct IMRO_Attr {
// -1 = unimpl
    int refIdx,
        apgIdx,
        lfgIdx,
        hpfIdx;
    IMRO_Attr() : refIdx(-1), apgIdx(-1), lfgIdx(-1), hpfIdx(-1)    {}
    IMRO_Attr( int refIdx, int apgIdx, int lfgIdx, int hpfIdx )
        :   refIdx(refIdx), apgIdx(apgIdx),
            lfgIdx(lfgIdx), hpfIdx(hpfIdx)                          {}
};

typedef std::vector<IMRO_Site>&         tImroSites;
typedef const std::vector<IMRO_Site>&   tconstImroSites;
typedef std::vector<IMRO_ROI>&          tImroROIs;
typedef const std::vector<IMRO_ROI>&    tconstImroROIs;

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
    virtual int maxBank( int ch, int shank = 0 ) const;
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

// Hardware

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
    virtual int selectRefs( int slot, int port, int dock ) const;
    virtual int selectGains( int slot, int port, int dock ) const;
    virtual int selectAPFlts( int slot, int port, int dock ) const;

// Edit

    virtual bool edit_able() const          {return false;}
    virtual void edit_init() const          {}
    virtual IMRO_GUI edit_GUI() const       {return IMRO_GUI();}
    virtual IMRO_Attr edit_Attr_def() const {return IMRO_Attr();}
    virtual IMRO_Attr edit_Attr_cur() const {return IMRO_Attr();}
    virtual void edit_strike_1( tImroSites vS, const IMRO_Site &s ) const
        {vS.clear(); Q_UNUSED( s )}
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
        {Q_UNUSED( vR ) Q_UNUSED( A ) }
    int edit_defaultROI( tImroROIs vR ) const;
    int edit_tbl2ROI( tImroROIs vR ) const;
    bool edit_isCanonical( tconstImroROIs vR ) const;
    void edit_strike( tImroSites vS, tconstImroROIs vR ) const;
    bool edit_isAllowed( tconstImroSites vS, const IMRO_ROI &B ) const;

// Allocate

    static bool pnToType( int &type, const QString &pn );
    static IMROTbl* alloc( int type );
    static QString default_imroLE( int type );
};

#endif  // IMROTBL_H


