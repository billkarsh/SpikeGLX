
#include "Util.h"
#include "MainApp.h"
#include "GraphsWindow.h"
#include "GWWidgetM.h"
#include "DAQ.h"

#include <QVBoxLayout>
#include <QTabBar>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* class GWWidgetM ------------------------------------------------ */
/* ---------------------------------------------------------------- */

GWWidgetM::GWWidgetM( GraphsWindow *gw, DAQ::Params &p )
    :   gw(gw), p(p), drawMtx(QMutex::Recursive),
        trgChan(p.trigChan()), lastMouseOverChan(-1), maximized(-1)
{
}


void GWWidgetM::init()
{
// ----------------
// Top-level layout
// ----------------

    tabs    = new QTabBar;
    theX    = new MGraphX;
    theM    = new MGraph( "gw", this, theX );

    QVBoxLayout *V = new QVBoxLayout( this );
    V->setSpacing( 0 );
    V->setMargin( 0 );
    V->addWidget( tabs );
    V->addWidget( theM, 1 );

// -----------
// Size arrays
// -----------

    int n = myChanCount();

    ic2Y.resize( n );
    ic2stat.resize( n );
    ic2iy.resize( n );
    ig2ic.resize( n );

// ----------
// Init items
// ----------

    ic2iy.fill( -1 );
    mySort_ig2ic();
    loadSettings();
    initTabs();
    initGraphs();

    tabChange( 0 );
}


// Note: Derived class dtors called before base class.
//
GWWidgetM::~GWWidgetM()
{
}


void GWWidgetM::eraseGraphs()
{
    drawMtx.lock();
    theX->dataMtx->lock();

    for( int ic = 0, nC = myChanCount(); ic < nC; ++ic )
        ic2Y[ic].yval.erase();

    theX->dataMtx->unlock();
    drawMtx.unlock();
}


void GWWidgetM::sortGraphs()
{
    drawMtx.lock();

    mySort_ig2ic();

// Set tab 0 if we don't have selection, else graphwWindow
// will call ensureVisible to perform redraw.

    if( theX->ySel < 0 )
        tabs->setCurrentIndex( 0 );

    drawMtx.unlock();
}


// Return first sorted graph to GraphsWindow.
//
int GWWidgetM::initialSelectedChan( QString &name ) const
{
    int ic = ig2ic[0];

    name = myChanName( ic );

    return ic;
}


void GWWidgetM::selectChan( int ic, bool selected )
{
    if( ic < 0 )
        return;

    theX->setYSelByUsrChan( (selected /*&& maximized < 0*/ ? ic : -1) );
    theM->update();
}


void GWWidgetM::ensureVisible( int ic )
{
// find tab with ic

    int itab = tabs->currentIndex();

    if( mainApp()->isSortUserOrder() ) {

        for( int ig = 0, nG = myChanCount(); ig < nG; ++ig ) {

            if( ig2ic[ig] == ic ) {
                itab = ig / set.grfPerTab;
                break;
            }
        }
    }
    else
        itab = ic / set.grfPerTab;

// Select that tab and redraw...
// However, tabChange won't get a signal if the
// current tab isn't changing...so we call it.

    if( itab != tabs->currentIndex() )
        tabs->setCurrentIndex( itab );
    else
        tabChange( itab, false );
}


void GWWidgetM::toggleMaximized( int newMaximized )
{
    int nT  = tabs->count(),
        cur = tabs->currentIndex();

    if( maximized >= 0 ) {   // restore multi-graph view

        newMaximized = -1;

        for( int it = 0; it < nT; ++it )
            tabs->setTabEnabled( it, true );
    }
    else {  // show only newMaximized

        for( int it = 0; it < nT; ++it )
            tabs->setTabEnabled( it, it == cur );
    }

    maximized = newMaximized;
    tabChange( cur, false );
}


void GWWidgetM::getGraphScales( double &xSpn, double &yScl, int ic ) const
{
    xSpn = theX->spanSecs();
    yScl = ic2Y[ic].yscl;
}


void GWWidgetM::graphSecsChanged( double secs, int ic )
{
    setGraphTimeSecs( ic, secs );

    set.secs = secs;
    saveSettings();
}


void GWWidgetM::graphYScaleChanged( double scale, int ic )
{
    MGraphY &Y = ic2Y[ic];

    drawMtx.lock();
    Y.yscl = scale;
    drawMtx.unlock();

    theM->update();

    saveSettings();
}


QColor GWWidgetM::getGraphColor( int ic ) const
{
    return theX->yColor[ic2Y[ic].iclr];
}


void GWWidgetM::colorChanged( QColor c, int ic )
{
    int iclr = theX->yColor.size();

    theX->yColor.push_back( c );

    MGraphY &Y = ic2Y[ic];

    drawMtx.lock();
    Y.iclr = iclr;
    drawMtx.unlock();

    theM->update();

    saveSettings();
}


void GWWidgetM::applyAll( int ichan )
{
// Copy settings to like usrType.

// BK: Not yet copying selected trace color to others

    const MGraphY   &Y = ic2Y[ichan];

    drawMtx.lock();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        if( ic2Y[ic].usrType == Y.usrType )
            ic2Y[ic].yscl = Y.yscl;
    }

    drawMtx.unlock();

    theM->update();

    if( !Y.usrType )
        set.ysclNa = Y.yscl;
    else if( Y.usrType == 1 )
        set.ysclNb = Y.yscl;
    else
        set.ysclAa = Y.yscl;

    saveSettings();
}


void GWWidgetM::showHideSaveChks()
{
}


void GWWidgetM::enableAllChecks( bool enabled )
{
    Q_UNUSED( enabled )
}


// This is where Mgraph is pointed to the active channels.
//
void GWWidgetM::tabChange( int itab, bool updateSecs )
{
    drawMtx.lock();

    theX->Y.clear();

    if( maximized >= 0 ) {

        theX->fixedNGrf = 1;
        theX->Y.push_back( &ic2Y[maximized] );
    }
    else {

        theX->fixedNGrf = 0;

        for( int ig = itab * set.grfPerTab, nG = ic2Y.size();
            ig < nG; ++ig ) {

            int ic = ig2ic[ig];

            // BK: Analog graphs only, for now
            if( ic2Y[ic].usrType != digitalType ) {

                theX->Y.push_back( &ic2Y[ic] );

                if( ++theX->fixedNGrf >= set.grfPerTab )
                    break;
            }
        }
    }

    update_ic2iy( itab );

    theX->calcYpxPerGrf();
    theX->setYSelByUsrChan( -1 );

    if( updateSecs ) {
        theX->setSpanSecs( set.secs, mySampRate() );
        theX->setVGridLinesAuto();
    }

    theM->update();

    drawMtx.unlock();
}


void GWWidgetM::dblClickGraph( double x, double y, int iy )
{
    myClickGraph( x, y, iy );
    toggleMaximized( lastMouseOverChan );
    gw->toggledMaximized();
}


static QString clrToString( QColor c )
{
    return QString("%1").arg( c.rgb(), 0, 16 );
}


void GWWidgetM::saveSettings()
{
// -------------------
// Appearance defaults
// -------------------

    STDSETTINGS( settings, "graphwin" );
    settings.beginGroup( mySettingsGrpName() );

    settings.setValue( "secs", set.secs );
    settings.setValue( "ysclNa", set.ysclNa );
    settings.setValue( "ysclNb", set.ysclNb );
    settings.setValue( "ysclAa", set.ysclAa );
    settings.setValue( "clrNa", clrToString( set.clrNa ) );
    settings.setValue( "clrNb", clrToString( set.clrNb ) );
    settings.setValue( "clrAa", clrToString( set.clrAa ) );
    settings.setValue( "grfPerTab", set.grfPerTab );

    settings.endGroup();
}


static QColor clrFromString( QString s )
{
    return QColor::fromRgba( (QRgb)s.toUInt( 0, 16 ) );
}


// Called only from init().
//
void GWWidgetM::loadSettings()
{
// -------------------
// Appearance defaults
// -------------------

    STDSETTINGS( settings, "graphwin" );
    settings.beginGroup( mySettingsGrpName() );

    drawMtx.lock();

    set.secs        = settings.value( "secs", 4.0 ).toDouble();
    set.ysclNa      = settings.value( "ysclNa", 1.0 ).toDouble();
    set.ysclNb      = settings.value( "ysclNb", 1.0 ).toDouble();
    set.ysclAa      = settings.value( "ysclAa", 1.0 ).toDouble();
    set.clrNa       = clrFromString( settings.value( "clrNa", "ffeedd82" ).toString() );
    set.clrNb       = clrFromString( settings.value( "clrNb", "ffff5500" ).toString() );
    set.clrAa       = clrFromString( settings.value( "clrAa", "ff44eeff" ).toString() );
    set.grfPerTab   = settings.value( "grfPerTab", 32 ).toInt();

    drawMtx.unlock();

    settings.endGroup();
}


void GWWidgetM::initTabs()
{
    int nC  = myChanCount(),
        nT  = nC / set.grfPerTab;

    if( nT*set.grfPerTab < nC )
        ++nT;

    for( int it = 0; it < nT; ++it )
        tabs->addTab( QString("%1").arg( it*set.grfPerTab ) );

    tabs->setFocusPolicy( Qt::StrongFocus );
    Connect( tabs, SIGNAL(currentChanged(int)), this, SLOT(tabChange(int)) );
}


void GWWidgetM::initGraphs()
{
    QFont   font;

    theM->setImmedUpdate( true );
    theM->setMinimumHeight( 0.80 * set.grfPerTab * font.pointSize() );
    ConnectUI( theM, SIGNAL(cursorOver(double,double,int)), this, SLOT(myMouseOverGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutClicked(double,double,int)), this, SLOT(myClickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutDoubleClicked(double,double,int)), this, SLOT(dblClickGraph(double,double,int)) );

    theX->Y.clear();
    theX->yColor.clear();
    theX->yColor.push_back( set.clrNa );
    theX->yColor.push_back( set.clrNb );
    theX->yColor.push_back( set.clrAa );

    digitalType = mySetUsrTypes();

    for( int ic = 0, nC = ic2Y.size(); ic < nC; ++ic ) {

        MGraphY &Y = ic2Y[ic];

        Y.yscl      = (Y.usrType == 0 ? set.ysclNa :
                        (Y.usrType == 1 ? set.ysclNb : set.ysclAa));
        Y.label     = myChanName( ic );
        Y.usrChan   = ic;
        Y.iclr      = Y.usrType;
        Y.isDigType = Y.usrType == digitalType;
    }
}


void GWWidgetM::setGraphTimeSecs( int ic, double t )
{
    drawMtx.lock();

    int nC = myChanCount();

    if( ic >= 0 && ic < nC ) {

        theX->setSpanSecs( t, mySampRate() );
        theX->setVGridLinesAuto();

        // If we're maximized while we get a time change,
        // we've got to propagate the change to all other
        // graphs on this tab.

        if( maximized >= 0 ) {

            int newSize = ic2Y[maximized].yval.capacity();

            for( int ic = 0; ic < nC; ++ic ) {

                if( ic2iy[ic] > -1 )
                    ic2Y[ic].resize( newSize );
            }
        }

        theM->update();

        ic2stat[ic].clear();
    }

    drawMtx.unlock();
}


// Same emplacement algorithm as tabChange().
//
void GWWidgetM::update_ic2iy( int itab )
{
    int nG  = ic2Y.size(),
        nY  = 0;

    ic2iy.fill( -1 );

    for( int ig = itab * set.grfPerTab; ig < nG; ++ig ) {

        int ic = ig2ic[ig];

        // BK: Analog graphs only, for now
        if( ic2Y[ic].usrType != digitalType ) {

            ic2iy[ic] = nY++;

            if( nY >= set.grfPerTab )
                break;
        }
    }
}


