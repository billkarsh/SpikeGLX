
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
    ConnectUI( mapUI->applyBut, SIGNAL(clicked()), this, SLOT(applyBut()) );
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


void ChanMapCtl::applyBut()
{
    mapUI->statusLbl->setText( "" );

// Make sure table order is valid because we will append
// unnamed channels according to current table order.

    if( !Table2M() )
        return;

// Make mapping from user channel name to chanMap entry index
// Make mapping from table order to entry index

    QMap<int,int>   nam2Idx;
    QMap<int,int>   ord2Idx;
    QRegExp         re(";(\\d+)");
    int             ne = M->e.size();

    for( int i = 0; i < ne; ++i ) {

        M->e[i].name.contains( re );
        nam2Idx[re.cap(1).toInt()]  = i;
        ord2Idx[M->e[i].order]      = i;
    }

// Initialize new order array with -1
// Mark all entry indices initially unused

    QVector<int>    newo( ne, -1 );
    QVector<bool>   used( ne, false );

// Parse user list and assign named chans to newo array

    QMap<int,int>::iterator it;
    int onext = 0;

    QString     s       = mapUI->listTE->toPlainText();
    QStringList terms   = s.split(
                            QRegExp("[,;]"),
                            QString::SkipEmptyParts );

    foreach( const QString &t, terms ) {

        QStringList rng = t.split(
                            QRegExp("[:-]"),
                            QString::SkipEmptyParts );
        int         n   = rng.count(),
                    r1, r2, id;
        bool        ok1, ok2;

        if( n > 2 ) {
            mapUI->statusLbl->setText(QString("Bad format: %1").arg( t ));
            return;
        }

        if( n == 2 ) {

            r1  = rng[0].toUInt( &ok1 ),
            r2  = rng[1].toUInt( &ok2 );

            if( !ok1 || !ok2 )
                continue;

            if( r1 == r2 )
                goto justR1;

            if( r1 < r2 ) {

                for( int r = r1; r <= r2; ++r ) {

                    it = nam2Idx.find( r );

                    if( it == nam2Idx.end() )
                        continue;

                    id = it.value();

                    if( used[id] )
                        continue;

                    newo[id] = onext++;
                    used[id] = true;
                }
            }
            else {

                for( int r = r1; r >= r2; --r ) {

                    it = nam2Idx.find( r );

                    if( it == nam2Idx.end() )
                        continue;

                    id = it.value();

                    if( used[id] )
                        continue;

                    newo[id] = onext++;
                    used[id] = true;
                }
            }
        }
        else if( n == 1 ) {

justR1:
            int r1 = rng[0].toUInt( &ok1 );

            if( !ok1 )
                continue;

            it = nam2Idx.find( r1 );

            if( it == nam2Idx.end() )
                continue;

            id = it.value();

            if( used[id] )
                continue;

            newo[id] = onext++;
            used[id] = true;
        }
    }

// Walk all entries in order, appending if unused

    for( it = ord2Idx.begin(); it != ord2Idx.end(); ++it ) {

        int id = it.value();

        if( !used[id] ) {
            newo[id] = onext++;
            used[id] = true;
        }
    }

// Update table

    for( int i = 0; i < ne; ++i )
        M->e[i].order = newo[i];

    M2Table();
}


void ChanMapCtl::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    mapDlg,
                    "Load a channel mapping",
                    lastDir,
                    QString("Map files (*.%1.cmp)").arg( M->type() ) );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void ChanMapCtl::saveBut()
{
    if( !Table2M() )
        return;

    QString fn = QFileDialog::getSaveFileName(
                    mapDlg,
                    "Save channel mapping",
                    lastDir,
                    QString("Map files (*.%1.cmp)").arg( M->type() ) );

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

        QTableWidgetItem    *ti;
        bool                ok;
        int                 val;

        // -----
        // Order
        // -----

        ti  = mapUI->tableWidget->item( i, 1 );
        val = ti->text().toInt( &ok );

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
                QString("Bad order value on row (%1)").arg( i ) );
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


