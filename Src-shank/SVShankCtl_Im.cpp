
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
    :   SVShankCtl(
            p,
            mainApp()->cfgCtl()->prbTab.get_iProbe( ip ).sr_mask,
            jpanel,
            parent),
        ip(ip)
{
}


void SVShankCtl_Im::init()
{
    parInit( jsIM, ip );

    initMenu();

    if( seTab )
        ConnectUI( seTab, SIGNAL(imroChanged(QString)), this, SLOT(imroChanged(QString)) );

    ConnectUI( this, SIGNAL(rbutReleased(QPoint)), this, SLOT(rbutReleased(QPoint)) );

    scUI->statusLbl->setToolTip( "Shift-key -> LF; Right-click -> more options" );

    setWindowTitle( QString("Imec%1 Shank Activity").arg( ip ) );
}


void SVShankCtl_Im::mapChanged()
{
    const ShankMap *S = &p.im.prbj[ip].sns.shankMap;
    svTab->mapChanged( S );
    view()->setShankMap( S );
}


void SVShankCtl_Im::qf_enable()
{
    svTab->qf_enable();
}


void SVShankCtl_Im::setAnatomyPP( const QString &elems, int sk )
{
    svTab->setAnatomyPP( elems, ip, sk );
}


void SVShankCtl_Im::setEmitters( int blue, int red )
{
    svTab->setEmitters( blue, red );
}


void SVShankCtl_Im::colorTraces( MGraphX *theX, std::vector<MGraphY> &vY )
{
    svTab->colorTraces( theX, vY );
}


QString SVShankCtl_Im::getLbl( int s, int r ) const
{
    return svTab->getLbl( s, r );
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

    const ShankMapDesc  &SMD = view()->getSmap()->e[ic];

    if( shift && E.roTbl->nLF() )
        ic += E.imCumTypCnt[CimCfg::imSumAP];

    setStatus(
        QString("row %1 %2 %3")
        .arg( SMD.r, 3, 10, QChar('0') )
        .arg( E.sns.chanMap.name( ic, p.trig_isChan( jsIM, ip, ic ) ) )
        .arg( svTab->getLbl( SMD.s, SMD.r ) ) );
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


void SVShankCtl_Im::rbutReleased( QPoint globalPos )
{
    m.exec( globalPos );
}


void SVShankCtl_Im::imroChanged( QString newName )
{
    Run *run = mainApp()->getRun();

    if( !run->dfIsSaving() ) {

        run->grfHardPause( true );
        run->grfWaitPaused();

        mainApp()->cfgCtl()->graphSetsImroFile( newName, ip );
        run->grfUpdateRHSFlagsAll();
        run->grfUpdateProbe( ip, true, true );
        run->imecUpdate( ip );

        run->grfHardPause( false );
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


QString SVShankCtl_Im::screenStateName() const
{
    return QString("WinLayout_GRF_%1_ShankView_Imec_T%2/geometry")
            .arg( jpanel ).arg( p.im.prbj[ip].roTbl->pn );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SVShankCtl_Im::initMenu()
{
    QAction *A;

    A = new QAction( "Listen Left Ear", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setAudioL()) );
    m.addAction( A );

    A = new QAction( "Listen Right Ear", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setAudioR()) );
    m.addAction( A );

    A = new QAction( "Listen Both Ears", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setAudioB()) );
    m.addAction( A );

    A = new QAction( this );
    A->setSeparator( true );
    m.addAction( A );

    A = new QAction( "Spike Viewer 1", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setSpike1()) );
    m.addAction( A );

    A = new QAction( "Spike Viewer 2", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setSpike2()) );
    m.addAction( A );

    A = new QAction( "Spike Viewer 3", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setSpike3()) );
    m.addAction( A );

    A = new QAction( "Spike Viewer 4", this );
    ConnectUI( A, SIGNAL(triggered()), this, SLOT(setSpike4()) );
    m.addAction( A );
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


