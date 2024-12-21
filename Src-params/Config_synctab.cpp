
#include "ui_SyncTab.h"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Config_synctab.h"


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

Config_synctab::Config_synctab( QWidget *tab )
    :   QObject(0), syncTabUI(0)
{
    syncTabUI = new Ui::SyncTab;
    syncTabUI->setupUi( tab );
    ConnectUI( syncTabUI->sourceCB, SIGNAL(currentIndexChanged(int)), this, SLOT(syncSourceCBChanged()) );
    ConnectUI( syncTabUI->niChanCB, SIGNAL(currentIndexChanged(int)), this, SLOT(syncNiChanTypeCBChanged()) );
    ConnectUI( syncTabUI->calChk, SIGNAL(clicked(bool)), this, SLOT(syncCalChkClicked()) );
}


Config_synctab::~Config_synctab()
{
    if( syncTabUI ) {
        delete syncTabUI;
        syncTabUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_synctab::toGUI( const DAQ::Params &p )
{
    ConfigCtl   *cfg = mainApp()->cfgCtl();

// Source

    // Delete old Imec entries
    while( syncTabUI->sourceCB->count() > DAQ::eSyncSourceIM )
        syncTabUI->sourceCB->removeItem( DAQ::eSyncSourceIM );

    // Add new Imec entries
    int nS = cfg->prbTab.nSelSlots(),
        ns = 0;

    for( int is = 0; is < nS; ++is ) {
        int slot = cfg->prbTab.getEnumSlot( is );
        if( !cfg->prbTab.simprb.isSimSlot( slot ) ) {
            syncTabUI->sourceCB->addItem( QString("Imec slot %1").arg( slot ) );
            ++ns;
        }
    }

    int sel = p.sync.sourceIdx;

    if( sel > DAQ::eSyncSourceNI + ns )
        sel = DAQ::eSyncSourceNI + (ns > 0);

    syncTabUI->sourceCB->setCurrentIndex( sel );
    syncTabUI->sourceSB->setValue( p.sync.sourcePeriod );

// Inputs

    syncTabUI->imSlotSB->setValue( p.sync.imPXIInputSlot );
    syncTabUI->niChanCB->setCurrentIndex( p.sync.niChanType );
    syncTabUI->niChanSB->setValue( p.sync.niChan );
    syncTabUI->niThreshSB->setValue( p.sync.niThresh );

// Calibration

    syncTabUI->minSB->setValue( p.sync.calMins );

// --------------------
// Observe dependencies
// --------------------

    syncSourceCBChanged();
}


void Config_synctab::fromGUI( DAQ::Params &q )
{
    q.sync.sourceIdx        = curSource();
    q.sync.sourcePeriod     = syncTabUI->sourceSB->value();

    q.sync.imPXIInputSlot   = syncTabUI->imSlotSB->value();

    q.sync.niChanType       = syncTabUI->niChanCB->currentIndex();
    q.sync.niChan           = syncTabUI->niChanSB->value();
    q.sync.niThresh         = syncTabUI->niThreshSB->value();

    q.sync.isCalRun         = syncTabUI->calChk->isChecked();
    q.sync.calMins          = syncTabUI->minSB->value();
}


void Config_synctab::resetCalRunMode()
{
    syncTabUI->calChk->setChecked( false );
}


DAQ::SyncSource Config_synctab::curSource() const
{
    return (DAQ::SyncSource)syncTabUI->sourceCB->currentIndex();
}

/* ---------------------------------------------------------------- */
/* Private slots -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void Config_synctab::syncSourceCBChanged()
{
    ConfigCtl   *cfg = mainApp()->cfgCtl();

    DAQ::SyncSource sourceIdx =
        (DAQ::SyncSource)syncTabUI->sourceCB->currentIndex();

    bool    sourceSBEnab    = sourceIdx != DAQ::eSyncSourceNone,
            calChkEnab      = sourceIdx != DAQ::eSyncSourceNone;

    if( sourceIdx == DAQ::eSyncSourceNone ) {
        syncTabUI->sourceLE->setText(
            "We will apply the most recently measured sample rates" );

        syncTabUI->calChk->setChecked( false );
    }
    else if( sourceIdx == DAQ::eSyncSourceExt ) {
        syncTabUI->sourceLE->setText(
            "Connect pulser output to stream inputs specified below" );
    }
    else if( sourceIdx == DAQ::eSyncSourceNI ) {
        if( cfg->usingNI ) {
            syncTabUI->sourceLE->setText(
                CniCfg::isDigitalDev( cfg->niCurDevName() ) ?
                "Connect line0 (pin-65/P0.0) to stream inputs specified below" :
                "Connect Ctr1Out (pin-40/PFI-13) to stream inputs specified below" );
        }
        else {
            syncTabUI->sourceLE->setText( "Error: Nidq not enabled" );

            sourceSBEnab    = false;
            calChkEnab      = false;
            syncTabUI->calChk->setChecked( false );
        }
    }
    else if( cfg->usingIM || cfg->usingOB ) {
        QString sma;
        int     slot = cfg->prbTab.getEnumSlot( sourceIdx - DAQ::eSyncSourceIM );
        if( cfg->prbTab.isSlotPXIType( slot ) )
            sma = "TRIG";
        else
            sma = "SMA 1";
        syncTabUI->sourceLE->setText(
            QString("Connect slot %1 '%2' to stream inputs specified below")
            .arg( slot ).arg( sma ) );
    }
    else {
        syncTabUI->sourceLE->setText( "Error: Imec not enabled" );

        sourceSBEnab    = false;
        calChkEnab      = false;
        syncTabUI->calChk->setChecked( false );
    }

    syncTabUI->imSlotSB->setEnabled(
        cfg->usingIM &&
        (   // source not PXI
            sourceIdx < DAQ::eSyncSourceIM ||
            !cfg->prbTab.isSlotPXIType(
                cfg->prbTab.getEnumSlot(
                    sourceIdx - DAQ::eSyncSourceIM ) )
        ) );

    syncTabUI->sourceSB->setEnabled( sourceSBEnab );
    syncTabUI->calChk->setEnabled( calChkEnab );

    syncNiChanTypeCBChanged();
    syncCalChkClicked();
}


void Config_synctab::syncNiChanTypeCBChanged()
{
    bool enab   = mainApp()->cfgCtl()->usingNI
                    &&
                    ((DAQ::SyncSource)syncTabUI->sourceCB->currentIndex()
                    != DAQ::eSyncSourceNone),
         enabT  = enab && syncTabUI->niChanCB->currentIndex() == 1;

    syncTabUI->niChanCB->setEnabled( enab );
    syncTabUI->niChanSB->setEnabled( enab );
    syncTabUI->niThreshSB->setEnabled( enabT );
}


void Config_synctab::syncCalChkClicked()
{
    syncTabUI->minSB->setEnabled(
        syncTabUI->calChk->isEnabled()
        && syncTabUI->calChk->isChecked() );
}


