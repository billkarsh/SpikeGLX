
#include "ui_ShankWindow.h"

#include "Util.h"
#include "ShankCtl_Ni.h"
#include "DAQ.h"
#include "Subset.h"
#include "Biquad.h"

#include <QSettings>


#define MAX16BIT    32768


/* ---------------------------------------------------------------- */
/* ShankCtl_Ni ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl_Ni::ShankCtl_Ni(
    const DAQ::Params   &p,
    int                 jpanel,
    QWidget             *parent )
    :   ShankCtl( p, jpanel, parent )
{
}


void ShankCtl_Ni::init()
{
    baseInit( 0, 0 );

    setWindowTitle( "Nidq Shank Activity" );

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 20, 160 );
//scUI->scroll->theV->setShankMap( &S );

    mapChanged();
}


void ShankCtl_Ni::mapChanged()
{
    scUI->scroll->theV->setShankMap( &p.ni.sns.shankMap );
}


void ShankCtl_Ni::putScans( const vec_i16 &_data )
{
    double      ysc     = 1e6 * p.ni.range.rmax / MAX16BIT;
    const int   nC      = p.ni.niCumTypCnt[CniCfg::niSumAll],
                nNu     = p.ni.niCumTypCnt[CniCfg::niSumNeural],
                ntpts   = int(_data.size()) / nC;

    drawMtx.lock();

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;
    Subset::subsetBlock( data, *(vec_i16*)&_data, 0, nNu, nC );

    hipass->applyBlockwiseMem( &data[0], MAX16BIT, ntpts, nNu, 0, nNu );

    if( lopass )
        lopass->applyBlockwiseMem( &data[0], MAX16BIT, ntpts, nNu, 0, nNu );

    zeroFilterTransient( &data[0], ntpts, nNu );

// --------------------------
// Process current data chunk
// --------------------------

    bool    done = false;

    if( set.what == 0 ) {

        done = tly.accumSpikes( &data[0], ntpts, nNu, 0, nNu,
                set.thresh, set.inarow );
    }
    else {

        done = tly.accumPkPk( &data[0], ntpts, nNu, 0, nNu );

        if( done ) {

            for( int i = 0; i < nNu; ++i )
                tly.sums[i] *= ysc / p.ni.chanGain( i );
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

void ShankCtl_Ni::cursorOver( int ic, bool shift )
{
    Q_UNUSED( shift )

    if( ic < 0 ) {
        scUI->statusLbl->setText( QString() );
        return;
    }

    int r = scUI->scroll->theV->getSmap()->e[ic].r;

    scUI->statusLbl->setText(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( p.ni.sns.chanMap.name( ic, p.trig_isChan( 0, 0, ic ) ) ) );
}


void ShankCtl_Ni::lbutClicked( int ic, bool shift )
{
    Q_UNUSED( shift )

    cursorOver( ic, false );
    emit selChanged( ic, false );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void ShankCtl_Ni::updateFilter( bool lock )
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

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/p.ni.srate );
    else {
        hipass = new Biquad( bq_type_highpass, 0.2/p.ni.srate );
        lopass = new Biquad( bq_type_lowpass,  300/p.ni.srate );
    }

    nzero = BIQUAD_TRANS_WIDE;

    if( lock )
        drawMtx.unlock();
}


QString ShankCtl_Ni::settingsName() const
{
    return "ShankView_Nidq";
}


// Called only from init().
//
void ShankCtl_Ni::loadSettings()
{
    STDSETTINGS( settings, "shankview_nidq" );

    settings.beginGroup( settingsName() );
    set.updtSecs    = settings.value( "updtSecs", 0.1 ).toDouble();
    set.yPix        = settings.value( "yPix", 8 ).toInt();
    set.what        = settings.value( "what", 1 ).toInt();
    set.thresh      = settings.value( "thresh", -75 ).toInt();
    set.inarow      = settings.value( "staylow", 5 ).toInt();
    set.rng[0]      = settings.value( "rngSpk", 100 ).toInt();
    set.rng[1]      = settings.value( "rngAP", 100 ).toInt();
    set.rng[2]      = settings.value( "rngLF", 100 ).toInt();
    settings.endGroup();
}


void ShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "shankview_nidq" );

    settings.beginGroup( settingsName() );
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


