
#include "ui_FVShankViewTab.h"

#include "Util.h"
#include "FVShankViewTab.h"
#include "FVShankCtl.h"
#include "FileViewerWindow.h"
#include "DataFile.h"
#include "SignalBlocker.h"

#include <QFileDialog>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* UsrSettings ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::UsrSettings::loadSettings( QSettings &S )
{
    yPix        = S.value( "yPix", 8 ).toInt();
    maxrow      = S.value( "maxrow", -1 ).toInt();
    what        = S.value( "what", 1 ).toInt();
    thresh      = S.value( "thresh", -75 ).toInt();
    inarow      = S.value( "staylow", 3 ).toInt();
    rng[0]      = S.value( "rngSpk", 100 ).toInt();
    rng[1]      = S.value( "rngAP", 100 ).toInt();
    rng[2]      = S.value( "rngLF", 100 ).toInt();
    gbldmx      = S.value( "gbldmx", true ).toBool();
    colorShanks = S.value( "colorShanks", true ).toBool();
    colorTraces = S.value( "colorTraces", false ).toBool();
}


void FVShankViewTab::UsrSettings::saveSettings( QSettings &S ) const
{
    S.setValue( "yPix", yPix );
    S.setValue( "maxrow", maxrow );
    S.setValue( "what", what );
    S.setValue( "thresh", thresh );
    S.setValue( "staylow", inarow );
    S.setValue( "rngSpk", rng[0] );
    S.setValue( "rngAP", rng[1] );
    S.setValue( "rngLF", rng[2] );
    S.setValue( "gbldmx", gbldmx );
    S.setValue( "colorShanks", colorShanks );
    S.setValue( "colorTraces", colorTraces );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

FVShankViewTab::FVShankViewTab(
    ShankCtlBase    *SC,
    QWidget         *tab,
    const DataFile  *df )
    :   QObject(0), SC(SC), fvTabUI(0), df(df), MW(0),
        chanMap(df->chanMap()), lfp(df->subtypeFromObj()=="imec.lf")
{
    svySums[0] = 0;
    svySums[1] = 0;
    svySums[2] = 0;

    heat.setStream( df );

    fvTabUI = new Ui::FVShankViewTab;
    fvTabUI->setupUi( tab );
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

    if( fvTabUI ) {
        delete fvTabUI;
        fvTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankViewTab::init( const ShankMap *map )
{
    SC->view()->setRowPix( set.yPix );
    SC->syncYPix( set.yPix );

    fvTabUI->ypixSB->installEventFilter( SC );
    fvTabUI->ypixSB->setValue( set.yPix );

    fvTabUI->gbldmxChk->setChecked( set.gbldmx );
    fvTabUI->maxrowSB->setValue( set.maxrow );

    FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(SC->parent());
    if( !f || !f->isSvy() )
        fvTabUI->howCB->removeItem( 1 );

    fvTabUI->updtBut->setEnabled( false );

    fvTabUI->whatCB->setCurrentIndex( set.what );
    fvTabUI->whatCB->setEnabled( !lfp );

    fvTabUI->TSB->installEventFilter( SC );
    fvTabUI->TSB->setValue( -set.thresh );
    fvTabUI->TSB->setEnabled( set.what == 0 );

    fvTabUI->inarowSB->installEventFilter( SC );
    fvTabUI->inarowSB->setValue( set.inarow );
    fvTabUI->inarowSB->setEnabled( set.what == 0 );

    fvTabUI->rngSB->installEventFilter( SC );
    fvTabUI->rngSB->setValue( set.rng[set.what] );

    fvTabUI->legendTE->setFontPointSize( 10 );
    fvTabUI->legendTE->setFontWeight( QFont::DemiBold );

    fvTabUI->shanksChk->setChecked( set.colorShanks );
    fvTabUI->shanksChk->setEnabled( false );
    fvTabUI->tracesChk->setChecked( set.colorTraces );
    fvTabUI->tracesChk->setEnabled( false );

    ConnectUI( fvTabUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( fvTabUI->gbldmxChk, SIGNAL(clicked(bool)), this, SLOT(gbldmxCheck(bool)) );
    ConnectUI( fvTabUI->maxrowSB, SIGNAL(valueChanged(int)), this, SLOT(maxrowChanged(int)) );
    ConnectUI( fvTabUI->howCB, SIGNAL(currentIndexChanged(int)), this, SLOT(howChanged(int)) );
    ConnectUI( fvTabUI->updtBut, SIGNAL(clicked()), this, SLOT(updtBut()) );
    ConnectUI( fvTabUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( fvTabUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( fvTabUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( fvTabUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( fvTabUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( fvTabUI->shanksChk, SIGNAL(clicked(bool)), this, SLOT(shanksCheck(bool)) );
    ConnectUI( fvTabUI->tracesChk, SIGNAL(clicked(bool)), this, SLOT(tracesCheck(bool)) );
    ConnectUI( fvTabUI->expBut, SIGNAL(clicked()), this, SLOT(expBut()) );
    ConnectUI( fvTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    mapChanged( map );
    selChan( 0, 0, 0 );
}


void FVShankViewTab::setAnatomyPP( const QString &elems, int sk )
{
    anat.parse( elems, df->imro(), sk );
    anat.fillLegend( fvTabUI->legendTE );
    fvTabUI->shanksChk->setEnabled( true );
    fvTabUI->tracesChk->setEnabled( true );
    anat.colorShanks( SC->view(), fvTabUI->shanksChk->isChecked() );

    if( fvTabUI->tracesChk->isChecked() )
        emit SC->gimmeTraces();
}


void FVShankViewTab::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY )
{
    anat.colorTraces( theX, vY, fvTabUI->tracesChk->isChecked() );
}


QString FVShankViewTab::getLbl( int s, int r ) const
{
    return anat.getLbl( s, r );
}


void FVShankViewTab::mapChanged( const ShankMap *map )
{
    if( !fvTabUI->howCB->currentIndex() ) {
        SC->drawMtx.lock();
            heat.updateMap( map, set.maxrow );
            SC->view()->setShankMap( map );
        SC->drawMtx.unlock();
    }
}


void FVShankViewTab::selChan( int sh, int bk, int ig )
{
    if( ig < 0 )
        return;

    const ShankMap  *M = SC->view()->getSmap();

    if( M && ig < int(M->e.size()) ) {

        int ic  = df->fileChans()[ig],
            sel = ig;

        if( fvTabUI->howCB->currentIndex() ) {
            if( w_sbg2ig.contains( SBG( sh, bk, ig ) ) )
                sel = w_sbg2ig[SBG( sh, bk, ig )];
            else
                sel = w_sbg2ig[SBG( sh, bk - 1, ig )];
        }

        SC->view()->setSel( sel );
        fvTabUI->chanBut->setText( chanMap->name( ig, df->trig_isChan( ic ) ) );
    }
}


void FVShankViewTab::putInit()
{
    if( fvTabUI->howCB->currentIndex() )
        return;

    SC->drawMtx.lock();
        heat.accumReset( true, set.what );
    SC->drawMtx.unlock();
}


void FVShankViewTab::putSamps( const vec_i16 &_data )
{
    if( fvTabUI->howCB->currentIndex() )
        return;

    vec_i16 data;

    SC->drawMtx.lock();
        switch( set.what ) {
            case 0:
                heat.apFilter( data, _data, 0, set.gbldmx );
                heat.accumSpikes( data, set.thresh, set.inarow );
                break;
            case 1:
                heat.apFilter( data, _data, 0, set.gbldmx );
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
    if( fvTabUI->howCB->currentIndex() )
        return;

    SC->drawMtx.lock();
        if( set.what == 0 )
            heat.normSpikes();
        else if( !heat.normPkPk( set.what ) )
            SC->setStatus( "Set LFP time span > 2 seconds" );
        color();
    SC->drawMtx.unlock();
}


void FVShankViewTab::cursorOver( int ig )
{
    if( ig < 0 ) {
        SC->setStatus( QString() );
        return;
    }

    const ShankMapDesc  &SMD = SC->view()->getSmap()->e[ig];
    int                 ic;

    if( fvTabUI->howCB->currentIndex() ) {
        ic = w_ig2sbg[ig].g;
        ig = ic;
    }
    else
        ic = df->fileChans()[ig];

    SC->setStatus(
        QString("row %1 %2 %3")
        .arg( SMD.r, 3, 10, QChar('0') )
        .arg( chanMap->name( ig, df->trig_isChan( ic ) ) )
        .arg( anat.getLbl( SMD.s, SMD.r ) ) );
}


void FVShankViewTab::lbutClicked( int ig )
{
    cursorOver( ig );

    if( fvTabUI->howCB->currentIndex() ) {

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
    SignalBlocker   b0(fvTabUI->ypixSB);
    set.yPix = y;
    fvTabUI->ypixSB->setValue( y );
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


void FVShankViewTab::gbldmxCheck( bool on )
{
    SC->drawMtx.lock();
        set.gbldmx = on;
    SC->drawMtx.unlock();
    SC->saveSettings();

    emit dynamic_cast<FVShankCtl*>(SC)->gbldmxChanged();
    emit dynamic_cast<FVShankCtl*>(SC)->feedMe( false );

    if( fvTabUI->howCB->currentIndex() )
        SC->setStatus( "Click Update!" );
}


void FVShankViewTab::maxrowChanged( int r )
{
    SC->drawMtx.lock();
        set.maxrow = r;
    SC->drawMtx.unlock();
    SC->saveSettings();

    emit dynamic_cast<FVShankCtl*>(SC)->maxrowChanged();

    if( fvTabUI->howCB->currentIndex() )
        SC->setStatus( "Click Update!" );
}


void FVShankViewTab::howChanged( int i )
{
    int sel = SC->view()->getSel();

    fvTabUI->updtBut->setEnabled( i );

    if( i ) {

        if( !MW )
            makeWorldMap();

        SC->view()->setSel( -1 );
        SC->drawMtx.lock();
            SC->view()->setShankMap( MW );
            color();
        SC->drawMtx.unlock();
        emit dynamic_cast<FVShankCtl*>(SC)->selChanged( sel );
    }
    else {
        if( w_ig2sbg.size() > sel )
            SC->view()->setSel( w_ig2sbg[sel].g, false );

        emit dynamic_cast<FVShankCtl*>(SC)->feedMe( true );
    }
}


void FVShankViewTab::updtBut()
{
    FileViewerWindow *f = dynamic_cast<FileViewerWindow*>(SC->parent());

    if( f ) {
        SC->drawMtx.lock();
            QGuiApplication::setOverrideCursor( QCursor(Qt::WaitCursor) );
            svySums[set.what] =
                f->svyAllBanks(
                    set.what, set.thresh, set.inarow, set.maxrow, set.gbldmx );
            color();
            QGuiApplication::restoreOverrideCursor();
        SC->drawMtx.unlock();
        SC->setStatus( "Updated" );
    }
}


void FVShankViewTab::whatChanged( int i )
{
    SC->drawMtx.lock();

        set.what = i;
        SignalBlocker   b0(fvTabUI->rngSB);
        fvTabUI->TSB->setEnabled( !i );
        fvTabUI->inarowSB->setEnabled( !i );
        fvTabUI->rngSB->setValue( set.rng[i] );
        SC->saveSettings();

        if( fvTabUI->howCB->currentIndex() ) {
            color();
            SC->drawMtx.unlock();
            return;
        }

    SC->drawMtx.unlock();

    emit dynamic_cast<FVShankCtl*>(SC)->feedMe( false );
}


void FVShankViewTab::threshChanged( int t )
{
    SC->drawMtx.lock();
        set.thresh = -t;
    SC->drawMtx.unlock();
    SC->saveSettings();

    if( fvTabUI->howCB->currentIndex() )
        SC->setStatus( "Click Update!" );
}


void FVShankViewTab::inarowChanged( int s )
{
    SC->drawMtx.lock();
        set.inarow = s;
    SC->drawMtx.unlock();
    SC->saveSettings();

    if( fvTabUI->howCB->currentIndex() )
        SC->setStatus( "Click Update!" );
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


void FVShankViewTab::shanksCheck( bool on )
{
    SC->drawMtx.lock();
        set.colorShanks = on;
    SC->drawMtx.unlock();
    SC->saveSettings();
    anat.colorShanks( SC->view(), on );
}


void FVShankViewTab::tracesCheck( bool on )
{
    set.colorTraces = on;
    SC->saveSettings();
    emit SC->gimmeTraces();
}


void FVShankViewTab::expBut()
{
    const double    *pSvy   = svySums[set.what];
    int             how     = fvTabUI->howCB->currentIndex();

// Something to export?

    if( how && !pSvy ) {
        SC->setStatus( "Click Update!" );
        ::Beep( 440, 200 );
        return;
    }

// Select file

    QString fn = QFileDialog::getSaveFileName(
                    SC,
                    "Save exported heatmap",
                    "",
                    "Text files (*.txt)" );

    if( !fn.length() )
        return;

// Open file

    QFile   f( fn );

    if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

        // Write file
        SC->drawMtx.lock();
            const double *src = (how ? pSvy : heat.sums() );
            dynamic_cast<FVShankCtl*>(SC)->exportHeat( &f, src );
        SC->drawMtx.unlock();
        SC->setStatus( "Export done" );
    }
    else {
        QFileInfo   fi( fn );
        SC->setStatus( QString("Error opening '%1'").arg( fi.fileName() ) );
        return;
    }
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
    IMROTbl             *Q = IMROTbl::alloc( R->pn );
    ShankMap            *m = new ShankMap;
    int                 nC = R->nAP(),
                        ns = R->nSvyShank(),
                        nb = 1 + f->svyMaxbank(),
                        ng = 0,
                        R0;

    Q->fillDefault();

    MW = new ShankMap( R->nShank(), R->nCol_vis(), R->nRow() );

    for( int is = 0; is < ns; ++is ) {

        for( int ib = 0; ib < nb; ++ib ) {

            Q->fillShankAndBank( is, ib );
            Q->toShankMap_vis( *m );
            R0 = Q->svy_minRow( is, ib );

            if( R0 < 0 ) {
                MW->e.insert( MW->e.end(), m->e.begin(), m->e.end() );
                for( int ic = 0; ic < nC; ++ic )
                    w_ig2sbg[ng++] = SBG( is, ib, ic );
            }
            else {
                for( int ic = 0; ic < nC; ++ic ) {
                    if( m->e[ic].r >= R0 ) {
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
    if( !fvTabUI->howCB->currentIndex() )
        SC->view()->colorPads( heat.sums(), set.rng[set.what] );
    else if( svySums[set.what] )
        SC->view()->colorPads( svySums[set.what], set.rng[set.what] );
    else {
        SC->setStatus( "Click Update!" );
        ::Beep( 440, 200 );
    }

    SC->update();
}


