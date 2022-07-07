
#include "ui_ShankWindow.h"

#include "Util.h"
#include "ShankCtl_Im.h"
#include "DAQ.h"
#include "Subset.h"
#include "Biquad.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* ShankCtl_Im ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl_Im::ShankCtl_Im(
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel,
    QWidget             *parent )
    :   ShankCtl( p, jpanel, parent ), ip(ip)
{
}


void ShankCtl_Im::init()
{
    baseInit( 2, ip );

    scUI->statusLbl->setToolTip(
        "Use shift-key or right-clicks to see/select LF chans" );

    setWindowTitle( QString("Imec%1 Shank Activity").arg( ip ) );

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 48, 384 );
//scUI->scroll->theV->setShankMap( &S );

    mapChanged();
}


void ShankCtl_Im::mapChanged()
{
    scUI->scroll->theV->setShankMap( &p.im.prbj[ip].sns.shankMap );
}


void ShankCtl_Im::putScans( const vec_i16 &_data )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    double      ysc;
    const int   nC      = E.imCumTypCnt[CimCfg::imSumAll],
                nNu     = E.imCumTypCnt[CimCfg::imSumNeural],
                nAP     = E.imCumTypCnt[CimCfg::imSumAP],
                maxInt  = E.roTbl->maxInt(),
                ntpts   = int(_data.size()) / nC;

    ysc = 1e6 * E.roTbl->maxVolts() / maxInt;

    drawMtx.lock();

// -----------------------------
// Make local copy we can filter
// -----------------------------

    vec_i16 data;

    if( set.what < 2 || !E.roTbl->nLF() )
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

    bool    done = false;

    if( set.what == 0 ) {

        done = tly.accumSpikes( &data[0], ntpts, nAP, 0, nAP,
                set.thresh, set.inarow );
    }
    else {

        done = tly.accumPkPk( &data[0], ntpts, nAP, 0, nAP );

        if( done ) {

            if( set.what == 1 ) {

                // AP gains

                for( int i = 0; i < nAP; ++i )
                    tly.sums[i] *= ysc / E.chanGain( i );
            }
            else {

                // LF gains

                for( int i = 0; i < nAP; ++i )
                    tly.sums[i] *= ysc / E.chanGain( i + nAP );
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
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( ic < 0 ) {
        scUI->statusLbl->setText( QString() );
        return;
    }

    int r = scUI->scroll->theV->getSmap()->e[ic].r;

    if( shift && E.roTbl->nLF() )
        ic += E.imCumTypCnt[CimCfg::imSumAP];

    scUI->statusLbl->setText(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( E.sns.chanMap.name( ic, p.trig_isChan( 2, ip, ic ) ) ) );
}


void ShankCtl_Im::lbutClicked( int ic, bool shift )
{
    if( shift ) {

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        if( !E.roTbl->nLF() )
            shift = false;
    }

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

    if( lopass ) {
        delete lopass;
        lopass = 0;
    }

    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( set.what < 2 )
        hipass = new Biquad( bq_type_highpass, 300/E.srate );
    else {
        // LFP

        hipass = new Biquad( bq_type_highpass, 0.2/E.srate );

        if( !E.roTbl->nLF() )
            lopass = new Biquad( bq_type_lowpass, 300/E.srate );
    }

    nzero = BIQUAD_TRANS_WIDE;

    if( lock )
        drawMtx.unlock();
}


QString ShankCtl_Im::settingsName() const
{
    return QString("ShankView_Imec_Panel%1").arg( jpanel );
}


// Called only from init().
//
void ShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        set.loadSettings( settings );
    settings.endGroup();
}


void ShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        set.saveSettings( settings );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


