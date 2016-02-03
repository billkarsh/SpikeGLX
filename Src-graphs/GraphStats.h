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
    uint    num;

public:
    GraphStats()                {clear();}
    void clear()                {s1 = s2 = num = 0;}
    inline void add( double v ) {s1 += v, s2 += v*v, ++num;}
    double mean() const {return (num > 1 ? s1/num : s1) / 32768.0;}
    double rms() const;
    double stdDev() const;
};

#endif  // GRAPHSTATS_H


