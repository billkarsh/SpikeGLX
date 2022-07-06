
#include "IMRO_Edit.h"
#include "IMROTbl.h"
#include "ShankMap.h"


/* ---------------------------------------------------------------- */
/* IMRO_Struck ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool IMRO_Struck::operator<( const IMRO_Struck &rhs ) const
{
    if( s < rhs.s )
        return true;

    if( s > rhs.s )
        return false;

    if( r < rhs.r )
        return true;

    if( r > rhs.r )
        return false;

    return c < rhs.c;
}

/* ---------------------------------------------------------------- */
/* IMRO_Edit ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Return ROI count.
//
int IMRO_Edit::defaultROI( std::vector<IMRO_ROI> &R, const IMROTbl &T )
{
    R.clear();
    R.push_back( IMRO_ROI( 0, 0, T.nAP() / T.nCol() ) );
    return 1;
}


// Scan up for ranges of consecutive like rows.
// Within range, scan across for contiguous 1s.
// That defines ROI boxes.
//
// Return ROI count.
//
int IMRO_Edit::imro2ROI( std::vector<IMRO_ROI> &R, const IMROTbl &T )
{
    R.clear();

    ShankMap    M;
    M.fillDefaultIm( T );
    qSort( M.e );   // s->r->c

    int nC = T.nCol();

    for( int ie = 0, ne = M.e.size(); ie < ne; ) {

        // Start pat0 and box row-range
        ShankMapDesc    &B0     = M.e[ie++];
        quint32         pat0    = 1 << B0.c;
        int             nrow    = 1;

        // Complete pat0
        while( ie < ne ) {
            ShankMapDesc    E = M.e[ie];
            if( E.s == B0.s && E.r == B0.r ) {
                pat0 |= 1 << E.c;
                ++ie;
            }
            else
                break;
        }

        // ie not yet included

        // Extend consecutive row-range?

        while( ie < ne ) {

            // Consecutive row?
            ShankMapDesc    X0 = M.e[ie];
            int             ix = ie + 1;
            if( X0.s == B0.s && X0.r == B0.r + nrow ) {

                // Start new patX and extension row
                quint32 patX = 1 << X0.c;

                // Complete patX
                while( ix < ne ) {
                    ShankMapDesc    X = M.e[ix];
                    if( X.s == X0.s && X.r == X0.r ) {
                        patX |= 1 << X.c;
                        ++ix;
                    }
                    else
                        break;
                }

                // Successful extension?
                if( patX == pat0 ) {
                    ++nrow;
                    ie = ix;
                }
                else
                    break;
            }
            else
                break;
        }

        // End range extension

        // Split into boxes

        IMRO_ROI    roi( B0.s, B0.r, B0.r + nrow, -1, -1 );

        // Note: c == nC kicks out last
        for( int c = 0; c <= nC; ++c ) {

            if( pat0 & (1 << c) ) {
                if( roi.c0 < 0 )
                    roi.c0 = c;
                roi.cLim = c + 1;
            }
            else if( roi.c0 >= 0 ) {
                R.push_back( roi );
                roi.c0 = -1;
            }
        }
    }

    return R.size();
}


