
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"
#include "DAQ.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SVToolsM.h"

#include <QVBoxLayout>




/* ---------------------------------------------------------------- */
/* class DCAve ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVGrafsM::DCAve::init( int nChannels, int nNeural )
{
    nC = nChannels;
    nN = nNeural;
    lvl.fill( 0.0F, nN );
    clock = 0.0;
}


void SVGrafsM::DCAve::setChecked( bool checked )
{
    sum.fill( 0.0F, nN );
    cnt.fill( 0, nN );

    if( !checked )
        lvl.fill( 0.0F, nN );
    else
        clock = 0.0;
}


// Every 5 seconds the level is updated, based
// upon averaging over the preceding 1 second.
//
// Return true if client should accumulate sums.
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
            lvl[ic] = (cnt[ic] ? sum[ic]/cnt[ic] : 0.0F);
            sum[ic] = 0.0F;
            cnt[ic] = 0;
        }
    }
    else if( T - clock >= 4.0 )
        updateSums( d, ntpts, dwnSmp );
}


// Accumulate data into sum and cnt.
//
void SVGrafsM::DCAve::updateSums(
    const qint16    *d,
    int             ntpts,
    int             dwnSmp )
{
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

SVGrafsM::SVGrafsM( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), drawMtx(QMutex::Recursive),
        lastMouseOverChan(-1), selected(-1), maximized(-1),
        externUpdateTimes(true)
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
    filterChkClicked( set.filterChkOn );
    sAveRadChanged( set.sAveRadius );

    ic2iy.fill( -1 );
    mySort_ig2ic();
    initGraphs();
    myInit();

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
    theX->dataMtx.lock();

    for( int ic = 0, nC = chanCount(); ic < nC; ++ic )
        ic2Y[ic].yval.erase();

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


void SVGrafsM::dcChkClicked( bool checked )
{
    drawMtx.lock();
    set.dcChkOn = checked;
    dc.setChecked( checked );
    drawMtx.unlock();

    saveSettings();
}


void SVGrafsM::binMaxChkClicked( bool checked )
{
    drawMtx.lock();
    set.binMaxOn = checked;
    drawMtx.unlock();

    saveSettings();
}


void SVGrafsM::dblClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
    toggleMaximized();
}


QString SVGrafsM::clrToString( QColor c ) const
{
    return QString("%1").arg( c.rgb(), 0, 16 );
}


QColor SVGrafsM::clrFromString( QString s ) const
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


// For each channel [c0,cLim), calculate an 8-way
// neighborhood of indices into a timepoint's channels.
// - The index list excludes the central channel.
// - The list is sorted for cache friendliness.
//
void SVGrafsM::sAveTable( const ShankMap &SM, int c0, int cLim, int radius )
{
    TSM.clear();
    TSM.resize( cLim - c0 );

    QMap<ShankMapDesc,uint> ISM;
    SM.inverseMap( ISM );

    for( int ic = c0; ic < cLim; ++ic ) {

        const ShankMapDesc  &E = SM.e[ic];

        if( !E.u )
            continue;

        QVector<int>    &V = TSM[ic];

        int xL  = qMax( int(E.c)  - radius, 0 ),
            xH  = qMin( uint(E.c) + radius + 1, SM.nc ),
            yL  = qMax( int(E.r)  - radius, 0 ),
            yH  = qMin( uint(E.r) + radius + 1, SM.nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it == ISM.end() )
                    continue;

                int i = it.value();

                // Exclude self
                // Make zero-based

                if( i != ic )
                    V.push_back( i - c0 );
            }
        }

        qSort( V );
    }
}


// Space and time averaging for value: d_ic = &data[ic].
//
int SVGrafsM::s_t_Ave( const qint16 *d_ic, int ic )
{
    const QVector<int>  &V = TSM[ic];
    const float         *L = &dc.lvl[0];

    int sum = 0,
        nv  = V.size();

    if( nv ) {

        const qint16    *d = d_ic - ic;

        for( int iv = 0; iv < nv; ++iv ) {

            int k = V[iv];

            sum += d[k] - L[k];
        }

        return *d_ic - sum/nv - L[ic];
    }

    return *d_ic - L[ic];
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

        ic2iy[ig2ic[ig]] = nY++;

        if( nY >= set.navNChan )
            break;
    }
}


