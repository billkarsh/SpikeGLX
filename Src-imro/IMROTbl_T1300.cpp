
#include "IMROTbl_T1300.h"

#define blue_emitter0_dist_from_elec0_um    74
#define  red_emitter0_dist_from_elec0_um    68
#define emitter_pitch_um                    100
#define blue_emitter_count                  14
#define  red_emitter_count                  14


/* ---------------------------------------------------------------- */
/* struct IMROTbl ------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Get array of emitter v-centers in units of rows.
//
void IMROTbl_T1300::optoGetEmts( std::vector<float> &vRows, int color ) const
{
    vRows.clear();

    if( color ) {
        for( int i = 0; i < red_emitter_count; ++i ) {
            vRows.push_back(
                ((float)red_emitter0_dist_from_elec0_um
                + i * emitter_pitch_um) / _zpitch );
        }
    }
    else {
        for( int i = 0; i < blue_emitter_count; ++i ) {
            vRows.push_back(
                ((float)blue_emitter0_dist_from_elec0_um
                + i * emitter_pitch_um) / _zpitch );
        }
    }
}


void IMROTbl_T1300::optoSetCur( int color, int site )
{
    if( color )
        curRed = site;
    else
        curBlue = site;
}


int IMROTbl_T1300::optoGetCur( int color ) const
{
    if( color )
        return curRed;
    else
        return curBlue;
}


// If an emitter is enabled (>=0) vChan gets a neighborhood of channels.
// The channel is [0..nC] if its row is within in_rad  of emitter.
// 100,000 is added if its row is within out_rad of emitter.
//
// Return enabled emitter index or -1.
//
int IMROTbl_T1300::optoGetNeib( std::vector<int> &vChan, int color ) const
{
    vChan.clear();

    if( color ) {
        if( curRed >= 0 ) {
            double  zE  = (double)red_emitter0_dist_from_elec0_um
                            + curRed * emitter_pitch_um;
            neighbors( vChan, zE );
        }
        return curRed;
    }
    else {
        if( curBlue >= 0 ) {
            double  zE  = (double)blue_emitter0_dist_from_elec0_um
                            + curBlue * emitter_pitch_um;
            neighbors( vChan, zE );
        }
        return curBlue;
    }
}


void IMROTbl_T1300::neighbors( std::vector<int> &vChan, double zE ) const
{
#define in_rad  1
#define out_rad 3

    if( s2k.isEmpty() )
        edit_init();

    int r0  = zE / _zpitch,
        rlo = qMax( r0 - out_rad - 1, 0 ),
        rhi = qMin( r0 + out_rad + 1, nRow() );

    for( int r = rlo; r < rhi; ++r ) {

        double  d = abs( r - zE/_zpitch );

        if( d <= in_rad ) {
            vChan.push_back( edit_site2Chan( IMRO_Site( 0, 0, r ) ) );
            vChan.push_back( edit_site2Chan( IMRO_Site( 0, 1, r ) ) );
        }
        else if( d <= out_rad ) {
            vChan.push_back( 100000 + edit_site2Chan( IMRO_Site( 0, 0, r ) ) );
            vChan.push_back( 100000 + edit_site2Chan( IMRO_Site( 0, 1, r ) ) );
        }
    }
}


