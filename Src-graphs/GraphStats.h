#ifndef GRAPHSTATS_H
#define GRAPHSTATS_H

#include <qglobal.h>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GraphStats
{
private:
    double  s1,
            s2;
    uint    num,
            maxInt;

public:
    GraphStats()                {clear(); maxInt = 1;}
    void clear()                {s1 = s2 = num = 0;}
    void setMaxInt( int imax )  {maxInt = imax;}
    inline void add( int v )    {s1 += v, s2 += v*v, ++num;}
    double mean() const {return (num > 1 ? s1/num : s1) / maxInt;}
    double rms() const;
    double stdDev() const;
};

#endif  // GRAPHSTATS_H


