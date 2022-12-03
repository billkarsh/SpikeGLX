
#include "ui_ConfigSlotsDialog.h"

#include "ConfigSlotsCtl.h"
#include "Util.h"
#include "SignalBlocker.h"

#include <QDialog>

#define TBL_SLOT    0
#define TBL_ID      1
#define TBL_DET     2
#define TBL_SHOW    3




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ConfigSlotsCtl::ConfigSlotsCtl( QObject *parent, CimCfg::ImProbeTable &prbTab )
    :   QObject(parent), prbTab(prbTab)
{
    csDlg = new QDialog;

    csDlg->setWindowFlags( csDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    csUI = new Ui::ConfigSlotsDialog;
    csUI->setupUi( csDlg );
    ConnectUI( csUI->addBut, SIGNAL(clicked()), this, SLOT(addBut()) );
    ConnectUI( csUI->tableWidget, SIGNAL(itemSelectionChanged()), this, SLOT(selectionChanged()) );
    ConnectUI( csUI->slotCB, SIGNAL(currentIndexChanged(int)), this, SLOT(slotCBChanged(int)) );
    ConnectUI( csUI->showCB, SIGNAL(currentIndexChanged(int)), this, SLOT(showCBChanged(int)) );
    ConnectUI( csUI->removeBut, SIGNAL(clicked()), this, SLOT(removeBut()) );
    ConnectUI( csUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( csUI->detectBut, SIGNAL(clicked()), this, SLOT(detectBut()) );
    ConnectUI( csUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( csUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );
}


ConfigSlotsCtl::~ConfigSlotsCtl()
{
    if( csUI ) {
        delete csUI;
        csUI = 0;
    }

    if( csDlg ) {
        delete csDlg;
        csDlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool ConfigSlotsCtl::run()
{
    prbTab.getCfgSlots( vCS );
    toGUI();

    detectBut();

    selectionChanged();

    return QDialog::Accepted == csDlg->exec();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigSlotsCtl::addBut()
{
// already exists?

    int slot = CimCfg::imSlotSIMMin + csUI->simCB->currentIndex();

    foreach( const CimCfg::CfgSlot &CS, vCS ) {
        if( CS.slot == slot ) {
            csUI->statusLbl->setText(
                QString("Slot %1 already exists").arg( slot ) );
            return;
        }
    }

    vCS.push_back( CimCfg::CfgSlot( slot, 0, 1, true ) );
    qSort( vCS.begin(), vCS.end() );
    csUI->statusLbl->setText( QString("Sim slot %1 added").arg( slot ) );
    toGUI();
}


void ConfigSlotsCtl::selectionChanged()
{
    SignalBlocker   b0(csUI->slotCB);
    SignalBlocker   b1(csUI->showCB);

// get selection

    QTableWidget        *T  = csUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 row = -1;

    if( ti ) {

        row = ti->row();
        csUI->statusLbl->setText( "" );

        if( row < 0 || row >= vCS.size() )
            goto none;

        const CimCfg::CfgSlot   &CS = vCS[row];

        if( CS.ID ) {

            int islot = 0;

            if( CS.slot > CimCfg::imSlotNone )
                islot = CS.slot - CimCfg::imSlotNone;

            csUI->slotCB->setCurrentIndex( islot );
            csUI->slotCB->setEnabled( true );
        }
        else {
            csUI->slotCB->setEditable( true );
            csUI->slotCB->setCurrentText( QString::number( CS.slot ) );
            csUI->slotCB->setEnabled( false );
        }

        csUI->showCB->setCurrentIndex( CS.show );
        csUI->showCB->setEnabled( true );
        csUI->removeBut->setEnabled( true );
    }
    else {
none:
        csUI->slotCB->setCurrentIndex( 0 );
        csUI->slotCB->setEnabled( false );
        csUI->showCB->setCurrentIndex( 0 );
        csUI->showCB->setEnabled( false );
        csUI->removeBut->setEnabled( false );
    }
}


void ConfigSlotsCtl::showCBChanged( int sel )
{
// get selection

    QTableWidget        *T  = csUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 row = -1;

    if( ti ) {
        row = ti->row();
        csUI->statusLbl->setText( "" );
    }
    else {
        csUI->statusLbl->setText( "Select a row in table" );
        return;
    }

// Show

    vCS[row].show = qint16(sel);

    toGUI();
}


void ConfigSlotsCtl::slotCBChanged( int sel )
{
// get selection

    QTableWidget        *T  = csUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 row = -1;

    if( ti ) {
        row = ti->row();
        csUI->statusLbl->setText( "" );
    }
    else {
        csUI->statusLbl->setText( "Select a row in table" );
        return;
    }

// Slot

    CimCfg::CfgSlot &CS = vCS[row];

    if( CS.ID ) {

        if( !sel ) {
            csUI->statusLbl->setText( "Assign a slot for this Onebox" );
            return;
        }

        CS.slot = sel + CimCfg::imSlotNone;
    }

    qSort( vCS.begin(), vCS.end() );
    toGUI();
}


void ConfigSlotsCtl::removeBut()
{
// get selection

    QTableWidget        *T  = csUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 row = -1;

    if( ti ) {
        row = ti->row();
        csUI->statusLbl->setText( "" );
    }
    else {
        csUI->statusLbl->setText( "Select an item in table" );
        return;
    }

// remove

    vCS.erase( vCS.begin() + row );
    toGUI();

// select next

    int nr = T->rowCount();

    if( row < nr )
        T->setCurrentCell( row, TBL_SLOT );
    else if( nr )
        T->setCurrentCell( nr - 1, TBL_SLOT );
}


void ConfigSlotsCtl::helpBut()
{
    showHelp( "Slot_Help" );
}


void ConfigSlotsCtl::detectBut()
{
    if( !fromGUI() )
        return;

    QString msg;
    bool    ok = prbTab.scanCfgSlots( vCS, msg );

    toGUI();

    if( !ok || vCS.size() )
        csUI->statusLbl->setText( msg );
    else {
        csUI->statusLbl->setText(
            "No PXI modules or Oneboxes detected...Click Help" );
    }
}


// On OK, setCfgSlots updates:
// - In-memory probe and slot tables.
// - Config file improbetable.
// - Config file imslottable.
//
void ConfigSlotsCtl::okBut()
{
    if( !fromGUI() )
        return;

    prbTab.setCfgSlots( vCS );

    csDlg->accept();
}


void ConfigSlotsCtl::cancelBut()
{
    csDlg->reject();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ConfigSlotsCtl::toGUI()
{
    QTableWidget    *T = csUI->tableWidget;
    int             nr = vCS.size();

    T->setRowCount( nr );

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem        *ti;
        const CimCfg::CfgSlot   &CS = vCS[i];

        // ---------
        // row label
        // ---------

        if( !(ti = T->verticalHeaderItem( i )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( i, ti );
        }

        ti->setText( QString::number( i ) );

        // ----
        // Slot
        // ----

        if( !(ti = T->item( i, TBL_SLOT )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_SLOT, ti );
            ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        }

        if( CS.slot != CimCfg::imSlotNone )
            ti->setText( QString::number( CS.slot ) );
        else
            ti->setText( "???" );

        // --
        // ID
        // --

        if( !(ti = T->item( i, TBL_ID )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_ID, ti );
            ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        }

        if( CS.ID )
            ti->setText( QString::number( CS.ID ) );
        else
            ti->setText( "N.A." );

        // --------
        // Detected
        // --------

        if( !(ti = T->item( i, TBL_DET )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_DET, ti );
            ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        }

        if( CS.slot >= CimCfg::imSlotSIMMin )
            ti->setText( "SIM" );
        else
            ti->setText( CS.detected ? "Y" : "N" );

        // ----
        // Show
        // ----

        if( !(ti = T->item( i, TBL_SHOW )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_SHOW, ti );
            ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        }

        if( !CS.show )
            ti->setText( "Hide" );
        else
            ti->setText( QString("%1 Dock/port").arg( CS.show ) );
    }
}


bool ConfigSlotsCtl::fromGUI()
{
    QVector<CimCfg::CfgSlot>    xCS;    // temp
    QSet<int>                   seen;
    QTableWidget                *T = csUI->tableWidget;
    int                         nr = T->rowCount();

    csUI->statusLbl->setText( "" );

    for( int i = 0; i < nr; ++i ) {

        CimCfg::CfgSlot     CS;
        QTableWidgetItem    *ti;
        QString             s;
        int                 val;
        bool                ok;

        // ----
        // Slot
        // ----

        ti = csUI->tableWidget->item( i, TBL_SLOT );
        s  = ti->text();

        if( s.contains( "???" ) ) {
none:
            csUI->statusLbl->setText(
                QString("Assign a slot for row %1").arg( i ) );
            return false;
        }
        else {

            val = s.toInt( &ok );

            if( !ok ) {
                csUI->statusLbl->setText(
                    QString("Bad slot value on row (%1)").arg( i ) );
                return false;
            }

            if( val < CimCfg::imSlotMin || val >= CimCfg::imSlotLogLim ) {
                csUI->statusLbl->setText(
                    QString("Slot value (%1) [row %2] out of range [%3..%4]")
                    .arg( val )
                    .arg( i )
                    .arg( CimCfg::imSlotMin )
                    .arg( CimCfg::imSlotLogLim - 1 ) );
                return false;
            }
            else if( val == CimCfg::imSlotNone )
                goto none;

            if( seen.contains( val ) ) {
                csUI->statusLbl->setText(
                    QString("Slot value (%1) [row %2] is not unique")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            CS.slot = val;
            seen.insert( val );
        }

        // --
        // ID
        // --

        ti = csUI->tableWidget->item( i, TBL_ID );
        s  = ti->text();

        if( s.contains( "N.A." ) )
            CS.ID = 0;
        else {

            val = s.toInt( &ok );

            if( ok ) {

                if( CS.slot <= CimCfg::imSlotPXILim )
                    val = 0;

                CS.ID = val;
            }
            else {
                csUI->statusLbl->setText(
                    QString("Bad ID value on row (%1)").arg( i ) );
                return false;
            }
        }

        // --------
        // Detected
        // --------

        if( CS.slot >= CimCfg::imSlotSIMMin )
            CS.detected= true;
        else {
            ti = csUI->tableWidget->item( i, TBL_DET );
            s  = ti->text();
            CS.detected = s.contains( "Y" );
        }

        // ----
        // Show
        // ----

        ti = csUI->tableWidget->item( i, TBL_SHOW );
        s  = ti->text();

        if( s.contains( "Hide" ) )
            CS.show = 0;
        else if( s.contains( "1" ) )
            CS.show = 1;
        else
            CS.show = 2;

        // ---
        // Add
        // ---

        xCS.push_back( CS );
    }

    vCS = xCS;
    return true;
}


