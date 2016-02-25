
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "GraphsWindow.h"
#include "SVGrafsG_Ni.h"
#include "FramePool.h"
#include "Biquad.h"

#include <QStatusBar>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* class SVGrafsG_Ni ---------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsG_Ni::SVGrafsG_Ni( GraphsWindow *gw, DAQ::Params &p )
    :   SVGrafsG( gw, p ), hipass(0)
{
}


SVGrafsG_Ni::~SVGrafsG_Ni()
{
    saveSettings();

    hipassMtx.lock();
    if( hipass )
        delete hipass;
    hipassMtx.unlock();
}


/*  Time Scaling
    ------------
    Each graph has its own wrapping data buffer (ydata) and its
    own time axis span. As fresh data arrive they wrap around such
    that the latest data are present as well as one span's worth of
    past data. We will draw the data using a wipe effect. Older data
    remain visible while they are progressively overwritten by the
    new from left to right. In this mode selection ranges do not
    make sense, nor do precise cursor readouts of time-coordinates.
    Rather, min_x and max_x suggest only the span of depicted data.
*/

void SVGrafsG_Ni::putScans( vec_i16 &data, quint64 headCt )
{
#if 0
    double	tProf	= getTime();
#endif
    double      ysc		= 1.0 / 32768.0;
    const int   nC      = ic2G.size(),
                ntpts   = (int)data.size() / nC;

/* ------------ */
/* Apply filter */
/* ------------ */

    hipassMtx.lock();

    if( hipass ) {
        hipass->applyBlockwiseMem(
                    &data[0], ntpts, nC,
                    0, p.ni.niCumTypCnt[CniCfg::niSumNeural] );
    }

    hipassMtx.unlock();

/* --------------------- */
/* Append data to graphs */
/* --------------------- */

    drawMtx.lock();

    QVector<float>  ybuf( ntpts );	// append en masse

    for( int ic = 0; ic < nC; ++ic ) {

        GLGraph *G = ic2G[ic];

        if( !G )
            continue;

        // Collect points, update mean, stddev

        GLGraphX    &X      = ic2X[ic];
        GraphStats  &stat   = ic2stat[ic];
        qint16      *d      = &data[ic];
        int         dwnSmp  = X.dwnSmp,
                    dstep   = dwnSmp * nC,
                    ny      = 0;

        stat.clear();

        if( ic < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {

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
        else if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {

            // ----------
            // Aux analog
            // ----------

pickNth:
            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep ) {

                ybuf[ny++] = *d * ysc;
                stat.add( *d );
            }
        }
        else {

            // -------
            // Digital
            // -------

            for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                ybuf[ny++] = *d;
        }

        // Append points en masse
        // Renormalize x-coords -> consecutive indices.

        X.dataMtx->lock();
        X.ydata.putData( &ybuf[0], ny );
        X.dataMtx->unlock();

        // Update pseudo time axis

        double  span =  X.spanSecs();

        X.max_x = (headCt + ntpts) / p.ni.srate;
        X.min_x = X.max_x - span;

        // Draw

        QMetaObject::invokeMethod( G, "update", Qt::QueuedConnection );
    }

    drawMtx.unlock();

/* --------- */
/* Profiling */
/* --------- */

#if 0
    tProf = getTime() - tProf;
    Log() << "Graph milis " << 1000*tProf;
#endif
}


bool SVGrafsG_Ni::isSelAnalog() const
{
    return selected < p.ni.niCumTypCnt[CniCfg::niSumAnalog];
}


void SVGrafsG_Ni::filterChkClicked( bool checked )
{
    hipassMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( checked )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );

    hipassMtx.unlock();

    set.filterChkOn = checked;
    saveSettings();
}


void SVGrafsG_Ni::mySaveGraphClicked( bool checked )
{
    int thisChan = sender()->objectName().toInt();

    mainApp()->cfgCtl()->graphSetsNiSaveBit( thisChan, checked );
}


void SVGrafsG_Ni::myMouseOverGraph( double x, double y )
{
    int		ic			= lastMouseOverChan = graph2Chan( sender() );
    bool	isNowOver	= true;

    if( ic < 0 || ic >= myChanCount() ) {
        gw->statusBar()->clearMessage();
        return;
    }

    QWidget	*w = QApplication::widgetAt( QCursor::pos() );

    if( !w || !dynamic_cast<GLGraph*>(w) )
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

    if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {

        // analog readout

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

        // digital readout

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


void SVGrafsG_Ni::myClickGraph( double x, double y )
{
    myMouseOverGraph( x, y );
    selectChan( lastMouseOverChan );
}


int SVGrafsG_Ni::myChanCount()
{
    return p.ni.niCumTypCnt[CniCfg::niSumAll];
}


double SVGrafsG_Ni::mySampRate()
{
    return p.ni.srate;
}


void SVGrafsG_Ni::mySort_ig2ic()
{
    if( set.usrOrder )
        p.sns.niChans.chanMap.userOrder( ig2ic );
    else
        p.sns.niChans.chanMap.defaultOrder( ig2ic );
}


int SVGrafsG_Ni::myGrfPerTab() const
{
    int lim = MAX_GRAPHS_PER_TAB;

    if( p.ni.isMuxingMode() )
        lim = p.ni.muxFactor * (lim / p.ni.muxFactor);

    if( p.sns.maxGrfPerTab && p.sns.maxGrfPerTab <= lim )
        return p.sns.maxGrfPerTab;

    return lim;
}


QString SVGrafsG_Ni::myChanName( int ic ) const
{
    return p.sns.niChans.chanMap.name( ic, ic == p.trigChan() );
}


QBitArray& SVGrafsG_Ni::mySaveBits()
{
    return p.sns.niChans.saveBits;
}


void SVGrafsG_Ni::myCustomXSettings( int ic )
{
    GLGraphX    &X = ic2X[ic];

    if( ic < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {
        X.bkgnd_Color   = NeuGraphBGColor;
        X.usrType       = 0;
    }
    else if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {
        X.bkgnd_Color   = AuxGraphBGColor;
        X.usrType       = 1;
    }
    else {
        X.yscale        = 1.0;
        X.bkgnd_Color   = DigGraphBGColor;
        X.usrType       = 2;
        X.isDigType     = true;
    }
}


void SVGrafsG_Ni::saveSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "graphs_G_Ni" );

    settings.beginGroup( "All" );
    settings.setValue( "filterChkOn", set.filterChkOn );
    settings.setValue( "usrOrder", set.usrOrder );
    settings.endGroup();

    settings.beginGroup( "Each" );
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

        settings.setValue(
            QString("chan%1").arg( ic ),
            ic2X[ic].toString() );
    }

    settings.endGroup();
}


void SVGrafsG_Ni::loadSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "graphs_G_Ni" );

    settings.beginGroup( "All" );
    set.filterChkOn = settings.value( "filterChkOn", false ).toBool();
    set.usrOrder    = settings.value( "usrOrder", false ).toBool();
    settings.endGroup();

    settings.beginGroup( "Each" );

// Note on digital channels:
// The default yscale and color settings loaded here are
// correct for digital channels, and we forbid editing
// those values (see updateToolbar()).

    double  srate = mySampRate();

    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

        QString s = settings.value(
                        QString("chan%1").arg( ic ),
                        "fg:ffeedd82 xsec:1.0 yscl:1" )
                        .toString();

        ic2X[ic].fromString( s, srate );
        ic2stat[ic].clear();
    }

    settings.endGroup();
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double SVGrafsG_Ni::scalePlotValue( double v, double gain )
{
    return p.ni.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for analog channels!
//
void SVGrafsG_Ni::computeGraphMouseOverVars(
    int         ic,
    double      &y,
    double      &mean,
    double      &stdev,
    double      &rms,
    const char* &unit )
{
    double  gain = p.ni.chanGain( ic );

    y       = scalePlotValue( y, gain );

    drawMtx.lock();

    mean    = scalePlotValue( ic2stat[ic].mean(), gain );
    stdev   = scalePlotValue( ic2stat[ic].stdDev(), gain );
    rms     = scalePlotValue( ic2stat[ic].rms(), gain );

    drawMtx.unlock();

    unit    = "V";

    if( p.ni.range.rmax < gain ) {
        y       *= 1000.0;
        mean    *= 1000.0;
        stdev   *= 1000.0;
        rms     *= 1000.0;
        unit     = "mV";
    }
}


