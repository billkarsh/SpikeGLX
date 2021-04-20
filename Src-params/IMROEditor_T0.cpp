
#include "ui_IMROEditor_T0.h"

#include "IMROTbl_T0base.h"
#include "IMROEditor_T0.h"
#include "Util.h"
#include "CimCfg.h"

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMROEditor_T0::IMROEditor_T0( QObject *parent, int type )
    :   QObject( parent ), Rini(0), Rref(0), Rcur(0), type(type),
        running(false)
{
    Rref = reinterpret_cast<IMROTbl_T0base*>(IMROTbl::alloc( type ));

    loadSettings();

    edDlg = new QDialog;

    edDlg->setWindowFlags( edDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    edUI = new Ui::IMROEditor_T0;
    edUI->setupUi( edDlg );
    ConnectUI( edUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( edUI->blockBut, SIGNAL(clicked()), this, SLOT(blockBut()) );
    ConnectUI( edUI->bankBut, SIGNAL(clicked()), this, SLOT(bankBut()) );
    ConnectUI( edUI->refidBut, SIGNAL(clicked()), this, SLOT(refidBut()) );
    ConnectUI( edUI->apBut, SIGNAL(clicked()), this, SLOT(apBut()) );
    ConnectUI( edUI->lfBut, SIGNAL(clicked()), this, SLOT(lfBut()) );
    ConnectUI( edUI->hipassBut, SIGNAL(clicked()), this, SLOT(hipassBut()) );
    ConnectUI( edUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( edUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    edUI->prbLbl->setText( QString::number( type ) );

    edUI->bankSB->setMinimum( 0 );
    edUI->bankSB->setMaximum( Rref->nBanks() - 1 );
    edUI->bankSB->setValue( 0 );

    fillRefidCB();
}


IMROEditor_T0::~IMROEditor_T0()
{
    saveSettings();

    if( edUI ) {
        delete edUI;
        edUI = 0;
    }

    if( edDlg ) {
        delete edDlg;
        edDlg = 0;
    }

    if( Rini ) {
        delete Rini;
        Rini = 0;
    }

    if( Rref ) {
        delete Rref;
        Rref = 0;
    }

    if( Rcur ) {
        delete Rcur;
        Rcur = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Return true if changed.
//
bool IMROEditor_T0::Edit( QString &outFile, const QString &file, int selectRow )
{
    iniFile = file;

    if( iniFile.contains( "*" ) )
        iniFile.clear();

    if( iniFile.isEmpty() )
        defaultBut();
    else
        loadFile( iniFile );

    if( !Rini )
        Rini = reinterpret_cast<IMROTbl_T0base*>(IMROTbl::alloc( type ));

    Rini->copyFrom( Rref );

// Optionally select gain on given row

    if( selectRow >= 0 && edUI->tableWidget->rowCount() ) {

        int nAP = Rcur->nAP(),
            row = selectRow,
            col = 2;    // APgain;

        if( row >= nAP ) {

            row -= nAP;
            col  = 3;   // LFgain;
        }

        QTableWidgetItem    *ti = edUI->tableWidget->item( row, col );

        if( ti ) {
            edUI->tableWidget->setFocus();
            edUI->tableWidget->setCurrentItem( ti );
            edUI->tableWidget->editItem( ti );
        }

        running = true;
    }

// Run the dialog

    int ret = edDlg->exec();

    outFile = refFile;

    return ret == (QDialog::Accepted && Rcur != Rini);
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROEditor_T0::defaultBut()
{
    createRcur();
    Rcur->fillDefault();

    copyRcur2ref();
    refFile.clear();

    Rcur2table();

    edUI->statusLbl->setText( "Default table set" );
}


void IMROEditor_T0::blockBut()
{
    setAllBlock( edUI->rowSB->value() );
}


void IMROEditor_T0::bankBut()
{
    setAllBank( edUI->bankSB->value() );
}


void IMROEditor_T0::refidBut()
{
    setAllRefid( edUI->refidCB->currentIndex() );
}


void IMROEditor_T0::apBut()
{
    setAllAPgain( edUI->apCB->currentText().toInt() );
}


void IMROEditor_T0::lfBut()
{
    setAllLFgain( edUI->lfCB->currentText().toInt() );
}


void IMROEditor_T0::hipassBut()
{
    setAllAPfilt( edUI->hipassCB->currentIndex() );
}


void IMROEditor_T0::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    edDlg,
                    "Load IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)" );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void IMROEditor_T0::saveBut()
{
    if( !table2Rcur() )
        return;

    QFileDialog::Options    options = 0;

    if( running )
        options = QFileDialog::DontConfirmOverwrite;

    QString fn = QFileDialog::getSaveFileName(
                    edDlg,
                    "Save IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)",
                    0,
                    options );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();

        QString msg;
        bool    ok = Rcur->saveFile( msg, fn );

        edUI->statusLbl->setText( msg );

        if( ok ) {

            copyRcur2ref();
            refFile = fn;

            if( running )
                okBut();
        }
    }
}


void IMROEditor_T0::okBut()
{
    if( !table2Rcur() )
        return;

    if( *Rcur != *Rref ) {
        edUI->statusLbl->setText( "Changed table is not saved" );
        return;
    }

    edDlg->accept();
}


void IMROEditor_T0::cancelBut()
{
    refFile = iniFile;
    edDlg->reject();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void IMROEditor_T0::fillRefidCB()
{
    QComboBox   *CB = edUI->refidCB;
    int         rch = IMROTbl_T0base::imType0baseChan / 2;

    CB->clear();
    CB->addItem( "0 = Ext" );
    CB->addItem( "1 = Tip" );

    for( int i = 2, n = Rref->nRefs(); i < n; ++i ) {

        CB->addItem( QString("%1 = %2")
            .arg( i )
            .arg( rch + (i - 2) * IMROTbl_T0base::imType0baseChan ) );
    }

    CB->setCurrentIndex( 0 );
}


void IMROEditor_T0::createRcur()
{
    if( Rcur )
        delete Rcur;

    Rcur = reinterpret_cast<IMROTbl_T0base*>(IMROTbl::alloc( type ));
}


void IMROEditor_T0::copyRcur2ref()
{
    if( !Rref )
        Rref = reinterpret_cast<IMROTbl_T0base*>(IMROTbl::alloc( type ));

    Rref->copyFrom( Rcur );
}


// Called only from ctor.
//
void IMROEditor_T0::loadSettings()
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void IMROEditor_T0::saveSettings() const
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    settings.setValue( "lastDlgDir", lastDir );
}


void IMROEditor_T0::emptyTable()
{
    edUI->tableWidget->setRowCount( 0 );
}


void IMROEditor_T0::Rcur2table()
{
    QTableWidget    *T = edUI->tableWidget;
    int             nr = Rcur->nChan();

    T->setRowCount( nr );

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem    *ti;

        // ---------
        // row label
        // ---------

        if( !(ti = T->verticalHeaderItem( i )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( i, ti );
        }

        ti->setText( QString::number( i ) );

        // ----
        // Bank
        // ----

        if( !(ti = T->item( i, 0 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 0, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( Rcur->bank( i ) ) );

        // -----
        // Refid
        // -----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( Rcur->refid( i ) ) );

        // ------
        // APgain
        // ------

        if( !(ti = T->item( i, 2 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 2, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( Rcur->apGain( i ) ) );

        // ------
        // LFgain
        // ------

        if( !(ti = T->item( i, 3 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 3, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( Rcur->lfGain( i ) ) );

        // -----
        // APflt
        // -----

        if( !(ti = T->item( i, 4 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 4, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable | Qt::ItemIsUserCheckable );
        }

        ti->setCheckState( Rcur->apFlt( i ) ? Qt::Checked : Qt::Unchecked );
    }
}


bool IMROEditor_T0::table2Rcur()
{
    if( !edUI->tableWidget->rowCount() ) {
        edUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int nr = Rcur->nChan();

    for( int i = 0; i < nr; ++i ) {

        IMRODesc_T0base     &E  = Rcur->e[i];
        QTableWidgetItem    *ti;
        int                 val;
        bool                ok;

        // ----
        // Bank
        // ----

        ti  = edUI->tableWidget->item( i, 0 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val > bankMax( i ) ) {
                edUI->statusLbl->setText(
                    QString("Bank value (%1) [row %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( bankMax( i ) ) );
                return false;
            }

            E.bank = qint16(val);
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad bank value on row (%1)").arg( i ) );
            return false;
        }

        // -----
        // Refid
        // -----

        ti  = edUI->tableWidget->item( i, 1 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val > refidMax() ) {
                edUI->statusLbl->setText(
                    QString("Refid value (%1) [row %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( refidMax() ) );
                return false;
            }

            E.refid = qint8(val);
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad refid value on row (%1)").arg( i ) );
            return false;
        }

        // ------
        // APgain
        // ------

        ti  = edUI->tableWidget->item( i, 2 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( !gainOK( val ) ) {
                edUI->statusLbl->setText(
                    QString("APgain value (%1) [row %2] illegal.")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            E.apgn = qint16(val);
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad APgain value on row (%1)").arg( i ) );
            return false;
        }

        // ------
        // LFgain
        // ------

        ti  = edUI->tableWidget->item( i, 3 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( !gainOK( val ) ) {
                edUI->statusLbl->setText(
                    QString("LFgain value (%1) [row %2] illegal.")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            E.lfgn = qint16(val);
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad LFgain value on row (%1)").arg( i ) );
            return false;
        }

        // -----
        // APflt
        // -----

        ti  = edUI->tableWidget->item( i, 4 );

        E.apflt = (ti->checkState() == Qt::Checked);
    }

    return true;
}


int IMROEditor_T0::bankMax( int ic )
{
    return (Rref->nElec() - ic - 1) / IMROTbl_T0base::imType0baseChan;
}


int IMROEditor_T0::refidMax()
{
    return Rref->nRefs() - 1;
}


bool IMROEditor_T0::gainOK( int val )
{
    switch( val ) {
        case 50:
            return true;
        case 125:
            return true;
        case 250:
            return true;
        case 500:
            return true;
        case 1000:
            return true;
        case 1500:
            return true;
        case 2000:
            return true;
        case 3000:
            return true;
        default:
            break;
    }

    return false;
}


// Select a contiguous block of electrodes centered
// on given row index (val).
//
void IMROEditor_T0::setAllBlock( int val )
{
    int nC = Rcur->nChan(),
        ne = Rcur->nElec(),
        eLo, eHi;

// Bound electrode range to [1..ne]

    eHi = 2 * val + nC / 2;

    if( eHi >= ne ) {

        eHi = ne;
        eLo = 1 + ne - nC;
    }
    else {

        eLo = 2 * val - nC / 2;

        if( eLo <= 1 ) {

            eLo = 1;
            eHi = nC;
        }
        else
            eHi = eLo + 1 + nC;
    }

// For each electrode, try each bank index.
// Only one bank will make a valid channel.
// elec = (chan+1) + bank*nC

    for( int ie = eLo; ie <= eHi; ++ie ) {

        int ic, ib;

        for( ib = 2; ib >= 0; --ib ) {

            ic = ie - 1 - ib * nC;

            if( ic >= 0 && ic < nC ) {
                Rcur->e[ic].bank = qint16(ib);
                break;
            }
        }
    }

    Rcur2table();

    edUI->statusLbl->setText(
        "Suggested: Order graphs using ChanMap/Auto-arrange/by-shank." );
}


void IMROEditor_T0::setAllBank( int val )
{
    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic )
        Rcur->e[ic].bank = qint16(qMin( val, bankMax( ic ) ));

    Rcur2table();

    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic ) {

        if( Rcur->e[ic].bank != val ) {

            QMessageBox::information(
                edDlg,
                "Bank Out of Range",
                QString(
                "Some channels are not set to %1...\n"
                "but to the highest legal bank instead.")
                .arg( val ) );
            break;
        }
    }
}


void IMROEditor_T0::setAllRefid( int val )
{
    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic )
        Rcur->e[ic].refid = qint8(val);

    Rcur2table();
}


void IMROEditor_T0::setAllAPgain( int val )
{
    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic )
        Rcur->e[ic].apgn = qint16(val);

    Rcur2table();
}


void IMROEditor_T0::setAllLFgain( int val )
{
    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic )
        Rcur->e[ic].lfgn = qint16(val);

    Rcur2table();
}


void IMROEditor_T0::setAllAPfilt( int val )
{
    for( int ic = 0, nC = Rcur->nChan(); ic < nC; ++ic )
        Rcur->e[ic].apflt = qint8(val);

    Rcur2table();
}


void IMROEditor_T0::loadFile( const QString &file )
{
    emptyTable();

    createRcur();

    QString msg;
    bool    ok = Rcur->loadFile( msg, file );

    edUI->statusLbl->setText( msg );

    if( ok ) {
        copyRcur2ref();
        refFile = file;
        Rcur2table();
    }
    else {
        defaultBut();
        edUI->statusLbl->setText(
            QString("Setting defaults: %1.").arg( msg ) );
    }
}


