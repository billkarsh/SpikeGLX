
#include "ui_ShankWindow.h"

#include "SVShankCtl_Im.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"

#include <QSettings>


/* ---------------------------------------------------------------- */
/* SVShankCtl_Im -------------------------------------------------- */
/* ---------------------------------------------------------------- */

SVShankCtl_Im::SVShankCtl_Im(
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel,
    QWidget             *parent )
    :   SVShankCtl(p, jpanel, parent), ip(ip)
{
}


void SVShankCtl_Im::init()
{
    parInit( jsIM, ip );

    if( seTab )
        ConnectUI( seTab, SIGNAL(imroChanged(QString)), this, SLOT(imroChanged(QString)) );

    scUI->statusLbl->setToolTip(
        "Use shift-key or right-clicks to see/select LF chans" );

    setWindowTitle( QString("Imec%1 Shank Activity").arg( ip ) );
}


void SVShankCtl_Im::mapChanged()
{
    view()->setShankMap( &p.im.prbj[ip].sns.shankMap );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankCtl_Im::cursorOver( int ic, bool shift )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( ic < 0 ) {
        setStatus( QString() );
        return;
    }

    int r = view()->getSmap()->e[ic].r;

    if( shift && E.roTbl->nLF() )
        ic += E.imCumTypCnt[CimCfg::imSumAP];

    setStatus(
        QString("row %1 %2")
        .arg( r, 3, 10, QChar('0') )
        .arg( E.sns.chanMap.name( ic, p.trig_isChan( jsIM, ip, ic ) ) ) );
}


void SVShankCtl_Im::lbutClicked( int ic, bool shift )
{
    if( shift ) {

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        if( !E.roTbl->nLF() )
            shift = false;
    }

    cursorOver( ic, shift );
    emit selChanged( ic, shift );
}


void SVShankCtl_Im::imroChanged( QString newName )
{
    Run *run = mainApp()->getRun();

    if( !run->dfIsSaving() ) {

        const CimCfg::ImProbeTable  &T = mainApp()->cfgCtl()->prbTab;
        const CimCfg::ImProbeDat    &P = T.get_iProbe( ip );

        if( !T.prbf.isSimProbe( P.slot, P.port, P.dock ) ) {

            run->grfHardPause( true );
            run->grfWaitPaused();
            mainApp()->cfgCtl()->graphSetsImroFile( newName, ip );
            run->grfHardPause( false );
            run->imecUpdate( ip );
        }
        else
            mainApp()->cfgCtl()->graphSetsImroFile( newName, ip );

        run->grfUpdateProbe( ip, true, true );
    }
    else
        seTab->beep( "Recording in progress!" );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

QString SVShankCtl_Im::settingsName() const
{
    return QString("ShankView_Imec_Panel%1").arg( jpanel );
}


// Called only from init().
//
void SVShankCtl_Im::loadSettings()
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->loadSettings( settings );
    settings.endGroup();
}


void SVShankCtl_Im::saveSettings() const
{
    STDSETTINGS( settings, "shankview_imec" );

    settings.beginGroup( settingsName() );
        svTab->saveSettings( settings );
    settings.endGroup();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


