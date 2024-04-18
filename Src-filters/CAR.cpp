
#include "CAR.h"
#include "GeomMap.h"
#include "ShankMap.h"
#include "IMROTbl.h"

#include <QSet>


/* ---------------------------------------------------------------- */
/* Median_1 ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CAR::Median_1::init( const SUList &SU, int nAP )
{
    const SUElem    *su = &SU[0];

    idx.clear();

    for( int ig = 0; ig < nAP; ++ig ) {
        if( su[ig].u )
            idx.push_back( ig );
    }

    arrange.resize( nU = idx.size() );
    ibeg = arrange.begin();
    iend = arrange.end();
    imid = ibeg + nU / 2;
}


void CAR::Median_1::apply( qint16 *d, int ntpts, int nC ) const
{
    const int   *pidx = &idx[0];
    qint16      *parr = &arrange[0];

    if( nU <= 0 )
        return;

    for( int it = 0; it < ntpts; ++it, d += nC ) {

        for( int ig = 0; ig < nU; ++ig )
            parr[ig] = d[pidx[ig]];

        std::nth_element( ibeg, imid, iend );
        int median = *imid;

        for( int ig = 0; ig < nU; ++ig )
            d[pidx[ig]] -= median;
    }
}

/* ---------------------------------------------------------------- */
/* Median_4 ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void CAR::Median_4::init( const SUList &SU, int nAP )
{
    const SUElem    *su = &SU[0];

    idx.clear();
    idx.resize( 4 );

    for( int ig = 0; ig < nAP; ++ig ) {
        if( su[ig].u )
            idx[su[ig].s].push_back( ig );
    }

    arrange.resize( 4 );

    for( int is = 0; is < 4; ++is ) {
        arrange[is].resize( nU[is] = idx[is].size() );
        ibeg[is] = arrange[is].begin();
        iend[is] = arrange[is].end();
        imid[is] = ibeg[is] + nU[is] / 2;
    }
}


void CAR::Median_4::apply( qint16 *d, int ntpts, int nC ) const
{
    for( int it = 0; it < ntpts; ++it, d += nC ) {

        for( int is = 0; is < 4; ++is ) {

            const int   *pidx = &idx[is][0];
            qint16      *parr = &arrange[is][0];
            int         nu    = nU[is];

            for( int ig = 0; ig < nu; ++ig )
                parr[ig] = d[pidx[ig]];

            std::nth_element( ibeg[is], imid[is], iend[is] );
            int median = *imid[is];

            for( int ig = 0; ig < nu; ++ig )
                d[pidx[ig]] -= median;
        }
    }
}

/* ---------------------------------------------------------------- */
/* CAR ------------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void CAR::setAuto( const IMROTbl *R )
{
    setSepShanks( R->type == 2020 ? 4 : 0 );
    setMuxTbl( R );
}


void CAR::setChans( int _nC, int _nAP, int _dwnSmp )
{
    nC      = _nC;
    nAP     = _nAP;
    dwnSmp  = _dwnSmp;
}


void CAR::setSU( const GeomMap *map )
{
    SU.clear();

    for( int ie = 0, ne = map->e.size(); ie < ne; ++ie ) {
        const GeomMapDesc   &E = map->e[ie];
        SU.push_back( SUElem( E.s, E.u ) );
    }
}


void CAR::setSU( const ShankMap *map )
{
    SU.clear();

    for( int ie = 0, ne = map->e.size(); ie < ne; ++ie ) {
        const ShankMapDesc  &E = map->e[ie];
        SU.push_back( SUElem( E.s, E.u ) );
    }
}


void CAR::setMuxTbl( const IMROTbl *R )
{
    R->muxTable( nADC, nGrp, muxTbl );
}


// For each channel [0,nAP), calculate an 8-way
// neighborhood of indices into a timepoint's channels.
// - Inherently per shank.
// - Annulus with {inner^2, outer^2} radii {id2, od2}.
// - The list is sorted for cache friendliness.
// Dependencies:
// {setChans}
//
void CAR::lcl_init( const GeomMap *map, double id2, double od2, bool bloc_tmp )
{
    neib.clear();

    if( nAP <= 0 )
        return;

    neib.resize( nAP );

    if( bloc_tmp )
        loc_tmp.resize( nC );

    for( int ig = 0; ig < nAP; ++ig ) {

        const GeomMapDesc   &E = map->e[ig];

        if( !E.u )
            continue;

        // ----------------------------------
        // Form set of excluded inner indices
        // ----------------------------------

        QSet<int>   inner;

        for( int ie = 0; ie < nAP; ++ie ) {

            if( ie == ig )
                continue;

            const GeomMapDesc   &e = map->e[ie];

            if( e.u && e.s == E.s ) {

                double  dx = e.x - E.x,
                        dz = e.z - E.z;

                if( dx*dx + dz*dz <= id2 )
                    inner.insert( ie );
            }
        }

        // -------------------------
        // Fill with annulus members
        // -------------------------

        std::vector<int>    &V = neib[ig];

        for( int ie = 0; ie < nAP; ++ie ) {

            if( ie == ig )
                continue;

            const GeomMapDesc   &e = map->e[ie];

            if( e.u && e.s == E.s ) {

                double  dx = e.x - E.x,
                        dz = e.z - E.z;

                if( dx*dx + dz*dz <= od2 && !inner.contains( ie ) )
                    V.push_back( ie );
            }
        }

        qSort( V );
    }
}


// For each channel [0,nAP), calculate an 8-way
// neighborhood of indices into a timepoint's channels.
// - Inherently per shank.
// - Annulus with {inner, outer} radii {id, od}.
// - The list is sorted for cache friendliness.
// Dependencies:
// {setChans}
//
void CAR::lcl_init( const ShankMap *map, int id, int od, bool bloc_tmp )
{
    neib.clear();

    if( nAP <= 0 )
        return;

    neib.resize( nAP );

    if( bloc_tmp )
        loc_tmp.resize( nC );

    QMap<ShankMapDesc,uint> ISM;
    map->inverseMap( ISM );

    for( int ig = 0; ig < nAP; ++ig ) {

        const ShankMapDesc  &E = map->e[ig];

        if( !E.u )
            continue;

        // ----------------------------------
        // Form set of excluded inner indices
        // ----------------------------------

        QSet<int>   inner;

        int xL  = qMax( int(E.c)  - id, 0 ),
            xH  = qMin( uint(E.c) + id + 1, map->nc ),
            yL  = qMax( int(E.r)  - id, 0 ),
            yH  = qMin( uint(E.r) + id + 1, map->nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it != ISM.end() )
                    inner.insert( it.value() );
            }
        }

        // -------------------------
        // Fill with annulus members
        // -------------------------

        std::vector<int>    &V = neib[ig];

        xL  = qMax( int(E.c)  - od, 0 );
        xH  = qMin( uint(E.c) + od + 1, map->nc );
        yL  = qMax( int(E.r)  - od, 0 );
        yH  = qMin( uint(E.r) + od + 1, map->nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it != ISM.end() ) {

                    int i = it.value();

                    // Exclude inners

                    if( !inner.contains( i ) )
                        V.push_back( i );
                }
            }
        }

        qSort( V );
    }
}


// Local space averaging for value: d_ig = &data[ig].
// Dependencies:
// {lcl_init}
//
int CAR::lcl_1( const qint16 *d_ig, int ig ) const
{
    const std::vector<int>  &V = neib[ig];

    int nv = V.size();

    if( nv ) {

        const qint16    *d  = d_ig - ig;
        const int       *v  = &V[0];
        int             sum = 0;

        for( int iv = 0; iv < nv; ++iv )
            sum += d[v[iv]];

        return *d_ig - sum/nv;
    }

    return *d_ig;
}


// Local space averaging for all values, inherently per shank.
// Dependencies:
// {setChans, setSU, lcl_init}
//
void CAR::lcl_auto(
    qint16          *d,
    int             ntpts,
    QVector<int>    &ig2ic,
    QVector<int>    &ic2ig ) const
{
    const SUElem    *su   = &SU[0];
    qint16          *tmp  = &loc_tmp[0];
    int             icMax = ig2ic[nAP-1];    // highest acquired channel saved

    for( int it = 0; it < ntpts; ++it, d += nC ) {

        memcpy( tmp, d, nC*sizeof(qint16) );

        for( int ic = 0; ic <= icMax; ++ic ) {
            int ig = ic2ig[ic];
            if( ig >= 0 && su[ig].u )
                tmp[ig] = lcl_1( d + ig, ig );
        }

        memcpy( d, tmp, nC*sizeof(qint16) );
    }
}


// Global median.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_med_auto_init()
{
    if( nS )
        med4.init( SU, nAP );
    else
        med1.init( SU, nAP );
}


// Global median.
// Dependencies:
// {setSepShanks, setChans, setSU, med_all_init()}
//
void CAR::gbl_med_auto( qint16 *d, int ntpts ) const
{
    if( nS )
        med4.apply( d, ntpts, nC );
    else
        med1.apply( d, ntpts, nC );
}


// Global median, all shanks.
// Dependencies:
// {setChans, setSU}
//
void CAR::gbl_med_all_init()
{
    med1.init( SU, nAP );
}


// Global median, all shanks.
// Dependencies:
// {setChans, setSU, med_all_init()}
//
void CAR::gbl_med_all( qint16 *d, int ntpts ) const
{
    med1.apply( d, ntpts, nC );
}


// Global median, per shank.
// Dependencies:
// {setChans, setSU}
//
void CAR::gbl_med_shk_init()
{
    med4.init( SU, nAP );
}


// Global median, per shank.
// Dependencies:
// {setChans, setSU, med_shk_init()}
//
void CAR::gbl_med_shk( qint16 *d, int ntpts ) const
{
    med4.apply( d, ntpts, nC );
}


// Global average.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_ave_auto( qint16 *d, int ntpts ) const
{
    if( nS )
        gbl_ave_shk( d, ntpts );
    else
        gbl_ave_all( d, ntpts );
}


// Global average, all shanks.
// Dependencies:
// {setChans, setSU}
//
void CAR::gbl_ave_all( qint16 *d, int ntpts ) const
{
    if( nAP <= 0 )
        return;

    const SUElem    *su     = &SU[0];
    int             dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        double  S = 0;
        int     A = 0,
                N = 0;

        for( int ig = 0; ig < nAP; ++ig ) {
            if( su[ig].u ) {
                S += d[ig];
                ++N;
            }
        }

        if( N > 1 )
            A = int(S / N);

        for( int ig = 0; ig < nAP; ++ig ) {
            if( su[ig].u )
                d[ig] -= A;
        }
    }
}


// Global average, per shank.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_ave_shk( qint16 *d, int ntpts ) const
{
    if( nAP <= 0 )
        return;

    const SUElem        *su     = &SU[0];
    std::vector<int>    _A( nS ),
                        _N( nS );
    std::vector<float>  _S( nS );
    int                 *A      = &_A[0],
                        *N      = &_N[0];
    float               *S      = &_S[0];
    int                 dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int is = 0; is < nS; ++is ) {
            S[is] = 0;
            N[is] = 0;
            A[is] = 0;
        }

        for( int ig = 0; ig < nAP; ++ig ) {
            const SUElem    *e = &su[ig];
            if( e->u ) {
                S[e->s] += d[ig];
                ++N[e->s];
            }
        }

        for( int is = 0; is < nS; ++is ) {
            if( N[is] > 1 )
                A[is] = int(S[is] / N[is]);
        }

        for( int ig = 0; ig < nAP; ++ig ) {
            const SUElem    *e = &su[ig];
            if( e->u )
                d[ig] -= A[e->s];
        }
    }
}


// Global demuxed average.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_dmx_stride_auto( qint16 *d, int ntpts, int stride ) const
{
    if( nS )
        gbl_dmx_stride_shk( d, ntpts, stride );
    else
        gbl_dmx_stride_all( d, ntpts, stride );
}


// Global demuxed average, all shanks.
// Dependencies:
// {setChans, setSU}
//
void CAR::gbl_dmx_stride_all( qint16 *d, int ntpts, int stride ) const
{
    if( nAP <= 0 )
        return;

    const SUElem    *su   = &SU[0];
    int             dStep = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic0 = 0; ic0 < stride; ++ic0 ) {

            double  S = 0;
            int     A = 0,
                    N = 0;

            for( int ic = ic0; ic <= nAP; ic += stride ) {
                if( su[ic].u ) {
                    S += d[ic];
                    ++N;
                }
            }

            if( N > 1 )
                A = int(S / N);

            for( int ic = ic0; ic <= nAP; ic += stride ) {
                if( su[ic].u )
                    d[ic] -= A;
            }
        }
    }
}


// Global demuxed average, per shank.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_dmx_stride_shk( qint16 *d, int ntpts, int stride ) const
{
    if( nAP <= 0 )
        return;

    const SUElem        *su     = &SU[0];
    std::vector<int>    _A( nS ),
                        _N( nS );
    std::vector<float>  _S( nS );
    int                 *A      = &_A[0],
                        *N      = &_N[0];
    float               *S      = &_S[0];
    int                 dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic0 = 0; ic0 < stride; ++ic0 ) {

            for( int is = 0; is < nS; ++is ) {
                S[is] = 0;
                N[is] = 0;
                A[is] = 0;
            }

            for( int ic = ic0; ic <= nAP; ic += stride ) {
                const SUElem    *e = &su[ic];
                if( e->u ) {
                    S[e->s] += d[ic];
                    ++N[e->s];
                }
            }

            for( int is = 0; is < nS; ++is ) {
                if( N[is] > 1 )
                    A[is] = int(S[is] / N[is]);
            }

            for( int ic = ic0; ic <= nAP; ic += stride ) {
                const SUElem    *e = &su[ic];
                if( e->u )
                    d[ic] -= A[e->s];
            }
        }
    }
}


// Global demuxed average.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_dmx_stride_auto(
    qint16          *d,
    int             ntpts,
    int             stride,
    QVector<int>    &ig2ic,
    QVector<int>    &ic2ig ) const
{
    if( nS )
        gbl_dmx_stride_shk( d, ntpts, stride, ig2ic, ic2ig );
    else
        gbl_dmx_stride_all( d, ntpts, stride, ig2ic, ic2ig );
}


// Global demuxed average, all shanks.
// Dependencies:
// {setChans, setSU}
//
void CAR::gbl_dmx_stride_all(
    qint16          *d,
    int             ntpts,
    int             stride,
    QVector<int>    &ig2ic,
    QVector<int>    &ic2ig ) const
{
    if( nAP <= 0 )
        return;

    const SUElem    *su   = &SU[0];
    int             icMax = ig2ic[nAP-1],    // highest acquired channel saved
                    dStep = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic0 = 0; ic0 < stride; ++ic0 ) {

            double  S = 0;
            int     A = 0,
                    N = 0;

            for( int ic = ic0; ic <= icMax; ic += stride ) {
                int ig = ic2ig[ic];
                if( ig >= 0 && su[ig].u ) {
                    S += d[ig];
                    ++N;
                }
            }

            if( N > 1 )
                A = int(S / N);

            for( int ic = ic0; ic <= icMax; ic += stride ) {
                int ig = ic2ig[ic];
                if( ig >= 0 && su[ig].u )
                    d[ig] -= A;
            }
        }
    }
}


// Global demuxed average, per shank.
// Dependencies:
// {setSepShanks, setChans, setSU}
//
void CAR::gbl_dmx_stride_shk(
    qint16          *d,
    int             ntpts,
    int             stride,
    QVector<int>    &ig2ic,
    QVector<int>    &ic2ig ) const
{
    if( nAP <= 0 )
        return;

    const SUElem        *su     = &SU[0];
    std::vector<int>    _A( nS ),
                        _N( nS );
    std::vector<float>  _S( nS );
    int                 *A      = &_A[0],
                        *N      = &_N[0];
    float               *S      = &_S[0];
    int                 icMax   = ig2ic[nAP-1],    // highest acquired channel saved
                        dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic0 = 0; ic0 < stride; ++ic0 ) {

            for( int is = 0; is < nS; ++is ) {
                S[is] = 0;
                N[is] = 0;
                A[is] = 0;
            }

            for( int ic = ic0; ic <= icMax; ic += stride ) {
                int ig = ic2ig[ic];
                if( ig >= 0 ) {
                    const SUElem    *e = &su[ig];
                    if( e->u ) {
                        S[e->s] += d[ig];
                        ++N[e->s];
                    }
                }
            }

            for( int is = 0; is < nS; ++is ) {
                if( N[is] > 1 )
                    A[is] = int(S[is] / N[is]);
            }

            for( int ic = ic0; ic <= icMax; ic += stride ) {
                int ig = ic2ig[ic];
                if( ig >= 0 ) {
                    const SUElem    *e = &su[ig];
                    if( e->u )
                        d[ig] -= A[e->s];
                }
            }
        }
    }
}


// Global demuxed average.
// Dependencies:
// {setSepShanks, setChans, setSU, setMuxTbl}
//
void CAR::gbl_dmx_tbl_auto( qint16 *d, int ntpts ) const
{
    if( nS )
        gbl_dmx_tbl_shk( d, ntpts );
    else
        gbl_dmx_tbl_all( d, ntpts );
}


// Global demuxed average, all shanks.
// Dependencies:
// {setChans, setSU, setMuxTbl}
//
void CAR::gbl_dmx_tbl_all( qint16 *d, int ntpts ) const
{
    if( nAP <= 0 )
        return;

    const SUElem    *su     = &SU[0];
    const int       *T      = &muxTbl[0];
    int             dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int irow = 0; irow < nGrp; ++irow ) {

            double  S = 0;
            int     A = 0,
                    N = 0;

            for( int icol = 0; icol < nADC; ++icol ) {
                int ic = T[nADC*irow + icol];
                if( ic < nAP ) {
                    if( su[ic].u ) {
                        S += d[ic];
                        ++N;
                    }
                }
                else
                    break;
            }

            if( N > 1 )
                A = int(S / N);

            for( int icol = 0; icol < nADC; ++icol ) {
                int ic = T[nADC*irow + icol];
                if( ic < nAP ) {
                    if( su[ic].u )
                        d[ic] -= A;
                }
                else
                    break;
            }
        }
    }
}


// Global demuxed average, per shank.
// Dependencies:
// {setSepShanks, setChans, setSU, setMuxTbl}
//
void CAR::gbl_dmx_tbl_shk( qint16 *d, int ntpts ) const
{
    if( nAP <= 0 )
        return;

    const SUElem        *su     = &SU[0];
    const int           *T      = &muxTbl[0];
    std::vector<int>    _A( nS ),
                        _N( nS );
    std::vector<float>  _S( nS );
    int                 *A      = &_A[0],
                        *N      = &_N[0];
    float               *S      = &_S[0];
    int                 dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int irow = 0; irow < nGrp; ++irow ) {

            for( int is = 0; is < nS; ++is ) {
                S[is] = 0;
                N[is] = 0;
                A[is] = 0;
            }

            for( int icol = 0; icol < nADC; ++icol ) {
                int ic = T[nADC*irow + icol];
                if( ic < nAP ) {
                    const SUElem    *e = &su[ic];
                    if( e->u ) {
                        S[e->s] += d[ic];
                        ++N[e->s];
                    }
                }
                else
                    break;
            }

            for( int is = 0; is < nS; ++is ) {
                if( N[is] > 1 )
                    A[is] = int(S[is] / N[is]);
            }

            for( int icol = 0; icol < nADC; ++icol ) {
                int ic = T[nADC*irow + icol];
                if( ic < nAP ) {
                    const SUElem    *e = &su[ic];
                    if( e->u )
                        d[ic] -= A[e->s];
                }
                else
                    break;
            }
        }
    }
}


// Global demuxed average.
// Dependencies:
// {setSepShanks, setChans, setSU, setMuxTbl}
//
// Note that ic2ig[] must have size >= nGrp * nADC.
//
void CAR::gbl_dmx_tbl_auto( qint16 *d, int ntpts, QVector<int> &ic2ig ) const
{
    if( nS )
        gbl_dmx_tbl_shk( d, ntpts, ic2ig );
    else
        gbl_dmx_tbl_all( d, ntpts, ic2ig );
}


// Global demuxed average, all shanks.
// Dependencies:
// {setChans, setSU, setMuxTbl}
//
// Note that ic2ig[] must have size >= nGrp * nADC.
//
void CAR::gbl_dmx_tbl_all( qint16 *d, int ntpts, QVector<int> &ic2ig ) const
{
    if( nAP <= 0 )
        return;

    const SUElem    *su     = &SU[0];
    const int       *T      = &muxTbl[0];
    int             dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int irow = 0; irow < nGrp; ++irow ) {

            double  S = 0;
            int     A = 0,
                    N = 0;

            for( int icol = 0; icol < nADC; ++icol ) {
                int ig = ic2ig[T[nADC*irow + icol]];
                if( ig >= 0 && su[ig].u ) {
                    S += d[ig];
                    ++N;
                }
            }

            if( N > 1 )
                A = int(S / N);

            for( int icol = 0; icol < nADC; ++icol ) {
                int ig = ic2ig[T[nADC*irow + icol]];
                if( ig >= 0 && su[ig].u )
                    d[ig] -= A;
            }
        }
    }
}


// Global demuxed average, per shank.
// Dependencies:
// {setSepShanks, setChans, setSU, setMuxTbl}
//
// Note that ic2ig[] must have size >= nGrp * nADC.
//
void CAR::gbl_dmx_tbl_shk( qint16 *d, int ntpts, QVector<int> &ic2ig ) const
{
    if( nAP <= 0 )
        return;

    const SUElem        *su     = &SU[0];
    const int           *T      = &muxTbl[0];
    std::vector<int>    _A( nS ),
                        _N( nS );
    std::vector<float>  _S( nS );
    int                 *A      = &_A[0],
                        *N      = &_N[0];
    float               *S      = &_S[0];
    int                 dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int irow = 0; irow < nGrp; ++irow ) {

            for( int is = 0; is < nS; ++is ) {
                S[is] = 0;
                N[is] = 0;
                A[is] = 0;
            }

            for( int icol = 0; icol < nADC; ++icol ) {
                int ig = ic2ig[T[nADC*irow + icol]];
                if( ig >= 0 ) {
                    const SUElem    *e = &su[ig];
                    if( e->u ) {
                        S[e->s] += d[ig];
                        ++N[e->s];
                    }
                }
            }

            for( int is = 0; is < nS; ++is ) {
                if( N[is] > 1 )
                    A[is] = int(S[is] / N[is]);
            }

            for( int icol = 0; icol < nADC; ++icol ) {
                int ig = ic2ig[T[nADC*irow + icol]];
                if( ig >= 0 ) {
                    const SUElem    *e = &su[ig];
                    if( e->u )
                        d[ig] -= A[e->s];
                }
            }
        }
    }
}


