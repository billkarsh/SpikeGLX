
#include "ui_ChanMapping.h"

#include "ChanMapCtl.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QDialog>
#include <QFileDialog>
#include <QSettings>
#include <QSet>




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ChanMapCtl::ChanMapCtl( QObject *parent, const ChanMap &defMap )
    :   QObject( parent ), D(defMap), Mref(0), Mcur(0)
{
    loadSettings();

    mapDlg = new QDialog;

    mapDlg->setWindowFlags( mapDlg->windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    mapUI = new Ui::ChanMapping;
    mapUI->setupUi( mapDlg );

    mapUI->autoCB->setCurrentIndex( D.type() == "nidq" ? 0 : 1 );

    ConnectUI( mapUI->applyAutoBut, SIGNAL(clicked()), this, SLOT(applyAutoBut()) );
    ConnectUI( mapUI->applyListBut, SIGNAL(clicked()), this, SLOT(applyListBut()) );
    ConnectUI( mapUI->loadBut, SIGNAL(clicked()), this, SLOT(loadBut()) );
    ConnectUI( mapUI->saveBut, SIGNAL(clicked()), this, SLOT(saveBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( mapUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );

    mapDlg->setWindowTitle(
            QString("%1 %2").arg( D.type() ).arg( mapDlg->windowTitle() ) );

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

    if( Mref ) {
        delete Mref;
        Mref = 0;
    }

    if( Mcur ) {
        delete Mcur;
        Mcur = 0;
    }
}


// Return values:
// - empty  = default (NI=acq order, IM=bottom-up)
// - file   = this cmp file
//
QString ChanMapCtl::Edit( const QString &file, int ip )
{
    iniFile     = file;
    this->ip    = ip;

    if( iniFile.contains( "*" ) )
        iniFile.clear();

    if( !iniFile.isEmpty() )
        loadFile( iniFile );
    else
        applyAutoBut( D.type() == "nidq" ? 0 : 1 );

    mapDlg->exec();

    return refFile;
}


// idx: 0=acq, 1=fwd, 2=rev
//
// On startup dialog calls this once with (idx >= 0) to set initial order.
// All other times it is called from user button with default (idx = -1).
//
void ChanMapCtl::applyAutoBut( int idx )
{
    if( idx < 0 )
        idx = mapUI->autoCB->currentIndex();

    defaultOrder();

    if( !idx )
        ;
    else {

        QString     s;
        ConfigCtl   *C = mainApp()->cfgCtl();

        if( C->isConfigDlg( parent() ) ) {

            if( !C->chanMapGetsShankOrder( s, D.type(), idx==2, mapDlg ) )
                return;
        }
        else {

            const DAQ::Params   &p = C->acceptedParams;

            if( D.type() == "nidq" ) {

                if( idx == 2 )
                    p.ni.sns.shankMap.revChanOrderFromMapNi( s );
                else
                    p.ni.sns.shankMap.chanOrderFromMapNi( s );
            }
            else {

                const CimCfg::AttrEach  &E = p.im.each[ip];

                if( idx == 2 )
                    E.sns.shankMap.revChanOrderFromMapIm( s, E.roTbl->nLF() );
                else
                    E.sns.shankMap.chanOrderFromMapIm( s, E.roTbl->nLF() );
            }
        }

        theseChansToTop( s );
    }

    QString msg, ext;
    bool    def = false;

    if( D.type() == "nidq" ) {

        if( !idx ) {
            def = true;
            ext = " (NI default)";
        }
    }
    else if( idx == 1 ) {
        def = true;
        ext = " (IM default)";
    }

    if( def ) {
        copyMcur2ref();
        refFile.clear();
    }

    if( !idx )
        msg = "Acquisition ordered" + ext;
    else if( idx == 1 )
        msg = "Bottom-up ordered" + ext;
    else
        msg = "Top-down ordered";

    mapUI->statusLbl->setText( msg );
}


void ChanMapCtl::applyListBut()
{
    theseChansToTop( mapUI->listTE->toPlainText() );
}


void ChanMapCtl::loadBut()
{
    QString fn = QFileDialog::getOpenFileName(
                    mapDlg,
                    "Load a channel mapping",
                    lastDir,
                    QString("Map files (*.%1.cmp)").arg( Mcur->type() ) );

    if( fn.length() ) {
        lastDir = QFileInfo( fn ).absolutePath();
        loadFile( fn );
    }
}


void ChanMapCtl::saveBut()
{
    if( !table2Mcur() )
        return;

    QString fn = QFileDialog::getSaveFileName(
                    mapDlg,
                    "Save channel mapping",
                    lastDir,
                    QString("Map files (*.%1.cmp)").arg( Mcur->type() ) );

    if( fn.length() ) {

        lastDir = QFileInfo( fn ).absolutePath();

        QString msg;
        bool    ok = Mcur->saveFile( msg, fn );

        mapUI->statusLbl->setText( msg );

        if( ok ) {
            copyMcur2ref();
            refFile = fn;
        }
    }
}


void ChanMapCtl::okBut()
{
    if( !table2Mcur() )
        return;

    if( *Mcur != *Mref ) {
        mapUI->statusLbl->setText( "Changed table is not saved" );
        return;
    }

    mapDlg->accept();
}


void ChanMapCtl::cancelBut()
{
    refFile = iniFile;
    mapDlg->reject();
}


void ChanMapCtl::defaultOrder()
{
    createMcur();
    Mcur->fillDefault();

    mapUI->mapLbl->setText( Mcur->hdrText() );
    Mcur2table();
}


void ChanMapCtl::createMcur()
{
    if( Mcur )
        delete Mcur;

    if( D.type() == "nidq" )
        Mcur  = new ChanMapNI( *dynamic_cast<const ChanMapNI*>(&D) );
    else
        Mcur  = new ChanMapIM( *dynamic_cast<const ChanMapIM*>(&D) );
}


void ChanMapCtl::copyMcur2ref()
{
    if( Mref )
        delete Mref;

    if( D.type() == "nidq" )
        Mref = new ChanMapNI( *dynamic_cast<ChanMapNI*>(Mcur) );
    else
        Mref = new ChanMapIM( *dynamic_cast<ChanMapIM*>(Mcur) );
}


// Called only from ctor.
//
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


void ChanMapCtl::Mcur2table()
{
    QTableWidget    *T = mapUI->tableWidget;
    int             nr = Mcur->e.size();

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

        ti->setText( Mcur->name( i ) );

        // -----
        // Order
        // -----

        if( !(ti = T->item( i, 1 )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, 1, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString::number( Mcur->e[i].order ) );
    }
}


bool ChanMapCtl::table2Mcur()
{
    if( !Mcur->equalHdr( D ) ) {
        mapUI->statusLbl->setText( "Header mismatch" );
        return false;
    }

    if( !mapUI->tableWidget->rowCount() ) {
        mapUI->statusLbl->setText( "Empty table" );
        return false;
    }

    int         nr      = Mcur->e.size(),
                vMax    = Mcur->i16Count();
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

            Mcur->e[i].order = val;
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

    createMcur();

    QString msg;
    bool    ok = Mcur->loadFile( msg, file );

    mapUI->statusLbl->setText( msg );

    if( ok ) {

        mapUI->mapLbl->setText( Mcur->hdrText() );

        if( Mcur->equalHdr( D ) ) {

            copyMcur2ref();
            refFile = file;

            Mcur2table();
        }
        else
            mapUI->statusLbl->setText( "Header mismatch" );
    }
}


void ChanMapCtl::theseChansToTop( const QString &s )
{
    mapUI->statusLbl->setText( "" );

    if( s.isEmpty() )
        return;

// Make sure table order is valid because we will append
// unnamed channels according to current table order.

    if( !table2Mcur() )
        return;

// Make mapping from channel name to chanMap entry index
// Make mapping from table order  to chanMap entry index

    QMap<int,int>   nam2Idx;
    QMap<int,int>   ord2Idx;
    QRegExp         re(";(\\d+)");
    int             ne = Mcur->e.size();

    for( int i = 0; i < ne; ++i ) {

        Mcur->e[i].name.contains( re );
        nam2Idx[re.cap(1).toInt()]  = i;
        ord2Idx[Mcur->e[i].order]      = i;
    }

// Initialize new order array with -1
// Mark all entry indices initially unused

    std::vector<int>    newo( ne, -1 );
    std::vector<bool>   used( ne, false );

// Parse list parameter (s) and assign named chans to newo array

    QMap<int,int>::iterator it;
    int o_next = 0;

    QStringList terms = s.split(
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

                    newo[id] = o_next++;
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

                    newo[id] = o_next++;
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

            newo[id] = o_next++;
            used[id] = true;
        }
    }

// Walk all entries in order, appending if unused

    for( it = ord2Idx.begin(); it != ord2Idx.end(); ++it ) {

        int id = it.value();

        if( !used[id] ) {
            newo[id] = o_next++;
            used[id] = true;
        }
    }

// Update table

    for( int i = 0; i < ne; ++i )
        Mcur->e[i].order = newo[i];

    Mcur2table();
}


