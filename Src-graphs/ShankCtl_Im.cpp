
#include "ui_ShankWindow.h"

#include "Util.h"
#include "ShankCtl_Im.h"
#include "DAQ.h"
#include "Subset.h"
#include "Biquad.h"

#include <QSettings>


#define MAX10BIT    512


/* ---------------------------------------------------------------- */
/* ShankCtl_Im ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl_Im::ShankCtl_Im( const DAQ::Params &p, QWidget *parent )
    :   ShankCtl( p, parent )
{
}


void ShankCtl_Im::init()
{
    baseInit();

    scUI->statusLbl->setToolTip(
        "Use shift-key or right-clicks to see/select LF chans" );

    tly.init( set.updtSecs, true );

    setWindowTitle( "Imec Shank Activity" );

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 48, 384 );
//scUI->scroll->theV->setShankMap( &S );

    scUI->scroll->theV->setShankMap( &p.sns.imChans.shankMap );
}


void ShankCtl_Im::putScans( const vec_i16 &_data )
{
    double      ysc     = 1e6 * p.im.range.rmax / MAX10BIT;
    const int   nC      = p.im.imCumTypCnt[CimCfg::imSumAll],
                nNu     = p.im.imCumTypCnt[CimCfg::imSumNeural],
                nAP     = p.im.imCumTypCnt[CimCfg::imSumAP],
                ntpts   = (int)_data.size() / nC;

    drawMtx.lock();

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;

    if( set.what < 2 )
        Subset::subsetBlock( data, *(vec_i16*)&_data, 0, nAP, nC );
    else
        Subset::subsetBlock( data, *(vec_i16*)&_data, nAP, nNu, nC );

    hipass->applyBlockwiseMem( &data[0], MAX10BIT, ntpts, nAP, 0, nAP );

    zeroFilterTransient( &data[0], ntpts, nAP );

// --------------------------
// Process current data chunk
// --------------------------

    bool    done = false;

    if( set.what == 0 ) {

        // Count spikes

        done = tly.countSpikes( &data[0], ntpts, nAP, 0, nAP,
                set.thresh, set.inarow );
    }
    else {

        // Peak to peak

        done = tly.accumPkPk( &data[0], ntpts, nAP, 0, nAP );

        if( done ) {

            if( set.what == 1 ) {

                // AP gains

                for( int i = 0; i < nAP; ++i )
                    tly.sums[i] *= ysc / p.im.chanGain( i );
            }
            else {

                // LF gains

                for( int i = 0; i < nAP; ++i )
                    tly.sums[i] *= ysc / p.im.chanGain( i + nAP );
            }
        }
    }

// ----------------------
// Draw results and reset
// ----------------------

    if( done ) {

        scUI->scroll->theV->colorPads( tly.sums, set.rng[set.what] );

        tly.zeroData();

        QMetaObject::invokeMethod(
            scUI->scroll->theV,
            "updateNow",
            Qt::QueuedConnection );
    }

    drawMtx.unlock();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl_Im::cursorOver( int ic, bool shift )
{
    if( ic < 0 ) {
        scUI->statusLbl->setText( QString::null );
        return;
    }

    int r = scUI->scroll->theV->getSmap()->e[ic].r;

    if( shift )
        ic += p.im.imCumTypCnt[CimCfg::imSumAP];

    scUI->statusLbl->setText(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( p.sns.imChans.chanMap.name(
            ic, p.isTrigChan( "imec", ic ) ) ) );
}


void ShankCtl_Im::lbutClicked( int ic, bool shift )
{
    cursorOver( ic, shift );
    emit selChanged( ic, shift );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void ShankCtl_Im::updateFilter( bool lock )
{
    if( lock )
        drawMtx.lock();

    if( hipass ) {
        delete hipass;
        hipass = 0;
    }

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );
    else
        hipass = new Biquad( bq_type_highpass, 0.2/p.ni.srate );

    nzero = BIQUAD_TRANS_WIDE;

    if( lock )
        drawMtx.unlock();
}


// Called only from init().
//
void ShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "shankView_Im" );

    settings.beginGroup( "All" );
    set.updtSecs    = settings.value( "updtSecs", 1.0 ).toDouble();
    set.yPix        = settings.value( "yPix", 8 ).toInt();
    set.what        = settings.value( "what", 0 ).toInt();
    set.thresh      = settings.value( "thresh", -100 ).toInt();
    set.inarow      = settings.value( "staylow", 5 ).toInt();
    set.rng[0]      = settings.value( "rngSpk", 1000 ).toInt();
    set.rng[1]      = settings.value( "rngAP", 100 ).toInt();
    set.rng[2]      = settings.value( "rngLF", 100 ).toInt();
    settings.endGroup();
}


void ShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "shankView_Im" );

    settings.beginGroup( "All" );
    settings.setValue( "updtSecs", set.updtSecs );
    settings.setValue( "yPix", set.yPix );
    settings.setValue( "what", set.what );
    settings.setValue( "thresh", set.thresh );
    settings.setValue( "staylow", set.inarow );
    settings.setValue( "rngSpk", set.rng[0] );
    settings.setValue( "rngAP", set.rng[1] );
    settings.setValue( "rngLF", set.rng[2] );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


