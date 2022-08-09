
#include "ui_ShankWindow.h"

#include "Util.h"
#include "MainApp.h"
#include "ShankCtlBase.h"

#include <QAction>


/* ---------------------------------------------------------------- */
/* ctor/dtor ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankCtlBase::ShankCtlBase( QWidget *parent, bool modal )
    :   QDialog(parent), scUI(0), seTab(0),
        modal_map(0), modal(modal)
{
}


ShankCtlBase::~ShankCtlBase()
{
    if( modal_map ) {
        delete modal_map;
        modal_map = 0;
    }

    if( seTab ) {
        delete seTab;
        seTab = 0;
    }

    if( scUI ) {
        delete scUI;
        scUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtlBase::baseInit( const IMROTbl *R, bool hasViewTab )
{
    scUI = new Ui::ShankWindow;
    scUI->setupUi( this );
    ConnectUI( view(), SIGNAL(gridHover(int,int,int)), this, SLOT(gridHover(int,int,int)) );
    ConnectUI( view(), SIGNAL(gridClicked(int,int,int)), this, SLOT(gridClicked(int,int,int)) );

    if( R )
        view()->setBnkRws( R->nAP() / R->nCol() );

// Modal

    if( modal ) {
        if( modal_map )
            delete modal_map;
        modal_map = new ShankMap( R->nShank(), R->nCol(), R->nRow() );
        view()->setShankMap( modal_map );
    }

// Tabs

    if( R && R->edit_able() ) {

        seTab = new ShankEditTab( this, scUI->seTab, R );

        if( hasViewTab )
            scUI->tabsW->setCurrentIndex( 1 );  // selects View
        else {
            scUI->tabsW->removeTab( 1 );
            scUI->tabsW->setCurrentIndex( 0 );  // selects Edit
        }
    }
    else
        scUI->tabsW->setTabEnabled( 0, false ); // selects View

// Window

    if( modal ) {
        setWindowModality( Qt::WindowModal );
        setWindowFlag( Qt::WindowContextHelpButtonHint, false );
        setWindowFlag( Qt::WindowCloseButtonHint, false );
        setWindowTitle( "IMRO Table Editor" );

        QSize   S = parentWidget()->size();
        int     w = 340;

        if( R->nShank() == 4 )
            w = 440;
        else if( R->nCol() == 8 )
            w = 410;

        resize( w, int(1.10 * S.height()) );
    }
    else {
        setWindowFlag( Qt::Tool, true );
        setAttribute( Qt::WA_DeleteOnClose, false );
    }
}


void ShankCtlBase::setOriginal( const QString &inFile )
{
    if( seTab ) {
        seTab->setCurrent( inFile );
        seTab->renameOriginal();
    }
}


void ShankCtlBase::showDialog()
{
    showNormal();
    scroll()->scrollToSelected();

    if( !modal )
        mainApp()->modelessOpened( this );
}


void ShankCtlBase::syncYPix( int y )
{
    if( scUI->tabsW->currentIndex() == 0 )
        emit viewTabSyncYPix( y );
    else
        seTab->syncYPix( y );
}


ShankView* ShankCtlBase::view() const
{
    return scUI->scroll->theV;
}


ShankScroll* ShankCtlBase::scroll() const
{
    return scUI->scroll;
}


void ShankCtlBase::setStatus( const QString &s )
{
    scUI->statusLbl->setText( s );
}


void ShankCtlBase::update()
{
    view()->updateNow();

//    QMetaObject::invokeMethod(
//        view(),
//        "updateNow",
//        Qt::QueuedConnection );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtlBase::gridHover( int s, int c, int r )
{
    if( !scUI->tabsW->currentIndex() )
        seTab->gridHover( s, c, r );
}


void ShankCtlBase::gridClicked( int s, int c, int r )
{
    if( !scUI->tabsW->currentIndex() )
        seTab->gridClicked( s, c, r );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool ShankCtlBase::eventFilter( QObject *watched, QEvent *event )
{
    if( !modal && event->type() == QEvent::KeyPress ) {

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


void ShankCtlBase::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QDialog::keyPressEvent( e );
}


