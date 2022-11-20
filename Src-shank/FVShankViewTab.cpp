
#include "ui_FVShankViewTab.h"

#include "Util.h"
#include "FVShankViewTab.h"
#include "FVShankCtl.h"
#include "FileViewerWindow.h"
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
    :   QObject(0), SC(SC), svTabUI(0), df(df), MW(0),
        chanMap(df->chanMap()), lfp(df->subtypeFromObj()=="imec.lf")
{
    svySums[0] = 0;
    svySums[1] = 0;
    svySums[2] = 0;

    heat.setStream( df );

    svTabUI = new Ui::FVShankViewTab;
    svTabUI->setupUi( tab );
}


FVShankViewTab::~FVShankViewTab()
{
    if( MW ) {
        delete MW;
        MW = 0;
    }

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

    FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(SC->parent());
    if( !f || !f->isSvy() )
        svTabUI->howCB->removeItem( 1 );

    svTabUI->updtBut->setEnabled( false );

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
    ConnectUI( svTabUI->howCB, SIGNAL(currentIndexChanged(int)), this, SLOT(howChanged(int)) );
    ConnectUI( svTabUI->updtBut, SIGNAL(clicked()), this, SLOT(updtBut()) );
    ConnectUI( svTabUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( svTabUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( svTabUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( svTabUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( svTabUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( svTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    mapChanged( map );
    selChan( 0, 0, 0 );
}


void FVShankViewTab::mapChanged( const ShankMap *map )
{
    if( !svTabUI->howCB->currentIndex() )
        SC->view()->setShankMap( map );
}


void FVShankViewTab::selChan( int sh, int bk, int ig )
{
    if( ig < 0 )
        return;

    const ShankMap  *M = SC->view()->getSmap();

    if( M && ig < int(M->e.size()) ) {

        int ic = df->channelIDs()[ig],
            sl = ig;

        if( svTabUI->howCB->currentIndex() ) {
            if( w_sbg2ig.contains( SBG( sh, bk, ig ) ) )
                sl = w_sbg2ig[SBG( sh, bk, ig )];
            else
                sl = w_sbg2ig[SBG( sh, bk - 1, ig )];
        }

        SC->view()->setSel( sl );
        svTabUI->chanBut->setText( chanMap->name( ig, df->trig_isChan( ic ) ) );
    }
}


void FVShankViewTab::putInit()
{
    if( svTabUI->howCB->currentIndex() )
        return;

    SC->drawMtx.lock();
        heat.accumReset( true );
    SC->drawMtx.unlock();
}


void FVShankViewTab::putSamps( const vec_i16 &_data )
{
    if( svTabUI->howCB->currentIndex() )
        return;

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
    if( svTabUI->howCB->currentIndex() )
        return;

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

    int r = SC->view()->getSmap()->e[ig].r,
        ic;

    if( svTabUI->howCB->currentIndex() ) {
        ic = w_ig2sbg[ig].g;
        ig = ic;
    }
    else
        ic = df->channelIDs()[ig];

    SC->setStatus(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( chanMap->name( ig, df->trig_isChan( ic ) ) ) );
}


void FVShankViewTab::lbutClicked( int ig )
{
    cursorOver( ig );

    if( svTabUI->howCB->currentIndex() ) {

        FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(SC->parent());
        SBG &S = w_ig2sbg[ig];
        f->svyScrollToShankBank( S.s, S.b );
        ig = S.g;
    }

    emit dynamic_cast<FVShankCtl*>(SC)->selChanged( ig );
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


void FVShankViewTab::howChanged( int i )
{
    int sel = SC->view()->getSel();

    svTabUI->updtBut->setEnabled( i );

    if( i ) {

        if( !MW )
            makeWorldMap();

        SC->view()->setSel( -1 );
        SC->view()->setShankMap( MW );
        color();
        emit dynamic_cast<FVShankCtl*>(SC)->selChanged( sel );
    }
    else {
        if( w_ig2sbg.size() > sel )
            SC->view()->setSel( w_ig2sbg[sel].g, false );

        emit SC->feedMe( true );
    }
}


void FVShankViewTab::updtBut()
{
    FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(SC->parent());

    if( f ) {
        svySums[set.what] =
            f->svyAllBanks( set.what, set.thresh, set.inarow );
        color();
    }
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

    if( svTabUI->howCB->currentIndex() )
        color();
    else
        emit SC->feedMe( false );
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

void FVShankViewTab::makeWorldMap()
{
    FileViewerWindow    *f = dynamic_cast<FileViewerWindow*>(SC->parent());
    const IMROTbl       *R = df->imro();
    IMROTbl             *Q = IMROTbl::alloc( R->type );
    ShankMap            *m = new ShankMap;
    int                 nC = R->nAP(),
                        ns = R->nShank(),
                        nb = 1 + f->svyMaxbank(),
                        ng = 0;

    Q->fillDefault();

    MW = new ShankMap( ns, R->nCol(), R->nRow() );

    for( int is = 0; is < ns; ++is ) {

        int rem = Q->nElecPerShank();

        for( int ib = 0; ib < nb; ++ib ) {

            Q->fillShankAndBank( is, ib );
            m->fillDefaultIm( *Q );

            if( rem >= nC ) {
                MW->e.insert( MW->e.end(), m->e.begin(), m->e.end() );
                rem -= nC;
                for( int ic = 0; ic < nC; ++ic )
                    w_ig2sbg[ng++] = SBG( is, ib, ic );
            }
            else {
                int rowMin = R->nRow() - rem / R->nCol();
                for( int ic = 0; ic < nC; ++ic ) {
                    if( m->e[ic].r >= rowMin ) {
                        MW->e.push_back( m->e[ic] );
                        w_ig2sbg[ng++] = SBG( is, ib, ic );
                    }
                }
            }
        }
    }

    for( int ig = 0; ig < ng; ++ig )
        w_sbg2ig[w_ig2sbg[ig]] = ig;

    delete m;
    delete Q;
}


// Caller locks drawMtx.
//
void FVShankViewTab::color()
{
    if( !svTabUI->howCB->currentIndex() )
        SC->view()->colorPads( heat.sums(), set.rng[set.what] );
    else if( svySums[set.what] )
        SC->view()->colorPads( svySums[set.what], set.rng[set.what] );
    else {
        SC->setStatus( "Click Update!" );
        ::Beep( 440, 200 );
    }

    SC->update();
}


