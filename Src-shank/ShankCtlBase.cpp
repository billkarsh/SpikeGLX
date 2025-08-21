
#include "ui_ShankWindow.h"

#include "Util.h"
#include "MainApp.h"
#include "ShankCtlBase.h"

#include <QAction>
#include <QKeyEvent>
#include <QSettings>
#include <QWindow>


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

    if( _scroll ) {
        delete _scroll;
        _scroll = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void ShankCtlBase::baseInit( const IMROTbl *R, bool hasViewTab )
{
    scUI = new Ui::ShankWindow;
    scUI->setupUi( this );

    // Edit the layout to replace the scroll QWidget with
    // a container holding a proper ShankScroll. Crucially,
    // the ShankScroll is not in the parent-child hierarchy
    // so does not interfere with owning windows. We must
    // show and delete the embedded _scroll manually.
    {
        _scroll = new ShankScroll();
        _scroll->setWindowFlags( Qt::FramelessWindowHint );

        QWindow     *W = QWindow::fromWinId( _scroll->winId() );
        QWidget     *C = QWidget::createWindowContainer( W );

        QSizePolicy szp( QSizePolicy::Policy::MinimumExpanding,
                         QSizePolicy::Policy::MinimumExpanding );
        szp.setHorizontalStretch( 0 );
        szp.setVerticalStretch( 0 );
        szp.setHeightForWidth( C->sizePolicy().hasHeightForWidth() );
        C->setSizePolicy( szp );
        C->setMinimumSize( QSize( 80, 0 ) );

        QGridLayout *G = findChild<QGridLayout*>("gridLayout");
        QWidget     *X = findChild<QWidget*>("scroll");
        QLayoutItem *I = G->replaceWidget( X, C );

        if( I ) {
            if( I->widget() )
                delete I->widget();
            delete I;
        }
    }

    ConnectUI( view(), SIGNAL(gridHover(int,int,bool)), this, SLOT(gridHover(int,int,bool)) );
    ConnectUI( view(), SIGNAL(gridClicked(int,int,int,bool)), this, SLOT(gridClicked(int,int,int,bool)) );
    ConnectUI( view(), SIGNAL(lbutReleased()), this, SLOT(lbutReleased()) );

    if( R )
        view()->setImro( R );

// Modal

    if( modal ) {
        if( modal_map )
            delete modal_map;
        modal_map = new ShankMap( R->nShank(), R->nCol_vis(), R->nRow() );
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

        // cur width of editor, height taller than parent
        int h = int(1.10 * parentWidget()->height());
        h = qMin( h,
            QApplication::primaryScreen()->availableGeometry().height()
            - 40 );

        // Actualize geometry using show()
        _scroll->show();
        show();
        guiBreathe();

        // update cur width based on square pads
        resize( width() + view()->deltaWidth(), h );
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
    _scroll->show();
    _scroll->scrollToSelected();

    if( !modal ) {
        qf_enable();
        mainApp()->modelessOpened( this );
    }
    else {
        // Run exec() here, but not needed
        //
        // exec();
    }
}


void ShankCtlBase::syncYPix( int y )
{
    if( scUI->tabsW->currentIndex() == 0 )
        emit viewTabSyncYPix( y );
    else if( seTab )
        seTab->syncYPix( y );
}


ShankView* ShankCtlBase::view() const
{
    return _scroll->theV;
}


ShankScroll* ShankCtlBase::scroll() const
{
    return _scroll;
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

void ShankCtlBase::gridHover( int s, int r, bool quiet )
{
    if( !scUI->tabsW->currentIndex() )
        seTab->gridHover( s, r, quiet );
}


void ShankCtlBase::gridClicked( int s, int c, int r, bool shift )
{
    if( !scUI->tabsW->currentIndex() )
        seTab->gridClicked( s, c, r, shift );
}


void ShankCtlBase::lbutReleased()
{
    if( !scUI->tabsW->currentIndex() )
        seTab->lbutReleased();
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


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void ShankCtlBase::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( screenStateName() ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void ShankCtlBase::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( screenStateName(), saveGeometry() );
}


