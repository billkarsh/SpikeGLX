
#include "ui_SVShankViewTab.h"

#include "Util.h"
#include "SVShankViewTab.h"
#include "ShankCtlBase.h"
#include "ShankView.h"
#include "DAQ.h"
#include "Subset.h"
#include "Biquad.h"
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
}

/* ---------------------------------------------------------------- */
/* class Tally ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::Tally::init( double sUpdt, double VMAX, int maxInt, int nAP )
{
    this->VMAX      = VMAX;
    this->maxInt    = maxInt;
    this->nAP       = nAP;

    updtChanged( sUpdt );
}


#define SEC_PER_FETCH   0.1

void SVShankViewTab::Tally::updtChanged( double s )
{
    chunksReqd = qMax( 1, int(s / SEC_PER_FETCH) );
    zeroData();
}


void SVShankViewTab::Tally::zeroData()
{
    vmin.assign( nAP,  99000 );
    vmax.assign( nAP, -99000 );
    sums.assign( nAP,  0 );
    sumSamps    = 0;
    chunksDone  = 0;
}


double SVShankViewTab::Tally::gain( int i )
{
    if( js == jsIM )
        return p.im.prbj[ip].chanGain( i );
    else
        return p.ni.chanGain( i );
}


bool SVShankViewTab::Tally::accumSpikes(
    const short *data,
    int         ntpts,
    int         thresh,
    int         inarow )
{
    if( !ntpts )
        return false;

    double  v2i = maxInt * thresh * 1e-6 / VMAX;

    sumSamps += ntpts;

    for( int c = 0; c < nAP; ++c ) {

        const short *d      = &data[c],
                    *dlim   = &data[c + ntpts*nAP];

        int spikes  = 0,
            T       = v2i * gain( c ),
            hiCnt   = (*d <= T ? inarow : 0);

        while( (d += nAP) < dlim ) {

            if( *d <= T ) {

                if( ++hiCnt == inarow )
                    ++spikes;
            }
            else
                hiCnt = 0;
        }

        sums[c] += spikes;
    }

    return ++chunksDone >= chunksReqd;
}


void SVShankViewTab::Tally::normSpikes()
{
    double  count2Rate = p.stream_rate( js, ip ) / sumSamps;

    for( int i = 0; i < nAP; ++i )
        sums[i] *= count2Rate;
}


bool SVShankViewTab::Tally::accumPkPk( const short *data, int ntpts )
{
    if( !ntpts )
        return false;

    for( int it = 0; it < ntpts; ++it, data += nAP ) {

        for( int c = 0; c < nAP; ++c ) {

            int v = data[c];

            if( v < vmin[c] )
                vmin[c] = v;

            if( v > vmax[c] )
                vmax[c] = v;
        }
    }

    return ++chunksDone >= chunksReqd;
}


void SVShankViewTab::Tally::normPkPk( int what )
{
    double  i2v = 1e6 * VMAX / maxInt;

    if( js == jsIM ) {
        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        if( what == 1 ) {
            for( int i = 0; i < nAP; ++i )
                sums[i] = (vmax[i] - vmin[i]) * i2v / E.chanGain( i );
        }
        else {
            for( int i = 0; i < nAP; ++i )
                sums[i] = (vmax[i] - vmin[i]) * i2v / E.chanGain( i + nAP );
        }
    }
    else if( js == jsNI ) {
        for( int i = 0; i < nAP; ++i )
            sums[i] = (vmax[i] - vmin[i]) * i2v / p.ni.chanGain( i );
    }
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
    :   SC(SC), svTabUI(0), p(p), tly(p, js, ip),
        hipass(0), lopass(0), js(js), ip(ip)
{
    switch( js ) {
        case jsNI:
            VMAX    = p.ni.range.rmax;
            maxInt  = SHRT_MAX;
            nAP     = p.ni.niCumTypCnt[CniCfg::niSumNeural];
            nNu     = 0;
            nC      = p.ni.niCumTypCnt[CniCfg::niSumAll];
            break;
        case jsOB:
            VMAX    = p.im.obxj[ip].range.rmax;
            maxInt  = SHRT_MAX;
            nAP     = 0;
            nNu     = 0;
            nC      = 0;
            break;
        case jsIM:
            const CimCfg::PrbEach   &E = p.im.prbj[ip];
            VMAX    = E.roTbl->maxVolts();
            maxInt  = E.roTbl->maxInt();
            nAP     = E.imCumTypCnt[CimCfg::imSumAP];
            nNu     = E.imCumTypCnt[CimCfg::imSumNeural];
            nC      = E.imCumTypCnt[CimCfg::imSumAll];
            break;
    }

    svTabUI = new Ui::SVShankViewTab;
    svTabUI->setupUi( tab );
}


SVShankViewTab::~SVShankViewTab()
{
    SC->drawMtx.lock();
        if( hipass ) {
            delete hipass;
            hipass = 0;
        }
        if( lopass ) {
            delete lopass;
            lopass = 0;
        }
    SC->drawMtx.unlock();

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

    svTabUI->rngSB->installEventFilter( SC );
    svTabUI->rngSB->setValue( set.rng[set.what] );

    ConnectUI( svTabUI->ypixSB, SIGNAL(valueChanged(int)), this, SLOT(ypixChanged(int)) );
    ConnectUI( svTabUI->whatCB, SIGNAL(currentIndexChanged(int)), this, SLOT(whatChanged(int)) );
    ConnectUI( svTabUI->TSB, SIGNAL(valueChanged(int)), this, SLOT(threshChanged(int)) );
    ConnectUI( svTabUI->inarowSB, SIGNAL(valueChanged(int)), this, SLOT(inarowChanged(int)) );
    ConnectUI( svTabUI->updtSB, SIGNAL(valueChanged(double)), this, SLOT(updtChanged(double)) );
    ConnectUI( svTabUI->rngSB, SIGNAL(valueChanged(int)), this, SLOT(rangeChanged(int)) );
    ConnectUI( svTabUI->chanBut, SIGNAL(clicked()), this, SLOT(chanBut()) );
    ConnectUI( svTabUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );

    updateFilter( true );

    tly.init( set.updtSecs, VMAX, maxInt, nAP );
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


void SVShankViewTab::putScans( const vec_i16 &_data )
{
    int ntpts = int(_data.size()) / nC;

    SC->drawMtx.lock();

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;

    if( set.what < 2 || !nNu )
        Subset::subsetBlock( data, *(vec_i16*)&_data, 0, nAP, nC );
    else
        Subset::subsetBlock( data, *(vec_i16*)&_data, nAP, nNu, nC );

    hipass->applyBlockwiseMem( &data[0], maxInt, ntpts, nAP, 0, nAP );

    if( lopass )
        lopass->applyBlockwiseMem( &data[0], maxInt, ntpts, nAP, 0, nAP );

    zeroFilterTransient( &data[0], ntpts, nAP );

// --------------------------
// Process current data chunk
// --------------------------

    bool    done;

    if( set.what == 0 ) {
        done = tly.accumSpikes( &data[0], ntpts, set.thresh, set.inarow );
        if( done )
            tly.normSpikes();
    }
    else {
        done = tly.accumPkPk( &data[0], ntpts );
        if( done )
            tly.normPkPk( set.what );
    }

    if( done ) {
        color();
        tly.zeroData();
    }

    SC->drawMtx.unlock();
}


void SVShankViewTab::winClosed()
{
    nzero = BIQUAD_TRANS_WIDE;
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
        updateFilter( false );
        tly.zeroData();
        SC->scroll()->theV->colorPads( tly.sums, 1e99 );
        SC->update();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::threshChanged( int t )
{
    SC->drawMtx.lock();
        set.thresh = -t;
        tly.zeroData();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::inarowChanged( int s )
{
    SC->drawMtx.lock();
        set.inarow = s;
        tly.zeroData();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::updtChanged( double s )
{
    SC->drawMtx.lock();
        set.updtSecs = s;
        tly.updtChanged( s );
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::rangeChanged( int r )
{
    SC->drawMtx.lock();
        set.rng[set.what] = r;
        tly.zeroData();
    SC->drawMtx.unlock();
    SC->saveSettings();
}


void SVShankViewTab::chanBut()
{
    SC->drawMtx.lock();
        SC->scroll()->scrollToSelected();
    SC->drawMtx.unlock();
}


void SVShankViewTab::helpBut()
{
    showHelp( "ShankView_Help" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankViewTab::updateFilter( bool lock )
{
    if( lock )
        SC->drawMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }

    double  srate = p.stream_rate( js, ip );

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/srate );
    else {
        // LFP
        hipass = new Biquad( bq_type_highpass, 0.2/srate );

        if( !nNu )
            lopass = new Biquad( bq_type_lowpass,  300/srate );
    }

    nzero = BIQUAD_TRANS_WIDE;

    if( lock )
        SC->drawMtx.unlock();
}


void SVShankViewTab::zeroFilterTransient( short *data, int ntpts, int nchans )
{
    if( nzero > 0 ) {

        // overwrite with zeros

        if( ntpts > nzero )
            ntpts = nzero;

        memset( data, 0, ntpts*nchans*sizeof(qint16) );
        nzero -= ntpts;
    }
}


// Caller locks drawMtx.
//
void SVShankViewTab::color()
{
    SC->view()->colorPads( tly.sums, set.rng[set.what] );
    SC->update();
}


