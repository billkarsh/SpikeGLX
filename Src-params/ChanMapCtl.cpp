
#include "ui_ChanMapping.h"

#include "ChanMapCtl.h"
#include "Util.h"

#include <QDialog>
#include <QFileDialog>
#include <QSettings>
#include <QSet>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ChanMapCtl::ChanMapCtl( QObject *parent, const ChanMap &defMap )
    :   QObject( parent ), D(defMap), M0(0), M(0)
{
    loadSettings();

    mapDlg = new QDialog;

    mapDlg->setWindowFlags( mapDlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    mapUI = new Ui::ChanMapping;
    mapUI->setupUi( mapDlg );
    ConnectUI( mapUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultBut()) );
    ConnectUI( mapUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( mapUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    mapUI->cfgLbl->setText( D.hdrText() );
}


ChanMapCtl::~ChanMapCtl()
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
// - empty  = default (acq order)
// - file   = this cmp file
//
QString ChanMapCtl::Edit( const QString &file )
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


void ChanMapCtl::defaultBut()
{
    createM();
    M->fillDefault();

    copyM2M0();
    M0File.clear();

    mapUI->mapLbl->setText( M->hdrText() );

    M2Table();

    mapUI->statusLbl->setText( "Default map set" );
}


void ChanMapCtl::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    mapDlg,
                    "Load a channel mapping",
                    lastDir,
                    "Map files (*.cmp)" );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void ChanMapCtl::saveBut()
{
    if( !Table2M() )
        return;

    QString filter  = QString("Map files (*.%1.cmp)").arg( M->type() );
    QString fn      = QFileDialog::getSaveFileName(
                        mapDlg,
                        "Save channel mapping",
                        lastDir,
                        filter );

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


void ChanMapCtl::okBut()
{
    if( !Table2M() )
        return;

    if( *M != *M0 ) {
        mapUI->statusLbl->setText( "Changed table is not saved" );
        return;
    }

    mapDlg->accept();
}


void ChanMapCtl::cancelBut()
{
    M0File = inFile;
    mapDlg->reject();
}


void ChanMapCtl::createM()
{
    if( M )
        delete M;

    if( D.type() == "nidq" )
        M  = new ChanMapNI( *dynamic_cast<const ChanMapNI*>(&D) );
    else
        M  = new ChanMapIM( *dynamic_cast<const ChanMapIM*>(&D) );
}


void ChanMapCtl::copyM2M0()
{
    if( M0 )
        delete M0;

    if( D.type() == "nidq" )
        M0 = new ChanMapNI( *dynamic_cast<ChanMapNI*>(M) );
    else
        M0 = new ChanMapIM( *dynamic_cast<ChanMapIM*>(M) );
}


void ChanMapCtl::loadSettings()
{
    STDSETTINGS( settings, "chanmap" );
    settings.beginGroup( "ChanMapDlg" );

    lastDir = settings.value( "lastDlgDir", QString() ).toString();
}


void ChanMapCtl::saveSettings() const
{
    STDSETTINGS( settings, "chanmap" );
    settings.beginGroup( "ChanMapDlg" );

    settings.setValue( "lastDlgDir", lastDir );
}


void ChanMapCtl::emptyTable()
{
    mapUI->tableWidget->setRowCount( 0 );
}


void ChanMapCtl::M2Table()
{
    QTableWidget    *T = mapUI->tableWidget;
    int             nr = M->e.size();

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
        // Name
        // ----

        if( !(ti = T->item( i, 0 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 0, ti );
            ti->setFlags( Qt::ItemIsEnabled );
        }

        ti->setText( M->name( i ) );

        // -----
        // Order
        // -----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( M->e[i].order ) );
    }
}


bool ChanMapCtl::Table2M()
{
    if( !M->equalHdr( D ) ) {
        mapUI->statusLbl->setText( "Header mismatch" );
        return false;
    }

    if( !mapUI->tableWidget->rowCount() ) {
        mapUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int         nr      = M->e.size(),
                vMax    = M->i16Count();
    QSet<int>   seen;

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem    *ti = mapUI->tableWidget->item( i, 1 );
        bool                ok;
        int                 val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val >= vMax ) {
                mapUI->statusLbl->setText(
                    QString("Order value (%1) [row %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( vMax - 1 ) );
                return false;
            }

            if( seen.contains( val ) ) {
                mapUI->statusLbl->setText(
                    QString("Order value (%1) [row %2] is not unique")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            M->e[i].order = val;
            seen.insert( val );
        }
        else {

            mapUI->statusLbl->setText(
                QString("Bad order value on row (%1)")
                .arg( i ) );

            return false;
        }
    }

    return true;
}


void ChanMapCtl::loadFile( const QString &file )
{
    mapUI->mapLbl->clear();
    emptyTable();

    createM();

    QString msg;
    bool    ok = M->loadFile( msg, file );

    mapUI->statusLbl->setText( msg );

    if( ok ) {

        mapUI->mapLbl->setText( M->hdrText() );

        if( M->equalHdr( D ) ) {

            copyM2M0();
            M0File = file;

            M2Table();
        }
        else
            mapUI->statusLbl->setText( "Header mismatch" );
    }
}


