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
};

struct IMRO_ROI {
    int s, r0, rLim;
    IMRO_ROI() : s(0), r0(0), rLim(384)                             {}
    IMRO_ROI( int s, int r0, int rLim ) : s(s), r0(r0), rLim(rLim)  {}
};

class IMRO_Edit {
public:
    static int defaultROI( std::vector<IMRO_ROI> &R, const IMROTbl &T );
};

#endif  // IMRO_EDIT_H


