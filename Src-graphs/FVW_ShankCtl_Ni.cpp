
#include "Util.h"
#include "FVW_ShankCtl_Ni.h"

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
    baseInit( map );
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
        svTab->loadSettings( settings );
    settings.endGroup();
}


void FVW_ShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "fvw_shankview_nidq" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}


QString FVW_ShankCtl_Ni::screenStateName() const
{
    return "WinLayout_FVW_ShankView_Nidq/geometry";
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


