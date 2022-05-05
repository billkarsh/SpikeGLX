
#include "ui_TrigTab.h"
#include "ui_TrigImmedPanel.h"
#include "ui_TrigTimedPanel.h"
#include "ui_TrigTTLPanel.h"
#include "ui_TrigSpikePanel.h"
#include "ui_TrigTCPPanel.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Config_trigtab.h"

#include <QButtonGroup>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_trigtab::Config_trigtab( QWidget *tab )
    : trigTabUI(0),
        trigImmPanelUI(0), trigTimPanelUI(0),
        trigTTLPanelUI(0), trigSpkPanelUI(0),
        trigTCPPanelUI(0)
{
    QVBoxLayout *L;
    QWidget     *panel;

    trigTabUI = new Ui::TrigTab;
    trigTabUI->setupUi( tab );
    ConnectUI( trigTabUI->trigModeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigModeChanged()) );

    L = new QVBoxLayout( trigTabUI->trigFrame );

// Immediate
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigImmed ) );
    trigImmPanelUI = new Ui::TrigImmedPanel;
    trigImmPanelUI->setupUi( panel );
    L->addWidget( panel );

// Timed
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTimed ) );
    trigTimPanelUI = new Ui::TrigTimedPanel;
    trigTimPanelUI->setupUi( panel );
    ConnectUI( trigTimPanelUI->HInfRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->cyclesRadio, SIGNAL(clicked()), this, SLOT(trigTimHInfClicked()) );
    ConnectUI( trigTimPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTimNInfClicked(bool)) );

    QButtonGroup    *bgTim = new QButtonGroup( panel );
    bgTim->addButton( trigTimPanelUI->HInfRadio );
    bgTim->addButton( trigTimPanelUI->cyclesRadio );
    L->addWidget( panel );

// TTL
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTTL ) );
    trigTTLPanelUI = new Ui::TrigTTLPanel;
    trigTTLPanelUI->setupUi( panel );
    ConnectUI( trigTTLPanelUI->analogRadio, SIGNAL(clicked()), this, SLOT(trigTTLAnalogChanged()) );
    ConnectUI( trigTTLPanelUI->digRadio, SIGNAL(clicked()), this, SLOT(trigTTLAnalogChanged()) );
    ConnectUI( trigTTLPanelUI->modeCB, SIGNAL(currentIndexChanged(int)), this, SLOT(trigTTLModeChanged(int)) );
    ConnectUI( trigTTLPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigTTLNInfClicked(bool)) );
    L->addWidget( panel );

// Spike
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigSpike ) );
    trigSpkPanelUI = new Ui::TrigSpikePanel;
    trigSpkPanelUI->setupUi( panel );
    ConnectUI( trigSpkPanelUI->NInfChk, SIGNAL(clicked(bool)), this, SLOT(trigSpkNInfClicked(bool)) );
    L->addWidget( panel );

// TCP
    panel = new QWidget;
    panel->setObjectName( QString("panel_%1").arg( DAQ::eTrigTCP ) );
    trigTCPPanelUI = new Ui::TrigTCPPanel;
    trigTCPPanelUI->setupUi( panel );
    L->addWidget( panel );
}


Config_trigtab::~Config_trigtab()
{
    if( trigTCPPanelUI ) {
        delete trigTCPPanelUI;
        trigTCPPanelUI = 0;
    }

    if( trigSpkPanelUI ) {
        delete trigSpkPanelUI;
        trigSpkPanelUI = 0;
    }

    if( trigTTLPanelUI ) {
        delete trigTTLPanelUI;
        trigTTLPanelUI = 0;
    }

    if( trigTimPanelUI ) {
        delete trigTimPanelUI;
        trigTimPanelUI = 0;
    }

    if( trigImmPanelUI ) {
        delete trigImmPanelUI;
        trigImmPanelUI = 0;
    }

    if( trigTabUI ) {
        delete trigTabUI;
        trigTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_trigtab::toGUI( const DAQ::Params &p )
{
    ConfigCtl   *cfg = mainApp()->cfgCtl();

// ------------
// TrgTimParams
// ------------

    trigTimPanelUI->L0SB->setValue( p.trgTim.tL0 );
    trigTimPanelUI->HSB->setValue( p.trgTim.tH );
    trigTimPanelUI->LSB->setValue( p.trgTim.tL );
    trigTimPanelUI->NSB->setValue( p.trgTim.nH );
    trigTimPanelUI->HInfRadio->setChecked( p.trgTim.isHInf );
    trigTimPanelUI->cyclesRadio->setChecked( !p.trgTim.isHInf );
    trigTimPanelUI->NInfChk->setChecked( p.trgTim.isNInf );

// ------------
// TrgTTLParams
// ------------

    QButtonGroup    *bg;

    bg = new QButtonGroup( this );
    bg->addButton( trigTTLPanelUI->analogRadio );
    bg->addButton( trigTTLPanelUI->digRadio );

    cfg->streamCB_fillConfig( trigTTLPanelUI->aStreamCB );
    p.streamCB_selItem( trigTTLPanelUI->aStreamCB, p.trgTTL.stream, false );
    cfg->streamCB_fillConfig( trigTTLPanelUI->dStreamCB );
    p.streamCB_selItem( trigTTLPanelUI->dStreamCB, p.trgTTL.stream, false );

    trigTTLPanelUI->TSB->setValue( p.trgTTL.T );
    trigTTLPanelUI->marginSB->setValue( p.trgTTL.marginSecs );
    trigTTLPanelUI->refracSB->setValue( p.trgTTL.refractSecs );
    trigTTLPanelUI->HSB->setValue( p.trgTTL.tH );
    trigTTLPanelUI->modeCB->setCurrentIndex( p.trgTTL.mode );
    trigTTLPanelUI->aChanSB->setValue( p.trgTTL.chan );
    trigTTLPanelUI->dBitSB->setValue( p.trgTTL.bit );
    trigTTLPanelUI->inarowSB->setValue( p.trgTTL.inarow );
    trigTTLPanelUI->NSB->setValue( p.trgTTL.nH );
    trigTTLPanelUI->analogRadio->setChecked( p.trgTTL.isAnalog );
    trigTTLPanelUI->digRadio->setChecked( !p.trgTTL.isAnalog );
    trigTTLPanelUI->NInfChk->setChecked( p.trgTTL.isNInf );

// --------------
// TrgSpikeParams
// --------------

    cfg->streamCB_fillConfig( trigSpkPanelUI->streamCB );
    p.streamCB_selItem( trigSpkPanelUI->streamCB, p.trgSpike.stream, false );

    trigSpkPanelUI->TSB->setValue( 1e6 * p.trgSpike.T );
    trigSpkPanelUI->periSB->setValue( p.trgSpike.periEvtSecs );
    trigSpkPanelUI->refracSB->setValue( p.trgSpike.refractSecs );
    trigSpkPanelUI->chanSB->setValue( p.trgSpike.aiChan );
    trigSpkPanelUI->inarowSB->setValue( p.trgSpike.inarow );
    trigSpkPanelUI->NSB->setValue( p.trgSpike.nS );
    trigSpkPanelUI->NInfChk->setChecked( p.trgSpike.isNInf );

// -------
// TrigTab
// -------

    trigTabUI->trigModeCB->setCurrentIndex( int(p.mode.mTrig) );

// --------------------
// Observe dependencies
// --------------------

    trigModeChanged();
    trigTimHInfClicked();
    trigTimNInfClicked( p.trgTim.isNInf );
    trigTTLAnalogChanged();
    trigTTLModeChanged( p.trgTTL.mode );
    trigTTLNInfClicked( p.trgTTL.isNInf );
    trigSpkNInfClicked( p.trgSpike.isNInf );
}


void Config_trigtab::fromGUI( DAQ::Params &q )
{
    q.mode.mTrig    = (DAQ::TrigMode)trigTabUI->trigModeCB->currentIndex();

// ------------
// TrgTimParams
// ------------

    q.trgTim.tL0    = trigTimPanelUI->L0SB->value();
    q.trgTim.tH     = trigTimPanelUI->HSB->value();
    q.trgTim.tL     = trigTimPanelUI->LSB->value();
    q.trgTim.nH     = trigTimPanelUI->NSB->value();
    q.trgTim.isHInf = trigTimPanelUI->HInfRadio->isChecked();
    q.trgTim.isNInf = trigTimPanelUI->NInfChk->isChecked();

// ------------
// TrgTTLParams
// ------------

    q.trgTTL.T              = trigTTLPanelUI->TSB->value();
    q.trgTTL.marginSecs     = trigTTLPanelUI->marginSB->value();
    q.trgTTL.refractSecs    = trigTTLPanelUI->refracSB->value();
    q.trgTTL.tH             = trigTTLPanelUI->HSB->value();
    q.trgTTL.mode           = trigTTLPanelUI->modeCB->currentIndex();
    q.trgTTL.chan           = trigTTLPanelUI->aChanSB->value();
    q.trgTTL.bit            = trigTTLPanelUI->dBitSB->value();
    q.trgTTL.inarow         = trigTTLPanelUI->inarowSB->value();
    q.trgTTL.nH             = trigTTLPanelUI->NSB->value();
    q.trgTTL.isAnalog       = trigTTLPanelUI->analogRadio->isChecked();
    q.trgTTL.isNInf         = trigTTLPanelUI->NInfChk->isChecked();

    if( q.trgTTL.isAnalog )
        q.trgTTL.stream = trigTTLPanelUI->aStreamCB->currentText();
    else
        q.trgTTL.stream = trigTTLPanelUI->dStreamCB->currentText();

// --------------
// TrgSpikeParams
// --------------

    q.trgSpike.T            = trigSpkPanelUI->TSB->value() / 1e6;
    q.trgSpike.periEvtSecs  = trigSpkPanelUI->periSB->value();
    q.trgSpike.refractSecs  = trigSpkPanelUI->refracSB->value();
    q.trgSpike.stream       = trigSpkPanelUI->streamCB->currentText();
    q.trgSpike.aiChan       = trigSpkPanelUI->chanSB->value();
    q.trgSpike.inarow       = trigSpkPanelUI->inarowSB->value();
    q.trgSpike.nS           = trigSpkPanelUI->NSB->value();
    q.trgSpike.isNInf       = trigSpkPanelUI->NInfChk->isChecked();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_trigtab::trigModeChanged()
{
    int     mode    = trigTabUI->trigModeCB->currentIndex();
    QString wName   = QString("panel_%1").arg( mode );

#if QT_VERSION >= 0x050300
    QList<QWidget*> wL =
        trigTabUI->trigFrame->findChildren<QWidget*>(
            QRegExp("panel_*"),
            Qt::FindDirectChildrenOnly );
#else
    QList<QWidget*> wL =
        trigTabUI->trigFrame->findChildren<QWidget*>(
            QRegExp("panel_*") );
#endif

    foreach( QWidget* w, wL ) {

        if( w->objectName() == wName )
            w->show();
        else
            w->hide();
    }
}


void Config_trigtab::trigTimHInfClicked()
{
    trigTimPanelUI->cyclesGB->setEnabled(
        trigTimPanelUI->cyclesRadio->isChecked() );
}


void Config_trigtab::trigTimNInfClicked( bool checked )
{
    trigTimPanelUI->NLabel->setDisabled( checked );
    trigTimPanelUI->NSB->setDisabled( checked );
}


void Config_trigtab::trigTTLAnalogChanged()
{
    bool isAna = trigTTLPanelUI->analogRadio->isChecked();

    trigTTLPanelUI->aStreamCB->setEnabled( isAna );
    trigTTLPanelUI->chanLabel->setEnabled( isAna );
    trigTTLPanelUI->aChanSB->setEnabled( isAna );
    trigTTLPanelUI->TLabel->setEnabled( isAna );
    trigTTLPanelUI->TSB->setEnabled( isAna );
    trigTTLPanelUI->TUnitLabel->setEnabled( isAna );

    trigTTLPanelUI->dStreamCB->setDisabled( isAna );
    trigTTLPanelUI->bitLabel->setDisabled( isAna );
    trigTTLPanelUI->dBitSB->setDisabled( isAna );
    trigTTLPanelUI->gohiLabel->setDisabled( isAna );
}


void Config_trigtab::trigTTLModeChanged( int _mode )
{
    DAQ::TrgTTLMode mode    = DAQ::TrgTTLMode(_mode);
    QString         txt;

    switch( mode ) {
        case DAQ::TrgTTLLatch:
            txt = "writing continues until gate closes.";
            break;
        case DAQ::TrgTTLTimed:
            txt = "write for N seconds";
            break;
        case DAQ::TrgTTLFollowV:
            txt = "write while voltage is high.";
            break;
    }

    trigTTLPanelUI->HLabel->setText( txt );
    trigTTLPanelUI->HSB->setVisible( mode == DAQ::TrgTTLTimed );
    trigTTLPanelUI->repeatGB->setHidden( mode == DAQ::TrgTTLLatch );
}


void Config_trigtab::trigTTLNInfClicked( bool checked )
{
    trigTTLPanelUI->NLabel->setDisabled( checked );
    trigTTLPanelUI->NSB->setDisabled( checked );
}


void Config_trigtab::trigSpkNInfClicked( bool checked )
{
    trigSpkPanelUI->NLabel->setDisabled( checked );
    trigSpkPanelUI->NSB->setDisabled( checked );
}


