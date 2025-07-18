
#include "Util.h"
#include "FVShankCtl_Im.h"
#include "DataFile.h"
#include "ShankView.h"

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
    parInit( map );
}


int FVShankCtl_Im::fvw_maxr() const
{
    return svTab->fvw_maxr();
}


void FVShankCtl_Im::setAnatomyPP( const QString &elems, int sk )
{
    svTab->setAnatomyPP( elems, sk );
}


void FVShankCtl_Im::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY )
{
    svTab->colorTraces( theX, vY );
}


QString FVShankCtl_Im::getLbl( int s, int r ) const
{
    return svTab->getLbl( s, r );
}


void FVShankCtl_Im::exportHeat( QFile *f, const double *val )
{
    QMap<ShankMapDesc,double>   T;
    const IMROTbl               *R          = df->imro();
    std::vector<int>            col2vis_ev  = R->col2vis_ev(),
                                col2vis_od  = R->col2vis_od();
    const ShankMap              *smap       = view()->getSmap();
    QTextStream                 ts( f );
    int                         ncvis       = R->nCol_vis(),
                                nchwr       = R->nCol_hwr();

// Full-size probe map

    for( int is = 0; is < smap->ns; ++is ) {
        for( int ir = 0; ir < smap->nr; ++ir ) {
            for( int ic = 0; ic < nchwr; ++ic ) {
                if( ir & 1 )
                    T[ShankMapDesc(is,col2vis_od[ic],ir,0)] = -1;
                else
                    T[ShankMapDesc(is,col2vis_ev[ic],ir,0)] = -1;
            }
        }
    }

// Overwrite active entries

    for( int i = 0, n = smap->e.size(); i < n; ++i ) {
        const ShankMapDesc  &E = smap->e[i];
        if( E.u )
            T[ShankMapDesc(E.s,E.c,E.r,0)] = val[i];
    }

// Write full-size sorted

    std::vector<float>  cv2x_ev,
                        cv2x_od;
    float   x0_ev   = R->x0EvenRow(),
            x0_od   = R->x0OddRow(),
            xpitch  = R->xPitch(),
            zpitch  = R->zPitch();

    QMap<ShankMapDesc,double>::const_iterator
    it  = T.begin(),
    end = T.end();

    cv2x_ev.assign( ncvis, -1 );
    cv2x_od.assign( ncvis, -1 );

    for( int ih = 0; ih < nchwr; ++ih ) {
       cv2x_ev[col2vis_ev[ih]] = x0_ev + ih * xpitch;
       cv2x_od[col2vis_od[ih]] = x0_od + ih * xpitch;
    }

    ts << "Shank\tXum\tZum\tVal\n";

    for( ; it != end; ++it ) {
        const ShankMapDesc  &E = it.key();
        ts << QString("%1\t%2\t%3\t%4\n")
                .arg( E.s )
                .arg( E.r & 1 ? cv2x_od[E.c] : cv2x_ev[E.c] )
                .arg( E.r * zpitch )
                .arg( it.value() );
    }
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
            .arg( df->imro()->pn ).arg( svTab->isLFP() ? "LF" : "AP" );
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
            .arg( df->imro()->pn ).arg( svTab->isLFP() ? "LF" : "AP" );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


