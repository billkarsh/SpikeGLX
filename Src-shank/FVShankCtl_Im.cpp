
#include "Util.h"
#include "FVShankCtl_Im.h"
#include "DataFile.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* FVShankCtl_Im -------------------------------------------------- */
/* ---------------------------------------------------------------- */

FVShankCtl_Im::FVShankCtl_Im( const DataFile *df, QWidget *parent )
    :   FVShankCtl(df, parent)
{
}


void FVShankCtl_Im::init( const ShankMap *map )
{
    parInit( map, df->imro()->nAP() / map->nc );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString FVShankCtl_Im::settingsName() const
{
    return QString("ShankView_Imec_T%1_%2")
            .arg( df->probeType() ).arg( svTab->isLFP() ? "LF" : "AP" );
}


// Called only from init().
//
void FVShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "fvw_shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->loadSettings( settings );
    settings.endGroup();

    if( svTab->isLFP() )
        svTab->setWhat( 2 );
}


void FVShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "fvw_shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}


QString FVShankCtl_Im::screenStateName() const
{
    return QString("WinLayout_FVW_ShankView_Imec_T%1_%2/geometry")
            .arg( df->probeType() ).arg( svTab->isLFP() ? "LF" : "AP" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


