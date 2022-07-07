
#include "ui_FVW_ShankWindow.h"

#include "Util.h"
#include "FVW_ShankCtl_Im.h"
#include "DataFile.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* FVW_ShankCtl_Im ------------------------------------------------ */
/* ---------------------------------------------------------------- */

FVW_ShankCtl_Im::FVW_ShankCtl_Im( const DataFile *df, QWidget *parent )
    :   FVW_ShankCtl( df, parent )
{
}


void FVW_ShankCtl_Im::init( const ShankMap *map )
{
    maxInt = qMax( df->getParam("imMaxInt").toInt(), 512 ) - 1;
    baseInit( map, df->imro()->nAP() / map->nc );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString FVW_ShankCtl_Im::settingsName() const
{
    return QString("ShankView_Imec_T%1_%2")
            .arg( df->probeType() ).arg( lfp ? "LF" : "AP" );
}


// Called only from init().
//
void FVW_ShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "fvw_shankview_imec" );

    settings.beginGroup( settingsName() );
        set.loadSettings( settings );
    settings.endGroup();

    if( lfp )
        set.what = 2;
}


void FVW_ShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "fvw_shankview_imec" );

    settings.beginGroup( settingsName() );
        set.saveSettings( settings );
    settings.endGroup();
}


QString FVW_ShankCtl_Im::screenStateName() const
{
    return QString("WinLayout_FVW_ShankView_Imec_T%1_%2/geometry")
            .arg( df->probeType() ).arg( lfp ? "LF" : "AP" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


