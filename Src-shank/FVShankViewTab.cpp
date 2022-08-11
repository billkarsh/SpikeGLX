
#include "ui_FVShankViewTab.h"

#include "Util.h"
#include "FVShankViewTab.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "DataFile.h"
#include "SignalBlocker.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* UsrSettings ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::UsrSettings::loadSettings( QSettings &S )
{
    yPix    = S.value( "yPix", 8 ).toInt();
    what    = S.value( "what", 1 ).toInt();
    thresh  = S.value( "thresh", -75 ).toInt();
    inarow  = S.value( "staylow", 5 ).toInt();
    rng[0]  = S.value( "rngSpk", 100 ).toInt();
    rng[1]  = S.value( "rngAP", 100 ).toInt();
    rng[2]  = S.value( "rngLF", 100 ).toInt();
}


void FVShankViewTab::UsrSettings::saveSettings( QSettings &S ) const
{
    S.setValue( "yPix", yPix );
    S.setValue( "what", what );
    S.setValue( "thresh", thresh );
    S.setValue( "staylow", inarow );
    S.setValue( "rngSpk", rng[0] );
    S.setValue( "rngAP", rng[1] );
    S.setValue( "rngLF", rng[2] );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

FVShankViewTab::FVShankViewTab(
    ShankCtlBase    *SC,
    QWidget         *tab,
    const DataFile  *df )
    :   SC(SC), svTabUI(0), df(df), chanMap(df->chanMap()),
        lfp(df->subtypeFromObj()=="imec.lf")
{
    heat.setStream( df );

    svTabUI = new Ui::FVShankViewTab;
    svTabUI->setupUi( tab );
}


FVShankViewTab::~FVShankViewTab()
{
    if( chanMap ) {
        delete chanMap;
        chanMap = 0;
    }

    if( svTabUI ) {
        delete svTabUI;
        svTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::init( const ShankMap *map )
{
    SC->view()->setRowPix( set.yPix );
    SC->syncYPix( set.yPix );

    svTabUI->ypixSB->installEventFilter( SC );
    svTabUI->ypixSB->setValue( set.yPix );

    svTabUI->whatCB->setCurrentIndex( set.what );
    svTabUI->whatCB->setEnabled( !lfp );

    svTabUI->TSB->installEventFilter( SC );
    svTabUI->TSB->setValue( -set.thresh );
    svTabUI->TSB->setEnabled( set.what == 0 );

    svTabUI->inarowSB->installEventFilter( SC );
    svTabUI->inarowSB->setValue( set.inarow );
    svTabUI->inarowSB->setEnabled( set.what == 0 );

    svTabUI->rngSB->installEventFilter( SC );
    svTabUI->rngSB->setValue( set.rng[set.what] );

    ConnectUI( svTabUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( svTabUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( svTabUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( svTabUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( svTabUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( svTabUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( svTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    mapChanged( map );
    selChan( 0 );
}


void FVShankViewTab::mapChanged( const ShankMap *map )
{
    SC->view()->setShankMap( map );
}


void FVShankViewTab::selChan( int ig )
{
    if( ig < 0 )
        return;

    const ShankMap  *M = SC->view()->getSmap();

    if( M && ig < int(M->e.size()) ) {

        int ic = df->channelIDs()[ig];

        SC->view()->setSel( ig );
        svTabUI->chanBut->setText( chanMap->name( ig, df->trig_isChan( ic ) ) );
    }
}


void FVShankViewTab::putInit()
{
    SC->drawMtx.lock();
        heat.accumReset( true );
    SC->drawMtx.unlock();
}


void FVShankViewTab::putScans( const vec_i16 &_data )
{
    vec_i16 data;

    SC->drawMtx.lock();
        switch( set.what ) {
            case 0:
                heat.apFilter( data, _data );
                heat.accumSpikes( data, set.thresh, set.inarow );
                break;
            case 1:
                heat.apFilter( data, _data );
                heat.accumPkPk( data );
                break;
            default:
                heat.lfFilter( data, _data );
                heat.accumPkPk( data );
                break;
        }
    SC->drawMtx.unlock();
}


void FVShankViewTab::putDone()
{
    SC->drawMtx.lock();
        if( set.what == 0 )
            heat.normSpikes();
        else
            heat.normPkPk( set.what );

        color();
    SC->drawMtx.unlock();
}


void FVShankViewTab::cursorOver( int ig )
{
    if( ig < 0 ) {
        SC->setStatus( QString() );
        return;
    }

    int r  = SC->view()->getSmap()->e[ig].r,
        ic = df->channelIDs()[ig];

    SC->setStatus(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( chanMap->name( ig, df->trig_isChan( ic ) ) ) );
}

/* ---------------------------------------------------------------- */
/* Public slots --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::syncYPix( int y )
{
    SignalBlocker   b0(svTabUI->ypixSB);
    set.yPix = y;
    svTabUI->ypixSB->setValue( y );
    SC->saveSettings();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::ypixChanged( int y )
{
    set.yPix = y;
    SC->drawMtx.lock();
        SC->scroll()->setRowPix( y );
    SC->drawMtx.unlock();
    SC->saveSettings();
    SC->syncYPix( y );
}


void FVShankViewTab::whatChanged( int i )
{
    SC->drawMtx.lock();
        set.what = i;
        SignalBlocker   b0(svTabUI->rngSB);
        svTabUI->TSB->setEnabled( !i );
        svTabUI->inarowSB->setEnabled( !i );
        svTabUI->rngSB->setValue( set.rng[i] );
    SC->drawMtx.unlock();
    SC->saveSettings();
    emit SC->feedMe();
}


void FVShankViewTab::threshChanged( int t )
{
    SC->drawMtx.lock();
        set.thresh = -t;
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void FVShankViewTab::inarowChanged( int s )
{
    SC->drawMtx.lock();
        set.inarow = s;
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void FVShankViewTab::rangeChanged( int r )
{
    SC->drawMtx.lock();
        set.rng[set.what] = r;
        color();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void FVShankViewTab::chanBut()
{
    SC->drawMtx.lock();
        SC->scroll()->scrollToSelected();
    SC->drawMtx.unlock();
}


void FVShankViewTab::helpBut()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Caller locks drawMtx.
//
void FVShankViewTab::color()
{
    SC->view()->colorPads( heat.sums(), set.rng[set.what] );
    SC->update();
}


