
#include "ui_ShankMapping.h"

#include "ShankMapCtl.h"
#include "Util.h"

#include <QDialog>
#include <QFileDialog>
#include <QSettings>
#include <QSet>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankMapCtl::ShankMapCtl(
    QObject         *parent,
    const IMROTbl   &imro,
    const QString   &type,
    const int       nChan )
    :   QObject( parent ),
        imro(imro), M0(0), M(0), type(type),
        nChan(nChan), NS(1), NC(2), NR(nChan/2)
{
    loadSettings();

    mapDlg = new QDialog;

    mapDlg->setWindowFlags( mapDlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    mapUI = new Ui::ShankMapping;
    mapUI->setupUi( mapDlg );

    mapDlg->setWindowTitle(
            QString("%1 %2").arg( type ).arg( mapDlg->windowTitle() ) );

    mapUI->nsSB->setValue( NS );
    mapUI->ncSB->setValue( NC );
    mapUI->nrSB->setValue( NR );

    if( type == "imec" ) {
        mapUI->nsSB->setDisabled( true );
        mapUI->ncSB->setDisabled( true );
        mapUI->nrSB->setDisabled( true );
    }

    mapUI->chanLbl->setText( QString("Acq chans: %1").arg( nChan ) );

    ConnectUI( mapUI->nsSB, SIGNAL(valueChanged(int)), this, SLOT(hdrChanged()) );
    ConnectUI( mapUI->ncSB, SIGNAL(valueChanged(int)), this, SLOT(hdrChanged()) );
    ConnectUI( mapUI->nrSB, SIGNAL(valueChanged(int)), this, SLOT(hdrChanged()) );
    ConnectUI( mapUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( mapUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( mapUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );
}


ShankMapCtl::~ShankMapCtl()
{
    saveSettings();

    if( mapUI ) {
        delete mapUI;
        mapUI = 0;
    }

    if( mapDlg ) {
        delete mapDlg;
        mapDlg = 0;
    }

    if( M0 ) {
        delete M0;
        M0 = 0;
    }

    if( M ) {
        delete M;
        M = 0;
    }
}


// Return values:
// - empty  = default
// - file   = this smp file
//
QString ShankMapCtl::Edit( const QString &file )
{
    inFile = file;

    if( inFile.contains( "*" ) )
        inFile.clear();

    if( !inFile.isEmpty() )
        loadFile( inFile );
    else
        defaultBut();

    mapDlg->exec();

    return M0File;
}


void ShankMapCtl::hdrChanged()
{
    NS = mapUI->nsSB->value();
    NC = mapUI->ncSB->value();
    NR = mapUI->nrSB->value();

    defaultBut();
}


void ShankMapCtl::defaultBut()
{
    emptyTable();
    emptyM();

    if( NS*NC*NR < nChan ) {
        mapUI->statusLbl->setText(
            QString("Too few probe sites (%1)").arg( NS*NC*NR ) );
        return;
    }

    if( type == "imec" ) {

        if( NS*NC*NR > nChan ) {
            mapUI->statusLbl->setText(
                QString("Wrong imec site count (%1)").arg( NS*NC*NR ) );
            return;
        }

        M->fillDefaultIm( imro );
    }
    else
        M->fillDefaultNi( NS, NC, NR, nChan );

    copyM2M0();
    M0File.clear();

    M2Table();

    mapUI->statusLbl->setText( "Default map set" );
}


void ShankMapCtl::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    mapDlg,
                    "Load a shank mapping",
                    lastDir,
                    QString("Map files (*.%1.smp)").arg( type ) );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void ShankMapCtl::saveBut()
{
    if( !Table2M() )
        return;

    QString fn = QFileDialog::getSaveFileName(
                    mapDlg,
                    "Save shank mapping",
                    lastDir,
                    QString("Map files (*.%1.smp)").arg( type ) );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();

        QString msg;
        bool    ok = M->saveFile( msg, fn );

        mapUI->statusLbl->setText( msg );

        if( ok ) {
            copyM2M0();
            M0File = fn;
        }
    }
}


void ShankMapCtl::okBut()
{
    if( !Table2M() )
        return;

    if( *M != *M0 ) {
        mapUI->statusLbl->setText( "Changed table is not saved" );
        return;
    }

    mapDlg->accept();
}


void ShankMapCtl::cancelBut()
{
    M0File = inFile;
    mapDlg->reject();
}


void ShankMapCtl::emptyM()
{
    if( M )
        delete M;

    M = new ShankMap;
}


void ShankMapCtl::copyM2M0()
{
    if( M0 )
        delete M0;

    M0 = new ShankMap( *M );
}


void ShankMapCtl::loadSettings()
{
    STDSETTINGS( settings, "shankmap" );
    settings.beginGroup( "ShankMapDlg" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void ShankMapCtl::saveSettings() const
{
    STDSETTINGS( settings, "shankmap" );
    settings.beginGroup( "ShankMapDlg" );

    settings.setValue( "lastDlgDir", lastDir );
}


void ShankMapCtl::emptyTable()
{
    mapUI->tableWidget->setRowCount( 0 );
}


void ShankMapCtl::M2Table()
{
    QTableWidget    *T = mapUI->tableWidget;
    int             nr = M->e.size();

    T->setRowCount( nr );

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem    *ti;
        const ShankMapDesc  &E = M->e[i];

        // ---------
        // row label
        // ---------

        if( !(ti = T->verticalHeaderItem( i )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( i, ti );
        }

        ti->setText( QString::number( i ) );

        // -----
        // Shank
        // -----

        if( !(ti = T->item( i, 0 )) ) {

            ti = new QTableWidgetItem;
            T->setItem( i, 0, ti );

            if( type == "imec" )
                ti->setFlags( Qt::ItemIsEnabled );
            else
                ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.s ) );

        // ---
        // Col
        // ---

        if( !(ti = T->item( i, 1 )) ) {

            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );

            if( type == "imec" )
                ti->setFlags( Qt::ItemIsEnabled );
            else
                ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.c ) );

        // ---
        // Row
        // ---

        if( !(ti = T->item( i, 2 )) ) {

            ti = new QTableWidgetItem;
            T->setItem( i, 2, ti );

            if( type == "imec" )
                ti->setFlags( Qt::ItemIsEnabled );
            else
                ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.r ) );

        // ----
        // Used
        // ----

        if( !(ti = T->item( i, 3 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 3, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( E.u ) );
    }
}


bool ShankMapCtl::Table2M()
{
    if( NS*NC*NR < nChan ) {
        mapUI->statusLbl->setText(
            QString("Too few probe sites (%1)").arg( NS*NC*NR ) );
        return false;
    }

    if( !mapUI->tableWidget->rowCount() ) {
        mapUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int             nr = M->e.size();
    QSet<QString>   seen;

    for( int i = 0; i < nr; ++i ) {

        ShankMapDesc        &E = M->e[i];
        QTableWidgetItem    *ti;
        bool                ok;
        int                 val;

        // -----
        // Shank
        // -----

        ti  = mapUI->tableWidget->item( i, 0 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val >= NS ) {
                mapUI->statusLbl->setText(
                    QString("Shank value (%1) [channel %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( NS - 1 ) );
                return false;
            }

            E.s = val;
        }
        else {
            mapUI->statusLbl->setText(
                QString("Bad shank index for channel (%1)").arg( i ) );
            return false;
        }

        // ---
        // Col
        // ---

        ti  = mapUI->tableWidget->item( i, 1 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val >= NC ) {
                mapUI->statusLbl->setText(
                    QString("Column value (%1) [channel %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( NC - 1 ) );
                return false;
            }

            E.c = val;
        }
        else {
            mapUI->statusLbl->setText(
                QString("Bad column index for channel (%1)").arg( i ) );
            return false;
        }

        // ---
        // Row
        // ---

        ti  = mapUI->tableWidget->item( i, 2 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val >= NR ) {
                mapUI->statusLbl->setText(
                    QString("Row value (%1) [channel %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( NR - 1 ) );
                return false;
            }

            E.r = val;
        }
        else {
            mapUI->statusLbl->setText(
                QString("Bad row index for channel (%1)").arg( i ) );
            return false;
        }

        // ----
        // Used
        // ----

        ti  = mapUI->tableWidget->item( i, 3 );
        val = ti->text().toInt( &ok );

        if( ok ) {

            if( val != 0 && val != 1 ) {
                mapUI->statusLbl->setText(
                    QString("'Used' value (%1) [channel %2] out of range [0 or 1]")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            E.u = val;
        }
        else {
            mapUI->statusLbl->setText(
                QString("Bad 'used' value for channel (%1)").arg( i ) );
            return false;
        }

        // ----------
        // Uniqueness
        // ----------

        QString key = QString("%1;%2;%3").arg( E.s ).arg( E.c ).arg( E.r );

        if( seen.contains( key ) ) {
            mapUI->statusLbl->setText(
                QString("Channel (%1) entry is not unique").arg( i ) );
            return false;
        }

        seen.insert( key );
    }

    return true;
}


void ShankMapCtl::loadFile( const QString &file )
{
    emptyTable();
    emptyM();

    QString msg;
    bool    ok = M->loadFile( msg, file );

    mapUI->statusLbl->setText( msg );

    if( ok ) {

        mapUI->nsSB->setValue( M->ns );
        mapUI->ncSB->setValue( M->nc );
        mapUI->nrSB->setValue( M->nr );

        if( M->nSites() >= nChan ) {

            NS = M->ns;
            NC = M->nc;
            NR = M->nr;

            copyM2M0();
            M0File = file;

            M2Table();
        }
        else {
            mapUI->statusLbl->setText(
                QString("Too few sites (%1) in file").arg( M->nSites() ) );
        }
    }
}


