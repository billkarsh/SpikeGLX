
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "GWNiWidget.h"
#include "GraphPool.h"
#include "Biquad.h"

#include <QFrame>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSettings>
#include <QStatusBar>
#include <QMessageBox>
#include <math.h>




/* ---------------------------------------------------------------- */
/* class TabWidget ------------------------------------------------ */
/* ---------------------------------------------------------------- */

// (TabWidget + gCurTab) solves the problem wherein Qt does not send
// any signal that a tab is about to change. Rather, there is only a
// signal that a tab HAS ALREADY changed and the corresponding tab
// page has already been shown...before we could configure it! This
// leads to briefly ugly screens. We use TabWidgets as the tab pages
// and keep them hidden until the configuring is completed.

static void* gCurTab = 0;

class TabWidget : public QWidget
{
public:
    void setVisible( bool visible )
    {
        if( (void*)this != gCurTab )
            visible = false;

        QWidget::setVisible( visible );
    }
};

/* ---------------------------------------------------------------- */
/* class GWNiWidget ----------------------------------------------- */
/* ---------------------------------------------------------------- */

GWNiWidget::GWNiWidget( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), hipass(0), drawMtx(QMutex::Recursive),
        trgChan(p.trigChan()), lastMouseOverChan(-1)
{
    gCurTab = 0;

    ic2G.resize(     p.ni.niCumTypCnt[CniCfg::niSumAll] );
    ic2X.resize(     p.ni.niCumTypCnt[CniCfg::niSumAll] );
    ic2stat.resize(  p.ni.niCumTypCnt[CniCfg::niSumAll] );
    ic2frame.resize( p.ni.niCumTypCnt[CniCfg::niSumAll] );
    ic2chk.resize(   p.ni.niCumTypCnt[CniCfg::niSumAll] );
    ig2ic.resize(    p.ni.niCumTypCnt[CniCfg::niSumAll] );

    if( mainApp()->isSortUserOrder() )
        p.sns.niChans.chanMap.userOrder( ig2ic );
    else
        p.sns.niChans.chanMap.defaultOrder( ig2ic );

    initTabs();
    loadSettings();
    initFrames();

    tabChange( 0 );
}


GWNiWidget::~GWNiWidget()
{
    drawMtx.lock();

    saveSettings();

    GraphPool	*pool = mainApp()->pool;

    if( pool ) {
        for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic )
            pool->putFrame( ic2frame[ic] );
    }

    hipassMtx.lock();

    if( hipass )
        delete hipass;

    hipassMtx.unlock();

    drawMtx.unlock();
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

void GWNiWidget::putScans( vec_i16 &data, quint64 firstSamp )
{
#if 0
    double	tProf	= getTime();
#endif
    double      ysc		= 1.0 / 32768.0;
    const int   nC      = p.ni.niCumTypCnt[CniCfg::niSumAll],
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

        X.max_x = (firstSamp + ntpts) / p.ni.srate;
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


void GWNiWidget::eraseGraphs()
{
    drawMtx.lock();

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        GLGraphX    &X = ic2X[ic];

        X.dataMtx->lock();
        X.ydata.erase();
        X.dataMtx->unlock();
    }

    drawMtx.unlock();
}


void GWNiWidget::sortGraphs()
{
    drawMtx.lock();

    if( mainApp()->isSortUserOrder() )
        p.sns.niChans.chanMap.userOrder( ig2ic );
    else
        p.sns.niChans.chanMap.defaultOrder( ig2ic );

    retileBySorting();

    drawMtx.unlock();
}


bool GWNiWidget::isChanAnalog( int ic )
{
    return ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog];
}


// Return first sorted graph to GraphsWindow.
//
int GWNiWidget::initialSelectedChan( QString &name )
{
    int ic = ig2ic[0];

    name = p.sns.niChans.chanMap.e[ic].name;

    return ic;
}


void GWNiWidget::selectChan( int ic, bool selected )
{
    if( ic < 0 )
        return;

    int style = QFrame::Plain;

    if( selected )
        style |= QFrame::Box;
    else
        style |= QFrame::StyledPanel;

    ic2frame[ic]->setFrameStyle( style );
}


void GWNiWidget::ensureVisible( int ic )
{
// find tab with ic

    int itab = currentIndex();

    if( mainApp()->isSortUserOrder() ) {

        for( int ig = 0; ig < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ig ) {

            if( ig2ic[ig] == ic ) {
                itab = ig / graphsPerTab;
                break;
            }
        }
    }
    else
        itab = ic / graphsPerTab;

// Select that tab and redraw its graphWidget...
// However, tabChange won't get a signal if the
// current tab isn't changing...so we call it.

    if( itab != currentIndex() )
        setCurrentIndex( itab );
    else
        tabChange( itab );
}


// Note: Resizing of the maximized graph is automatic
// upon hiding the other frames in the layout.
//
void GWNiWidget::toggleMaximized( int iSel, bool wasMaximized )
{
    int nTabs = count();

    if( wasMaximized ) {   // restore multi-graph view

        hide();

        for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {
            if( ic != iSel )
                ic2frame[ic]->show();
        }

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, true );
    }
    else {  // show only selected

        ensureVisible( iSel );

        hide();

        for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {
            if( ic != iSel )
                ic2frame[ic]->hide();
        }

        int cur = currentIndex();

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, itab == cur );
    }

    show();
}


void GWNiWidget::getGraphScales( double &xSpn, double &yScl, int ic ) const
{
    const GLGraphX  &X = ic2X[ic];

    xSpn = X.spanSecs();
    yScl = X.yscale;
}


void GWNiWidget::graphSecsChanged( double secs, int ic )
{
    setGraphTimeSecs( ic, secs );
    saveSettings();
}


void GWNiWidget::graphYScaleChanged( double scale, int ic )
{
    GLGraphX    &X = ic2X[ic];

    drawMtx.lock();
    X.yscale = scale;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveSettings();
}


QColor GWNiWidget::getGraphColor( int ic ) const
{
    return ic2X[ic].trace_Color;
}


void GWNiWidget::colorChanged( QColor c, int ic )
{
    GLGraphX    &X = ic2X[ic];

    drawMtx.lock();
    X.trace_Color = c;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveSettings();
}


void GWNiWidget::applyAll( int ic )
{
    const GLGraphX  &X = ic2X[ic];

// Copy settings to like type {neural, aux, digital}.
// For digital, only secs is used.

    int c0, cLim;

    if( ic < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {
        c0      = 0;
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumNeural];
    }
    else if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {
        c0      = p.ni.niCumTypCnt[CniCfg::niSumNeural];
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
    }
    else {
        c0      = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumAll];
    }

    drawMtx.lock();

    for( int ic = c0; ic < cLim; ++ic ) {

        ic2X[ic].yscale         = X.yscale;
        ic2X[ic].trace_Color    = X.trace_Color;
        setGraphTimeSecs( ic, X.spanSecs() );   // calls update
    }

    drawMtx.unlock();

    saveSettings();
}


void GWNiWidget::hipassChecked( bool checked )
{
    hipassMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( checked )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );

    hipassMtx.unlock();
}


void GWNiWidget::showHideSaveChks()
{
    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic )
        ic2chk[ic]->setHidden( !mainApp()->areSaveChksShowing() );
}


void GWNiWidget::enableAllChecks( bool enabled )
{
    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic )
        ic2chk[ic]->setEnabled( enabled );
}


void GWNiWidget::tabChange( int itab )
{
    QSet<GLGraph*>  &gwPool = gw->gwGraphPool();

    drawMtx.lock();

// Retire current graphs

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        GLGraph* &G = ic2G[ic];

        if( G ) {
            G->detach();
            gwPool.insert( G );
            G = 0;
        }
    }

// Reattach graphs to their new frames and set their states.
// Remember that some frames will already have graphs we can
// repurpose. Otherwise we fetch a graph from extraGraphs.

    for( int ig = itab * graphsPerTab;
        !gwPool.isEmpty() && ig < p.ni.niCumTypCnt[CniCfg::niSumAll];
        ++ig ) {

        int             ic  = ig2ic[ig];
        QFrame          *f  = ic2frame[ic];
        QList<GLGraph*> GL  = f->findChildren<GLGraph*>();

        GLGraph *G = (GL.empty() ? *gwPool.begin() : GL[0]);
        gwPool.remove( G );
        ic2G[ic] = G;

        QVBoxLayout *l = dynamic_cast<QVBoxLayout*>(f->layout());
        QWidget		*gpar = G->parentWidget();

        if( l )
            l->addWidget( gpar );
        else
            f->layout()->addWidget( gpar );

        gpar->setParent( f );

        G->attach( &ic2X[ic] );
        G->setImmedUpdate( true );
   }

// Page configured, now make it visible

    gCurTab = graphTabs[itab];
    graphTabs[itab]->show();

    drawMtx.unlock();
}


void GWNiWidget::saveGraphClicked( bool checked )
{
    int thisChan = sender()->objectName().toInt();

    mainApp()->cfgCtl()->graphSetsNiSaveBit( thisChan, checked );
}


void GWNiWidget::mouseOverGraph( double x, double y )
{
    int		ic			= lastMouseOverChan = graph2Chan( sender() );
    bool	isNowOver	= true;

    if( ic < 0 || ic >= p.ni.niCumTypCnt[CniCfg::niSumAll] ) {
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
            .arg( STR2CHR( p.sns.niChans.chanMap.name( ic, ic == trgChan ) ) )
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
            .arg( STR2CHR( p.sns.niChans.chanMap.name( ic ) ) )
            .arg( h, 2, 10, QChar('0') )
            .arg( m, 2, 10, QChar('0') )
            .arg( x, 0, 'f', 3 );
    }

    gw->statusBar()->showMessage( msg );
}


void GWNiWidget::mouseClickGraph( double x, double y )
{
    mouseOverGraph( x, y );

    gw->niSetSelection(
        lastMouseOverChan,
        p.sns.niChans.chanMap.e[lastMouseOverChan].name );
}


void GWNiWidget::mouseDoubleClickGraph( double x, double y )
{
    mouseClickGraph( x, y );
    gw->toggleMaximized();
}


int GWNiWidget::getNumGraphsPerTab() const
{
    int lim = MAX_GRAPHS_PER_TAB;

    if( p.ni.isMuxingMode() )
        lim = p.ni.muxFactor * (lim / p.ni.muxFactor);

    if( p.sns.maxGrfPerTab && p.sns.maxGrfPerTab <= lim )
        return p.sns.maxGrfPerTab;

    return lim;
}


void GWNiWidget::initTabs()
{
    graphsPerTab = getNumGraphsPerTab();

    int nTabs = p.ni.niCumTypCnt[CniCfg::niSumAll] / graphsPerTab;

    if( nTabs * graphsPerTab < p.ni.niCumTypCnt[CniCfg::niSumAll] )
        ++nTabs;

    graphTabs.resize( nTabs );

    setFocusPolicy( Qt::StrongFocus );
    Connect( this, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)) );
}


bool GWNiWidget::initFrameCheckBox( QFrame* &f, int ic )
{
    QList<QCheckBox*>   CL = f->findChildren<QCheckBox*>();
    QCheckBox*          &C = ic2chk[ic] = (CL.size() ? CL.front() : 0);

    if( !C ) {

        Error() << "INTERNAL ERROR: GLGraph " << ic << " is invalid!";

        QMessageBox::critical(
            0,
            "INTERNAL ERROR",
            QString("GLGraph %1 is invalid!").arg( ic ) );

        QApplication::exit( 1 );

        delete f;
        f = 0;

        return false;
    }

    C->setText(
        QString("Save %1")
        .arg( p.sns.niChans.chanMap.name( ic, ic == p.trigChan() ) ) );

    C->setObjectName( QString().number( ic ) );
    C->setChecked( p.sns.niChans.saveBits.at( ic ) );
    C->setEnabled( p.mode.trgInitiallyOff );
    C->setHidden( !mainApp()->areSaveChksShowing() );
    ConnectUI( C, SIGNAL(toggled(bool)), this, SLOT(saveGraphClicked(bool)) );

    return true;
}


void GWNiWidget::initFrameGraph( QFrame* &f, int ic )
{
    QList<GLGraph*> GL  = f->findChildren<GLGraph*>();
    GLGraph         *G	= ic2G[ic] = (GL.size() ? GL.front() : 0);
    GLGraphX        &X  = ic2X[ic];

    if( G ) {
        Connect( G, SIGNAL(cursorOver(double,double)), this, SLOT(mouseOverGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutClicked(double,double)), this, SLOT(mouseClickGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutDoubleClicked(double,double)), this, SLOT(mouseDoubleClickGraph(double,double)) );
    }

    X.num = ic;   // link graph to hwr channel

    if( ic < p.ni.niCumTypCnt[CniCfg::niSumNeural] )
        X.bkgnd_Color = NeuGraphBGColor;
    else if( ic < p.ni.niCumTypCnt[CniCfg::niSumAnalog] )
        X.bkgnd_Color = AuxGraphBGColor;
    else {
        X.yscale        = 1.0;
        X.bkgnd_Color   = DigGraphBGColor;
        X.isDigType     = true;
    }
}


void GWNiWidget::initFrames()
{
    int nTabs   = graphTabs.size(),
        ig      = 0;

    for( int itab = 0; itab < nTabs; ++itab ) {

        QWidget     *graphsWidget   = new TabWidget;
        QGridLayout *grid           = new QGridLayout( graphsWidget );

        graphTabs[itab] = graphsWidget;

        grid->setHorizontalSpacing( 1 );
        grid->setVerticalSpacing( 1 );

        int remChans    = p.ni.niCumTypCnt[CniCfg::niSumAll]
                            - itab*graphsPerTab,
            numThisPage = (remChans >= graphsPerTab ?
                            graphsPerTab : remChans),
            nrows       = (int)sqrtf( numThisPage ),
            ncols       = nrows;

        while( nrows * ncols < numThisPage ) {

            if( nrows > ncols )
                ++ncols;
            else
                ++nrows;
        }

        for( int r = 0; r < nrows; ++r ) {

            for( int c = 0; c < ncols; ++c, ++ig ) {

                if( c + ncols * r >= numThisPage )
                    goto add_tab;

                int ic = ig2ic[ig];

                QFrame*	&f = ic2frame[ic] =
                    mainApp()->pool->getFrame( ig >= graphsPerTab );

                f->setParent( graphsWidget );
                f->setLineWidth( 2 );
                f->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );

                if( initFrameCheckBox( f, ic ) ) {
                    initFrameGraph( f, ic );
                    grid->addWidget( f, r, c );
                }

                f->show();
            }
        }

add_tab:
        addTab( graphsWidget, QString::null );
        setTabText( itab, ig - 1 );
    }
}


int GWNiWidget::graph2Chan( QObject *graphObj )
{
    GLGraph *G = dynamic_cast<GLGraph*>(graphObj);

    if( G )
        return G->getX()->num;
    else
        return -1;
}


void GWNiWidget::setGraphTimeSecs( int ic, double t )
{
    drawMtx.lock();

    if( ic >= 0 && ic < p.ni.niCumTypCnt[CniCfg::niSumAll] ) {

        GLGraphX    &X = ic2X[ic];

        X.setSpanSecs( t, p.ni.srate );
        X.setVGridLinesAuto();

        if( X.G )
            X.G->update();

        ic2stat[ic].clear();
    }

    drawMtx.unlock();
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double GWNiWidget::scalePlotValue( double v, double gain )
{
    return p.ni.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for analog channels!
//
void GWNiWidget::computeGraphMouseOverVars(
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


void GWNiWidget::setTabText( int itab, int igLast )
{
   QTabWidget::setTabText(
        itab,
        QString("%1-%2")
            .arg( itab*graphsPerTab )
            .arg( igLast ) );
//   QTabWidget::setTabText(
//        itab,
//        QString("%1 %2-%3")
//            .arg( mainApp()->isSortUserOrder() ? "Usr" : "Acq" )
//            .arg( itab*graphsPerTab )
//            .arg( igLast ) );
}


void GWNiWidget::retileBySorting()
{
    int nTabs   = graphTabs.size(),
        ig      = 0;

    for( int itab = 0; itab < nTabs; ++itab ) {

        QWidget *graphsWidget = graphTabs[itab];

        delete graphsWidget->layout();
        QGridLayout *grid = new QGridLayout( graphsWidget );

        grid->setHorizontalSpacing( 1 );
        grid->setVerticalSpacing( 1 );

        int remChans    = p.ni.niCumTypCnt[CniCfg::niSumAll]
                            - itab*graphsPerTab,
            numThisPage = (remChans >= graphsPerTab ?
                            graphsPerTab : remChans),
            nrows       = (int)sqrtf( numThisPage ),
            ncols       = nrows;

        while( nrows * ncols < numThisPage ) {

            if( nrows > ncols )
                ++ncols;
            else
                ++nrows;
        }

        // now re-add them in the sorting order

        for( int r = 0; r < nrows; ++r ) {

            for( int c = 0; c < ncols; ++c, ++ig ) {

                if( c + ncols * r >= numThisPage )
                    goto name_tab;

                QFrame* &f = ic2frame[ig2ic[ig]];

                f->setParent( graphsWidget );
                grid->addWidget( f, r, c );
            }
        }

name_tab:
        setTabText( itab, ig - 1 );
    }
}


void GWNiWidget::saveSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "PlotOptions_nidq" );

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        settings.setValue(
            QString("chan%1").arg( ic ),
            ic2X[ic].toString() );
    }

    settings.endGroup();
}


void GWNiWidget::loadSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "PlotOptions_nidq" );

    drawMtx.lock();

// Note on digital channels:
// The default yscale and color settings loaded here are
// correct for digital channels, and we forbid editing
// those values (see updateToolbar()).

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        QString s = settings.value(
                        QString("chan%1").arg( ic ),
                        "fg:ffeedd82 xsec:1.0 yscl:1" )
                        .toString();

        ic2X[ic].fromString( s, p.ni.srate );
        ic2stat[ic].clear();
    }

    drawMtx.unlock();

    settings.endGroup();
}


