
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphFetcher.h"
#include "GraphsWindow.h"
#include "RunToolbar.h"
#include "GWSelectWidget.h"
#include "GWLEDWidget.h"
#include "SView.h"
#include "SVGrafsM.h"
#include "ColorTTLCtl.h"
#include "SOCtl.h"
#include "ConfigCtl.h"

#include <QSplitter>
//#include <QVBoxLayout>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMessageBox>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

std::vector<QByteArray> GraphsWindow::vShankGeom;

/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QColor    NeuGraphBGColor( 0x20, 0x3c, 0x3c, 0xff ),
                LfpGraphBGColor( 0x55, 0x55, 0x7f, 0xff ),
                AuxGraphBGColor( 0x4f, 0x4f, 0x4f, 0xff ),
                DigGraphBGColor( 0x1f, 0x1f, 0x1f, 0xff );

/* ---------------------------------------------------------------- */
/* class GraphsWindow --------------------------------------------- */
/* ---------------------------------------------------------------- */

// Solution for invisible grab handle thanks to:
// David Walthall--
// <http://stackoverflow.com/questions/2545577/
// qsplitter-becoming-undistinguishable-between-qwidget-and-qtabwidget>
//
#if 0
static void visibleGrabHandle( QSplitter *sp )
{
    QSplitterHandle *handle = sp->handle( 1 );
    QVBoxLayout     *layout = new QVBoxLayout( handle );
    layout->setSpacing( 0 );
    layout->setContentsMargins( 0, 0, 0, 0 );

    QFrame  *line = new QFrame( handle );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    layout->addWidget( line );
}
#endif


GraphsWindow::GraphsWindow( const DAQ::Params &p, int igw )
    :   QMainWindow(0), p(p), tbar(0), LED(0),
        lW(0), rW(0), TTLCC(0), soctl(0), igw(igw)
{
// Install widgets

    if( igw == 0 ) {

        addToolBar( tbar = new RunToolbar( this, p ) );

        statusBar()->
            addPermanentWidget( LED = new GWLEDWidget( p ) );
    }

    statusBar()->
        insertPermanentWidget( 0, SEL = new GWSelectWidget( this, p ) );

    QSplitter   *sp = new QSplitter;
    sp->setOrientation( Qt::Horizontal );   // streams left to right

#if 0
    visibleGrabHandle( sp );
#endif

    setCentralWidget( sp );

// Screen state

    loadShankScreenState();
    restoreScreenState();

// Other helpers

    TTLCC = new ColorTTLCtl( this, p );

    if( igw == 0 && p.stream_nIM() ) {
        soctl = new SOCtl( p, this );
        ConnectUI( soctl, SIGNAL(closed(QWidget*)), mainApp(), SLOT(modelessClosed(QWidget*)) );
        soctl->init();
    }
}


// Note:
// Destructor order: {GW, toolbar, LED, niW}.
//
GraphsWindow::~GraphsWindow()
{
    saveShankScreenState();
    saveScreenState();
    setUpdatesEnabled( false );
}


void GraphsWindow::soSetChan( int gp, int ip, int ch )
{
    if( soctl )
        soctl->setChan( gp, ip, ch );
}


void GraphsWindow::soStopFetching()
{
    if( soctl )
        soctl->stopFetching();
}


void GraphsWindow::shankCtlWantGeom( int jpanel ) const
{
    SVGrafsM    *W = (jpanel == 2*igw ? lW : rW);
    W->shankCtlGeomSet( vShankGeom[jpanel], false );
}


void GraphsWindow::eraseGraphs()
{
    if( lW )
        lW->eraseGraphs();

    if( rW )
        rW->eraseGraphs();
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::initViews()
{
    Run     *run        = mainApp()->getRun();
    bool    wasPaused   = run->grfHardPause( true, igw ),
            recording   = run->dfIsRecordingEnabled();

    run->grfWaitPaused( igw );

    QSplitter   *sp = dynamic_cast<QSplitter*>(centralWidget());

    setUpdatesEnabled( false );

    installLeft( sp );

    if( installRight( sp ) ) {

        sp->setOrientation( SEL->grid() == 1 ? Qt::Horizontal : Qt::Vertical );
        sp->refresh();

        QList<int>  sz  = sp->sizes();
        int         sm  = sz[0] + sz[1],
                    av  = sm/2;

        sz[0] = av;
        sz[1] = sm - av;

        sp->setSizes( sz );

        if( SEL->grid() == 2 )
            eraseGraphs();
    }

    if( lW )
        lW->setRecordingEnabled( recording );

    if( rW )
        rW->setRecordingEnabled( recording );

    SEL->updateSelections();
    setUpdatesEnabled( true );
    setCursor( Qt::ArrowCursor );

// -------
// Helpers
// -------

    initColorTTL();
    initGFStreams();

    if( !wasPaused )
        run->grfHardPause( false, igw );
}


void GraphsWindow::updateRHSFlags()
{
    if( lW )
        lW->updateRHSFlags();

    if( rW )
        rW->updateRHSFlags();
}


void GraphsWindow::updateProbe( int ip, bool shankMap, bool chanMap )
{
    int _ip;

    if( lW && SEL->ljsip( _ip ) == 2 && _ip == ip )
        lW->updateProbe( shankMap, chanMap );

    if( rW && SEL->rjsip( _ip ) == 2 && _ip == ip )
        rW->updateProbe( shankMap, chanMap );
}


bool GraphsWindow::remoteIsUsrOrder( int js, int ip )
{
    int _ip;

    if( lW && SEL->ljsip( _ip ) == js && (!js || _ip == ip) )
        return lW->isUsrOrder();
    else if( rW && SEL->rjsip( _ip ) == js && (!js || _ip == ip) )
        return rW->isUsrOrder();

    return false;
}


void GraphsWindow::remoteSetRecordingEnabled( bool on )
{
    if( tbar )
        tbar->setRecordingEnabled( on );

    tbSetRecordingEnabled( on );
}


void GraphsWindow::remoteSetRunLE( const QString &name )
{
    if( tbar )
        tbar->setRunLE( name );
}


void GraphsWindow::remoteSetAnatomyPP( const QString &elems, int ip, int sk )
{
    int _ip;

    if( lW && SEL->ljsip( _ip ) == 2 && _ip == ip )
        lW->setAnatomyPP( elems, sk );

    if( rW && SEL->rjsip( _ip ) == 2 && _ip == ip )
        rW->setAnatomyPP( elems, sk );
}


void GraphsWindow::setGateLED( bool on )
{
    if( LED )
        LED->setGateLED( on );
}


void GraphsWindow::setTriggerLED( bool on )
{
    if( LED )
        LED->setTriggerLED( on );
}


void GraphsWindow::blinkTrigger()
{
    if( LED )
        LED->blinkTrigger();
}


void GraphsWindow::updateOnTime( const QString &s )
{
    if( tbar )
        tbar->updateOnTime( s );
}


void GraphsWindow::updateRecTime( const QString &s )
{
    if( tbar )
        tbar->updateRecTime( s );
}


void GraphsWindow::updateGT( const QString &s )
{
    if( tbar )
        tbar->updateGT( s );
}


void GraphsWindow::tbSetRecordingEnabled( bool checked )
{
    if( igw == 0 ) {

        ConfigCtl*  cfg = mainApp()->cfgCtl();
        Run*        run = mainApp()->getRun();

        if( checked ) {

            QString name = tbar->getRunLE();

            if( name.compare( p.sns.runName, Qt::CaseInsensitive ) != 0 ) {

                // Name changed or decorated

                QString err;

                if( !cfg->externSetsRunName( err, name, this ) ) {

                    if( !err.isEmpty() )
                        QMessageBox::warning( this, "Run Name Error", err );

                    tbar->setRecordingEnabled( false );
                    return;
                }

                tbar->setRunLE( p.sns.runName );
                run->grfUpdateWindowTitles();
                run->dfForceGTCounters( p.mode.initG, p.mode.initT );
            }

            LED->setOnColor( QLED::Green );
        }
        else
            LED->setOnColor( QLED::Yellow );

        tbar->enableRunLE( !checked );
        run->dfSetRecordingEnabled( checked );
    }

    if( lW )
        lW->setRecordingEnabled( checked );

    if( rW )
        rW->setRecordingEnabled( checked );
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool GraphsWindow::eventFilter( QObject *watched, QEvent *event )
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

    return QMainWindow::eventFilter( watched, event );
}


void GraphsWindow::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
        return;
    }

    QWidget::keyPressEvent( e );
}


// Detect show: pause if hidden.
//
void GraphsWindow::showEvent( QShowEvent *e )
{
    QWidget::showEvent( e );

    QMetaObject::invokeMethod(
        mainApp()->getRun(),
        "grfSoftPause",
        Qt::QueuedConnection,
        Q_ARG(bool, false),
        Q_ARG(int, igw) );
}


// Detect hide: pause if hidden.
//
void GraphsWindow::hideEvent( QHideEvent *e )
{
    QWidget::hideEvent( e );

    QMetaObject::invokeMethod(
        mainApp()->getRun(),
        "grfSoftPause",
        Qt::QueuedConnection,
        Q_ARG(bool, true),
        Q_ARG(int, igw) );
}


// Detect window minimized: pause graphing if so.
//
void GraphsWindow::changeEvent( QEvent *e )
{
    if( e->type() == QEvent::WindowStateChange ) {

        QWindowStateChangeEvent *wsce =
            static_cast<QWindowStateChangeEvent*>( e );

        if( wsce->oldState() & Qt::WindowMinimized ) {

            if( !(windowState() & Qt::WindowMinimized) ) {

                QMetaObject::invokeMethod(
                    mainApp()->getRun(),
                    "grfSoftPause",
                    Qt::QueuedConnection,
                    Q_ARG(bool, false),
                    Q_ARG(int, igw) );
            }
        }
        else {
            if( windowState() & Qt::WindowMinimized ) {

                QMetaObject::invokeMethod(
                    mainApp()->getRun(),
                    "grfSoftPause",
                    Qt::QueuedConnection,
                    Q_ARG(bool, true),
                    Q_ARG(int, igw) );
            }
        }
    }

    QWidget::changeEvent( e );
}


// Intercept the close box. Rather than close here,
// we ask the run manager if it's OK to close and if
// so, the run manager will delete us as part of the
// stopTask sequence.
//
void GraphsWindow::closeEvent( QCloseEvent *e )
{
    e->ignore();

    if( igw == 0 ) {

        QMetaObject::invokeMethod(
            mainApp(), "file_AskStopRun",
            Qt::QueuedConnection );
    }
    else
        mainApp()->getRun()->grfClose( this );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::installLeft( QSplitter *sp )
{
    if( !SEL->lChanged() )
        return;

    QWidget *w;
    int     ip, js  = SEL->ljsip( ip ),
            jpanel  = 2 * igw;

    if( sp->count() > 0 ) {

        bool    vis = lW->shankCtlGeomGet( vShankGeom[jpanel] );

        switch( js ) {
            case jsNI: w = new SViewM_Ni( lW, this, p, jpanel ); break;
            case jsOB: w = new SViewM_Ob( lW, this, p, ip, jpanel ); break;
            case jsIM: w = new SViewM_Im( lW, this, p, ip, jpanel ); break;
        }

        w = sp->replaceWidget( 0, w );

        if( w )
            delete w;

        lW->shankCtlGeomSet( vShankGeom[jpanel], vis );

        QMetaObject::invokeMethod(
            SEL, "setFocus",
            Qt::QueuedConnection,
            Q_ARG(bool, false) );
    }
    else {

        switch( js ) {
            case jsNI: w = new SViewM_Ni( lW, this, p, jpanel ); break;
            case jsOB: w = new SViewM_Ob( lW, this, p, ip, jpanel ); break;
            case jsIM: w = new SViewM_Im( lW, this, p, ip, jpanel ); break;
        }

        sp->addWidget( w );
        lW->shankCtlGeomSet( vShankGeom[jpanel], false );
    }
}


bool GraphsWindow::installRight( QSplitter *sp )
{
// ----------------
// Already have two
// ----------------

    if( sp->count() == 2 ) {

        if( SEL->grid() ) {

            if( SEL->rChanged() ) {

                QWidget *w;
                int     ip, js  = SEL->rjsip( ip ),
                        jpanel  = 2*igw + 1;
                bool    vis     = rW->shankCtlGeomGet( vShankGeom[jpanel] );

                switch( js ) {
                    case jsNI: w = new SViewM_Ni( rW, this, p, jpanel ); break;
                    case jsOB: w = new SViewM_Ob( rW, this, p, ip, jpanel ); break;
                    case jsIM: w = new SViewM_Im( rW, this, p, ip, jpanel ); break;
                }

                w = sp->replaceWidget( 1, w );

                if( w )
                    delete w;

                rW->shankCtlGeomSet( vShankGeom[jpanel], vis );

                QMetaObject::invokeMethod(
                    SEL, "setFocus",
                    Qt::QueuedConnection,
                    Q_ARG(bool, true) );
            }
            else
                return true;
        }
        else {
            delete sp->widget( 1 );
            rW = 0;
        }

        return false;
    }

// -----------
// Adding one?
// -----------

    if( SEL->grid() ) {

        QWidget *w;
        int     ip, js  = SEL->rjsip( ip ),
                jpanel  = 2*igw + 1;

        switch( js ) {
            case jsNI: w = new SViewM_Ni( rW, this, p, jpanel ); break;
            case jsOB: w = new SViewM_Ob( rW, this, p, ip, jpanel ); break;
            case jsIM: w = new SViewM_Im( rW, this, p, ip, jpanel ); break;
        }

        sp->addWidget( w );
        rW->shankCtlGeomSet( vShankGeom[jpanel], false );

        return true;
    }

// ----------
// Not adding
// ----------

    return false;
}


void GraphsWindow::initColorTTL()
{
    const AIQ   *Qa = 0, *Qb = 0;
    MGraphX     *Xa = 0, *Xb = 0;
    Run         *run = mainApp()->getRun();
    int         ljs = -1, lip = 0,
                rjs = -1, rip = 0;

    if( lW ) {
        ljs = SEL->ljsip( lip );
        Qa  = run->getQ( ljs, lip );
        Xa  = lW->getTheX();
    }

    if( rW ) {
        rjs = SEL->rjsip( rip );
        Qb  = run->getQ( rjs, rip );
        Xb  = rW->getTheX();
    }

    TTLCC->setClients( ljs, lip, Qa, Xa, rjs, rip, Qb, Xb );
}


void GraphsWindow::initGFStreams()
{
    std::vector<GFStream>   gfs;

    if( lW )
        gfs.push_back( GFStream( SEL->lStream(), lW ) );

    if( rW )
        gfs.push_back( GFStream( SEL->rStream(), rW ) );

    mainApp()->getRun()->grfSetStreams( gfs, igw );
}


void GraphsWindow::loadShankScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    int numPanels = 2 * (1 + igw);

    if( int(vShankGeom.size()) < numPanels )
        vShankGeom.resize( numPanels );

    for( int jpanel = 2*igw; jpanel <= 2*igw + 1; ++jpanel ) {

        vShankGeom[jpanel] =
            settings.value(
                QString("WinLayout_ShankView_Panel%1/geometry")
                .arg( jpanel ) ).toByteArray();
    }
}


void GraphsWindow::saveShankScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    for( int jpanel = 2*igw; jpanel <= 2*igw + 1; ++jpanel ) {

        SVGrafsM    *W = (jpanel == 2*igw ? lW : rW);

        if( W )
            W->shankCtlGeomGet( vShankGeom[jpanel] );

        settings.setValue(
            QString("WinLayout_ShankView_Panel%1/geometry")
            .arg( jpanel ), vShankGeom[jpanel] );
    }
}


void GraphsWindow::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
            settings.value(
                QString("WinLayout_Graphs%1/geometry").arg( igw ) )
                .toByteArray() )
        ||
        !restoreState(
            settings.value(
                QString("WinLayout_Graphs%1/windowState").arg( igw ) )
                .toByteArray() ) ) {

        resize( 1280, 768 );
    }
}


void GraphsWindow::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue(
        QString("WinLayout_Graphs%1/geometry").arg( igw ), saveGeometry() );
    settings.setValue(
        QString("WinLayout_Graphs%1/windowState").arg( igw ), saveState() );
}


