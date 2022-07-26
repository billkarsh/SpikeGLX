
#include "ui_FVW_ShankWindow.h"

#include "Util.h"
#include "MainApp.h"
#include "FVW_ShankCtl.h"
#include "DataFile.h"

#include <QAction>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* FVW_ShankCtl --------------------------------------------------- */
/* ---------------------------------------------------------------- */

FVW_ShankCtl::FVW_ShankCtl( const DataFile *df, QWidget *parent )
    :   QDialog(parent), df(df), scUI(0), svTab(0)
{
    setWindowFlag( Qt::Tool, true );
}


FVW_ShankCtl::~FVW_ShankCtl()
{
    if( svTab ) {
        delete svTab;
        svTab = 0;
    }

    if( scUI ) {
        delete scUI;
        scUI = 0;
    }

    emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::showDialog()
{
    showNormal();
    scUI->scroll->scrollToSelected();
    mainApp()->modelessOpened( this );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::cursorOver( int ig )
{
    svTab->cursorOver( ig );
}


void FVW_ShankCtl::lbutClicked( int ig )
{
    svTab->cursorOver( ig );
    emit selChanged( ig );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FVW_ShankCtl::baseInit( const ShankMap *map, int bnkRws )
{
    scUI = new Ui::FVW_ShankWindow;
    scUI->setupUi( this );
    ConnectUI( view(), SIGNAL(cursorOver(int,bool)), this, SLOT(cursorOver(int)) );
    ConnectUI( view(), SIGNAL(lbutClicked(int,bool)), this, SLOT(lbutClicked(int)) );

// Tabs

    svTab = new ShankViewTab( this, scUI->svTab );

    loadSettings();

    svTab->baseInit( map, bnkRws );

// Window

    setAttribute( Qt::WA_DeleteOnClose, false );
    restoreScreenState();

    QString type = df->fileLblFromObj();
    type.front() = type.front().toUpper();
    setWindowTitle( QString("%1 Shank Activity (Offline)").arg( type ) );
}


// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool FVW_ShankCtl::eventFilter( QObject *watched, QEvent *event )
{
    if( event->type() == QEvent::KeyPress ) {

        QKeyEvent   *e = static_cast<QKeyEvent*>(event);

        if( e->modifiers() == Qt::ControlModifier ) {

            if( e->key() == Qt::Key_A ) {
                mainApp()->act.aoDlgAct->trigger();
                e->ignore();
                return true;
            }
        }
    }

    return QDialog::eventFilter( watched, event );
}


void FVW_ShankCtl::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QDialog::keyPressEvent( e );
}


void FVW_ShankCtl::closeEvent( QCloseEvent *e )
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
void FVW_ShankCtl::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( screenStateName() ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void FVW_ShankCtl::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( screenStateName(), saveGeometry() );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankView* FVW_ShankCtl::view() const
{
    return scUI->scroll->theV;
}


ShankScroll* FVW_ShankCtl::scroll() const
{
    return scUI->scroll;
}


void FVW_ShankCtl::setStatus( const QString &s )
{
    scUI->statusLbl->setText( s );
}


