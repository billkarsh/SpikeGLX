
#include "ui_ShankViewTab.h"

#include "Util.h"
#include "ShankViewTab.h"
#include "FVW_ShankCtl.h"
#include "ShankView.h"
#include "DataFile.h"
#include "Subset.h"
#include "Biquad.h"
#include "SignalBlocker.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* UsrSettings ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankViewTab::UsrSettings::loadSettings( QSettings &S )
{
    yPix    = S.value( "yPix", 8 ).toInt();
    what    = S.value( "what", 1 ).toInt();
    thresh  = S.value( "thresh", -75 ).toInt();
    inarow  = S.value( "staylow", 5 ).toInt();
    rng[0]  = S.value( "rngSpk", 100 ).toInt();
    rng[1]  = S.value( "rngAP", 100 ).toInt();
    rng[2]  = S.value( "rngLF", 100 ).toInt();
}


void ShankViewTab::UsrSettings::saveSettings( QSettings &S ) const
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
/* class Tally ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankViewTab::Tally::init( int maxInt, int nNu )
{
    this->maxInt    = maxInt;
    this->nNu       = nNu;
}


void ShankViewTab::Tally::zeroData()
{
    vmin.assign( nNu,  99000 );
    vmax.assign( nNu, -99000 );
    sums.assign( nNu,  0 );
    sumSamps = 0;
}


void ShankViewTab::Tally::accumSpikes(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim,
    int         thresh,
    int         inarow )
{
    if( !ntpts )
        return;

    double  v = maxInt * thresh * 1e-6 / VMAX;

    sumSamps += ntpts;

    for( int c = c0; c < cLim; ++c ) {

        const short *d      = &data[c],
                    *dlim   = &data[c + ntpts*nchans];

        int i       = c - c0,
            spikes  = 0,
            T,
            hiCnt;

        T = v * df->ig2Gain( i );

        hiCnt = (*d <= T ? inarow : 0);

        while( (d += nchans) < dlim ) {

            if( *d <= T ) {

                if( ++hiCnt == inarow )
                    ++spikes;
            }
            else
                hiCnt = 0;
        }

        sums[i] += spikes;
    }
}


void ShankViewTab::Tally::normSpikes()
{
    double  count2Rate = df->samplingRateHz() / sumSamps;

    for( int i = 0; i < nNu; ++i )
        sums[i] *= count2Rate;
}


void ShankViewTab::Tally::accumPkPk(
    const short *data,
    int         ntpts,
    int         nchans,
    int         c0,
    int         cLim )
{
    if( !ntpts )
        return;

    for( int it = 0; it < ntpts; ++it, data += nchans ) {

        for( int c = c0; c < cLim; ++c ) {

            int v = data[c],
                i = c - c0;

            if( v < vmin[i] )
                vmin[i] = v;

            if( v > vmax[i] )
                vmax[i] = v;
        }
    }
}


void ShankViewTab::Tally::normPkPk()
{
    double  ysc = 1e6 * VMAX / maxInt;

    for( int i = 0; i < nNu; ++i )
        sums[i] = (vmax[i] - vmin[i]) * ysc / df->ig2Gain( i );
}

/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankViewTab::ShankViewTab( FVW_ShankCtl *SC, QWidget *tab )
    :   VMAX(SC->df->vRange().rmax), SC(SC), svTabUI(0), df(SC->df),
        chanMap(df->chanMap()), tly(df, VMAX), hipass(0), lopass(0),
        nC(df->numChans()), lfp(df->subtypeFromObj()=="imec.lf")
{
    if( df->streamFromObj().startsWith( "i" ) )
        maxInt = qMax( df->getParam("imMaxInt").toInt(), 512 ) - 1;
    else
        maxInt = SHRT_MAX;

    svTabUI = new Ui::ShankViewTab;
    svTabUI->setupUi( tab );
}


ShankViewTab::~ShankViewTab()
{
    drawMtx.lock();
        if( hipass ) {
            delete hipass;
            hipass = 0;
        }
        if( lopass ) {
            delete lopass;
            lopass = 0;
        }
    drawMtx.unlock();

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

void ShankViewTab::baseInit( const ShankMap *map, int bnkRws )
{
    SC->view()->setRowPix( set.yPix );
    SC->view()->setBnkRws( bnkRws );

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

    updateFilter( true );

    mapChanged( map );
    selChan( 0 );
}


void ShankViewTab::mapChanged( const ShankMap *map )
{
    nNu = map->e.size();
    SC->view()->setShankMap( map );
    tly.init( maxInt, nNu );
}


void ShankViewTab::selChan( int ig )
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


void ShankViewTab::putInit()
{
    tly.zeroData();

// FVW allows random access to file so we
// invalidate filter history on each draw.

    nzero = BIQUAD_TRANS_WIDE;
}


void ShankViewTab::putScans( const vec_i16 &_data )
{
    int ntpts = int(_data.size()) / nC;

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;
    Subset::subsetBlock( data, *(vec_i16*)&_data, 0, nNu, nC );

    hipass->applyBlockwiseMem( &data[0], maxInt, ntpts, nNu, 0, nNu );

    if( lopass )
        lopass->applyBlockwiseMem( &data[0], maxInt, ntpts, nNu, 0, nNu );

    zeroFilterTransient( &data[0], ntpts, nNu );

// --------------------------
// Process current data chunk
// --------------------------

    if( set.what == 0 )
        tly.accumSpikes( &data[0], ntpts, nNu, 0, nNu, set.thresh, set.inarow );
    else
        tly.accumPkPk( &data[0], ntpts, nNu, 0, nNu );
}


void ShankViewTab::putDone()
{
    if( set.what == 0 )
        tly.normSpikes();
    else
        tly.normPkPk();

    color();
}


void ShankViewTab::cursorOver( int ig )
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
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankViewTab::ypixChanged( int y )
{
    set.yPix = y;
    SC->scroll()->setRowPix( y );
    SC->saveSettings();
}


void ShankViewTab::whatChanged( int i )
{
    drawMtx.lock();
        set.what = i;
        SignalBlocker   b0(svTabUI->rngSB);
        svTabUI->TSB->setEnabled( !i );
        svTabUI->inarowSB->setEnabled( !i );
        svTabUI->rngSB->setValue( set.rng[i] );
        updateFilter( false );
    drawMtx.unlock();
    SC->saveSettings();
    emit SC->feedMe();
}


void ShankViewTab::threshChanged( int t )
{
    drawMtx.lock();
        set.thresh = -t;
    drawMtx.unlock();
    SC->saveSettings();
}


void ShankViewTab::inarowChanged( int s )
{
    drawMtx.lock();
        set.inarow = s;
    drawMtx.unlock();
    SC->saveSettings();
}


void ShankViewTab::rangeChanged( int r )
{
    drawMtx.lock();
        set.rng[set.what] = r;
        color();
    drawMtx.unlock();
    SC->saveSettings();
}


void ShankViewTab::chanBut()
{
    SC->scroll()->scrollToSelected();
}


void ShankViewTab::helpBut()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankViewTab::updateFilter( bool lock )
{
    if( lock )
        drawMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }

    double  srate = df->samplingRateHz();

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/srate );
    else {
        // LFP
        hipass = new Biquad( bq_type_highpass, 0.2/srate );
        lopass = new Biquad( bq_type_lowpass,  300/srate );
    }

    if( lock )
        drawMtx.unlock();
}


void ShankViewTab::zeroFilterTransient( short *data, int ntpts, int nchans )
{
    if( nzero > 0 ) {

        // overwrite with zeros

        if( ntpts > nzero )
            ntpts = nzero;

        memset( data, 0, ntpts*nchans*sizeof(qint16) );
        nzero -= ntpts;
    }
}


void ShankViewTab::color()
{
    SC->view()->colorPads( tly.sums, set.rng[set.what] );
    update();
}


void ShankViewTab::update()
{
    SC->view()->updateNow();

//    QMetaObject::invokeMethod(
//        view(),
//        "updateNow",
//        Qt::QueuedConnection );
}




