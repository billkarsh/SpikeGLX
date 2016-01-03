
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

ChanMapCtl::ChanMapCtl( QObject *parent, ChanMap &defMap )
    :   QObject( parent ), D(defMap)
{
    loadSettings();

    mapDlg  = new QDialog;
    mapUI   = new Ui::ChanMapping;
    mapUI->setupUi( mapDlg );
    ConnectUI( mapUI->defaultBut, SIGNAL(clicked()), this, SLOT(defaultButPushed()) );
    ConnectUI( mapUI->loadBut, SIGNAL(clicked()), this, SLOT(loadButPushed()) );
    ConnectUI( mapUI->saveBut, SIGNAL(clicked()), this, SLOT(saveButPushed()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    mapUI->cfgLbl->setText(
        QString("%1, %2, %3, %4, %5, %6, %7")
        .arg( D.AP ).arg( D.LF ).arg( D.MN ).arg( D.MA )
        .arg( D.C  ).arg( D.XA ).arg( D.XD ) );
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
}


QString ChanMapCtl::Edit( const QString &file )
{
    inFile = file;

    if( inFile.contains( "*" ) )
        inFile.clear();

    if( inFile.isEmpty() )
        defaultButPushed();
    else
        loadFile( inFile );

    mapDlg->exec();

    return M0File;
}


void ChanMapCtl::defaultButPushed()
{
    M = D;
    M.fillDefault();

    M0 = M;
    M0File.clear();

    mapUI->mapLbl->setText(
        QString("%1, %2, %3, %4, %5, %6, %7")
        .arg( M.AP ).arg( M.LF ).arg( M.MN ).arg( M.MA )
        .arg( M.C  ).arg( M.XA ).arg( M.XD ) );

    M2Table();

    mapUI->statusLbl->setText( "Default map set" );
}


void ChanMapCtl::loadButPushed()
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


void ChanMapCtl::saveButPushed()
{
    if( !Table2M() )
        return;

    QString fn = QFileDialog::getSaveFileName(
                    mapDlg,
                    "Save channel mapping",
                    lastDir,
                    "Map files (*.cmp)" );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();

        QString msg;
        bool    ok = M.saveFile( msg, fn );

        mapUI->statusLbl->setText( msg );

        if( ok ) {

            M0      = M;
            M0File  = fn;
        }
    }
}


void ChanMapCtl::okBut()
{
    if( !Table2M() )
        return;

    if( M != M0 ) {
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
    int             nr = M.i16Count();

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

        ti->setText( M.name( i ) );

        // -----
        // Order
        // -----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( M.e[i].order ) );
    }
}


bool ChanMapCtl::Table2M()
{
    if( !M.equalHdr( D ) ) {
        mapUI->statusLbl->setText( "Header mismatch" );
        return false;
    }

    if( !mapUI->tableWidget->rowCount() ) {
        mapUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int         nr   = M.i16Count();
    QSet<int>   seen;

    for( int i = 0; i < nr; ++i ) {

        QTableWidgetItem    *ti = mapUI->tableWidget->item( i, 1 );
        bool                ok;
        int                 val = ti->text().toInt( &ok );

        if( ok ) {

            if( val < 0 || val >= nr ) {
                mapUI->statusLbl->setText(
                    QString("Order value (%1) [row %2] out of range [0..%3]")
                    .arg( val )
                    .arg( i )
                    .arg( nr - 1 ) );
                return false;
            }

            if( seen.contains( val ) ) {
                mapUI->statusLbl->setText(
                    QString("Order value (%1) [row %2] is not unique")
                    .arg( val )
                    .arg( i ) );
                return false;
            }

            M.e[i].order = val;
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

    QString msg;
    bool    ok = M.loadFile( msg, file );

    mapUI->statusLbl->setText( msg );

    if( ok ) {

        mapUI->mapLbl->setText(
            QString("%1, %2, %3, %4, %5, %6, %7")
            .arg( M.AP ).arg( M.LF ).arg( M.MN ).arg( M.MA )
            .arg( M.C  ).arg( M.XA ).arg( M.XD ) );

        if( M.equalHdr( D ) ) {

            M0      = M;
            M0File  = file;

            M2Table();
        }
        else
            mapUI->statusLbl->setText( "Header mismatch" );
    }
}


#ifdef TEST_CH_MAP_CNTRL

#include <QApplication>

int main( int argc, char *argv[] )
{
    QApplication    app( argc, argv );

    ChanMapCtl  *c = new ChanMapCtl( 0, 0, 0, 8, 32, 0 );

    c->Edit( QString() );

    return app.exec();
}

#endif


