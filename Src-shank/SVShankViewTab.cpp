
#include "ui_SVShankViewTab.h"

#include "Util.h"
#include "SVShankViewTab.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "DAQ.h"
#include "SignalBlocker.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* UsrSettings ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::UsrSettings::loadSettings( QSettings &S )
{
    updtSecs    = S.value( "updtSecs", 0.1 ).toDouble();
    yPix        = S.value( "yPix", 8 ).toInt();
    what        = S.value( "what", 1 ).toInt();
    thresh      = S.value( "thresh", -75 ).toInt();
    inarow      = S.value( "staylow", 5 ).toInt();
    rng[0]      = S.value( "rngSpk", 100 ).toInt();
    rng[1]      = S.value( "rngAP", 100 ).toInt();
    rng[2]      = S.value( "rngLF", 100 ).toInt();
    colorShanks = S.value( "colorShanks", true ).toBool();
    colorTraces = S.value( "colorTraces", false ).toBool();
}


void SVShankViewTab::UsrSettings::saveSettings( QSettings &S ) const
{
    S.setValue( "updtSecs", updtSecs );
    S.setValue( "yPix", yPix );
    S.setValue( "what", what );
    S.setValue( "thresh", thresh );
    S.setValue( "staylow", inarow );
    S.setValue( "rngSpk", rng[0] );
    S.setValue( "rngAP", rng[1] );
    S.setValue( "rngLF", rng[2] );
    S.setValue( "colorShanks", colorShanks );
    S.setValue( "colorTraces", colorTraces );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SVShankViewTab::SVShankViewTab(
    ShankCtlBase        *SC,
    QWidget             *tab,
    const DAQ::Params   &p,
    int                 js,
    int                 ip )
    :   QObject(0), SC(SC), svTabUI(0), p(p)
{
    heat.setStream( p, js, ip );

    svTabUI = new Ui::SVShankViewTab;
    svTabUI->setupUi( tab );
}


SVShankViewTab::~SVShankViewTab()
{
    if( svTabUI ) {
        delete svTabUI;
        svTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::init()
{
    SC->view()->setRowPix( set.yPix );
    SC->syncYPix( set.yPix );

    svTabUI->ypixSB->installEventFilter( SC );
    svTabUI->ypixSB->setValue( set.yPix );

    svTabUI->whatCB->setCurrentIndex( set.what );

    svTabUI->TSB->installEventFilter( SC );
    svTabUI->TSB->setValue( -set.thresh );
    svTabUI->TSB->setEnabled( set.what == 0 );

    svTabUI->inarowSB->installEventFilter( SC );
    svTabUI->inarowSB->setValue( set.inarow );
    svTabUI->inarowSB->setEnabled( set.what == 0 );

    svTabUI->updtSB->installEventFilter( SC );
    svTabUI->updtSB->setValue( set.updtSecs );
    setReqdChunks( set.updtSecs );

    svTabUI->rngSB->installEventFilter( SC );
    svTabUI->rngSB->setValue( set.rng[set.what] );

    svTabUI->legendTE->setFontPointSize( 10 );
    svTabUI->legendTE->setFontWeight( QFont::DemiBold );

    svTabUI->shanksChk->setChecked( set.colorShanks );
    svTabUI->shanksChk->setEnabled( false );
    svTabUI->tracesChk->setChecked( set.colorTraces );
    svTabUI->tracesChk->setEnabled( false );

    ConnectUI( svTabUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( svTabUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( svTabUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( svTabUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( svTabUI->updtSB, SIGNAL(valueChanged(double)), this, SLOT(updtChanged(double)) );
    ConnectUI( svTabUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( svTabUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( svTabUI->shanksChk, SIGNAL(clicked(bool)), this, SLOT(shanksCheck(bool)) );
    ConnectUI( svTabUI->tracesChk, SIGNAL(clicked(bool)), this, SLOT(tracesCheck(bool)) );
    ConnectUI( svTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
}


void SVShankViewTab::setAnatomyPP( const QString &elems, int ip, int sk )
{
    anat.parse( elems, p.im.prbj[ip].roTbl, sk );
    anat.fillLegend( svTabUI->legendTE );
    svTabUI->shanksChk->setEnabled( true );
    svTabUI->tracesChk->setEnabled( true );
    anat.colorShanks( SC->view(), svTabUI->shanksChk->isChecked() );

    if( svTabUI->tracesChk->isChecked() )
        emit SC->gimmeTraces();
}


void SVShankViewTab::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY )
{
    anat.colorTraces( theX, vY, svTabUI->tracesChk->isChecked() );
}


void SVShankViewTab::selChan( int ic, const QString &name )
{
    if( ic < 0 )
        return;

    const ShankMap  *M = SC->view()->getSmap();

    if( M && ic < int(M->e.size()) ) {

        SC->view()->setSel( ic );
        svTabUI->chanBut->setText( name );
    }
}


void SVShankViewTab::putSamps( const vec_i16 &_data )
{
    vec_i16 data;
    bool    done = ++chunksDone >= chunksReqd;

    SC->drawMtx.lock();
        switch( set.what ) {
            case 0:
                heat.apFilter( data, _data, 0 );
                heat.accumSpikes( data, set.thresh, set.inarow );
                if( done ) heat.normSpikes();
                break;
            case 1:
                heat.apFilter( data, _data, 0 );
                heat.accumPkPk( data );
                if( done ) heat.normPkPk( 1 );
                break;
            default:
                heat.lfFilter( data, _data );
                heat.accumPkPk( data );
                if( done ) heat.normPkPk( 2 );
                break;
        }

        if( done ) {
            color();
            resetAccum();
        }
    SC->drawMtx.unlock();
}


void SVShankViewTab::winClosed()
{
    heat.resetFilter();
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::syncYPix( int y )
{
    SignalBlocker   b0(svTabUI->ypixSB);
    set.yPix = y;
    svTabUI->ypixSB->setValue( y );
    SC->saveSettings();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::ypixChanged( int y )
{
    set.yPix = y;
    SC->drawMtx.lock();
        SC->scroll()->setRowPix( y );
    SC->drawMtx.unlock();
    SC->saveSettings();
    SC->syncYPix( y );
}


void SVShankViewTab::whatChanged( int i )
{
    SC->drawMtx.lock();
        set.what = i;
        SignalBlocker   b0(svTabUI->rngSB);
        svTabUI->TSB->setEnabled( !i );
        svTabUI->inarowSB->setEnabled( !i );
        svTabUI->rngSB->setValue( set.rng[i] );
        resetAccum();
        SC->scroll()->theV->colorPads( heat.sums(), 1e99 );
        SC->update();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::threshChanged( int t )
{
    SC->drawMtx.lock();
        set.thresh = -t;
        resetAccum();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::inarowChanged( int s )
{
    SC->drawMtx.lock();
        set.inarow = s;
        resetAccum();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::updtChanged( double s )
{
    SC->drawMtx.lock();
        set.updtSecs = s;
        setReqdChunks( s );
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::rangeChanged( int r )
{
    SC->drawMtx.lock();
        set.rng[set.what] = r;
        color();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::chanBut()
{
    SC->drawMtx.lock();
        SC->scroll()->scrollToSelected();
    SC->drawMtx.unlock();
}


void SVShankViewTab::shanksCheck( bool on )
{
    SC->drawMtx.lock();
        set.colorShanks = on;
    SC->drawMtx.unlock();
    SC->saveSettings();
    anat.colorShanks( SC->view(), on );
}


void SVShankViewTab::tracesCheck( bool on )
{
    set.colorTraces = on;
    SC->saveSettings();
    emit SC->gimmeTraces();
}


void SVShankViewTab::helpBut()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::setReqdChunks( double s )
{
#define SEC_PER_FETCH   0.1

    chunksReqd = qMax( 1, int(s / SEC_PER_FETCH) );
    resetAccum();
}


void SVShankViewTab::resetAccum()
{
    heat.accumReset( false );
    chunksDone = 0;
}


// Caller locks drawMtx.
//
void SVShankViewTab::color()
{
    SC->view()->colorPads( heat.sums(), set.rng[set.what] );
    SC->update();
}


