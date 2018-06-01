#ifndef SGLTYPES_H
#define SGLTYPES_H

#include <qglobal.h>
#include <limits>
#include <vector>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define UNSET64 std::numeric_limits<qlonglong>::max()


typedef std::vector<qint16> vec_i16;


struct VRange {
    double  rmin,
            rmax;

    VRange() : rmin(0), rmax(0) {}
    VRange( double rmin, double rmax ) : rmin(rmin), rmax(rmax) {}

    double span() const
        {return rmax - rmin;}
    double unityToVolts( double u ) const
        {return u*(rmax - rmin) + rmin;}
    double voltsToUnity( double v ) const
        {return (v - rmin) / (rmax - rmin);}
    bool operator==( const VRange &rhs ) const
        {return rmin == rhs.rmin && rmax == rhs.rmax;}
    bool operator!=( const VRange &rhs ) const
        {return !(*this == rhs);}
};

#endif  // SGLTYPES_H


