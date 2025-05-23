
#include "Util.h"
#include "FVShankCtl_Ni.h"
#include "ShankView.h"

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


void FVShankCtl_Ni::exportHeat( QFile *f, const double *val )
{
    QMap<ShankMapDesc,double>   T;
    const ShankMap              *smap = view()->getSmap();
    QTextStream                 ts( f );

// Full-size probe map

    for( int is = 0; is < smap->ns; ++is ) {
        for( int ir = 0; ir < smap->nr; ++ir ) {
            for( int ic = 0; ic < smap->nc; ++ic )
                T[ShankMapDesc(is,ic,ir,0)] = -1;
        }
    }

// Overwrite active entries

    for( int i = 0, n = smap->e.size(); i < n; ++i ) {
        const ShankMapDesc  &E = smap->e[i];
        if( E.u )
            T[ShankMapDesc(E.s,E.c,E.r,0)] = val[i];
    }

// Write full-size sorted

    QMap<ShankMapDesc,double>::const_iterator
    it  = T.begin(),
    end = T.end();

    ts << "Shank\tCol\tRow\tVal\n";

    for( ; it != end; ++it ) {
        const ShankMapDesc  &E = it.key();
        ts << QString("%1\t%2\t%3\t%4\n")
                .arg( E.s ).arg( E.c ).arg( E.r ).arg( it.value() );
    }
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


