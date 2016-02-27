
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "GraphsWindow.h"
#include "SVGrafsM_Im.h"

#include <QStatusBar>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* class SVGrafsM_Im ---------------------------------------------- */
/* ---------------------------------------------------------------- */

// BK: Of course, need expanded trigChan

SVGrafsM_Im::SVGrafsM_Im( GraphsWindow *gw, DAQ::Params &p )
    :   SVGrafsM( gw, p )
{
}


SVGrafsM_Im::~SVGrafsM_Im()
{
    saveSettings();
}


// BK: We should superpose the traces and not add.

static void addLFP(
    const DAQ::Params   &p,
    short               *data,
    int                 ntpts,
    int                 nchans,
    int                 nNeu )
{
    float   fgain = p.im.apGain / p.im.lfGain;

    for( int it = 0; it < ntpts; ++it, data += nchans ) {

        for( int ic = 0; ic < nNeu; ++ic )
            data[ic] += fgain*data[ic+nNeu];
    }
}


/*  Time Scaling
    ------------
    Each graph has its own wrapping data buffer (yval) but shares
    the time axis span. As fresh data arrive they wrap around such
    that the latest data are present as well as one span's worth of
    past data. We will draw the data using a wipe effect. Older data
    remain visible while they are progressively overwritten by the
    new from left to right. In this mode selection ranges do not
    make sense, nor do precise cursor readouts of time-coordinates.
    Rather, min_x and max_x suggest only the span of depicted data.
*/

void SVGrafsM_Im::putScans( vec_i16 &data, quint64 headCt )
{
#if 0
    double	tProf	= getTime();
#endif
    double      ysc		= 1.0 / 32768.0;
    const int   nC      = chanCount(),
                ntpts   = (int)data.size() / nC;

// ------------------------
// Add LFP to AP if !hipass
// ------------------------

    if( set.filterChkOn
        && p.im.imCumTypCnt[CimCfg::imSumNeural] ==
            2 * p.im.imCumTypCnt[CimCfg::imSumAP] ) {

        addLFP( p, &data[0], ntpts, nC, p.im.imCumTypCnt[CimCfg::imSumAP] );
    }

// ---------------------
// Append data to graphs
// ---------------------

    drawMtx.lock();

    QVector<float>  ybuf( ntpts );	// append en masse

    for( int ic = 0; ic < nC; ++ic ) {

        if( ic2iy[ic] < 0 )
            continue;

        // Collect points, update mean, stddev

        GraphStats  &stat   = ic2stat[ic];
        qint16      *d      = &data[ic];
        int         dwnSmp  = theX->dwnSmp,
                    dstep   = dwnSmp * nC,
                    ny      = 0;

        stat.clear();

        if( ic < p.im.imCumTypCnt[CimCfg::imSumAP] ) {

            // -------------------
            // Neural downsampling
            // -------------------

            // Withing each bin, report the greatest
            // amplitude (pos or neg) extremum. This
            // ensures spikes are not missed.

            if( dwnSmp <= 1 )
                goto pickNth;

            int ndRem = ntpts;

            for( int it = 0; it < ntpts; it += dwnSmp ) {

                int binMin = *d,
                    binMax = binMin,
                    binWid = dwnSmp;

                    stat.add( *d );

                    d += nC;

                    if( ndRem < binWid )
                        binWid = ndRem;

                for( int ib = 1; ib < binWid; ++ib, d += nC ) {

                    int	val = *d;

                    stat.add( *d );

                    if( val < binMin )
                        binMin = val;

                    if( val > binMax )
                        binMax = val;
                }

                ndRem -= binWid;

                if( abs( binMin ) > abs( binMax ) )
                    binMax = binMin;

                ybuf[ny++] = binMax * ysc;
            }
        }
        else if( ic < p.im.imCumTypCnt[CimCfg::imSumNeural] ) {

            // ---
            // LFP
            // ---

pickNth:
            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                ybuf[ny++] = *d * ysc;
                stat.add( *d );
            }
        }
        else {

            // ----
            // Sync
            // ----

            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                ybuf[ny++] = *d;
        }

        // Append points en masse
        // Renormalize x-coords -> consecutive indices.

        theX->dataMtx->lock();
        ic2Y[ic].yval.putData( &ybuf[0], ny );
        theX->dataMtx->unlock();
    }

// -----------------------
// Update pseudo time axis
// -----------------------

    double  span =  theX->spanSecs();

    theX->max_x = (headCt + ntpts) / p.im.srate;
    theX->min_x = theX->max_x - span;

// ----
// Draw
// ----

    QMetaObject::invokeMethod( theM, "update", Qt::QueuedConnection );

    drawMtx.unlock();

// ---------
// Profiling
// ---------

#if 0
    tProf = getTime() - tProf;
    Log() << "Graph milis " << 1000*tProf;
#endif
}


int SVGrafsM_Im::chanCount() const
{
    return p.im.imCumTypCnt[CimCfg::imSumAll];
}


bool SVGrafsM_Im::isSelAnalog() const
{
    return selected < p.im.imCumTypCnt[CimCfg::imSumNeural];
}


void SVGrafsM_Im::filterChkClicked( bool checked )
{
    set.filterChkOn = checked;
    saveSettings();
}


void SVGrafsM_Im::mySaveGraphClicked( bool checked )
{
    Q_UNUSED( checked )
}


void SVGrafsM_Im::myMouseOverGraph( double x, double y, int iy )
{
    int		ic			= lastMouseOverChan = theX->Y[iy]->usrChan;
    bool	isNowOver	= true;

    if( ic < 0 || ic >= chanCount() ) {
        gw->statusBar()->clearMessage();
        return;
    }

    QWidget	*w = QApplication::widgetAt( QCursor::pos() );

    if( !w || !dynamic_cast<MGraph*>(w) )
        isNowOver = false;

    double      mean, rms, stdev;
    QString		msg;
    const char	*unit,
                *swhere = (isNowOver ? "Mouse over" : "Last mouse-over");
    int			h,
                m;

    h = int(x / 3600);
    x = x - h * 3600;
    m = x / 60;
    x = x - m * 60;

    if( ic < p.im.imCumTypCnt[CimCfg::imSumNeural] ) {

        // neural readout

        computeGraphMouseOverVars( ic, y, mean, stdev, rms, unit );

        msg = QString(
            "%1 %2 @ pos (%3h%4m%5s, %6 %7)"
            " -- {mean, rms, stdv} %7: {%8, %9, %10}")
            .arg( swhere )
            .arg( STR2CHR( myChanName( ic ) ) )
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( x, 0, 'f', 3 )
            .arg( y, 0, 'f', 4 )
            .arg( unit )
            .arg( mean, 0, 'f', 4 )
            .arg( rms, 0, 'f', 4 )
            .arg( stdev, 0, 'f', 4 );
    }
    else {

        // sync readout

        msg = QString(
            "%1 %2 @ pos %3h%4m%5s")
            .arg( swhere )
            .arg( STR2CHR( myChanName( ic ) ) )
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( x, 0, 'f', 3 );
    }

    gw->statusBar()->showMessage( msg );
}


void SVGrafsM_Im::myClickGraph( double x, double y, int iy )
{
    myMouseOverGraph( x, y, iy );
    selectChan( lastMouseOverChan );
}


double SVGrafsM_Im::mySampRate()
{
    return p.im.srate;
}


void SVGrafsM_Im::mySort_ig2ic()
{
    if( set.usrOrder )
        p.sns.imChans.chanMap.userOrder( ig2ic );
    else
        p.sns.imChans.chanMap.defaultOrder( ig2ic );
}


QString SVGrafsM_Im::myChanName( int ic ) const
{
    return p.sns.imChans.chanMap.name( ic, ic == p.trigChan() );
}


QBitArray& SVGrafsM_Im::mySaveBits()
{
    return p.sns.imChans.saveBits;
}


// Return type number of digital channels, or -1 if none.
//
int SVGrafsM_Im::mySetUsrTypes()
{
    int c0, cLim;

    c0      = 0;
    cLim    = p.im.imCumTypCnt[CimCfg::imSumAP];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 0;

    c0      = p.im.imCumTypCnt[CimCfg::imSumAP];
    cLim    = p.im.imCumTypCnt[CimCfg::imSumNeural];

    for( int ic = c0; ic < cLim; ++ic )
        ic2Y[ic].usrType = 1;


    c0      = p.im.imCumTypCnt[CimCfg::imSumNeural];

    ic2Y[c0].usrType = 2;

    return 2;
}


void SVGrafsM_Im::saveSettings()
{
// -------------------
// Appearance defaults
// -------------------

    STDSETTINGS( settings, "graphs_M_Im" );

    settings.beginGroup( "All" );
    settings.setValue( "secs", set.secs );
    settings.setValue( "yscl0", set.yscl0 );
    settings.setValue( "yscl1", set.yscl1 );
    settings.setValue( "yscl2", set.yscl2 );
    settings.setValue( "clr0", clrToString( set.clr0 ) );
    settings.setValue( "clr1", clrToString( set.clr1 ) );
    settings.setValue( "clr2", clrToString( set.clr2 ) );
    settings.setValue( "navNChan", set.navNChan );
    settings.setValue( "filterChkOn", set.filterChkOn );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();
}


// Called only from init().
//
void SVGrafsM_Im::loadSettings()
{
// -------------------
// Appearance defaults
// -------------------

    STDSETTINGS( settings, "graphs_M_Im" );

    settings.beginGroup( "All" );
    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.yscl0       = settings.value( "yscl0", 1.0 ).toDouble();
    set.yscl1       = settings.value( "yscl1", 1.0 ).toDouble();
    set.yscl2       = settings.value( "yscl2", 1.0 ).toDouble();
    set.clr0        = clrFromString( settings.value( "clr0", "ffeedd82" ).toString() );
    set.clr1        = clrFromString( settings.value( "clr1", "ffff5500" ).toString() );
    set.clr2        = clrFromString( settings.value( "clr2", "ff44eeff" ).toString() );
    set.navNChan    = settings.value( "navNChan", 32 ).toInt();
    set.filterChkOn = settings.value( "filterChkOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", false ).toBool();
    settings.endGroup();
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsM_Im::scalePlotValue( double v, double gain )
{
    return p.im.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for neural channels!
//
void SVGrafsM_Im::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit )
{
    double  gain = p.im.chanGain( ic );

    y       = scalePlotValue( y, gain );

    drawMtx.lock();

    mean    = scalePlotValue( ic2stat[ic].mean(), gain );
    stdev   = scalePlotValue( ic2stat[ic].stdDev(), gain );
    rms     = scalePlotValue( ic2stat[ic].rms(), gain );

    drawMtx.unlock();

    unit    = "V";

    if( p.im.range.rmax < gain ) {
        y       *= 1000.0;
        mean    *= 1000.0;
        stdev   *= 1000.0;
        rms     *= 1000.0;
        unit     = "mV";
    }
}


