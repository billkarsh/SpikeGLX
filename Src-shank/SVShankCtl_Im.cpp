
#include "ui_ShankWindow.h"

#include "SVShankCtl_Im.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "AOCtl.h"
#include "Run.h"

#include <QAction>
#include <QMessageBox>
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

    initMenu();

    scUI->statusLbl->setToolTip( "Shift-key -> LF; Right-click -> more options" );

    setWindowTitle( QString("Imec%1 Shank Activity").arg( ip ) );
}


void SVShankCtl_Im::mapChanged()
{
    view()->setShankMap( &p.im.prbj[ip].sns.shankMap );
}


void SVShankCtl_Im::setAnatomyPP( const QString &elems, int sk )
{
    svTab->setAnatomyPP( elems, ip, sk );
}


void SVShankCtl_Im::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY )
{
    svTab->colorTraces( theX, vY );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankCtl_Im::cursorOver( int ic, bool shift )
{
    const CimCfg::PrbEach   &E = p.im.prbj[ip];

    if( ic < 0 ) {
        view()->setContextMenuPolicy( Qt::NoContextMenu );
        setStatus( QString() );
        return;
    }

    view()->setContextMenuPolicy( Qt::ActionsContextMenu );

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

        if( !T.simprb.isSimProbe( P.slot, P.port, P.dock ) ) {

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
    else {
        seTab->beep( "Recording in progress!" );
        QMessageBox::warning( 0,
            "Recording in Progress",
            "You can create and save new imro tables at any time.\n"
            "However, you can't apply a change while recording." );
    }
}


void SVShankCtl_Im::setAudioL()
{
    setAudio( -1 );
}


void SVShankCtl_Im::setAudioB()
{
    setAudio( 0 );
}


void SVShankCtl_Im::setAudioR()
{
    setAudio( 1 );
}


void SVShankCtl_Im::setSpike1()
{
    setSpike( 1 );
}


void SVShankCtl_Im::setSpike2()
{
    setSpike( 2 );
}


void SVShankCtl_Im::setSpike3()
{
    setSpike( 3 );
}


void SVShankCtl_Im::setSpike4()
{
    setSpike( 4 );
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

void SVShankCtl_Im::initMenu()
{
    audioLAction = new QAction( "Listen Left Ear", this );
    ConnectUI( audioLAction, SIGNAL(triggered()), this, SLOT(setAudioL()) );

    audioBAction = new QAction( "Listen Both Ears", this );
    ConnectUI( audioBAction, SIGNAL(triggered()), this, SLOT(setAudioB()) );

    audioRAction = new QAction( "Listen Right Ear", this );
    ConnectUI( audioRAction, SIGNAL(triggered()), this, SLOT(setAudioR()) );

    spike1Action = new QAction( "Spike Viewer 1", this );
    ConnectUI( spike1Action, SIGNAL(triggered()), this, SLOT(setSpike1()) );

    spike2Action = new QAction( "Spike Viewer 2", this );
    ConnectUI( spike2Action, SIGNAL(triggered()), this, SLOT(setSpike2()) );

    spike3Action = new QAction( "Spike Viewer 3", this );
    ConnectUI( spike3Action, SIGNAL(triggered()), this, SLOT(setSpike3()) );

    spike4Action = new QAction( "Spike Viewer 4", this );
    ConnectUI( spike4Action, SIGNAL(triggered()), this, SLOT(setSpike4()) );

    QAction *sep0 = new QAction( this );
    sep0->setSeparator( true );

    view()->addAction( audioLAction );
    view()->addAction( audioRAction );
    view()->addAction( audioBAction );
    view()->addAction( sep0 );
    view()->addAction( spike1Action );
    view()->addAction( spike2Action );
    view()->addAction( spike3Action );
    view()->addAction( spike4Action );
    view()->setContextMenuPolicy( Qt::ActionsContextMenu );
}


void SVShankCtl_Im::setAudio( int LBR )
{
    int ic = view()->getSel();

    if( QApplication::keyboardModifiers() & Qt::ShiftModifier ) {

        const CimCfg::PrbEach   &E = p.im.prbj[ip];

        if( E.roTbl->nLF() )
            ic += E.imCumTypCnt[CimCfg::imSumAP];
    }

    mainApp()->getAOCtl()->
        graphSetsChannel( ic, LBR, p.jsip2stream( jsIM, ip ) );
}


void SVShankCtl_Im::setSpike( int gp )
{
    mainApp()->getRun()->grfShowSpikes( gp, ip, view()->getSel() );
}


