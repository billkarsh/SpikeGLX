
#include "ui_DataDirDialog.h"

#include "DataDirCtl.h"
#include "Util.h"
#include "MainApp.h"

#include <QFileDialog>

#define TBL_PATH    0




/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

DataDirCtl::DataDirCtl( const QStringList &sl, bool isMD )
    :   QDialog(0), sl(sl), isMD(isMD)
{
    setWindowFlags( windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ddUI = new Ui::DataDirDialog;
    ddUI->setupUi( this );

    toDialog();

    ConnectUI( ddUI->mainBut, SIGNAL(clicked()), this, SLOT(mainBut()) );
    ConnectUI( ddUI->addBut, SIGNAL(clicked()), this, SLOT(addBut()) );
    ConnectUI( ddUI->rmvBut, SIGNAL(clicked()), this, SLOT(rmvBut()) );
    ConnectUI( ddUI->editBut, SIGNAL(clicked()), this, SLOT(editBut()) );
    ConnectUI( ddUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );
    ConnectUI( ddUI->buttonBox, SIGNAL(rejected()), this, SLOT(cancelBut()) );
}


DataDirCtl::~DataDirCtl()
{
    if( ddUI ) {
        delete ddUI;
        ddUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DataDirCtl::run()
{
    exec();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void DataDirCtl::mainBut()
{
    ddUI->statusLbl->setText( "" );

    QString dir;

    if( sl.size() )
        dir = sl[0];

    if( selectDir( dir ) ) {
        resizeSL( 1 );
        sl[0] = dir;
        updateMain();
    }
}


void DataDirCtl::addBut()
{
    ddUI->statusLbl->setText( "" );

    QString dir;

    if( selectDir( dir ) ) {

        QTableWidget        *T  = ddUI->tableWidget;
        QTableWidgetItem    *ti = T->currentItem();
        int                 nsl = sl.size() + 1,
                            row;

        sl.reserve( nsl );

        if( ti )
            row = ti->row() + 2;
        else
            row = nsl - 1;

        sl.insert( sl.begin() + row, dir );
        updateTable();

        row = qMax( row - 1, 0 );
        T->setCurrentCell( row, TBL_PATH );
    }
}


void DataDirCtl::rmvBut()
{
// get selection

    QTableWidget        *T  = ddUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 row = -1;

    if( ti ) {
        row = ti->row();
        ddUI->statusLbl->setText( "" );
    }
    else {
        ddUI->statusLbl->setText( "No selected item in table" );
        return;
    }

// remove

    sl.erase( sl.begin() + 1 + row );
    updateTable();

// select next

    int nr = T->rowCount();

    if( row < nr )
        T->setCurrentCell( row, TBL_PATH );
    else if( nr )
        T->setCurrentCell( nr - 1, TBL_PATH );
}


void DataDirCtl::editBut()
{
// get selection

    QTableWidget        *T  = ddUI->tableWidget;
    QTableWidgetItem    *ti = T->currentItem();
    int                 isl = -1;

    if( ti ) {
        isl = ti->row() + 1;
        ddUI->statusLbl->setText( "" );
    }
    else {
        ddUI->statusLbl->setText( "No selected item in table" );
        return;
    }

// browse and edit

    QString dir = sl[isl];

    if( selectDir( dir ) ) {
        sl[isl] = dir;
        ti->setText( sl[isl] );
    }
}


void DataDirCtl::okBut()
{
    if( !fromDialog() )
        return;

    mainApp()->dataDirCtlUpdate( sl, isMD );

    accept();
}


void DataDirCtl::cancelBut()
{
    reject();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool DataDirCtl::selectDir( QString &dir_inout )
{
    if( dir_inout.isEmpty() || !QDir( dir_inout ).exists() )
        dir_inout = QDir::rootPath();

    if( dir_inout.endsWith( ":" )  )
        dir_inout += "/";

    dir_inout =
        QFileDialog::getExistingDirectory(
            this,
            "Choose Data Directory",
            dir_inout,
            QFileDialog::DontResolveSymlinks
            | QFileDialog::ShowDirsOnly );

    if( !dir_inout.isEmpty() ) {

        rmvLastSlash( dir_inout );
        return true;
    }

    return false;
}


void DataDirCtl::resizeSL( int n )
{
    sl.reserve( n );

    for( int i = sl.size(); i < n; ++i )
        sl.push_back( QString() );
}


void DataDirCtl::updateMain()
{
    ddUI->mainLbl->setText( sl.size() ? sl[0] : "" );
}


void DataDirCtl::updateTable()
{
    QTableWidget    *T  = ddUI->tableWidget;
    int             nr  = qMax( sl.size() - 1, 0 );

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

        ti->setText( QString::number( i + 1 ) );

        // ----
        // Path
        // ----

        if( !(ti = T->item( i, TBL_PATH )) ) {
            ti = new QTableWidgetItem;
            T->setItem( i, TBL_PATH, ti );
            ti->setFlags( Qt::ItemIsEnabled );
        }

        ti->setText( sl[i + 1] );
    }
}


void DataDirCtl::toDialog()
{
    updateMain();
    ddUI->mdGB->setChecked( isMD );
    updateTable();
}


bool DataDirCtl::fromDialog()
{
    QTableWidget    *T  = ddUI->tableWidget;
    int             nsl = T->rowCount() + 1;

// get data

    resizeSL( nsl );

    sl[0]   = ddUI->mainLbl->text().trimmed();
    isMD    = ddUI->mdGB->isChecked();

    for( int i = 1; i < nsl; ++i )
        sl[i] = T->item( i - 1, TBL_PATH )->text().trimmed();

    for( int i = 0; i < nsl; ++i )
        rmvLastSlash( sl[i] );

// main nonempty?

    if( sl[0].isEmpty() ) {
        ddUI->statusLbl->setText( "Missing required main directory" );
        return false;
    }

// sanity check table if MD mode

    if( isMD ) {

        if( nsl <= 1 ) {
            ddUI->statusLbl->setText( "Multidisk enabled but table empty" );
            return false;
        }

        for( int i = 1; i < nsl; ++i ) {

            if( sl[i].isEmpty() ) {
                ddUI->statusLbl->setText(
                        QString("Empty table entry: %1").arg( i ) );
                return false;
            }
        }
    }

    ddUI->statusLbl->setText( "" );
    return true;
}


