#ifndef CAR_H
#define CAR_H

#include "SGLTypes.h"

struct GeomMap;
struct ShankMap;
struct IMROTbl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SUElem {
// The common parts of GeomMap & ShankMap.
// This device allows unification of code whichever map is loaded.
    qint16 s, u;
    SUElem( qint16 s, qint16 u ) : s(s), u(u)   {}
};

typedef std::vector<SUElem> SUList;

class CAR
{
private:
    struct Median_1 {
        std::vector<int>    idx;
        mutable vec_i16     arrange;
        vec_i16::iterator   ibeg, imid, iend;
        int                 nU;
    public:
        Median_1()          {}
        virtual ~Median_1() {}
        void init( const SUList &SU, int nAP );
        void apply( qint16 *d, int ntpts, int nC ) const;
    };

    struct Median_4 {
        std::vector<std::vector<int> >  idx;
        mutable std::vector<vec_i16 >   arrange;
        vec_i16::iterator               ibeg[4], imid[4], iend[4];
        int                             nU[4];
    public:
        Median_4()          {}
        virtual ~Median_4() {}
        void init( const SUList &SU, int nAP );
        void apply( qint16 *d, int ntpts, int nC ) const;
    };

    SUList                          SU;
    Median_1                        med1;
    Median_4                        med4;
    std::vector<std::vector<int> >  neib;
    mutable std::vector<qint16>     loc_tmp;
    std::vector<int>                muxTbl;
    int                             nADC,
                                    nGrp,
                                    nS,     // #shanks, 0=ignore shanks
                                    nC,
                                    nAP,
                                    dwnSmp;

public:
    CAR() : nS(0)   {}
    virtual ~CAR()  {}

    void setAuto( const IMROTbl *R );
    void setSepShanks( int _nS )    {nS = _nS;}
    void setChans( int _nC, int _nAP, int _dwnSmp = 1 );
    void setSU( const GeomMap *map );
    void setSU( const ShankMap *map );
    const SUList &getSU() const     {return SU;}
    void setMuxTbl( const IMROTbl *R );
    const int* getMuxTbl( int &nADC, int &nGrp ) const;
    int getMuxTblSize() const       {return nADC * nGrp;}

/* ---------------------------------------------------------------- */
/* Local CAR is inherently per shank ------------------------------ */
/* ---------------------------------------------------------------- */

    void lcl_init( const GeomMap *map, double id2, double od2, bool bloc_tmp );
    void lcl_init( const ShankMap *map, int id, int od, bool bloc_tmp );
    int lcl_1( const qint16 *d_ig, int ig ) const;
    void lcl_auto(
        qint16          *d,
        int             ntpts,
        QVector<int>    &ig2ic,
        QVector<int>    &ic2ig ) const;

/* ---------------------------------------------------------------- */
/* Global median -------------------------------------------------- */
/* ---------------------------------------------------------------- */

    void gbl_med_auto_init();
    void gbl_med_auto( qint16 *d, int ntpts ) const;

    void gbl_med_all_init();
    void gbl_med_all( qint16 *d, int ntpts ) const;

    void gbl_med_shk_init();
    void gbl_med_shk( qint16 *d, int ntpts ) const;

/* ---------------------------------------------------------------- */
/* Global average ------------------------------------------------- */
/* ---------------------------------------------------------------- */

    void gbl_ave_auto( qint16 *d, int ntpts ) const;
    void gbl_ave_all( qint16 *d, int ntpts ) const;
    void gbl_ave_shk( qint16 *d, int ntpts ) const;

/* ---------------------------------------------------------------- */
/* Global demux stride -------------------------------------------- */
/* ---------------------------------------------------------------- */

    void gbl_dmx_stride_auto( qint16 *d, int ntpts, int stride ) const;
    void gbl_dmx_stride_all( qint16 *d, int ntpts, int stride ) const;
    void gbl_dmx_stride_shk( qint16 *d, int ntpts, int stride ) const;

    void gbl_dmx_stride_auto(
        qint16          *d,
        int             ntpts,
        int             stride,
        QVector<int>    &ig2ic,
        QVector<int>    &ic2ig ) const;
    void gbl_dmx_stride_all(
        qint16          *d,
        int             ntpts,
        int             stride,
        QVector<int>    &ig2ic,
        QVector<int>    &ic2ig ) const;
    void gbl_dmx_stride_shk(
        qint16          *d,
        int             ntpts,
        int             stride,
        QVector<int>    &ig2ic,
        QVector<int>    &ic2ig ) const;

/* ---------------------------------------------------------------- */
/* Global demux table --------------------------------------------- */
/* ---------------------------------------------------------------- */

    void gbl_dmx_tbl_auto( qint16 *d, int ntpts ) const;
    void gbl_dmx_tbl_all( qint16 *d, int ntpts ) const;
    void gbl_dmx_tbl_shk( qint16 *d, int ntpts ) const;

    void gbl_dmx_tbl_auto( qint16 *d, int ntpts, QVector<int> &ic2ig ) const;
    void gbl_dmx_tbl_all( qint16 *d, int ntpts, QVector<int> &ic2ig ) const;
    void gbl_dmx_tbl_shk( qint16 *d, int ntpts, QVector<int> &ic2ig ) const;
};

#endif  // CAR_H


