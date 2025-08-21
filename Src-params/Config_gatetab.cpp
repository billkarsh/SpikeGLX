
#include "ui_GateTab.h"
#include "ui_GateImmedPanel.h"
#include "ui_GateTCPPanel.h"

#include "Util.h"
#include "Config_gatetab.h"


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_gatetab::Config_gatetab( QWidget *tab )
    :   QObject(0), gateTabUI(0),
        gateImmPanelUI(0), gateTCPPanelUI(0)
{
    QVBoxLayout *L;
    QWidget     *panel;

    gateTabUI = new Ui::GateTab;
    gateTabUI->setupUi( tab );
    ConnectUI( gateTabUI->gateModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(gateModeChanged()) );
    ConnectUI( gateTabUI->manOvShowButChk, SIGNAL(clicked(bool)), this, SLOT(manOvShowBut(bool)) );

    L = new QVBoxLayout( gateTabUI->gateFrame );

// Immediate
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateImmed ) );
    gateImmPanelUI = new Ui::GateImmedPanel;
    gateImmPanelUI->setupUi( panel );
    L->addWidget( panel );

// TCP
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eGateTCP ) );
    gateTCPPanelUI = new Ui::GateTCPPanel;
    gateTCPPanelUI->setupUi( panel );
    L->addWidget( panel );
}


Config_gatetab::~Config_gatetab()
{
    if( gateTCPPanelUI ) {
        delete gateTCPPanelUI;
        gateTCPPanelUI = 0;
    }

    if( gateImmPanelUI ) {
        delete gateImmPanelUI;
        gateImmPanelUI = 0;
    }

    if( gateTabUI ) {
        delete gateTabUI;
        gateTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_gatetab::toGUI( const DAQ::Params &p )
{
    gateTabUI->gateModeCB->setCurrentIndex( int(p.mode.mGate) );
    gateTabUI->manOvShowButChk->setChecked( p.mode.manOvShowBut );
    gateTabUI->manOvInitOffChk->setChecked( p.mode.manOvInitOff );
    gateTabUI->manOvConfirmChk->setChecked( p.mode.manOvConfirm );

// --------------------
// Observe dependencies
// --------------------

    gateModeChanged();
    manOvShowBut( p.mode.manOvShowBut );
}


void Config_gatetab::fromGUI( DAQ::Params &q )
{
    q.mode.mGate        = (DAQ::GateMode)gateTabUI->gateModeCB->currentIndex();
    q.mode.manOvShowBut = gateTabUI->manOvShowButChk->isChecked();
    q.mode.manOvInitOff = gateTabUI->manOvInitOffChk->isChecked();
    q.mode.manOvConfirm = gateTabUI->manOvConfirmChk->isChecked();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_gatetab::gateModeChanged()
{
    int     mode    = gateTabUI->gateModeCB->currentIndex();
    QString wName   = QString("panel_%1").arg( mode );

#if QT_VERSION >= 0x050300
    QList<QWidget*> wL =
        gateTabUI->gateFrame->findChildren<QWidget*>(
            QRegularExpression("panel_*"),
            Qt::FindDirectChildrenOnly );
#else
    QList<QWidget*> wL =
        gateTabUI->gateFrame->findChildren<QWidget*>(
            QRegularExpression("panel_*") );
#endif

    foreach( QWidget* w, wL ) {

        if( w->objectName() == wName )
            w->show();
        else
            w->hide();
    }
}


void Config_gatetab::manOvShowBut( bool checked )
{
    gateTabUI->manOvInitOffChk->setEnabled( checked );
    gateTabUI->manOvConfirmChk->setEnabled( checked );

    if( !checked )
        gateTabUI->manOvInitOffChk->setChecked( false );
}


