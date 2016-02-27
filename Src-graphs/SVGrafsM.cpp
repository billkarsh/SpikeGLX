
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SVToolsM.h"
#include "DAQ.h"

#include <QVBoxLayout>




/* ---------------------------------------------------------------- */
/* class SVGrafsM ------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM::SVGrafsM( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), drawMtx(QMutex::Recursive),
        trgChan(p.trigChan()), lastMouseOverChan(-1),
        selected(-1), maximized(-1), externUpdateTimes(true)
{
}


void SVGrafsM::init( SVToolsM *tb )
{
    this->tb = tb;

    loadSettings();

// ----------------
// Top-level layout
// ----------------

    nv      = new MNavbar( this );
    theX    = new MGraphX;
    theM    = new MGraph( "gw", this, theX );

    QVBoxLayout *V = new QVBoxLayout( this );
    V->setSpacing( 0 );
    V->setMargin( 0 );
    V->addWidget( nv );
    V->addWidget( theM, 1 );

// -----------
// Size arrays
// -----------

    int n = chanCount();

    ic2Y.resize( n );
    ic2stat.resize( n );
    ic2iy.resize( n );
    ig2ic.resize( n );

// ----------
// Init items
// ----------

    tb->init();
    filterChkClicked( set.filterChkOn );

    ic2iy.fill( -1 );
    mySort_ig2ic();
    initGraphs();

    pageChange( 0 );
    selectChan( ig2ic[0] );
    nv->update();
}


// Note: Derived class dtors called before base class.
//
SVGrafsM::~SVGrafsM()
{
}


void SVGrafsM::eraseGraphs()
{
    drawMtx.lock();
    theX->dataMtx->lock();

    for( int ic = 0, nC = chanCount(); ic < nC; ++ic )
        ic2Y[ic].yval.erase();

    theX->dataMtx->unlock();
    drawMtx.unlock();
}


void SVGrafsM::getSelScales( double &xSpn, double &yScl ) const
{
    xSpn = theX->spanSecs();
    yScl = ic2Y[selected].yscl;
}


QColor SVGrafsM::getSelColor() const
{
    return theX->yColor[ic2Y[selected].iclr];
}


void SVGrafsM::nchanChanged( int val, int first )
{
    drawMtx.lock();
    set.navNChan = val;
    drawMtx.unlock();

    pageChange( first );
    selectChan( selected );

    saveSettings();
}


void SVGrafsM::firstChanged( int first )
{
    pageChange( first );
}


void SVGrafsM::toggleSorting()
{
    if( maximized >= 0 )
        toggleMaximized();

    drawMtx.lock();
    set.usrOrder = !set.usrOrder;
    mySort_ig2ic();
    externUpdateTimes = true;
    drawMtx.unlock();

    ensureSelectionVisible();

    saveSettings();
    tb->update();
    nv->update();
}


void SVGrafsM::ensureSelectionVisible()
{
    ensureVisible();
    selectChan( selected );
}


void SVGrafsM::toggleMaximized()
{
    int first;

    if( maximized >= 0 ) {   // restore multi-graph view

        first = nv->first();
        nv->setEnabled( true );
        maximized = -1;
    }
    else {  // show only selected

        ensureVisible();
        first = nv->first();
        nv->setEnabled( false );
        maximized = selected;
    }

    pageChange( first, false );
    selectChan( selected );
    tb->update();
}


void SVGrafsM::graphSecsChanged( double d )
{
    drawMtx.lock();

    set.secs = d;
    setGraphTimeSecs();
    theM->update();

    drawMtx.unlock();

    saveSettings();
}


void SVGrafsM::graphYScaleChanged( double d )
{
    MGraphY &Y = ic2Y[selected];

    drawMtx.lock();
    Y.yscl = d;
    drawMtx.unlock();

    theM->update();
}


void SVGrafsM::showColorDialog()
{
    MGraphY &Y  = ic2Y[selected];
    QColor  c   = tb->selectColor( theX->yColor[Y.iclr] );

    if( c.isValid() ) {

        int iclr = theX->yColor.size();

        drawMtx.lock();
        theX->yColor.push_back( c );
        Y.iclr = iclr;
        drawMtx.unlock();

        theM->update();

        tb->update();
    }
}


void SVGrafsM::applyAll()
{
// Copy settings to like usrType.

// BK: Not yet copying selected trace color to others

    const MGraphY   &Y = ic2Y[selected];

    drawMtx.lock();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        if( ic2Y[ic].usrType == Y.usrType )
            ic2Y[ic].yscl = Y.yscl;
    }

    drawMtx.unlock();

    theM->update();

    if( !Y.usrType )
        set.yscl0 = Y.yscl;
    else if( Y.usrType == 1 )
        set.yscl1 = Y.yscl;
    else
        set.yscl2 = Y.yscl;

    saveSettings();
}


void SVGrafsM::dblClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
    toggleMaximized();
}


QString SVGrafsM::clrToString( QColor c )
{
    return QString("%1").arg( c.rgb(), 0, 16 );
}


QColor SVGrafsM::clrFromString( QString s )
{
    return QColor::fromRgba( (QRgb)s.toUInt( 0, 16 ) );
}


void SVGrafsM::selectChan( int ic )
{
    if( ic < 0 )
        return;

    if( ic != selected ) {

        selected = ic;
        tb->setSelName( myChanName( ic ) );
        tb->update();
    }

    theX->setYSelByUsrChan( ic );
    theM->update();
}


void SVGrafsM::initGraphs()
{
    theM->setImmedUpdate( true );
    ConnectUI( theM, SIGNAL(cursorOver(double,double,int)), this, SLOT(myMouseOverGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutClicked(double,double,int)), this, SLOT(myClickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutDoubleClicked(double,double,int)), this, SLOT(dblClickGraph(double,double,int)) );

    theX->Y.clear();
    theX->yColor.clear();
    theX->yColor.push_back( set.clr0 );
    theX->yColor.push_back( set.clr1 );
    theX->yColor.push_back( set.clr2 );

    digitalType = mySetUsrTypes();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        Y.yscl      = (Y.usrType == 0 ? set.yscl0 :
                        (Y.usrType == 1 ? set.yscl1 : set.yscl2));
        Y.label     = myChanName( ic );
        Y.usrChan   = ic;
        Y.iclr      = Y.usrType;
        Y.isDigType = Y.usrType == digitalType;
    }
}


// This is where Mgraph is pointed to the active channels.
//
void SVGrafsM::pageChange( int first, bool internUpdateTimes )
{
    drawMtx.lock();

    theX->Y.clear();

    if( maximized >= 0 ) {

        theX->fixedNGrf = 1;
        theX->Y.push_back( &ic2Y[maximized] );
    }
    else {

        theX->fixedNGrf = 0;

        for( int ig = first, nG = ic2Y.size(); ig < nG; ++ig ) {

            int ic = ig2ic[ig];

            // BK: Analog graphs only, for now
            if( ic2Y[ic].usrType != digitalType ) {

                theX->Y.push_back( &ic2Y[ic] );

                if( ++theX->fixedNGrf >= set.navNChan )
                    break;
            }
        }
    }

    update_ic2iy( first );

    theX->calcYpxPerGrf();
    theX->setYSelByUsrChan( selected );

    if( internUpdateTimes || externUpdateTimes ) {
        setGraphTimeSecs();
        externUpdateTimes = false;
    }

    theM->update();

    drawMtx.unlock();
}


void SVGrafsM::ensureVisible()
{
// Find page with selected

    int page = nv->page();

    if( set.usrOrder ) {

        for( int ig = 0, nG = chanCount(); ig < nG; ++ig ) {

            if( ig2ic[ig] == selected ) {
                page = ig / set.navNChan;
                break;
            }
        }
    }
    else
        page = selected / set.navNChan;

// Select that page and redraw...
// However, pageChange won't get called if the
// current page isn't changing...so we call it.

    if( page != nv->page() )
        nv->setPage( page );
    else
        pageChange( page*set.navNChan, false );
}


void SVGrafsM::setGraphTimeSecs()
{
    theX->setSpanSecs( set.secs, mySampRate() );
    theX->setVGridLinesAuto();

// setSpanSecs will automatically propagate changes
// to all theX->Y[], but, if we're maximized we've
// got to propagate the change to all other graphs
// on this page manually.

    int nC = chanCount();

    if( maximized >= 0 ) {

        int newSize = ic2Y[maximized].yval.capacity();

        for( int ic = 0; ic < nC; ++ic ) {

            if( ic2iy[ic] > -1 )
                ic2Y[ic].resize( newSize );
        }
    }

// -----------
// Reset stats
// -----------

    for( int ic = 0; ic < nC; ++ic ) {

        if( ic2iy[ic] > -1 )
            ic2stat[ic].clear();
    }
}


// Same emplacement algorithm as pageChange().
//
void SVGrafsM::update_ic2iy( int first )
{
    int nG  = ic2Y.size(),
        nY  = 0;

    ic2iy.fill( -1 );

    for( int ig = first; ig < nG; ++ig ) {

        int ic = ig2ic[ig];

        // BK: Analog graphs only, for now
        if( ic2Y[ic].usrType != digitalType ) {

            ic2iy[ic] = nY++;

            if( nY >= set.navNChan )
                break;
        }
    }
}


