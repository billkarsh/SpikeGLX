
#include "ui_ShankWindow.h"

#include "Util.h"
#include "ShankCtl_Im.h"
#include "DAQ.h"

#include <QSettings>

/* ---------------------------------------------------------------- */
/* ShankCtl_Im ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl_Im::ShankCtl_Im( DAQ::Params &p, QWidget *parent )
    :   ShankCtl( p, parent )
{
}


ShankCtl_Im::~ShankCtl_Im()
{
    saveSettings();
}


void ShankCtl_Im::init()
{
    baseInit();

    setWindowTitle( "Imec Shank Activity" );

    ConnectUI( scUI->scroll->theV, SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int,bool)) );
    ConnectUI( scUI->scroll->theV, SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int,bool)) );

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 20, 160 );
//scUI->scroll->theV->setShankMap( &S );

    scUI->scroll->theV->setShankMap( &p.sns.imChans.shankMap );
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

// Called only from init().
//
void ShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "shankView_Im" );

    settings.beginGroup( "All" );
    set.yPix    = settings.value( "yPix", 8 ).toInt();
    settings.endGroup();
}


void ShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "shankView_Im" );

    settings.beginGroup( "All" );
    settings.setValue( "yPix", set.yPix );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


