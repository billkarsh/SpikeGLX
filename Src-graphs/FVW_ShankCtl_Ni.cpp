
#include "ui_FVW_ShankWindow.h"

#include "Util.h"
#include "FVW_ShankCtl_Ni.h"
#include "DataFile.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* FVW_ShankCtl_Ni ------------------------------------------------ */
/* ---------------------------------------------------------------- */

FVW_ShankCtl_Ni::FVW_ShankCtl_Ni( const DataFile *df, QWidget *parent )
    :   FVW_ShankCtl( df, parent )
{
}


void FVW_ShankCtl_Ni::init( const ShankMap *map )
{
    maxInt = SHRT_MAX;

    baseInit();

//static ShankMap S;
//S.fillDefaultNi( 4, 2, 20, 160 );
//scUI->scroll->theV->setShankMap( &S );

    mapChanged( map );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString FVW_ShankCtl_Ni::settingsName() const
{
    return "ShankView_Nidq";
}


// Called only from init().
//
void FVW_ShankCtl_Ni::loadSettings()
{
    STDSETTINGS( settings, "fvw_shankview_nidq" );

    settings.beginGroup( settingsName() );
    set.yPix        = settings.value( "yPix", 8 ).toInt();
    set.what        = settings.value( "what", 1 ).toInt();
    set.thresh      = settings.value( "thresh", -75 ).toInt();
    set.inarow      = settings.value( "staylow", 5 ).toInt();
    set.rng[0]      = settings.value( "rngSpk", 100 ).toInt();
    set.rng[1]      = settings.value( "rngAP", 100 ).toInt();
    set.rng[2]      = settings.value( "rngLF", 100 ).toInt();
    settings.endGroup();
}


void FVW_ShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "fvw_shankview_nidq" );

    settings.beginGroup( settingsName() );
    settings.setValue( "yPix", set.yPix );
    settings.setValue( "what", set.what );
    settings.setValue( "thresh", set.thresh );
    settings.setValue( "staylow", set.inarow );
    settings.setValue( "rngSpk", set.rng[0] );
    settings.setValue( "rngAP", set.rng[1] );
    settings.setValue( "rngLF", set.rng[2] );
    settings.endGroup();
}


QString FVW_ShankCtl_Ni::screenStateName() const
{
    return "WinLayout_FVW_ShankView_Nidq/geometry";
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


