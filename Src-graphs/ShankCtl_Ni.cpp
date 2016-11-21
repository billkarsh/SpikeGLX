
#include "ui_ShankWindow.h"

#include "Util.h"
#include "ShankCtl_Ni.h"
#include "DAQ.h"

#include <QSettings>

/* ---------------------------------------------------------------- */
/* ShankCtl_Ni ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankCtl_Ni::ShankCtl_Ni( DAQ::Params &p, QWidget *parent )
    :   ShankCtl( p, parent )
{
}


ShankCtl_Ni::~ShankCtl_Ni()
{
    saveSettings();
}


void ShankCtl_Ni::init()
{
    baseInit();

    setWindowTitle( "Nidq Shank Activity" );

    ConnectUI( scUI->scroll->theV, SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int)) );
    ConnectUI( scUI->scroll->theV, SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int)) );

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 20, 160 );
//scUI->scroll->theV->setShankMap( &S );

    scUI->scroll->theV->setShankMap( &p.sns.niChans.shankMap );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtl_Ni::cursorOver( int ic )
{
    if( ic < 0 ) {
        scUI->statusLbl->setText( QString::null );
        return;
    }

    int r = scUI->scroll->theV->getSmap()->e[ic].r;

    scUI->statusLbl->setText(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( p.sns.niChans.chanMap.name(
            ic, p.isTrigChan( "nidq", ic ) ) ) );
}


void ShankCtl_Ni::lbutClicked( int ic )
{
    cursorOver( ic );
    emit selChanged( ic, false );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Called only from init().
//
void ShankCtl_Ni::loadSettings()
{
    STDSETTINGS( settings, "shankView_Ni" );

    settings.beginGroup( "All" );
    set.yPix    = settings.value( "yPix", 8 ).toInt();
    settings.endGroup();
}


void ShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "shankView_Ni" );

    settings.beginGroup( "All" );
    settings.setValue( "yPix", set.yPix );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


