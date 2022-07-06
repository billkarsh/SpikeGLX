#ifndef IMRO_EDIT_H
#define IMRO_EDIT_H

#include <QVector>

struct IMROTbl;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct IMRO_Struck {
    int s, c, r;
    IMRO_Struck() : s(0), c(0), r(0)                        {}
    IMRO_Struck( int s, int c, int r ) : s(s), c(c), r(r)   {}
    bool operator<( const IMRO_Struck &rhs ) const;
};

struct IMRO_ROI {
// if c0   <= 0  full left  included
// if cLim == -1 full right included
    int s, r0, rLim, c0, cLim;
    IMRO_ROI() : s(0), r0(0), rLim(384), c0(-1), cLim(-1)   {}
    IMRO_ROI( int s, int r0, int rLim, int c0 = -1, int cLim = -1 )
        :   s(s), r0(r0), rLim(rLim), c0(c0), cLim(cLim)    {}
};

class IMRO_Edit {
public:
    static int defaultROI( std::vector<IMRO_ROI> &R, const IMROTbl &T );
    static int imro2ROI( std::vector<IMRO_ROI> &R, const IMROTbl &T );
};

#endif  // IMRO_EDIT_H


