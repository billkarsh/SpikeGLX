
#include "ui_SimProbesDialog.h"

#include "SimProbesCtl.h"
#include "Util.h"

#include <QDialog>
#include <QFileDialog>
#include <QTimer>

#define TBL_SLOT    0
#define TBL_PORT    1
#define TBL_DOCK    2
#define TBL_ENAB    3
#define TBL_PATH    4




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

SimProbesCtl::SimProbesCtl( QObject *parent, SimProbes &SP )
    :   QObject(parent), SP(SP)
{
    spDlg = new QDialog;

    spDlg->setWindowFlags( spDlg->windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    spUI = new Ui::SimProbesDialog;
    spUI->setupUi( spDlg );

    SP.loadSettings();
    maddr = SP.getProbes();
    toTable();
    spUI->statusLbl->setText( "" );

    ConnectUI( spUI->sortBut, SIGNAL(clicked()), this, SLOT(sortBut()) );
    ConnectUI( spUI->addBut, SIGNAL(clicked()), this, SLOT(addBut()) );
    ConnectUI( spUI->rmvBut, SIGNAL(clicked()), this, SLOT(rmvBut()) );
    ConnectUI( spUI->helpBut, SIGNAL(clicked()), this, SLOT(helpBut()) );
    ConnectUI( spUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( spUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );
    ConnectUI( spUI->tableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(cellDoubleClicked(int,int)) );
}


SimProbesCtl::~SimProbesCtl()
{
    if( spUI ) {
        delete spUI;
        spUI = 0;
    }

    if( spDlg ) {
        delete spDlg;
        spDlg = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SimProbesCtl::run()
{
    spDlg->exec();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void SimProbesCtl::sortBut()
{
    if( fromTable() ) {

        SPAddr  A = selectedAddr();
        toTable();
        selectAddr( A );
    }
}


void SimProbesCtl::addBut()
{
    if( fromTable() ) {

        QString file;

        if( selectFile( file ) ) {

            SPAddr  A = lowestAvailAddr();

            maddr[A] = file;
            toTable();
            selectAddr( A );
        }
    }
}


void SimProbesCtl::rmvBut()
{
// get selection

    QTableWidget        *T  = spUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 ir  = -1;

    if( ti ) {
        ir = ti->row();
        spUI->statusLbl->setText( "" );
    }
    else {
        spUI->statusLbl->setText( "No selected item in table" );
        return;
    }

// remove

    maddr.erase( maddr.begin() + ir );
    toTable();

// select next

    int nr = T->rowCount();

    if( ir < nr )
        T->setCurrentCell( ir, TBL_SLOT );
    else if( nr )
        T->setCurrentCell( nr - 1, TBL_SLOT );
}


void SimProbesCtl::helpBut()
{
    showHelp( "SimProbes_Help" );
}


void SimProbesCtl::okBut()
{
    if( fromTable() ) {

        SP.saveSettings( maddr );
        spDlg->accept();
    }
}


void SimProbesCtl::cancelBut()
{
    spDlg->reject();
}


void SimProbesCtl::cellDoubleClicked( int row, int col )
{
    if( col == TBL_PATH )
        QTimer::singleShot( 150, this, SLOT(editPath( row )) );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

SPAddr SimProbesCtl::lowestAvailAddr()
{
    SPAddr  A;

    A.e = 1;

    while( maddr.contains( A ) )
        ++A.d;

    return A;
}


SPAddr SimProbesCtl::selectedAddr()
{
    SPAddr              A;
    QTableWidget        *T  = spUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();

    if( ti ) {
        int ir = ti->row();
        A.s = T->item( ir, TBL_SLOT )->text().trimmed().toInt();
        A.p = T->item( ir, TBL_PORT )->text().trimmed().toInt();
        A.d = T->item( ir, TBL_DOCK )->text().trimmed().toInt();
        A.e = (T->item( ir, TBL_ENAB )->checkState() == Qt::Checked);
    }

    return A;
}


int SimProbesCtl::addr2row( SPAddr &A )
{
    QMap<SPAddr,QString>::const_iterator
        it  = maddr.begin(),
        end = maddr.end();

    for( int ir = 0; it != end; ++it, ++ir ) {
        if( it.key() == A )
            return ir;
    }

    return -1;
}


void SimProbesCtl::selectAddr( SPAddr &A )
{
    QTableWidget    *T  = spUI->tableWidget;
    int             ir  = addr2row( A );

    guiBreathe();

    if( ir >= 0 )
        T->setCurrentCell( ir, TBL_SLOT );
    else
        T->setCurrentCell( -1, -1 );
}


bool SimProbesCtl::selectFile( QString &file )
{
    if( file.isEmpty() || !QFile( file ).exists() )
        file = QDir::rootPath();

    if( file.endsWith( ":" )  )
        file += "/";

    file =
        QFileDialog::getOpenFileName(
            spDlg,
            "Choose AP bin file",
            file,
            "AP binary files (*.ap.bin)" );

    if( !file.isEmpty() ) {

        QRegExp re("\\.ap\\..*");
        re.setCaseSensitivity( Qt::CaseInsensitive );

        file = file.remove( re );
        return true;
    }

    return false;
}


void SimProbesCtl::editPath( int row )
{
    QString file = (maddr.begin() + row).value();

    if( selectFile( file ) ) {
        maddr[(maddr.begin() + row).key()] = file;
        toTable();
    }
}


void SimProbesCtl::toTable()
{
    QTableWidget    *T  = spUI->tableWidget;
    int             nr  = maddr.size(),
                    ir  = 0;

    T->setRowCount( nr );

    QMap<SPAddr,QString>::const_iterator
        it  = maddr.begin(),
        end = maddr.end();

    for( ; it != end; ++it, ++ir ) {

        QTableWidgetItem    *ti;

        // ---------
        // row label
        // ---------

        if( !(ti = T->verticalHeaderItem( ir )) ) {
            ti = new QTableWidgetItem;
            T->setVerticalHeaderItem( ir, ti );
        }

        ti->setText( QString::number( ir ) );

        // ----
        // Slot
        // ----

        if( !(ti = T->item( ir, TBL_SLOT )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ir, TBL_SLOT, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString("%1").arg( it.key().s ) );

        // ----
        // Port
        // ----

        if( !(ti = T->item( ir, TBL_PORT )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ir, TBL_PORT, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString("%1").arg( it.key().p ) );

        // ----
        // Dock
        // ----

        if( !(ti = T->item( ir, TBL_DOCK )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ir, TBL_DOCK, ti );
            ti->setFlags( Qt::ItemIsEnabled | Qt::ItemIsEditable );
        }

        ti->setText( QString("%1").arg( it.key().d ) );

        // ----
        // Enab
        // ----

        if( !(ti = T->item( ir, TBL_ENAB )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ir, TBL_ENAB, ti );
            ti->setFlags( Qt::ItemIsSelectable
                            | Qt::ItemIsEnabled
                            | Qt::ItemIsUserCheckable );
        }

        ti->setCheckState( it.key().e ? Qt::Checked : Qt::Unchecked );

        // ----
        // Path
        // ----

        if( !(ti = T->item( ir, TBL_PATH )) ) {
            ti = new QTableWidgetItem;
            T->setItem( ir, TBL_PATH, ti );
            ti->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled );
        }

        ti->setText( it.value() );
    }

    T->resizeColumnToContents( TBL_SLOT );
    T->resizeColumnToContents( TBL_PORT );
    T->resizeColumnToContents( TBL_DOCK );
    T->resizeColumnToContents( TBL_ENAB );
    T->resizeColumnToContents( TBL_PATH );
}


bool SimProbesCtl::fromTable()
{
    maddr.clear();

    QTableWidget    *T = spUI->tableWidget;
    int             nr = T->rowCount();

    for( int ir = 0; ir < nr; ++ir ) {

        SPAddr  A;
        bool    ok;

        A.s = T->item( ir, TBL_SLOT )->text().trimmed().toInt( &ok );
        if( !ok ) {
            spUI->statusLbl->setText( QString("Row %1: bad slot").arg( ir ) );
            return false;
        }

        A.p = T->item( ir, TBL_PORT )->text().trimmed().toInt( &ok );
        if( !ok ) {
            spUI->statusLbl->setText( QString("Row %1: bad port").arg( ir ) );
            return false;
        }

        A.d = T->item( ir, TBL_DOCK )->text().trimmed().toInt( &ok );
        if( !ok ) {
            spUI->statusLbl->setText( QString("Row %1: bad dock").arg( ir ) );
            return false;
        }

        A.e = (T->item( ir, TBL_ENAB )->checkState() == Qt::Checked);

        maddr[A] = T->item( ir, TBL_PATH )->text();
    }

    spUI->statusLbl->setText( "" );
    return true;
}


