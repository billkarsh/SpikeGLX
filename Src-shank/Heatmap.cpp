
#include "Heatmap.h"
#include "DAQ.h"
#include "DataFile.h"
#include "Subset.h"
#include "Biquad.h"


/* ---------------------------------------------------------------- */
/* Heatmap -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

Heatmap::~Heatmap()
{
    if( aphipass ) delete aphipass;
    if( lfhipass ) delete lfhipass;
    if( lflopass ) delete lflopass;
}


void Heatmap::setStream( const DAQ::Params &p, int js, int ip )
{
    srate = p.stream_rate( js, ip );

    switch( js ) {
        case jsNI:
            VMAX    = p.ni.range.rmax;
            niGain  = p.ni.mnGain;
            maxInt  = SHRT_MAX;
            nAP     = p.ni.niCumTypCnt[CniCfg::niSumNeural];
            nLF     = 0;
            nC      = p.ni.niCumTypCnt[CniCfg::niSumAll];
            break;
        case jsOB:
            VMAX    = p.im.obxj[ip].range.rmax;
            maxInt  = SHRT_MAX;
            nAP     = 0;
            nLF     = 0;
            nC      = 0;
            break;
        case jsIM:
            const IMROTbl   *R = p.im.prbj[ip].roTbl;
            VMAX    = R->maxVolts();
            maxInt  = R->maxInt();
            nAP     = R->nAP();
            nLF     = R->nLF();
            nC      = nAP + nLF + R->nSY();
            for( int ic = 0; ic < nAP; ++ic ) {
                vapg.push_back( R->apGain( ic ) );
                vlfg.push_back( R->lfGain( ic ) );
            }
            break;
    }

    aphipass = new Biquad( bq_type_highpass, 300/srate );
    lfhipass = new Biquad( bq_type_highpass, 0.2/srate );
    lflopass = new Biquad( bq_type_lowpass,  300/srate );

    zeroData();
    resetFilter();
}


void Heatmap::setStream( const DataFile *df )
{
    srate   = df->samplingRateHz();
    VMAX    = df->vRange().rmax;
    nAP     = df->numNeuralChans();
    nLF     = 0;
    nC      = df->numChans();

    if( df->streamFromObj().startsWith( "i" ) ) {
        for( int ic = 0; ic < nAP; ++ic )
            vapg.push_back( int(df->ig2Gain( ic )) );
        vlfg    = vapg;
        maxInt  = qMax( df->getParam("imMaxInt").toInt(), 512 ) - 1;
    }
    else {
        niGain  = df->ig2Gain( 0 );
        maxInt  = SHRT_MAX;
    }

    aphipass = new Biquad( bq_type_highpass, 300/srate );
    lfhipass = new Biquad( bq_type_highpass, 0.2/srate );
    lflopass = new Biquad( bq_type_lowpass,  300/srate );

    zeroData();
    resetFilter();
}


void Heatmap::resetFilter()
{
    nzero = BIQUAD_TRANS_WIDE;
}


void Heatmap::apFilter( vec_i16 &odata, const vec_i16 &idata )
{
    int ntpts = int(idata.size()) / nC;

    Subset::subsetBlock( odata, *(vec_i16*)&idata, 0, nAP, nC );

    aphipass->applyBlockwiseMem( &odata[0], maxInt, ntpts, nAP, 0, nAP );

    zeroFilterTransient( &odata[0], ntpts, nAP );
}


void Heatmap::lfFilter( vec_i16 &odata, const vec_i16 &idata )
{
    int ntpts = int(idata.size()) / nC;

    if( !nLF )
        Subset::subsetBlock( odata, *(vec_i16*)&idata, 0, nAP, nC );
    else
        Subset::subsetBlock( odata, *(vec_i16*)&idata, nAP, nLF, nC );

    lfhipass->applyBlockwiseMem( &odata[0], maxInt, ntpts, nAP, 0, nAP );
    lflopass->applyBlockwiseMem( &odata[0], maxInt, ntpts, nAP, 0, nAP );

    zeroFilterTransient( &odata[0], ntpts, nAP );
}


void Heatmap::accumReset( bool resetFlt )
{
    zeroData();

    if( resetFlt )
        resetFilter();
}


void Heatmap::accumSpikes( const vec_i16 &data, int thresh, int inarow )
{
    int ntpts = int(data.size()) / nC;

    if( !ntpts )
        return;

    double          v2i     = maxInt * thresh * 1e-6 / VMAX;
    const qint16    *src    = &data[0];
    const int       *gn     = &vapg[0];

    nSmp += ntpts;

    for( int c = 0; c < nAP; ++c ) {

        const short *d      = &src[c],
                    *dlim   = &src[c + ntpts*nAP];

        int spikes  = 0,
            T       = int(v2i * gn[c]),
            hiCnt   = (*d <= T ? inarow : 0);

        while( (d += nAP) < dlim ) {

            if( *d <= T ) {

                if( ++hiCnt == inarow )
                    ++spikes;
            }
            else
                hiCnt = 0;
        }

        vsum[c] += spikes;
    }
}


void Heatmap::accumPkPk( const vec_i16 &data )
{
    int ntpts = int(data.size()) / nC;

    if( !ntpts )
        return;

    const qint16    *src = &data[0];
    int             *vmn = &vmin[0],
                    *vmx = &vmax[0];

    for( int it = 0; it < ntpts; ++it, src += nAP ) {

        for( int c = 0; c < nAP; ++c ) {

            int v = src[c];

            if( v < vmn[c] )
                vmn[c] = v;

            if( v > vmx[c] )
                vmx[c] = v;
        }
    }
}


void Heatmap::normSpikes()
{
    double  count2Rate = srate / nSmp;
    double  *dst = &vsum[0];

    for( int i = 0; i < nAP; ++i )
        dst[i] *= count2Rate;
}


void Heatmap::normPkPk( int what )
{
    double      i2v  = 1e6 * VMAX / maxInt;
    double      *dst = &vsum[0];
    const int   *vmn = &vmin[0],
                *vmx = &vmax[0];

    if( vapg.size() ) {
        if( what == 1 ) {
            for( int i = 0; i < nAP; ++i )
                dst[i] = (vmx[i] - vmn[i]) * i2v / vapg[i];
        }
        else {
            for( int i = 0; i < nAP; ++i )
                dst[i] = (vmx[i] - vmn[i]) * i2v / vlfg[i];
        }
    }
    else if( niGain > 0 ) {
        for( int i = 0; i < nAP; ++i )
            dst[i] = (vmx[i] - vmn[i]) * i2v / niGain;
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Heatmap::zeroFilterTransient( short *data, int ntpts, int nchans )
{
    if( nzero > 0 ) {

        // overwrite with zeros

        if( ntpts > nzero )
            ntpts = nzero;

        memset( data, 0, ntpts*nchans*sizeof(qint16) );
        nzero -= ntpts;
    }
}


void Heatmap::zeroData()
{
    vmin.assign( nAP,  99000 );
    vmax.assign( nAP, -99000 );
    vsum.assign( nAP,  0 );
    nSmp = 0;
}


