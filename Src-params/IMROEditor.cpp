
#include "ui_IMROEditor.h"

#include "IMROEditor.h"
#include "Util.h"
#include "CimCfg.h"

#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMROEditor::IMROEditor( QObject *parent, int type )
    :   QObject( parent ), R0(0), R(0), type(type),
        running(false)
{
    loadSettings();

    edDlg = new QDialog;

    edDlg->setWindowFlags( edDlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    edUI = new Ui::IMROEditor;
    edUI->setupUi( edDlg );
    ConnectUI( edUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( edUI->bankBut, SIGNAL(clicked()), this, SLOT(bankBut()) );
    ConnectUI( edUI->refidBut, SIGNAL(clicked()), this, SLOT(refidBut()) );
    ConnectUI( edUI->apBut, SIGNAL(clicked()), this, SLOT(apBut()) );
    ConnectUI( edUI->lfBut, SIGNAL(clicked()), this, SLOT(lfBut()) );
    ConnectUI( edUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( edUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    edUI->prbLbl->setText( QString::number( type ) );

    edUI->bankSB->setMinimum( 0 );
    edUI->bankSB->setValue( 0 );
    edUI->refidSB->setMinimum( 0 );
    edUI->refidSB->setValue( 0 );

    if( type == 4 ) {

        edUI->bankSB->setMaximum( IMROTbl::imOpt4Banks - 1 );
        edUI->refidSB->setMaximum( IMROTbl::imOpt4Refs - 1 );
    }
    else if( type == 3 ) {

        edUI->bankSB->setMaximum( IMROTbl::imOpt3Banks - 1 );
        edUI->refidSB->setMaximum( IMROTbl::imOpt3Refs - 1 );
    }
    else {

        edUI->bankSB->setMaximum( IMROTbl::imOpt2Banks - 1 );
        edUI->bankSB->setDisabled( true );
        edUI->bankBut->setDisabled( true );
        edUI->refidSB->setMaximum( IMROTbl::imOpt2Refs - 1 );
    }
}


IMROEditor::~IMROEditor()
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

    if( R0 ) {
        delete R0;
        R0 = 0;
    }

    if( R ) {
        delete R;
        R = 0;
    }
}


// Return true if changed.
//
bool IMROEditor::Edit( QString &outFile, const QString &file, int selectRow )
{
    inFile = file;

    if( inFile.contains( "*" ) )
        inFile.clear();

    if( inFile.isEmpty() )
        defaultBut();
    else
        loadFile( inFile );

// Optionally select gain on given row

    if( selectRow >= 0 && edUI->tableWidget->rowCount() ) {

        int row = selectRow,
            col = 2;    // APgain;

        if( R->type == 4 ) {

            if( row >= IMROTbl::imOpt4Chan ) {

                row -= IMROTbl::imOpt4Chan;
                col  = 3;
            }
        }
        else if( row >= IMROTbl::imOpt3Chan ) {

            row -= IMROTbl::imOpt3Chan;
            col  = 3;
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

    outFile = R0File;

    return ret == QDialog::Accepted;
}


void IMROEditor::defaultBut()
{
    createR();
    R->fillDefault( type );

    copyR2R0();
    R0File.clear();

    R2Table();

    edUI->statusLbl->setText( "Default table set" );
}


void IMROEditor::bankBut()
{
    setAllBank( edUI->bankSB->value() );
}


void IMROEditor::refidBut()
{
    setAllRefid( edUI->refidSB->value() );
}


void IMROEditor::apBut()
{
    setAllAPgain( edUI->apCB->currentText().toInt() );
}


void IMROEditor::lfBut()
{
    setAllLFgain( edUI->lfCB->currentText().toInt() );
}


void IMROEditor::loadBut()
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


void IMROEditor::saveBut()
{
    if( !table2R() )
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
        bool    ok = R->saveFile( msg, fn );

        edUI->statusLbl->setText( msg );

        if( ok ) {

            copyR2R0();
            R0File = fn;

            if( running )
                okBut();
        }
    }
}


void IMROEditor::okBut()
{
    if( !table2R() )
        return;

    if( *R != *R0 ) {
        edUI->statusLbl->setText( "Changed table is not saved" );
        return;
    }

    edDlg->accept();
}


void IMROEditor::cancelBut()
{
    R0File = inFile;
    edDlg->reject();
}


void IMROEditor::createR()
{
    if( R )
        delete R;

    R = new IMROTbl;
}


void IMROEditor::copyR2R0()
{
    if( R0 )
        delete R0;

    R0 = new IMROTbl( *R );
}


// Called only from ctor.
//
void IMROEditor::loadSettings()
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void IMROEditor::saveSettings() const
{
    STDSETTINGS( settings, "imroedit" );
    settings.beginGroup( "IMROEditor" );

    settings.setValue( "lastDlgDir", lastDir );
}


void IMROEditor::emptyTable()
{
    edUI->tableWidget->setRowCount( 0 );
}


void IMROEditor::R2Table()
{
    QTableWidget    *T = edUI->tableWidget;
    int             nr = R->nChan();
    Qt::ItemFlags   bankFlags = Qt::ItemIsEnabled;

// MS: Revise features lookup by probe type (everywhere)
    if( type >= 3 )
        bankFlags |= Qt::ItemIsEditable;

    T->setRowCount( nr );

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem    *ti;
        const IMRODesc      &E = R->e[i];

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
            ti->setFlags( bankFlags );
        }

        ti->setText( QString::number( E.bank ) );

        // -----
        // Refid
        // -----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.refid ) );

        // ------
        // APgain
        // ------

        if( !(ti = T->item( i, 2 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 2, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.apgn ) );

        // ------
        // LFgain
        // ------

        if( !(ti = T->item( i, 3 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 3, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.lfgn ) );
    }
}


bool IMROEditor::table2R()
{
    if( !edUI->tableWidget->rowCount() ) {
        edUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int nr = R->nChan();

    for( int i = 0; i < nr; ++i ) {

        IMRODesc            &E  = R->e[i];
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

            E.bank = val;
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

            E.refid = val;
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

            E.apgn = val;
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

            E.lfgn = val;
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad LFgain value on row (%1)").arg( i ) );
            return false;
        }
    }

    return true;
}


int IMROEditor::bankMax( int ic )
{
    if( type == 4 )
        return (IMROTbl::imOpt4Elec - ic - 1) / IMROTbl::imOpt4Chan;
    else if( type == 3 )
        return (IMROTbl::imOpt3Elec - ic - 1) / IMROTbl::imOpt3Chan;
    else
        return 0;
}


int IMROEditor::refidMax()
{
    if( type == 4 )
        return IMROTbl::imOpt4Refs - 1;
    else
        return IMROTbl::imOpt3Refs - 1;
}


bool IMROEditor::gainOK( int val )
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
        case 2500:
            return true;
        default:
            break;
    }

    return false;
}


void IMROEditor::setAllBank( int val )
{
    for( int ic = 0, nC = R->nChan(); ic < nC; ++ic )
        R->e[ic].bank = qMin( val, bankMax( ic ) );

    R2Table();

    for( int ic = 0, nC = R->nChan(); ic < nC; ++ic ) {

        if( R->e[ic].bank != val ) {

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


void IMROEditor::setAllRefid( int val )
{
    for( int ic = 0, nC = R->nChan(); ic < nC; ++ic )
        R->e[ic].refid = val;

    R2Table();
}


void IMROEditor::setAllAPgain( int val )
{
    for( int ic = 0, nC = R->nChan(); ic < nC; ++ic )
        R->e[ic].apgn = val;

    R2Table();
}


void IMROEditor::setAllLFgain( int val )
{
    for( int ic = 0, nC = R->nChan(); ic < nC; ++ic )
        R->e[ic].lfgn = val;

    R2Table();
}


void IMROEditor::adjustType()
{
    if( type >= 3 )
        return;

    if( R->type < 3 )
        return;

    edUI->statusLbl->setText( "Forcing all banks to zero." );

    setAllBank( 0 );
}


void IMROEditor::loadFile( const QString &file )
{
    emptyTable();

    createR();

    QString msg;
    bool    ok = R->loadFile( msg, file );

    edUI->statusLbl->setText( msg );

    if( ok ) {

        if( (R->type == 4 && type == 4)
            || (R->type < 4 && type < 4) ) {

            copyR2R0();
            R0File = file;

            R2Table();
            adjustType();
            R->type = type;
        }
        else {
            edUI->statusLbl->setText(
                QString("Can't use type %1 file for this probe.")
                .arg( R->type ) );
        }
    }
}


