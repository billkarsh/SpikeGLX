
#include "ui_ShankWindow.h"

#include "SVShankCtl_Ni.h"
#include "Util.h"
#include "DAQ.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* SVShankCtl_Ni -------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVShankCtl_Ni::SVShankCtl_Ni(
    const DAQ::Params   &p,
    int                 jpanel,
    QWidget             *parent )
    :   SVShankCtl(p, jpanel, parent)
{
}


void SVShankCtl_Ni::init()
{
    parInit( jsNI, 0 );

    setWindowTitle( "Nidq Shank Activity" );
}


void SVShankCtl_Ni::mapChanged()
{
    const ShankMap *S = &p.ni.sns.shankMap;
    svTab->mapChanged( S );
    view()->setShankMap( S );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankCtl_Ni::cursorOver( int ic, bool shift )
{
    Q_UNUSED( shift )

    if( ic < 0 ) {
        setStatus( QString() );
        return;
    }

    int r = view()->getSmap()->e[ic].r;

    setStatus(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( p.ni.sns.chanMap.name( ic, p.trig_isChan( jsNI, 0, ic ) ) ) );
}


void SVShankCtl_Ni::lbutClicked( int ic, bool shift )
{
    Q_UNUSED( shift )
    cursorOver( ic, false );
    emit selChanged( ic, false );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString SVShankCtl_Ni::settingsName() const
{
    return "ShankView_Nidq";
}


// Called only from init().
//
void SVShankCtl_Ni::loadSettings()
{
    STDSETTINGS( settings, "shankview_nidq" );

    settings.beginGroup( settingsName() );
        svTab->loadSettings( settings );
    settings.endGroup();
}


void SVShankCtl_Ni::saveSettings() const
{
    STDSETTINGS( settings, "shankview_nidq" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}


QString SVShankCtl_Ni::screenStateName() const
{
    return QString("WinLayout_GRF_%1_ShankView_Nidq/geometry").arg( jpanel );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


