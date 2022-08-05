
#include "Util.h"
#include "FVShankCtl_Ni.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* FVShankCtl_Ni -------------------------------------------------- */
/* ---------------------------------------------------------------- */

FVShankCtl_Ni::FVShankCtl_Ni( const DataFile *df, QWidget *parent )
    :   FVShankCtl(df, parent)
{
}


void FVShankCtl_Ni::init( const ShankMap *map )
{
    parInit( map );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString FVShankCtl_Ni::settingsName() const
{
    return "ShankView_Nidq";
}


// Called only from init().
//
void FVShankCtl_Ni::loadSettings()
{
    STDSETTINGS( settings, "fvw_shankview_nidq" );

    settings.beginGroup( settingsName() );
        svTab->loadSettings( settings );
    settings.endGroup();
}


void FVShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "fvw_shankview_nidq" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}


QString FVShankCtl_Ni::screenStateName() const
{
    return "WinLayout_FVW_ShankView_Nidq/geometry";
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


