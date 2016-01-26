
#include "Pixmaps/play.xpm"
#include "Pixmaps/pause.xpm"
#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphPool.h"
#include "GraphsWindow.h"
#include "Biquad.h"
#include "ClickableLabel.h"
#include "QLED.h"
#include "ConfigCtl.h"
#include "Subset.h"

#include <QToolBar>
#include <QAction>
#include <QCloseEvent>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QStatusBar>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QIcon>
#include <QPixmap>
#include <QFrame>
#include <QCursor>
#include <QFont>
#include <QPainter>
#include <QColorDialog>
#include <QMessageBox>
#include <QTimer>
#include <QSettings>
#include <math.h>


/* ---------------------------------------------------------------- */
/* TabWidget class ------------------------------------------------ */
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
/* struct GraphStats ---------------------------------------------- */
/* ---------------------------------------------------------------- */

double GraphsWindow::GraphStats::rms() const
{
    double	rms = 0.0;

    if( s2 > 0.0 ) {

        if( num > 1 )
            rms = s2 / num;

        rms = sqrt( rms );
    }

    return rms / 32768.0;
}


double GraphsWindow::GraphStats::stdDev() const
{
    double stddev = 0.0;

    if( num > 1 ) {

        double	var = (s2 - s1*s1/num) / (num - 1);

        if( var > 0.0 )
            stddev = sqrt( var );
    }

    return stddev / 32768.0;
}

/* ---------------------------------------------------------------- */
/* GraphsWindow --------------------------------------------------- */
/* ---------------------------------------------------------------- */

//#define GRAPHS_ON_TIMERS

#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )


static const QIcon  *playIcon           = 0,
                    *pauseIcon          = 0,
                    *graphMaxedIcon     = 0,
                    *graphNormalIcon    = 0,
                    *applyAllIcon       = 0;

static const QColor NeuGraphBGColor( 0x2f, 0x4f, 0x4f, 0xff ),
                    AuxGraphBGColor( 0x4f, 0x4f, 0x4f, 0xff ),
                    DigGraphBGColor( 0x1f, 0x1f, 0x1f, 0xff );




static void initIcons()
{
    if( !playIcon )
        playIcon = LOADICON( play_xpm );

    if( !pauseIcon )
        pauseIcon = LOADICON( pause_xpm );

    if( !graphMaxedIcon )
        graphMaxedIcon = LOADICON( window_fullscreen_xpm );

    if( !graphNormalIcon )
        graphNormalIcon = LOADICON( window_nofullscreen_xpm );

    if( !applyAllIcon )
        applyAllIcon = LOADICON( apply_all_xpm );
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Public interface ----------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

GraphsWindow::GraphsWindow( DAQ::Params &p )
    :   QMainWindow(0), tAvg(0.0), tNum(0.0), p(p),
        maximized(0), hipass(0), drawMtx(QMutex::Recursive),
        lastMouseOverChan(-1), selChan(0), paused(false)
{
    initWindow();
}


GraphsWindow::~GraphsWindow()
{
    drawMtx.lock();

    setUpdatesEnabled( false );

    saveGraphSettings();

    if( maximized )
        toggleMaximize(); // resets graphs to original state..

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


void GraphsWindow::remoteSetTrgEnabled( bool on )
{
    QCheckBox   *trgChk = graphCtls->findChild<QCheckBox*>( "trgchk" );

    trgChk->setChecked( on );
    setTrgEnable( on );
}


void GraphsWindow::remoteSetRunLE( const QString &name )
{
    QLineEdit   *runLE = graphCtls->findChild<QLineEdit*>( "runedit" );

    if( runLE )
        runLE->setText( name );
}


void GraphsWindow::showHideSaveChks()
{
    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic )
        ic2chk[ic]->setHidden( !mainApp()->areSaveChksShowing() );
}


void GraphsWindow::sortGraphs()
{
    drawMtx.lock();

// Sort

    if( mainApp()->isSortUserOrder() )
        p.sns.niChans.chanMap.userOrder( ig2ic );
    else
        p.sns.niChans.chanMap.defaultOrder( ig2ic );

    setSortButText();
    retileGraphsAccordingToSorting();
    selectChan( selChan );

    drawMtx.unlock();
}


/* ---------------------------------------------------------------- */
/* putScans ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

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

void GraphsWindow::putScans( vec_i16 &data, quint64 firstSamp )
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


void GraphsWindow::eraseGraphs()
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

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::setTriggerLED( bool on )
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = this->findChild<QLED*>( "trigLED" );

    if( led )
        led->setValue( on );
}


void GraphsWindow::setGateLED( bool on )
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = this->findChild<QLED*>( "gateLED" );

    if( led )
        led->setValue( on );
}


void GraphsWindow::blinkTrigger()
{
    setTriggerLED( true );
    QTimer::singleShot( 50, this, SLOT(blinkTrigger_Off()) );
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::blinkTrigger_Off()
{
    setTriggerLED( false );
}


void GraphsWindow::toggleFetcher()
{
    paused = !paused;
    mainApp()->getRun()->grfPause( paused );
    updateToolbar();
}


// Note: Resizing of the maximized graph is automatic
// upon hiding the other frames in the layout.
//
void GraphsWindow::toggleMaximize()
{
    QTabWidget	*tabs = mainTabWidget();
    int			nTabs = tabs->count();

    if( maximized ) {   // restore multi-graph view

        tabs->hide();

        for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {
            if( ic != selChan )
                ic2frame[ic]->show();
        }

        for( int itab = 0; itab < nTabs; ++itab )
            tabs->setTabEnabled( itab, true );

        tabs->show();
        maximized = 0;
    }
    else {  // show only selected

        selectSelChanTab();

        tabs->hide();

        for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {
            if( ic != selChan )
                ic2frame[ic]->hide();
        }

        int cur = tabs->currentIndex();

        for( int itab = 0; itab < nTabs; ++itab )
            tabs->setTabEnabled( itab, itab == cur );

        tabs->show();
        maximized = ic2G[selChan];
    }

    updateToolbar();
}


void GraphsWindow::graphSecsChanged( double secs )
{
    setGraphTimeSecs( selChan, secs );
    saveGraphSettings();
}


void GraphsWindow::graphYScaleChanged( double scale )
{
    GLGraphX    &X = ic2X[selChan];

    drawMtx.lock();
    X.yscale = scale;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveGraphSettings();
}


void GraphsWindow::doGraphColorDialog()
{
    QColorDialog::setCustomColor( 0, NeuGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 1, AuxGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 2, DigGraphBGColor.rgb() );

    QColor c = QColorDialog::getColor( ic2X[selChan].trace_Color, this );

    if( c.isValid() ) {

        GLGraphX    &X = ic2X[selChan];

        drawMtx.lock();
        X.trace_Color = c;
        drawMtx.unlock();

        if( X.G )
            X.G->update();

        updateToolbar();
        saveGraphSettings();
    }
}


void GraphsWindow::applyAll()
{
    QDoubleSpinBox  *xspin, *yspin;

    xspin = graphCtls->findChild<QDoubleSpinBox*>( "xspin" );
    yspin = graphCtls->findChild<QDoubleSpinBox*>( "yspin" );

    if( !xspin
        || !yspin
        || !xspin->hasAcceptableInput()
        || !yspin->hasAcceptableInput() ) {

        return;
    }

    double  secs    = xspin->text().toDouble(),
            scale   = yspin->text().toDouble();
    QColor  c       = ic2X[selChan].trace_Color;
    int     c0, cLim;

// Copy settings to like type {neural, aux, digital}.
// For digital, only secs is used.

    if( selChan < p.ni.niCumTypCnt[CniCfg::niSumNeural] ) {
        c0      = 0;
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumNeural];
    }
    else if( selChan < p.ni.niCumTypCnt[CniCfg::niSumAnalog] ) {
        c0      = p.ni.niCumTypCnt[CniCfg::niSumNeural];
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
    }
    else {
        c0      = p.ni.niCumTypCnt[CniCfg::niSumAnalog];
        cLim    = p.ni.niCumTypCnt[CniCfg::niSumAll];
    }

    drawMtx.lock();

    for( int ic = c0; ic < cLim; ++ic ) {

        ic2X[ic].yscale         = scale;
        ic2X[ic].trace_Color    = c;
        setGraphTimeSecs( ic, secs );   // calls update
    }

    drawMtx.unlock();

    saveGraphSettings();
}


void GraphsWindow::hpfChk( bool b )
{
    hipassMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( b )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );

    hipassMtx.unlock();

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "DataOptions" );
    settings.setValue( "filter", b );
}


void GraphsWindow::setTrgEnable( bool checked )
{
    QCheckBox   *trgChk = graphCtls->findChild<QCheckBox*>( "trgchk" );
    QLineEdit   *rLE    = graphCtls->findChild<QLineEdit*>( "runedit" );
    QLED        *led    = this->findChild<QLED*>( "trigLED" );

    if( !trgChk || !rLE )
        return;

    ConfigCtl*  cfg = mainApp()->cfgCtl();
    Run*        run = mainApp()->getRun();

    if( checked ) {

        QRegExp re("(.*)_[gG](\\d+)_[tT](\\d+)$");
        QString name    = rLE->text().trimmed();
        int     g       = -1,
                t       = -1;

        if( sender() ) {

            if( name.contains( re ) ) {

                name    = re.cap(1);
                g       = re.cap(2).toInt();
                t       = re.cap(3).toInt();
            }

            if( name.compare( p.sns.runName, Qt::CaseInsensitive ) != 0 ) {

                // different run name...reset gt counters

                QString err;

                if( !cfg->validRunName( err, name, this, true ) ) {

                    if( !err.isEmpty() )
                        QMessageBox::warning( this, "Run Name Error", err );

                    trgChk->setChecked( false );
                    return;
                }

                cfg->setRunName( name );
                run->grfUpdateWindowTitles();
                run->dfResetGTCounters();
            }
            else if( t > -1 ) {

                // Same run name...adopt given gt counters;
                // then obliterate user indices so on a
                // subsequent pause they don't get read again.

                run->dfForceGTCounters( g, t );
                rLE->setText( name );
            }
        }

        if( led )
            led->setOnColor( QLED::Green );
    }
    else if( led )
        led->setOnColor( QLED::Yellow );

    rLE->setDisabled( checked );
    run->dfSetTrgEnabled( checked );

// update graph checks

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic )
        ic2chk[ic]->setDisabled( checked );
}


void GraphsWindow::saveNiGraphChecked( bool checked )
{
    int thisChan = sender()->objectName().toInt();

    mainApp()->cfgCtl()->graphSetsNiSaveBit( thisChan, checked );
}


void GraphsWindow::selectChan( int ic )
{
    if( ic < 0 )
        return;

    QLabel  *chanLbl = graphCtls->findChild<QLabel*>( "chanlbl" );

    ic2frame[selChan]->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
    ic2frame[selChan = ic]->setFrameStyle( QFrame::Box | QFrame::Plain );

    chanLbl->setText( p.sns.niChans.chanMap.e[ic].name );

    updateToolbar();
}


void GraphsWindow::selectSelChanTab()
{
// find tab with selChan

    QTabWidget  *tabs   = mainTabWidget();
    int         itab    = tabs->currentIndex();

    if( mainApp()->isSortUserOrder() ) {

        for( int ig = 0; ig < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ig ) {

            if( ig2ic[ig] == selChan ) {
                itab = ig / graphsPerTab;
                break;
            }
        }
    }
    else
        itab = selChan / graphsPerTab;

// Select that tab and redraw its graphWidget...
// However, tabChange won't get a signal if the
// current tab isn't changing...so we call it.

    if( itab != tabs->currentIndex() )
        tabs->setCurrentIndex( itab );
    else
        tabChange( itab );
}


void GraphsWindow::tabChange( int itab )
{
    drawMtx.lock();

// Retire current graphs

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        GLGraph* &G = ic2G[ic];

        if( G ) {
            G->detach();
            extraGraphs.insert( G );
            G = 0;
        }
    }

// Reattach graphs to their new frames and set their states.
// Remember that some frames will already have graphs we can
// repurpose. Otherwise we fetch a graph from extraGraphs.

    for( int ig = itab * graphsPerTab;
        !extraGraphs.isEmpty() && ig < p.ni.niCumTypCnt[CniCfg::niSumAll];
        ++ig ) {

        int             ic  = ig2ic[ig];
        QFrame          *f  = ic2frame[ic];
        QList<GLGraph*> GL  = f->findChildren<GLGraph*>();

        GLGraph *G = (GL.empty() ? *extraGraphs.begin() : GL[0]);
        extraGraphs.remove( G );
        ic2G[ic] = G;

        QVBoxLayout *l = dynamic_cast<QVBoxLayout*>(f->layout());
        QWidget		*gpar = G->parentWidget();

        if( l )
            l->addWidget( gpar );
        else
            f->layout()->addWidget( gpar );

        gpar->setParent( f );

        G->attach( &ic2X[ic] );

#ifdef  GRAPHS_ON_TIMERS
        G->setImmedUpdate( false );
#else
        G->setImmedUpdate( true );
#endif
   }

// Page configured, now make it visible

    gCurTab = graphTabs[itab];
    graphTabs[itab]->show();

    drawMtx.unlock();
}


void GraphsWindow::timerUpdateGraphs()
{
    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        GLGraph	*G = ic2G[ic];

        if( G && G->needsUpdateGL() )
            G->updateNow();
    }
}


void GraphsWindow::timerUpdateMouseOver()
{
    int		ic			= lastMouseOverChan;
    bool	isNowOver	= true;

    if( ic < 0 || ic >= p.ni.niCumTypCnt[CniCfg::niSumAll] ) {
        statusBar()->clearMessage();
        return;
    }

    QWidget	*w = QApplication::widgetAt( QCursor::pos() );

    if( !w || !dynamic_cast<GLGraph*>(w) )
        isNowOver = false;

    double		x       = lastMousePos.x,
                y       = lastMousePos.y,
                mean, rms, stdev;
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

    statusBar()->showMessage( msg );
}


void GraphsWindow::mouseOverGraph( double x, double y )
{
    lastMouseOverChan   = graph2Chan( sender() );
    lastMousePos        = Vec2( x, y );
    timerUpdateMouseOver();
}


void GraphsWindow::mouseClickGraph( double x, double y )
{
    mouseOverGraph( x, y );
    selectChan( lastMouseOverChan );
}


void GraphsWindow::mouseDoubleClickGraph( double x, double y )
{
    mouseClickGraph( x, y );
    toggleMaximize();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Force Ctrl+A events to be treated as 'show AO-dialog',
// instead of 'text-field select-all'.
//
bool GraphsWindow::eventFilter( QObject *watched, QEvent *event )
{
    if( event->type() == QEvent::KeyPress ) {

        QKeyEvent   *e = static_cast<QKeyEvent*>(event);

        if( e->modifiers() == Qt::ControlModifier ) {

            if( e->key() == Qt::Key_A ) {
                mainApp()->act.aoDlgAct->trigger();
                e->ignore();
                return true;
            }
        }
    }

    return QMainWindow::eventFilter( watched, event );
}


void GraphsWindow::keyPressEvent( QKeyEvent *e )
{
    if( e->modifiers() == Qt::ControlModifier ) {

        if( e->key() == Qt::Key_L ) {

            mainApp()->act.shwHidConsAct->trigger();
            e->accept();
            return;
        }
    }
    else if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
        return;
    }

    QWidget::keyPressEvent( e );
}


// Intercept the close box. Rather than close here,
// we ask the run manager if it's OK to close and if
// so, the the run manager will delete us as part of
// the stopTask sequence.
//
void GraphsWindow::closeEvent( QCloseEvent *e )
{
    mainApp()->file_AskStopRun();
    e->ignore();
}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* Private helpers ------------------------------------------------ */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */

int GraphsWindow::getNumGraphsPerTab() const
{
    int lim = MAX_GRAPHS_PER_TAB;

    if( p.ni.isMuxingMode() )
        lim = p.ni.muxFactor * (lim / p.ni.muxFactor);

    if( p.sns.maxGrfPerTab && p.sns.maxGrfPerTab <= lim )
        return p.sns.maxGrfPerTab;

    return lim;
}


void GraphsWindow::initTabs()
{
    graphsPerTab = getNumGraphsPerTab();

    int nTabs = p.ni.niCumTypCnt[CniCfg::niSumAll] / graphsPerTab;

    if( nTabs * graphsPerTab < p.ni.niCumTypCnt[CniCfg::niSumAll] )
        ++nTabs;

    graphTabs.resize( nTabs );

    QTabWidget *tabs = new QTabWidget( this );
    tabs->setFocusPolicy( Qt::StrongFocus );
    Connect( tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)) );

    setCentralWidget( tabs );
}


/*	Toolbar layout:
    ---------------
    Sort selector
    selected channel label
    |sep|
    Pause
    Expand
    |sep|
    Seconds
    Yscale
    Color
    Apply all
    |sep|
    Filter
    |sep|
    Trigger enable
    Next run name
    |sep|
    Stop Run
*/

void GraphsWindow::initToolbar()
{
    QAction         *A;
    QDoubleSpinBox  *S;
    QPushButton     *B;
    QCheckBox       *C;
    QLineEdit       *E;
    QLabel          *L;

    graphCtls = addToolBar( "Graph Controls" );

// Sort selector

    B = new QPushButton( graphCtls );
    B->setObjectName( "sortbtn" );
    B->setToolTip(
        "Toggle graph sort order: user/acquired" );
    setSortButText();
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.srtUsrOrderAct, SLOT(trigger()) );
    graphCtls->addWidget( B );

// Channel label

    L = new ClickableLabel( graphCtls );
    L->setObjectName( "chanlbl" );
    L->setToolTip( "Selected graph (click to find)" );
    L->setMargin( 3 );
    L->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( L, SIGNAL(clicked()), this, SLOT(selectSelChanTab()) );
    graphCtls->addWidget( L );

// Pause

    graphCtls->addSeparator();

    A = graphCtls->addAction(
            *pauseIcon,
            "Pause/unpause all graphs",
            this, SLOT(toggleFetcher()) );
    A->setObjectName( "pauseact" );
    A->setCheckable( true );

// Expand

    A = graphCtls->addAction(
            *graphMaxedIcon,
            "Maximize/Restore graph",
            this, SLOT(toggleMaximize()) );
    A->setObjectName( "maxact" );
    A->setCheckable( true );

// Seconds

    graphCtls->addSeparator();

    L = new QLabel( "Secs", graphCtls );
    graphCtls->addWidget( L );

    S = new QDoubleSpinBox( graphCtls );
    S->setObjectName( "xspin" );
    S->installEventFilter( this );
    S->setDecimals( 3 );
    S->setRange( .001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(graphSecsChanged(double)) );
    graphCtls->addWidget( S );

// Yscale

    L = new QLabel( "YScale", graphCtls );
    graphCtls->addWidget( L );

    S = new QDoubleSpinBox( graphCtls );
    S->setObjectName( "yspin" );
    S->installEventFilter( this );
    S->setRange( 0.01, 9999.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(graphYScaleChanged(double)) );
    graphCtls->addWidget( S );

// Color

    L = new QLabel( "Color", graphCtls );
    graphCtls->addWidget( L );

    B = new QPushButton( graphCtls );
    B->setObjectName( "colorbtn" );
    ConnectUI( B, SIGNAL(clicked(bool)), this, SLOT(doGraphColorDialog()) );
    graphCtls->addWidget( B );

// Apply all

    graphCtls->addAction(
        *applyAllIcon,
        "Apply {secs,scale,color} to all graphs of like type",
        this, SLOT(applyAll()) );

// Filter

    graphCtls->addSeparator();

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "DataOptions" );

    bool    setting_flt = settings.value( "filter", false ).toBool();

    C = new QCheckBox( "Filter <300Hz", graphCtls );
    C->setObjectName( "filterchk" );
    C->setToolTip( "Applied only to neural channels" );
    C->setChecked( setting_flt );
    ConnectUI( C, SIGNAL(clicked(bool)), this, SLOT(hpfChk(bool)) );
    graphCtls->addWidget( C );

// Trigger enable

    graphCtls->addSeparator();

    C = new QCheckBox( "Trig enabled", graphCtls );
    C->setObjectName( "trgchk" );
    C->setChecked( !p.mode.trgInitiallyOff );
    ConnectUI( C, SIGNAL(clicked(bool)), this, SLOT(setTrgEnable(bool)) );
    graphCtls->addWidget( C );

// Run name

    E = new QLineEdit( p.sns.runName, graphCtls );
    E->setObjectName( "runedit" );
    E->installEventFilter( this );
    E->setToolTip( "<newName>, or, <curName_gxx_txx>" );
    E->setEnabled( p.mode.trgInitiallyOff );
    E->setMinimumWidth( 100 );
    graphCtls->addWidget( E );

// Stop

    graphCtls->addSeparator();

    B = new QPushButton( graphCtls );
    B->setText( "Stop Run" );
    B->setToolTip(
        "Stop experiment, close files, close this window" );
    B->setAutoFillBackground( true );
    B->setStyleSheet( "color: rgb(170, 0, 0)" );    // brick red
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.stopAcqAct, SLOT(trigger()) );
    graphCtls->addWidget( B );
}


QWidget *GraphsWindow::initLEDWidget()
{
    QMutexLocker    ml( &LEDMtx );
    QHBoxLayout     *HBX = new QHBoxLayout;
    QLabel          *LBL;
    QLED            *LED;

// gate LED label

    LBL = new QLabel( "Gate:" );
    HBX->addWidget( LBL );

// gate LED

    LED = new QLED;
    LED->setObjectName( "gateLED" );
    LED->setOffColor( QLED::Red );
    LED->setOnColor( QLED::Green );
    LED->setMinimumSize( 20, 20 );
    HBX->addWidget( LED );

// trigger LED label

    LBL = new QLabel( "Trigger:" );
    HBX->addWidget( LBL );

// trigger LED

    LED = new QLED;
    LED->setObjectName( "trigLED" );
    LED->setOffColor( QLED::Red );
    LED->setOnColor( p.mode.trgInitiallyOff ? QLED::Yellow : QLED::Green );
    LED->setMinimumSize( 20, 20 );
    HBX->addWidget( LED );

// combine LEDs into widget

    QWidget *LEDs = new QWidget;
    LEDs->setLayout( HBX );

    return LEDs;
}


void GraphsWindow::initStatusBar()
{
    statusBar();
    statusBar()->addPermanentWidget( initLEDWidget() );
}


bool GraphsWindow::initNiFrameCheckBox( QFrame* &f, int ic )
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
    ConnectUI( C, SIGNAL(toggled(bool)), this, SLOT(saveNiGraphChecked(bool)) );

    return true;
}


void GraphsWindow::initFrameGraph( QFrame* &f, int ic )
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


void GraphsWindow::initFrames()
{
    QTabWidget  *tabs   = mainTabWidget();
    int         nTabs   = graphTabs.size(),
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

                if( initNiFrameCheckBox( f, ic ) ) {
                    initFrameGraph( f, ic );
                    grid->addWidget( f, r, c );
                }
            }
        }

add_tab:
        tabs->addTab( graphsWidget, QString::null );
        setTabText( itab, ig - 1 );
    }
}


void GraphsWindow::initAssertFilters()
{
    QCheckBox*   C;

    if( (C = graphCtls->findChild<QCheckBox*>( "filterchk" )) )
        hpfChk( C->isChecked() );
}


void GraphsWindow::initUpdateTimers()
{
#ifdef GRAPHS_ON_TIMERS

    QTimer *t = new QTimer( this );
    Connect( t, SIGNAL(timeout()), this, SLOT(timerUpdateGraphs()), Qt::QueuedConnection );
    t->start( 127 );

//    t = new QTimer( this );
//    Connect( t, SIGNAL(timeout()), this, SLOT(timerUpdateMouseOver()), Qt::QueuedConnection );
//    t->start( 1000 ); // 1 second

#endif
}


void GraphsWindow::initWindow()
{
    gCurTab = 0;
    trgChan = p.trigChan();

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

    resize( 1024, 768 );

    initIcons();
    initTabs();
    initToolbar();
    initStatusBar();

    loadGraphSettings();
    initFrames();
    initAssertFilters();

// select first tab and graph
    selectChan( ig2ic[0] );
    tabChange( 0 );

    initUpdateTimers();
}


int GraphsWindow::graph2Chan( QObject *graphObj )
{
    GLGraph *G = dynamic_cast<GLGraph*>(graphObj);

    if( G )
        return G->getX()->num;
    else
        return -1;
}


struct SignalBlocker
{
    QObject *obj;
    bool    wasBlocked;

    SignalBlocker( QObject *obj ) : obj(obj)
    {
        wasBlocked = obj->signalsBlocked();
        obj->blockSignals( true );
    }

    virtual ~SignalBlocker() {obj->blockSignals(wasBlocked);}
};


void GraphsWindow::updateToolbar()
{
    QAction         *pause, *maxmz;
    QDoubleSpinBox  *xspin, *yspin;
    QPushButton     *colorbtn;

    pause       = graphCtls->findChild<QAction*>( "pauseact" );
    maxmz       = graphCtls->findChild<QAction*>( "maxact" );
    xspin       = graphCtls->findChild<QDoubleSpinBox*>( "xspin" );
    yspin       = graphCtls->findChild<QDoubleSpinBox*>( "yspin" );
    colorbtn    = graphCtls->findChild<QPushButton*>( "colorbtn" );

    SignalBlocker
            b0(pause),
            b1(maxmz),
            b2(xspin),
            b3(yspin),
            b4(colorbtn);

    pause->setChecked( paused );
    pause->setIcon( paused ? *playIcon : *pauseIcon );

    if( maximized ) {
        maxmz->setChecked( true );
        maxmz->setIcon( *graphNormalIcon );
    }
    else {
        maxmz->setChecked( false );
        maxmz->setIcon( *graphMaxedIcon );
    }

// Analog and digital

    GLGraphX    &X = ic2X[selChan];

    xspin->setValue( X.spanSecs() );
    yspin->setValue( X.yscale );

    QPixmap     pm( 22, 22 );
    QPainter    pnt;

    pnt.begin( &pm );
    pnt.fillRect( 0, 0, 22, 22, QBrush( X.trace_Color ) );
    pnt.end();
    colorbtn->setIcon( QIcon( pm ) );

// Type-specific

    bool    enabled = selChan < p.ni.niCumTypCnt[CniCfg::niSumAnalog];

    yspin->setEnabled( enabled );
    colorbtn->setEnabled( enabled );
}


void GraphsWindow::setGraphTimeSecs( int ic, double t )
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
double GraphsWindow::scalePlotValue( double v, double gain )
{
    return p.ni.range.unityToVolts( (v+1)/2 ) / gain;
}


// Call this only for analog channels!
//
void GraphsWindow::computeGraphMouseOverVars(
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


void GraphsWindow::setSortButText()
{
    QPushButton *B = graphCtls->findChild<QPushButton*>( "sortbtn" );

    if( B ) {
        if( mainApp()->isSortUserOrder() )
            B->setText( "Usr Order" );
        else
            B->setText( "Acq Order" );
    }
}


void GraphsWindow::setTabText( int itab, int igLast )
{
   mainTabWidget()->setTabText(
        itab,
        QString("%1 %2-%3")
            .arg( mainApp()->isSortUserOrder() ? "Usr" : "Acq" )
            .arg( itab*graphsPerTab )
            .arg( igLast ) );
}


void GraphsWindow::retileGraphsAccordingToSorting()
{
    int nTabs   = graphTabs.size(),
        ig      = 0;

    if( maximized )
        toggleMaximize();

    for( int itab = 0; itab < nTabs; ++itab ) {

        QWidget     *graphsWidget   = graphTabs[itab];

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

    selectSelChanTab();
}


void GraphsWindow::saveGraphSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "PlotOptions" );

    for( int ic = 0; ic < p.ni.niCumTypCnt[CniCfg::niSumAll]; ++ic ) {

        settings.setValue(
            QString("chan%1").arg( ic ),
            ic2X[ic].toString() );
    }

    settings.endGroup();
}


void GraphsWindow::loadGraphSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "PlotOptions" );

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


