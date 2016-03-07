
#include "ui_IMROEditor.h"

#include "IMROEditor.h"
#include "Util.h"
#include "CimCfg.h"

#include <QDialog>
#include <QFileDialog>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

IMROEditor::IMROEditor( QObject *parent, int probe )
    :   QObject( parent ), R0(0), R(0), probe(probe)
{
    loadSettings();

    edDlg   = new QDialog;
    edUI    = new Ui::IMROEditor;
    edUI->setupUi( edDlg );
    ConnectUI( edUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( edUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( edUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( edUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    edUI->prbLbl->setText( QString::number( probe ) );
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


QString IMROEditor::Edit( const QString &file )
{
    inFile = file;

    if( inFile.contains( "*" ) )
        inFile.clear();

    if( inFile.isEmpty() )
        defaultBut();
    else
        loadFile( inFile );

    edDlg->exec();

    return R0File;
}


void IMROEditor::defaultBut()
{
    createR();
    R->fillDefault( 0, probe );

    copyR2R0();
    R0File.clear();

    edUI->tblLbl->setText( QString::number( probe ) );

    R2Table();

    edUI->statusLbl->setText( "Default table set" );
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
    if( !Table2R() )
        return;

    QString fn = QFileDialog::getSaveFileName(
                    edDlg,
                    "Save IMEC readout table",
                    lastDir,
                    "Imro files (*.imro)" );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();

        QString msg;
        bool    ok = R->saveFile( msg, fn );

        edUI->statusLbl->setText( msg );

        if( ok ) {
            copyR2R0();
            R0File = fn;
        }
    }
}


void IMROEditor::okBut()
{
    if( !Table2R() )
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
    int             nr = R->e.size();
    Qt::ItemFlags   bankFlags = Qt::ItemIsEnabled;

    if( probe >= 3 )
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


bool IMROEditor::Table2R()
{
    if( !edUI->tableWidget->rowCount() ) {
        edUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int nr = R->e.size();

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
                QString("Bad bank value on row (%1)")
                .arg( i ) );
            return false;
        }

        // -----
        // Refid
        // -----

        ti  = edUI->tableWidget->item( i, 1 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val > refMax() ) {
                edUI->statusLbl->setText(
                    QString("Refid value (%1) [row %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( refMax() ) );
                return false;
            }

            E.refid = val;
        }
        else {
            edUI->statusLbl->setText(
                QString("Bad refid value on row (%1)")
                .arg( i ) );
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
                QString("Bad APgain value on row (%1)")
                .arg( i ) );
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
                QString("Bad LFgain value on row (%1)")
                .arg( i ) );
            return false;
        }
    }

    return true;
}


int IMROEditor::bankMax( int ic )
{
    if( probe == 4 )
        return (IMROTbl::imOpt4Elec - ic - 1) / 276;
    else if( probe == 3 )
        return (IMROTbl::imOpt3Elec - ic - 1) / 384;
    else
        return 0;
}


int IMROEditor::refMax()
{
    if( probe == 4 )
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
    for( int ic = 0, nC = R->e.size(); ic < nC; ++ic )
        R->e[ic].bank = val;

    R2Table();
}


void IMROEditor::setAllRefid( int val )
{
    for( int ic = 0, nC = R->e.size(); ic < nC; ++ic )
        R->e[ic].refid = val;

    R2Table();
}


void IMROEditor::setAllAPgain( int val )
{
    for( int ic = 0, nC = R->e.size(); ic < nC; ++ic )
        R->e[ic].apgn = val;

    R2Table();
}


void IMROEditor::setAllLFgain( int val )
{
    for( int ic = 0, nC = R->e.size(); ic < nC; ++ic )
        R->e[ic].lfgn = val;

    R2Table();
}


void IMROEditor::adjustOption()
{
    if( probe >= 3 )
        return;

    if( R->opt < 3 )
        return;

    edUI->statusLbl->setText( "Forcing all banks to zero." );

    setAllBank( 0 );
}


void IMROEditor::loadFile( const QString &file )
{
    edUI->tblLbl->clear();
    emptyTable();

    createR();

    QString msg;
    bool    ok = R->loadFile( msg, file );

    edUI->statusLbl->setText( msg );

    if( ok ) {

        edUI->tblLbl->setText( QString::number( R->opt ) );

        if( (R->opt == 4 && probe == 4)
            || (R->opt < 4 && probe < 4) ) {

            copyR2R0();
            R0File = file;

            R2Table();
            adjustOption();
            R->opt = probe;
        }
        else {
            edUI->statusLbl->setText(
                QString("Can't use option %1 file for this probe.")
                .arg( R->opt ) );
        }
    }
}


