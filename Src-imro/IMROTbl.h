#ifndef IMROTBL_H
#define IMROTBL_H

#include <QString>

#include <vector>

struct GeomMap;
struct ShankMap;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Editing helper - columns in hwr coords
//
struct IMRO_Site {
    int s, c, r;
    IMRO_Site() : s(0), c(0), r(0)                        {}
    IMRO_Site( int s, int c, int r ) : s(s), c(c), r(r)   {}
    bool operator<( const IMRO_Site &rhs ) const;
};

// Editing helper - columns in hwr coords
//
struct IMRO_ROI {
// if c0   <= 0  full left  included
// if cLim == -1 full right included
    int s, r0, rLim, c0, cLim;
    IMRO_ROI() : s(0), r0(0), rLim(0), c0(-1), cLim(-1)     {}
    IMRO_ROI( int s, int r0, int rLim, int c0 = -1, int cLim = -1 )
        :   s(s), r0(r0), rLim(rLim), c0(c0), cLim(cLim)    {}
    bool operator<( const IMRO_ROI &rhs ) const;
};

// Editing helper
//
struct IMRO_GUI {
    std::vector<int>    gains;
    int                 grid;
    bool                apEnab,
                        lfEnab,
                        hpEnab;
    IMRO_GUI() : grid(1), apEnab(false), lfEnab(false), hpEnab(false)   {}
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
    friend class ShankView;

protected:
    std::vector<int>    col2vis_ev,
                        col2vis_od;
    float               _shankpitch,
                        _shankwid,
                        _x0_ev,
                        _x0_od,
                        _xpitch,
                        _zpitch;
    int                 _ncolhwr,
                        _ncolvis;
public:
    QString             pn;
    int                 type;

    IMROTbl( const QString &pn, int type );
    virtual ~IMROTbl()  {}

    virtual void copyFrom( const IMROTbl *rhs ) = 0;
    virtual void fillDefault() = 0;
    virtual void fillShankAndBank( int shank, int bank ) = 0;

    virtual int nElec() const = 0;
    virtual int nShank() const = 0;
            int nElecPerShank() const   {return nElec()/nShank();}
            int nCol_hwr() const        {return _ncolhwr;}
            int nCol_vis() const        {return _ncolvis;}
            int nRow() const            {return nElecPerShank()/_ncolhwr;}
    virtual int nChan() const = 0;
    virtual int nAP() const = 0;
    virtual int nLF() const = 0;
            int nSY() const             {return 1;}
    virtual int nBanks() const = 0;
    virtual int nRefs() const = 0;
    virtual int maxInt() const = 0;
    virtual double maxVolts() const = 0;
    virtual bool needADCCal() const = 0;

    // {0=NP1000, 1=NP2000, 2=NP2010, 3=NP1110}-like
    virtual int chanMapping() const     {return 0;}

    // {0=NP1000, 2=NP2000}-like
    virtual int apiFetchType() const    {return 0;}

    virtual bool operator==( const IMROTbl &rhs ) const = 0;
    virtual bool operator!=( const IMROTbl &rhs ) const = 0;

    virtual bool isConnectedSame( const IMROTbl *rhs ) const = 0;

    virtual QString toString() const = 0;
    virtual bool fromString( QString *msg, const QString &s ) = 0;

    virtual bool loadFile( QString &msg, const QString &path ) = 0;
    virtual bool saveFile( QString &msg, const QString &path ) const = 0;

    void toShankMap_hwr( ShankMap &S ) const;
    void toShankMap_vis( ShankMap &S ) const;
    void toShankMap_snsFileChans(
        ShankMap            &S,
        const QVector<uint> &saved,
        int                 offset ) const;
    void toGeomMap_snsFileChans(
        GeomMap             &G,
        const QVector<uint> &saved,
        int                 offset ) const;
    void andOutRefs( ShankMap &S ) const;

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
    QString muxTable_toString() const;

// Hardware

    virtual int selectSites( int slot, int port, int dock, bool write ) const;
    virtual int selectRefs( int slot, int port, int dock ) const;
    virtual int selectGains( int slot, int port, int dock ) const;
    virtual int selectAPFlts( int slot, int port, int dock ) const;

// Edit

    virtual bool edit_able() const              {return false;}
    virtual void edit_init() const              {}
    virtual IMRO_GUI edit_GUI() const           {return IMRO_GUI();}
    virtual IMRO_Attr edit_Attr_def() const     {return IMRO_Attr();}
    virtual IMRO_Attr edit_Attr_cur() const     {return IMRO_Attr();}
    virtual bool edit_Attr_canonical() const    {return false;}
    virtual void edit_exclude_1( tImroSites vX, const IMRO_Site &s ) const
        {vX.clear(); Q_UNUSED( s )}
    virtual void edit_ROI2tbl( tconstImroROIs vR, const IMRO_Attr &A )
        {Q_UNUSED( vR ) Q_UNUSED( A )}
    int edit_defaultROI( tImroROIs vR ) const;
    bool edit_isCanonical( tconstImroROIs vR ) const;
    int edit_tbl2ROI( tImroROIs vR ) const;
    void edit_exclude( tImroSites vX, tconstImroROIs vR ) const;
    bool edit_isAllowed( tconstImroSites vX, const IMRO_ROI &B ) const;

// Allocate

    static bool pnToType( int &type, const QString &pn );
    static IMROTbl* alloc( const QString &pn );
    static QString default_imroLE( int type );
};

#endif  // IMROTBL_H


