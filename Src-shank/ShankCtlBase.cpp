
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

ShankCtlBase::ShankCtlBase( QWidget *parent, uint8_t sr_mask, bool modal )
    :   QDialog(modal ? 0 : parent), cfgdlg(parent), scUI(0), _scroll(0),
        seTab(0), modal_map(0), sr_mask(sr_mask), modal(modal)
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

// Swap in real scrolling opengl widget for placeholder

    _scroll = new ShankScroll( this );

    QSizePolicy szp( QSizePolicy::Policy::MinimumExpanding,
                     QSizePolicy::Policy::MinimumExpanding );
    szp.setHorizontalStretch( 0 );
    szp.setVerticalStretch( 0 );
    szp.setHeightForWidth( _scroll->sizePolicy().hasHeightForWidth() );
    _scroll->setSizePolicy( szp );
    _scroll->setMinimumSize( QSize( 80, 0 ) );

    QGridLayout *G = findChild<QGridLayout*>("gridLayout");
    QWidget     *X = findChild<QWidget*>("scroll");
    QLayoutItem *I = G->replaceWidget( X, _scroll );

    if( I ) {
        if( I->widget() )
            delete I->widget();
        delete I;
    }

// Signals

    ConnectUI( view(), SIGNAL(gridHover(int,int,bool)), this, SLOT(gridHover(int,int,bool)) );
    ConnectUI( view(), SIGNAL(gridClicked(int,int,int,bool)), this, SLOT(gridClicked(int,int,int,bool)) );
    ConnectUI( view(), SIGNAL(lbutReleased()), this, SLOT(lbutReleased()) );

// Imro

    if( R )
        view()->setImro( R, sr_mask );

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
        setWindowModality( Qt::ApplicationModal );
        setWindowFlag( Qt::WindowStaysOnTopHint, true );
        setWindowFlag( Qt::WindowContextHelpButtonHint, false );
        setWindowFlag( Qt::WindowCloseButtonHint, false );
        setWindowTitle( "IMRO Table Editor" );

        // cur width of editor, height taller than parent
        int h = int(1.10 * cfgdlg->height());
        h = qMin( h,
            QApplication::primaryScreen()->availableGeometry().height()
            - 40 );

        // actualize geometry using show()
        show();

        // update cur width based on square pads
        resize( width() + view()->deltaWidth(), h );

        // center over cfgdlg
        QRect   rCfg = cfgdlg->frameGeometry(),
                rDlg = frameGeometry();
        move(
            rCfg.center().x() - rDlg.width()/2,
            rCfg.center().y() - rDlg.height()/2 );
    }
    else {
#ifdef Q_OS_WIN
        setWindowFlags( Qt::Tool );
#endif
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
    _scroll->scrollToSelected();

    if( !modal ) {
        qf_enable();
        mainApp()->modelessOpened( this );
    }
    else
        exec();
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


