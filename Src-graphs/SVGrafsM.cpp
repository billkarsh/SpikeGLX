
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "DAQ.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SVToolsM.h"
#include "SVShankCtl.h"
#include "ShankMap.h"
#include "Biquad.h"
#include "ColorTTLCtl.h"

#include <QStatusBar>
#include <QVBoxLayout>


/* ---------------------------------------------------------------- */
/* class DCAve ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::DCAve::init( int nChannels, int c0, int cLim )
{
    nC  = nChannels;
    i0  = c0;
    nI  = cLim - c0;
    lvl.assign( nI, 0 );
    clock = 0.0;
}


void SVGrafsM::DCAve::setChecked( bool checked )
{
    sum.assign( nI, 0.0F );
    cnt = 0;

    if( !checked )
        lvl.assign( nI, 0 );
    else
        clock = 0.0;
}


// Every 5 seconds the level is updated, based
// upon averaging over the preceding 1 second.
//
void SVGrafsM::DCAve::updateLvl( const qint16 *d, int ntpts, int dwnSmp )
{
    if( nI <= 0 )
        return;

// -------------------
// Time to update lvl?
// -------------------

    double  T = getTime();

    if( !clock )
        clock = T - 4.0;

    if( T - clock >= 5.0 ) {

        clock = T;

        if( cnt ) {
            for( int i = 0; i < nI; ++i )
                lvl[i] = sum[i]/cnt;
        }

        sum.assign( nI, 0.0F );
        cnt = 0;
    }
    else if( T - clock >= 4.0 )
        updateSums( d, ntpts, dwnSmp );
}


// Apply level subtraction to range channels.
//
void SVGrafsM::DCAve::apply( qint16 *d, int ntpts, int dwnSmp )
{
    if( nI <= 0 )
        return;

    int *L      = &lvl[0];
    int dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int i = 0; i < nI; ++i )
            d[i0 + i] -= L[i];
    }
}


// Apply level subtraction to second half of range channels.
//
void SVGrafsM::DCAve::applyLF( qint16 *d, int ntpts, int dwnSmp )
{
    if( nI <= 0 )
        return;

    int *L      = &lvl[0];
    int dStep   = nC * dwnSmp,
        ihf     = nI / 2;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int i = ihf; i < nI; ++i )
            d[i0 + i] -= L[i];
    }
}


// Accumulate data into sum and cnt.
//
void SVGrafsM::DCAve::updateSums( const qint16 *d, int ntpts, int dwnSmp )
{
    if( nI <= 0 )
        return;

    float   *S      = &sum[0];
    int     dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int i = 0; i < nI; ++i )
            S[i] += d[i0 + i];
    }

    cnt += (ntpts + dwnSmp - 1) / dwnSmp;
}

/* ---------------------------------------------------------------- */
/* class SVGrafsM ------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM::SVGrafsM(
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 js,
    int                 ip,
    int                 jpanel )
    :   gw(gw), shankCtl(0), p(p), hipass(0), lopass(0),
        drawMtx(QMutex::Recursive), timStatBar(250, this),
        js(js), ip(ip), jpanel(jpanel), lastMouseOverChan(-1),
        selected(-1), maximized(-1), externUpdateTimes(true),
        inConstructor(true)
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
    int nN = neurChanCount();
    Tn.init( n, 0, nN );
    Tx.init( n, nN, analogChanCount() );
    tnChkClicked( set.tnChkOn );
    txChkClicked( set.txChkOn );
    binMaxChkClicked( set.binMaxOn );
    bandSelChanged( set.bandSel );
    sAveSelChanged( set.sAveSel );

    ic2iy.fill( -1 );
    mySort_ig2ic();
    initGraphs();
    myInit();

    pageChange( 0 );
    selectChan( ig2ic[0] );

    if( shankCtl )
        shankCtl->selChan( selected, myChanName( selected ) );

    nv->update();

    ConnectUI( &timStatBar, SIGNAL(draw(QString,int)), this, SLOT(statBarDraw(QString)) );

// ----
// Done
// ----

    inConstructor = false;
}


// Note: Derived class dtors called before base class.
//
SVGrafsM::~SVGrafsM()
{
// Wait here until not drawing

    drawMtx.lock();
    drawMtx.unlock();

// OK to destroy

    fltMtx.lock();
        if( hipass )
            delete hipass;
        if( lopass )
            delete lopass;
    fltMtx.unlock();
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool SVGrafsM::shankCtlGeomGet( QByteArray &geom ) const
{
    if( shankCtl && shankCtl->isVisible() ) {

        geom = shankCtl->saveGeometry();
        return true;
    }

    return false;
}


void SVGrafsM::shankCtlGeomSet( const QByteArray &geom, bool show )
{
    if( shankCtl && geom.size() ) {

        shankCtl->restoreGeometry( geom );

        if( show )
            showShanks( false );
    }
}


void SVGrafsM::eraseGraphs()
{
    drawMtx.lock();
    theX->dataMtx.lock();

    for( int ic = 0, nC = chanCount(); ic < nC; ++ic )
        ic2Y[ic].erase();

    theX->dataMtx.unlock();
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

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::toggleSorting()
{
    setSorting( !set.usrOrder );
}


void SVGrafsM::showShanks( bool getGeom )
{
    if( shankCtl ) {

        if( getGeom )
            gw->shankCtlWantGeom( jpanel );

        shankCtl->showDialog();
    }
}


void SVGrafsM::nchanChanged( int val, int first )
{
    drawMtx.lock();
    set.navNChan = val;
    saveSettings();
    drawMtx.unlock();

    pageChange( first );
    selectChan( selected );
}


void SVGrafsM::firstChanged( int first )
{
    pageChange( first );
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

    theX->dataMtx.lock();
    setGraphTimeSecs();
    theX->dataMtx.unlock();

    saveSettings();
    drawMtx.unlock();

    theM->update();
}


void SVGrafsM::graphYScaleChanged( double d )
{
    MGraphY &Y = ic2Y[selected];

    drawMtx.lock();
    theX->dataMtx.lock();

    Y.yscl = d;

    theX->dataMtx.unlock();
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
        theX->dataMtx.lock();

        theX->yColor.push_back( c );
        Y.iclr = iclr;

        theX->dataMtx.unlock();
        drawMtx.unlock();

        theM->update();
        tb->update();
    }
}


void SVGrafsM::applyAll()
{
// Copy settings to like usrType.

    const MGraphY   &Y = ic2Y[selected];

    drawMtx.lock();
    theX->dataMtx.lock();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        if( ic2Y[ic].usrType == Y.usrType )
            ic2Y[ic].yscl = Y.yscl;
    }

    theX->dataMtx.unlock();

    if( !Y.usrType )
        set.yscl0 = Y.yscl;
    else if( Y.usrType == 1 )
        set.yscl1 = Y.yscl;
    else
        set.yscl2 = Y.yscl;

    saveSettings();
    drawMtx.unlock();

    theM->update();
}


void SVGrafsM::tnChkClicked( bool checked )
{
    drawMtx.lock();
    set.tnChkOn = checked;
    Tn.setChecked( checked );
    saveSettings();
    drawMtx.unlock();
}


void SVGrafsM::txChkClicked( bool checked )
{
    drawMtx.lock();
    set.txChkOn = checked;
    Tx.setChecked( checked );
    saveSettings();
    drawMtx.unlock();
}


void SVGrafsM::binMaxChkClicked( bool checked )
{
    drawMtx.lock();
    set.binMaxOn = checked;
    saveSettings();
    drawMtx.unlock();

    eraseGraphs();
}


void SVGrafsM::refresh()
{
    mainApp()->getRun()->grfRefresh();
}


void SVGrafsM::colorTTL()
{
    gw->getTTLColorCtl()->showDialog();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::dblClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
    toggleMaximized();
}


void SVGrafsM::statBarDraw( QString s )
{
    gw->statusBar()->showMessage( s );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString SVGrafsM::clrToString( QColor c ) const
{
    return QString("%1").arg( c.rgb(), 0, 16 );
}


QColor SVGrafsM::clrFromString( QString s ) const
{
    return QColor::fromRgba( (QRgb)s.toUInt( 0, 16 ) );
}


void SVGrafsM::setSorting( bool userSorted )
{
    if( maximized >= 0 )
        toggleMaximized();

    drawMtx.lock();
    set.usrOrder = userSorted;
    mySort_ig2ic();
    externUpdateTimes = true;
    saveSettings();
    drawMtx.unlock();

    ensureSelectionVisible();

    tb->update();
    nv->update();
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

    theX->dataMtx.lock();
    theX->setYSelByUsrChan( ic );
    theX->dataMtx.unlock();

    theM->update();
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


// For each channel [0,nSpikeChan), calculate an 8-way
// neighborhood of indices into a timepoint's channels.
// - Annulus with {inner, outer} radii {self, 2} or {2, 8}.
// - The list is sorted for cache friendliness.
//
// Sel: {0=Off; 1=Loc 1,2; 2=Loc 2,8; 3=Glb All, 4=Glb Dmx}.
//
void SVGrafsM::sAveTable( const ShankMap &SM, int nSpikeChans, int sel )
{
    if( nSpikeChans > 0 && (sel == 1 || sel == 2) ) {

        int rin, rout;
        setLocalFilters( rin, rout, sel );
        car.lcl_init( &SM, rin, rout, false );
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::initGraphs()
{
    theM->setImmedUpdate( true );
    ConnectUI( theM, SIGNAL(cursorOver(double,double,int)), this, SLOT(myMouseOverGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutClicked(double,double,int)), this, SLOT(myClickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutDoubleClicked(double,double,int)), this, SLOT(dblClickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(rbutClicked(double,double,int)), this, SLOT(myRClickGraph(double,double,int)) );

    theX->Y.clear();
    theX->yColor.clear();
    theX->yColor.push_back( set.clr0 );
    theX->yColor.push_back( set.clr1 );

    digitalType = mySetUsrTypes();

    const IMROTbl   *R  = 0;
    int             nAP = 0;

    if( js == jsIM ) {
        R   = p.im.prbj[ip].roTbl;
        nAP = R->nAP();
    }

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];
        int     idum;

        switch( Y.usrType ) {
            case 0:
                Y.yscl = set.yscl0;
                if( R )
                    Y.anashank = R->elShankColRow( idum, Y.anarow, ic );
                break;
            case 1:
                Y.yscl = set.yscl1;
                if( R )
                    Y.anashank = R->elShankColRow( idum, Y.anarow, ic - nAP );
                break;
            default:
                Y.yscl = set.yscl2;
                break;
        }

        Y.lhsLabel      = myChanName( ic );
        Y.usrChan       = ic;
        Y.iclr          = (Y.usrType < 2 ? Y.usrType : 1);
        Y.anaclr        = -1;
        Y.drawBinMax    = false;
        Y.isDigType     = Y.usrType == digitalType;
    }

    updateRHSFlags();
}


// This is where Mgraph is pointed to the active channels.
//
void SVGrafsM::pageChange( int first, bool internUpdateTimes )
{
    drawMtx.lock();
    theX->dataMtx.lock();

    theX->Y.clear();

    if( maximized >= 0 ) {

        theX->fixedNGrf = 1;
        theX->Y.push_back( &ic2Y[maximized] );
    }
    else {

        theX->fixedNGrf = 0;

        for( int ig = first, nG = ic2Y.size(); ig < nG; ++ig ) {

            theX->Y.push_back( &ic2Y[ig2ic[ig]] );

            if( ++theX->fixedNGrf >= set.navNChan )
                break;
        }
    }

    update_ic2iy( first );

    theX->calcYpxPerGrf();
    theX->setYSelByUsrChan( selected );

    if( internUpdateTimes || externUpdateTimes ) {
        setGraphTimeSecs();
        externUpdateTimes = false;
    }

    theX->dataMtx.unlock();
    drawMtx.unlock();

    theM->update();
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

        ic2iy[ig2ic[ig]] = nY++;

        if( nY >= set.navNChan )
            break;
    }
}


