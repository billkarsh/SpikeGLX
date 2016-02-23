
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"
#include "SVGrafsG.h"
#include "SVToolsG.h"
#include "FramePool.h"
#include "TabPage.h"
#include "DAQ.h"

#include <QFrame>
#include <QCheckBox>
#include <QVBoxLayout>
#include <math.h>




/* ---------------------------------------------------------------- */
/* class SVGrafsG ------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsG::SVGrafsG( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), drawMtx(QMutex::Recursive),
        trgChan(p.trigChan()), lastMouseOverChan(-1),
        selected(-1), maximized(-1)
{
}


void SVGrafsG::init( SVToolsG *tb )
{
    this->tb = tb;

    int n = myChanCount();

    ic2G.resize( n );
    ic2X.resize( n );
    ic2stat.resize( n );
    ic2frame.resize( n );
    ic2chk.resize( n );
    ig2ic.resize( n );

    loadSettings();
    hipassClicked( set.filter );

    mySort_ig2ic();
    initTabs();
    initFrames();

    tabChange( 0 );
    selectChan( ig2ic[0] );
    tb->update();
}


// Note: Derived class dtors called before base class.
//
SVGrafsG::~SVGrafsG()
{
    drawMtx.lock();
    returnFramesToPool();
    drawMtx.unlock();
}


void SVGrafsG::eraseGraphs()
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


void SVGrafsG::getSelScales( double &xSpn, double &yScl ) const
{
    const GLGraphX  &X = ic2X[selected];

    xSpn = X.spanSecs();
    yScl = X.yscale;
}


QColor SVGrafsG::getSelColor() const
{
    return ic2X[selected].trace_Color;
}


void SVGrafsG::showHideSaveChks()
{
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic )
        ic2chk[ic]->setHidden( !mainApp()->areSaveChksShowing() );
}


void SVGrafsG::enableAllChecks( bool enabled )
{
    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic )
        ic2chk[ic]->setEnabled( enabled );
}


void SVGrafsG::toggleSorting()
{
    if( maximized >= 0 )
        toggleMaximized();

    drawMtx.lock();
    set.usrOrder = !set.usrOrder;
    mySort_ig2ic();
    retileBySorting();
    drawMtx.unlock();

    ensureSelectionVisible();

    saveSettings();
    tb->update();
}


void SVGrafsG::ensureSelectionVisible()
{
    ensureVisible();
    selectChan( selected );
}


// Note: Resizing of the maximized graph is automatic
// upon hiding the other frames in the layout.
//
void SVGrafsG::toggleMaximized()
{
    int nTabs   = count(),
        nC      = myChanCount();

    hide();

    if( maximized >= 0 ) {   // restore multi-graph view

        for( int ic = 0; ic < nC; ++ic ) {
            if( ic != maximized )
                ic2frame[ic]->show();
        }

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, true );

        maximized = -1;
    }
    else {  // show only selected

        ensureVisible();

        for( int ic = 0; ic < nC; ++ic ) {
            if( ic != selected )
                ic2frame[ic]->hide();
        }

        int cur = currentIndex();

        for( int itab = 0; itab < nTabs; ++itab )
            setTabEnabled( itab, itab == cur );

        maximized = selected;
    }

    selectChan( selected );
    show();
    tb->update();
}


void SVGrafsG::graphSecsChanged( double d )
{
    setGraphTimeSecs( selected, d );
    saveSettings();
}


void SVGrafsG::graphYScaleChanged( double d )
{
    GLGraphX    &X = ic2X[selected];

    drawMtx.lock();
    X.yscale = d;
    drawMtx.unlock();

    if( X.G )
        X.G->update();

    saveSettings();
}


void SVGrafsG::showColorDialog()
{
    GLGraphX    &X  = ic2X[selected];
    QColor      c   = tb->selectColor( X.trace_Color );

    if( c.isValid() ) {

        drawMtx.lock();
        X.trace_Color = c;
        drawMtx.unlock();

        if( X.G )
            X.G->update();

        saveSettings();

        tb->update();
    }
}


void SVGrafsG::applyAll()
{
// Copy settings to like usrType.
// For digital, only secs is used.

    const GLGraphX  &X = ic2X[selected];

    drawMtx.lock();

    for( int ic = 0, nC = ic2X.size(); ic < nC; ++ic ) {

        if( ic2X[ic].usrType == X.usrType ) {

            ic2X[ic].yscale         = X.yscale;
            ic2X[ic].trace_Color    = X.trace_Color;
            setGraphTimeSecs( ic, X.spanSecs() );   // calls update
        }
    }

    drawMtx.unlock();

    saveSettings();
}


void SVGrafsG::tabChange( int itab )
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


void SVGrafsG::dblClickGraph( double x, double y )
{
    myClickGraph( x, y );
    toggleMaximized();
}


int SVGrafsG::graph2Chan( QObject *graphObj )
{
    GLGraph *G = dynamic_cast<GLGraph*>(graphObj);

    if( G )
        return G->getX()->usrChan;
    else
        return -1;
}


void SVGrafsG::selectChan( int ic )
{
    if( ic < 0 )
        return;

    int style;

// Deselect old

    if( selected >= 0 && ic != selected ) {

        style = QFrame::Plain | QFrame::StyledPanel;
        ic2frame[selected]->setFrameStyle( style );
    }

// Select new

    if( ic != selected ) {

        selected = ic;
        tb->setSelName( myChanName( ic ) );

        style = QFrame::Plain | QFrame::Box;
        ic2frame[ic]->setFrameStyle( style );
    }
}


void SVGrafsG::initTabs()
{
    graphsPerTab = myGrfPerTab();

    int nTabs = myChanCount() / graphsPerTab;

    if( nTabs * graphsPerTab < myChanCount() )
        ++nTabs;

    graphTabs.resize( nTabs );

    setFocusPolicy( Qt::StrongFocus );
    Connect( this, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)) );
}


bool SVGrafsG::initFrameCheckBox( QFrame* &f, int ic )
{
    QCheckBox*  &C = ic2chk[ic] = f->findChild<QCheckBox*>();

    C->setText(
        QString("Save %1")
        .arg( myChanName( ic ) ) );

    C->setObjectName( QString().number( ic ) );
    C->setChecked( mySaveBits().at( ic ) );
    C->setEnabled( p.mode.manOvInitOff );
    C->setHidden( !mainApp()->areSaveChksShowing() );
    ConnectUI( C, SIGNAL(toggled(bool)), this, SLOT(mySaveGraphClicked(bool)) );

    return true;
}


void SVGrafsG::initFrameGraph( QFrame* &f, int ic )
{
    GLGraph     *G  = ic2G[ic] = f->findChild<GLGraph*>();
    GLGraphX    &X  = ic2X[ic];

    if( G ) {
        Connect( G, SIGNAL(cursorOver(double,double)), this, SLOT(myMouseOverGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutClicked(double,double)), this, SLOT(myClickGraph(double,double)) );
        ConnectUI( G, SIGNAL(lbutDoubleClicked(double,double)), this, SLOT(dblClickGraph(double,double)) );
    }

    X.usrChan = ic;

    myCustomXSettings( ic );
}


void SVGrafsG::initFrames()
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
        addTab( graphsWidget, QString("%1").arg( itab*graphsPerTab ) );
    }
}


void SVGrafsG::ensureVisible()
{
// find tab with ic

    int itab = currentIndex();

    if( set.usrOrder ) {

        for( int ig = 0, nG = myChanCount(); ig < nG; ++ig ) {

            if( ig2ic[ig] == selected ) {
                itab = ig / graphsPerTab;
                break;
            }
        }
    }
    else
        itab = selected / graphsPerTab;

// Select that tab and redraw its graphWidget...
// However, tabChange won't get a signal if the
// current tab isn't changing...so we call it.

    if( itab != currentIndex() )
        setCurrentIndex( itab );
    else
        tabChange( itab );
}


void SVGrafsG::setGraphTimeSecs( int ic, double t )
{
    if( ic >= 0 && ic < myChanCount() ) {

        GLGraphX    &X = ic2X[ic];

        drawMtx.lock();

        X.setSpanSecs( t, mySampRate() );
        X.setVGridLinesAuto();

        if( X.G )
            X.G->update();

        ic2stat[ic].clear();

        drawMtx.unlock();
    }
}


void SVGrafsG::retileBySorting()
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
                    goto next_tab;

                QFrame* &f = ic2frame[ig2ic[ig]];

                f->setParent( graphsWidget );
                grid->addWidget( f, r, c );
            }
        }

next_tab:;
    }
}


void SVGrafsG::cacheAllGraphs()
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


void SVGrafsG::returnFramesToPool()
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


