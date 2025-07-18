#ifndef HEATMAP_H
#define HEATMAP_H

#include "CAR.h"

namespace DAQ {
struct Params;
}

class AIQ;
class Biquad;
class DataFile;
struct ShankMap;

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
    const AIQ           *Qf;
    Biquad              *aphipass,
                        *lfhipass,
                        *lflopass;
    CAR                 car;
    int                 js,
                        maxInt,
                        nzero,
                        nAP,
                        nLF,
                        nC;
    bool                offline;

public:
    Heatmap() : niGain(0), Qf(0), aphipass(0), lfhipass(0), lflopass(0) {}
    virtual ~Heatmap();

    void setStream( const DAQ::Params &p, int js, int ip );
    void setStream( const DataFile *df );

    void updateMap( const ShankMap *S, int maxr = -1 );

    void qf_enable( bool on );
    void resetFilter( int what );
    void apFilter(
            vec_i16         &odata,
            const vec_i16   &idata,
            quint64         headCt,
            bool            fvw_gbldmx = false );
    void lfFilter( vec_i16 &odata, const vec_i16 &idata );

    void accumReset( bool resetFlt, int what );
    void accumSpikes( const vec_i16 &data, int thresh, int inarow );
    void accumPkPk( const vec_i16 &data );
    void normSpikes();
    bool normPkPk( int what );
    const double* sums()        {return &vsum[0];}

private:
    void zeroFilterTransient( short *data, int ntpts, int nchans );
    void zeroData();
};

#endif  // HEATMAP_H


