
#include "GraphStats.h"

#include <math.h>




double GraphStats::rms() const
{
    double  rms = 0.0;

    if( s2 > 0.0 ) {

        if( num > 1 )
            rms = s2 / num;

        rms = sqrt( rms );
    }

    return rms / 32768.0;
}


double GraphStats::stdDev() const
{
    double stddev = 0.0;

    if( num > 1 ) {

        double  var = (s2 - s1*s1/num) / (num - 1);

        if( var > 0.0 )
            stddev = sqrt( var );
    }

    return stddev / 32768.0;
}


