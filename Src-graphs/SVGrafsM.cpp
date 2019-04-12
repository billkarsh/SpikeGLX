
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SVToolsM.h"
#include "ShankCtl.h"
#include "ShankMap.h"
#include "Biquad.h"
#include "ColorTTLCtl.h"

#include <QStatusBar>
#include <QVBoxLayout>




/* ---------------------------------------------------------------- */
/* class DCAve ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::DCAve::init( int nChannels, int nNeural )
{
    nC = nChannels;
    nN = nNeural;
    lvl.assign( nN, 0 );
    clock = 0.0;
}


void SVGrafsM::DCAve::setChecked( bool checked )
{
    sum.assign( nN, 0.0F );
    cnt.assign( nN, 0 );

    if( !checked )
        lvl.assign( nN, 0 );
    else
        clock = 0.0;
}


// Every 5 seconds the level is updated, based
// upon averaging over the preceding 1 second.
//
void SVGrafsM::DCAve::updateLvl(
    const qint16    *d,
    int             ntpts,
    int             dwnSmp )
{
// -------------------
// Time to update lvl?
// -------------------

    double  T = getTime();

    if( !clock )
        clock = T - 4.0;

    if( T - clock >= 5.0 ) {

        clock = T;

        for( int ic = 0; ic < nN; ++ic ) {
            lvl[ic] = (cnt[ic] ? sum[ic]/cnt[ic] : 0);
            sum[ic] = 0.0F;
            cnt[ic] = 0;
        }
    }
    else if( T - clock >= 4.0 )
        updateSums( d, ntpts, dwnSmp );
}


// Apply level subtraction to neural channels.
//
void SVGrafsM::DCAve::apply(
    qint16          *d,
    int             ntpts,
    int             c0,
    int             dwnSmp )
{
    if( nN <= 0 )
        return;

    int *L      = &lvl[0];
    int dStep   = nC * dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic = c0; ic < nN; ++ic )
            d[ic] -= L[ic];
    }
}


// Accumulate data into sum and cnt.
//
void SVGrafsM::DCAve::updateSums(
    const qint16    *d,
    int             ntpts,
    int             dwnSmp )
{
    if( nN <= 0 )
        return;

    float   *S      = &sum[0];
    int     dStep   = nC * dwnSmp,
            dtpts   = (ntpts + dwnSmp - 1) / dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic = 0; ic < nN; ++ic )
            S[ic] += d[ic];
    }

    for( int ic = 0; ic < nN; ++ic )
        cnt[ic] += dtpts;
}

/* ---------------------------------------------------------------- */
/* class SVGrafsM ------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVGrafsM::SVGrafsM( GraphsWindow *gw, const DAQ::Params &p )
    :   gw(gw), shankCtl(0), p(p), hipass(0), lopass(0),
        drawMtx(QMutex::Recursive), timStatBar(250, this),
        lastMouseOverChan(-1), selected(-1), maximized(-1),
        externUpdateTimes(true), inConstructor(true)
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
    dc.init( n, neurChanCount() );
    dcChkClicked( set.dcChkOn );
    binMaxChkClicked( set.binMaxOn );
    bandSelChanged( set.bandSel );
    sAveSelChanged( set.sAveSel );

    ic2iy.fill( -1 );
    mySort_ig2ic();
    initGraphs();
    myInit();

    pageChange( 0 );
    selectChan( ig2ic[0] );
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

    if( shankCtl ) {
        delete shankCtl;
        shankCtl = 0;
    }
}


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
            shankCtl->showDialog();
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


void SVGrafsM::toggleSorting()
{
    setSorting( !set.usrOrder );
}


void SVGrafsM::showShanks()
{
    shankCtl->showDialog();
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


void SVGrafsM::dcChkClicked( bool checked )
{
    drawMtx.lock();
    set.dcChkOn = checked;
    dc.setChecked( checked );
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


void SVGrafsM::dblClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
    toggleMaximized();
}


void SVGrafsM::statBarDraw( QString s )
{
    gw->statusBar()->showMessage( s );
}


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
    TSM.clear();

    if( !(sel == 1 || sel == 2) || nSpikeChans <= 0 )
        return;

    TSM.resize( nSpikeChans );

    QMap<ShankMapDesc,uint> ISM;
    SM.inverseMap( ISM );

    int rIn, rOut;

    if( sel == 1 ) {
        rIn     = 0;
        rOut    = 2;
    }
    else if( sel == 2 ) {
        rIn     = 2;
        rOut    = 8;
    }

    for( int ic = 0; ic < nSpikeChans; ++ic ) {

        const ShankMapDesc  &E = SM.e[ic];

        if( !E.u )
            continue;

        // ----------------------------------
        // Form map of excluded inner indices
        // ----------------------------------

        QMap<int,int>   inner;  // keys sorted, value is arbitrary

        int xL  = qMax( int(E.c)  - rIn, 0 ),
            xH  = qMin( uint(E.c) + rIn + 1, SM.nc ),
            yL  = qMax( int(E.r)  - rIn, 0 ),
            yH  = qMin( uint(E.r) + rIn + 1, SM.nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it != ISM.end() )
                    inner[it.value()] = 1;
            }
        }

        // -------------------------
        // Fill with annulus members
        // -------------------------

        std::vector<int>    &V = TSM[ic];

        xL  = qMax( int(E.c)  - rOut, 0 );
        xH  = qMin( uint(E.c) + rOut + 1, SM.nc );
        yL  = qMax( int(E.r)  - rOut, 0 );
        yH  = qMin( uint(E.r) + rOut + 1, SM.nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it != ISM.end() ) {

                    int i = it.value();

                    // Exclude inners

                    if( inner.find( i ) == inner.end() )
                        V.push_back( i );
                }
            }
        }

        qSort( V );
    }
}


// Space averaging for value: d_ic = &data[ic].
//
int SVGrafsM::sAveApplyLocal( const qint16 *d_ic, int ic )
{
    const std::vector<int>  &V = TSM[ic];

    int nv = V.size();

    if( nv ) {

        const qint16    *d  = d_ic - ic;
        const int       *v  = &V[0];
        int             sum = 0;

        for( int iv = 0; iv < nv; ++iv )
            sum += d[v[iv]];

        return *d_ic - sum/nv;
    }

    return *d_ic;
}


// Space averaging for all values.
//
void SVGrafsM::sAveApplyGlobal(
    const ShankMap  &SM,
    qint16          *d,
    int             ntpts,
    int             nC,
    int             nAP,
    int             dwnSmp )
{
    if( nAP <= 0 )
        return;

    const ShankMapDesc  *E = &SM.e[0];

    int                 ns      = SM.ns,
                        dStep   = nC * dwnSmp;
    std::vector<int>    _A( ns ),
                        _N( ns );
    std::vector<float>  _S( ns );
    int                 *A  = &_A[0],
                        *N  = &_N[0];
    float               *S  = &_S[0];

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int is = 0; is < ns; ++is ) {
            S[is] = 0;
            N[is] = 0;
            A[is] = 0;
        }

        for( int ic = 0; ic < nAP; ++ic ) {

            const ShankMapDesc  *e = &E[ic];

            if( e->u ) {
                S[e->s] += d[ic];
                ++N[e->s];
            }
        }

        for( int is = 0; is < ns; ++is ) {

            if( N[is] )
                A[is] = S[is] / N[is];
        }

        for( int ic = 0; ic < nAP; ++ic )
            d[ic] -= A[E[ic].s];
    }
}


// Space averaging for all values.
//
void SVGrafsM::sAveApplyGlobalStride(
    const ShankMap  &SM,
    qint16          *d,
    int             ntpts,
    int             nC,
    int             nAP,
    int             stride,
    int             dwnSmp )
{
    if( nAP <= 0 )
        return;

    const ShankMapDesc  *E = &SM.e[0];

    int                 ns      = SM.ns,
                        dStep   = nC * dwnSmp;
    std::vector<int>    _A( ns ),
                        _N( ns );
    std::vector<float>  _S( ns );
    int                 *A  = &_A[0],
                        *N  = &_N[0];
    float               *S  = &_S[0];

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic0 = 0; ic0 < stride; ++ic0 ) {

            for( int is = 0; is < ns; ++is ) {
                S[is] = 0;
                N[is] = 0;
                A[is] = 0;
            }

            for( int ic = ic0; ic < nAP; ic += stride ) {

                const ShankMapDesc  *e = &E[ic];

                if( e->u ) {
                    S[e->s] += d[ic];
                    ++N[e->s];
                }
            }

            for( int is = 0; is < ns; ++is ) {

                if( N[is] )
                    A[is] = S[is] / N[is];
            }

            for( int ic = ic0; ic < nAP; ic += stride )
                d[ic] -= A[E[ic].s];
        }
    }
}


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
    theX->yColor.push_back( set.clr2 );

    digitalType = mySetUsrTypes();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        Y.yscl          = (Y.usrType == 0 ? set.yscl0 :
                            (Y.usrType == 1 ? set.yscl1 : set.yscl2));
        Y.lhsLabel      = myChanName( ic );
        Y.usrChan       = ic;
        Y.iclr          = Y.usrType;
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


