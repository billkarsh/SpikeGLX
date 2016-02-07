
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"
#include "GWWidgetG.h"
#include "FramePool.h"
#include "TabPage.h"
#include "DAQ.h"

#include <QFrame>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QSettings>
#include <math.h>




/* ---------------------------------------------------------------- */
/* class GWWidgetG ------------------------------------------------ */
/* ---------------------------------------------------------------- */

GWWidgetG::GWWidgetG( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), drawMtx(QMutex::Recursive),
        trgChan(p.trigChan()), lastMouseOverChan(-1), maximized(-1)
{
}


void GWWidgetG::init()
{
    int n = myChanCount();

    ic2G.resize( n );
    ic2X.resize( n );
    ic2stat.resize( n );
    ic2frame.resize( n );
    ic2chk.resize( n );
    ig2ic.resize( n );

    sort_ig2ic();

    initTabs();
    loadSettings();
    initFrames();

    tabChange( 0 );
}


// Note: Derived class dtors called before base class.
//
GWWidgetG::~GWWidgetG()
{
    drawMtx.lock();
    returnFramesToPool();
    drawMtx.unlock();
}


void GWWidgetG::eraseGraphs()
{
    drawMtx.lock();

    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

        GLGraphX    &X = ic2X[ic];

        X.dataMtx->lock();
        X.ydata.erase();
        X.dataMtx->unlock();
    }

    drawMtx.unlock();
}


void GWWidgetG::sortGraphs()
{
    drawMtx.lock();
    sort_ig2ic();
    retileBySorting();
    drawMtx.unlock();
}


// Return first sorted graph to GraphsWindow.
//
int GWWidgetG::initialSelectedChan( QString &name ) const
{
    int ic = ig2ic[0];

    name = chanName( ic );

    return ic;
}


void GWWidgetG::selectChan( int ic, bool selected )
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


void GWWidgetG::ensureVisible( int ic )
{
// find tab with ic

    int itab = currentIndex();

    if( mainApp()->isSortUserOrder() ) {

        for( int ig = 0, nG = myChanCount(); ig < nG; ++ig ) {

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
void GWWidgetG::toggleMaximized( int newMaximized )
{
    int nTabs   = count(),
        nC      = myChanCount();

    if( maximized >= 0 ) {   // restore multi-graph view

        hide();

        for( int ic = 0; ic < nC; ++ic ) {
            if( ic != maximized )
                ic2frame[ic]->show();
        }

        newMaximized = -1;

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, true );
    }
    else {  // show only newMaximized

        ensureVisible( newMaximized );

        hide();

        for( int ic = 0; ic < nC; ++ic ) {
            if( ic != newMaximized )
                ic2frame[ic]->hide();
        }

        int cur = currentIndex();

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, itab == cur );
    }

    maximized = newMaximized;

    show();
}


void GWWidgetG::getGraphScales( double &xSpn, double &yScl, int ic ) const
{
    const GLGraphX  &X = ic2X[ic];

    xSpn = X.spanSecs();
    yScl = X.yscale;
}


void GWWidgetG::graphSecsChanged( double secs, int ic )
{
    setGraphTimeSecs( ic, secs );
    saveSettings();
}


void GWWidgetG::graphYScaleChanged( double scale, int ic )
{
    GLGraphX    &X = ic2X[ic];

    drawMtx.lock();
    X.yscale = scale;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveSettings();
}


QColor GWWidgetG::getGraphColor( int ic ) const
{
    return ic2X[ic].trace_Color;
}


void GWWidgetG::colorChanged( QColor c, int ic )
{
    GLGraphX    &X = ic2X[ic];

    drawMtx.lock();
    X.trace_Color = c;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveSettings();
}


void GWWidgetG::applyAll( int ic )
{
// Copy settings to like type {neural, aux, digital}.
// For digital, only secs is used.

    int c0, cLim;

    if( !indexRangeThisType( c0, cLim, ic ) )
        return;

    const GLGraphX  &X = ic2X[ic];

    drawMtx.lock();

    for( int ic = c0; ic < cLim; ++ic ) {

        ic2X[ic].yscale         = X.yscale;
        ic2X[ic].trace_Color    = X.trace_Color;
        setGraphTimeSecs( ic, X.spanSecs() );   // calls update
    }

    drawMtx.unlock();

    saveSettings();
}


void GWWidgetG::showHideSaveChks()
{
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic )
        ic2chk[ic]->setHidden( !mainApp()->areSaveChksShowing() );
}


void GWWidgetG::enableAllChecks( bool enabled )
{
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic )
        ic2chk[ic]->setEnabled( enabled );
}


void GWWidgetG::tabChange( int itab )
{
    drawMtx.lock();

    cacheAllGraphs();

// Reattach graphs to their new frames and set their states.
// Remember that some frames will already have graphs we can
// repurpose. Otherwise we fetch a graph from graphCache.

    for( int ig = itab * graphsPerTab, nG = myChanCount();
        !graphCache.isEmpty() && ig < nG; ++ig ) {

        int     ic  = ig2ic[ig];
        QFrame  *f  = ic2frame[ic];
        GLGraph *G  = f->findChild<GLGraph*>();

        if( !G )
            G = *graphCache.begin();

        graphCache.remove( G );
        ic2G[ic] = G;

        QVBoxLayout *l = dynamic_cast<QVBoxLayout*>(f->layout());
        QWidget		*gpar = G->parentWidget();

        l->addWidget( gpar );
        gpar->setParent( f );

        G->attach( &ic2X[ic] );
        G->setImmedUpdate( true );
   }

// Page configured, now make it visible

    curTab = graphTabs[itab];
    graphTabs[itab]->show();

    drawMtx.unlock();
}


void GWWidgetG::mouseDoubleClickGraph( double x, double y )
{
    mouseClickGraph( x, y );
    toggleMaximized( lastMouseOverChan );
    gw->toggledMaximized();
}


int GWWidgetG::graph2Chan( QObject *graphObj )
{
    GLGraph *G = dynamic_cast<GLGraph*>(graphObj);

    if( G )
        return G->getX()->num;
    else
        return -1;
}


void GWWidgetG::saveSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( settingsGrpName() );

    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

        settings.setValue(
            QString("chan%1").arg( ic ),
            ic2X[ic].toString() );
    }

    settings.endGroup();
}


void GWWidgetG::loadSettings()
{
// -----------------------------------
// Display options, channel by channel
// -----------------------------------

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( settingsGrpName() );

    drawMtx.lock();

// Note on digital channels:
// The default yscale and color settings loaded here are
// correct for digital channels, and we forbid editing
// those values (see updateToolbar()).

    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

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


void GWWidgetG::initTabs()
{
    graphsPerTab = getNumGraphsPerTab();

    int nTabs = myChanCount() / graphsPerTab;

    if( nTabs * graphsPerTab < myChanCount() )
        ++nTabs;

    graphTabs.resize( nTabs );

    setFocusPolicy( Qt::StrongFocus );
    Connect( this, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)) );
}


bool GWWidgetG::initFrameCheckBox( QFrame* &f, int ic )
{
    QCheckBox*  &C = ic2chk[ic] = f->findChild<QCheckBox*>();

    C->setText(
        QString("Save %1")
        .arg( chanName( ic ) ) );

    C->setObjectName( QString().number( ic ) );
    C->setChecked( mySaveBits().at( ic ) );
    C->setEnabled( p.mode.trgInitiallyOff );
    C->setHidden( !mainApp()->areSaveChksShowing() );
    ConnectUI( C, SIGNAL(toggled(bool)), this, SLOT(saveGraphClicked(bool)) );

    return true;
}


void GWWidgetG::initFrameGraph( QFrame* &f, int ic )
{
    GLGraph     *G  = ic2G[ic] = f->findChild<GLGraph*>();
    GLGraphX    &X  = ic2X[ic];

    if( G ) {
        Connect( G, SIGNAL(cursorOver(double,double)), this, SLOT(mouseOverGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutClicked(double,double)), this, SLOT(mouseClickGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutDoubleClicked(double,double)), this, SLOT(mouseDoubleClickGraph(double,double)) );
    }

    X.num = ic;   // link graph to hwr channel

    customXSettings( ic );
}


void GWWidgetG::initFrames()
{
    int nTabs   = graphTabs.size(),
        nG      = myChanCount(),
        ig      = 0;

    for( int itab = 0; itab < nTabs; ++itab ) {

        QWidget     *graphsWidget   = new TabPage( &curTab );
        QGridLayout *grid           = new QGridLayout( graphsWidget );

        graphTabs[itab] = graphsWidget;

        grid->setHorizontalSpacing( 1 );
        grid->setVerticalSpacing( 1 );

        int remChans    = nG - itab*graphsPerTab,
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


void GWWidgetG::setGraphTimeSecs( int ic, double t )
{
    drawMtx.lock();

    if( ic >= 0 && ic < myChanCount() ) {

        GLGraphX    &X = ic2X[ic];

        X.setSpanSecs( t, p.ni.srate );
        X.setVGridLinesAuto();

        if( X.G )
            X.G->update();

        ic2stat[ic].clear();
    }

    drawMtx.unlock();
}


void GWWidgetG::setTabText( int itab, int igLast )
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


void GWWidgetG::retileBySorting()
{
    int nTabs   = graphTabs.size(),
        nG      = myChanCount(),
        ig      = 0;

    for( int itab = 0; itab < nTabs; ++itab ) {

        QWidget *graphsWidget = graphTabs[itab];

        delete graphsWidget->layout();
        QGridLayout *grid = new QGridLayout( graphsWidget );

        grid->setHorizontalSpacing( 1 );
        grid->setVerticalSpacing( 1 );

        int remChans    = nG - itab*graphsPerTab,
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


void GWWidgetG::cacheAllGraphs()
{
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic ) {

        GLGraph* &G = ic2G[ic];

        if( G ) {
            G->detach();
            graphCache.insert( G );
            G = 0;
        }
    }
}


void GWWidgetG::returnFramesToPool()
{
// Return cached graphs to graph-less frames

    if( !graphCache.isEmpty() ) {

        for( int ic = 0, nC = ic2frame.size(); ic < nC; ++ic ) {

            QFrame  *f = ic2frame[ic];

            if( f->findChild<GLGraph*>() )
                continue;

            // No graph, stash here

            QVBoxLayout *l = dynamic_cast<QVBoxLayout*>(f->layout());
            GLGraph     *G = *graphCache.begin();
            QWidget     *P = G->parentWidget();

            graphCache.remove( G );
            l->addWidget( P );
            P->setParent( f );

            if( graphCache.isEmpty() )
                break;
        }
    }

// Return frames to pool

    mainApp()->pool->allFramesToPool( ic2frame );
}


