#ifndef HEATMAP_H
#define HEATMAP_H

#include "SGLTypes.h"

namespace DAQ {
struct Params;
}

class Biquad;
class DataFile;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Heatmap
{
private:
    double              srate,
                        VMAX,
                        niGain,
                        nSmp;
    std::vector<double> vsum;
    std::vector<int>    vmin,
                        vmax,
                        vapg,
                        vlfg;
    Biquad              *aphipass,
                        *lfhipass,
                        *lflopass;
    int                 maxInt,
                        nzero,
                        nAP,
                        nNU,    // 0 if nNU == nAP
                        nC;

public:
    Heatmap() : niGain(0), aphipass(0), lfhipass(0), lflopass(0)    {}
    virtual ~Heatmap();

    void setStream( const DAQ::Params &p, int js, int ip );
    void setStream( const DataFile *df );

    void resetFilter();
    void apFilter( vec_i16 &odata, const vec_i16 &idata );
    void lfFilter( vec_i16 &odata, const vec_i16 &idata );

    void accumReset( bool resetFlt );
    void accumSpikes( const vec_i16 &data, int thresh, int inarow );
    void accumPkPk( const vec_i16 &data );
    void normSpikes();
    void normPkPk( int what );
    const double* sums()        {return &vsum[0];}

private:
    void zeroFilterTransient( short *data, int ntpts, int nchans );
    void zeroData();
};

#endif  // HEATMAP_H


