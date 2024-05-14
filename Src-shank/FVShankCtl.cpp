
#include "ui_ShankWindow.h"

#include "Util.h"
#include "FVShankCtl.h"
#include "DataFile.h"

#include <QCloseEvent>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

FVShankCtl::FVShankCtl( const DataFile *df, QWidget *parent )
    :   ShankCtlBase(parent, false), df(df), svTab(0)
{
}


FVShankCtl::~FVShankCtl()
{
    if( svTab ) {
        delete svTab;
        svTab = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVShankCtl::lbutClicked( int ig )
{
    svTab->lbutClicked( ig );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FVShankCtl::parInit( const ShankMap *map )
{
    baseInit( df->imro(), true );

    ConnectUI( view(), SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int)) );
    ConnectUI( view(), SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int)) );

// Tabs

    if( seTab ) {
        seTab->hideCurrent();
        seTab->hideOKBut();
        seTab->renameApplyRevert();
    }

    svTab = new FVShankViewTab( this, scUI->svTab, df );

    ConnectUI( this, SIGNAL(viewTabSyncYPix(int)), svTab, SLOT(syncYPix(int)) );

    loadSettings();

    svTab->init( map );

// Window

    restoreScreenState();

    QString type = df->fileLblFromObj();
    type.front() = type.front().toUpper();
    setWindowTitle( QString("%1 Shank Activity (Offline)").arg( type ) );
}


void FVShankCtl::closeEvent( QCloseEvent *e )
{
    QDialog::closeEvent( e );

    if( e->isAccepted() ) {

        saveScreenState();
        emit closed( this );
    }
}


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void FVShankCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( screenStateName() ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void FVShankCtl::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( screenStateName(), saveGeometry() );
}


