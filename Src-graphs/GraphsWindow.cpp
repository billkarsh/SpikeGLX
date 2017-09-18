
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "RunToolbar.h"
#include "GWLEDWidget.h"
#include "SView.h"
#include "SVGrafsM_Im.h"
#include "SVGrafsM_Ni.h"
#include "ColorTTLCtl.h"
#include "ConfigCtl.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QAction>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMessageBox>
#include <QSettings>


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
    layout->setMargin( 0 );

    QFrame  *line = new QFrame( handle );
    line->setFrameShape( QFrame::HLine );
    line->setFrameShadow( QFrame::Sunken );
    layout->addWidget( line );
}
#endif


GraphsWindow::GraphsWindow( const DAQ::Params &p )
    : QMainWindow(0), p(p), imW(0), niW(0), TTLCC(0)
{
// Install widgets

    addToolBar( tbar = new RunToolbar( this, p ) );
    statusBar()->addPermanentWidget( LED = new GWLEDWidget( p ) );

    QSplitter   *sp = new QSplitter;
    sp->setOrientation( Qt::Horizontal );   // streams left to right

// MS: Generalize, need some UI device to select streams
    if( p.im.enabled )
        sp->addWidget( new SViewM_Im( imW, this, p, 0 ) );

    if( p.ni.enabled )
        sp->addWidget( new SViewM_Ni( niW, this, p ) );

#if 0
    visibleGrabHandle( sp );
#endif

    setCentralWidget( sp );

// Equal size above and below

    if( sp->count() == 2 ) {

        sp->refresh();

        QList<int>  sz  = sp->sizes();
        int         av  = (sz[0] + sz[1])/2;

        sz[0] = sz[1] = av;
        sp->setSizes( sz );
    }

// Screen state

    restoreScreenState();

// Other helpers

    MGraphX *Xa = (imW ? imW->getTheX() : 0),
            *Xb = (niW ? niW->getTheX() : 0);

    TTLCC = new ColorTTLCtl( this, Xa, Xb, p );
}


// Note:
// Destructor order: {GW, toolbar, LED, niW}.
//
GraphsWindow::~GraphsWindow()
{
    saveScreenState();
    setUpdatesEnabled( false );
}


void GraphsWindow::eraseGraphs()
{
    if( imW )
        imW->eraseGraphs();

    if( niW )
        niW->eraseGraphs();
}


void GraphsWindow::imPutScans( vec_i16 &data, quint64 headCt )
{
    imW->putScans( data, headCt );
}


void GraphsWindow::niPutScans( vec_i16 &data, quint64 headCt )
{
    niW->putScans( data, headCt );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::updateRHSFlags()
{
    if( imW )
        imW->updateRHSFlags();

    if( niW )
        niW->updateRHSFlags();
}


bool GraphsWindow::remoteIsUsrOrderIm()
{
    return imW && imW->isUsrOrder();
}


bool GraphsWindow::remoteIsUsrOrderNi()
{
    return niW && niW->isUsrOrder();
}


void GraphsWindow::remoteSetRecordingEnabled( bool on )
{
    tbar->setRecordingEnabled( on );
    tbSetRecordingEnabled( on );
}


void GraphsWindow::remoteSetRunLE( const QString &name )
{
    tbar->setRunLE( name );
}


void GraphsWindow::setGateLED( bool on )
{
    LED->setGateLED( on );
}


void GraphsWindow::setTriggerLED( bool on )
{
    LED->setTriggerLED( on );
}


void GraphsWindow::blinkTrigger()
{
    LED->blinkTrigger();
}


void GraphsWindow::updateOnTime( const QString &s )
{
    tbar->updateOnTime( s );
}


void GraphsWindow::updateRecTime( const QString &s )
{
    tbar->updateRecTime( s );
}


void GraphsWindow::updateGT( const QString &s )
{
    tbar->updateGT( s );
}


void GraphsWindow::tbSetRecordingEnabled( bool checked )
{
    ConfigCtl*  cfg = mainApp()->cfgCtl();
    Run*        run = mainApp()->getRun();

    if( checked ) {

        QRegExp re("(.*)_[gG](\\d+)_[tT](\\d+)$");
        QString name    = tbar->getRunLE();
        int     g       = -1,
                t       = -1;

        if( name.contains( re ) ) {

            name    = re.cap(1);
            g       = re.cap(2).toInt();
            t       = re.cap(3).toInt();
        }

        if( name.compare( p.sns.runName, Qt::CaseInsensitive ) != 0 ) {

            // different run name...reset gt counters

            QString err;

            if( !cfg->validRunName( err, name, this, true ) ) {

                if( !err.isEmpty() )
                    QMessageBox::warning( this, "Run Name Error", err );

                tbar->setRecordingEnabled( false );
                return;
            }

            cfg->setRunName( name );
            run->grfUpdateWindowTitles();
            run->dfResetGTCounters();
        }
        else if( t > -1 ) {

            // Same run name...adopt given gt counters;
            // then obliterate user indices so on a
            // subsequent pause they don't get read again.

            run->dfForceGTCounters( g, t );
            tbar->setRunLE( name );
        }

        LED->setOnColor( QLED::Green );
    }
    else
        LED->setOnColor( QLED::Yellow );

    if( imW )
        imW->setRecordingEnabled( checked );

    if( niW )
        niW->setRecordingEnabled( checked );

    tbar->enableRunLE( !checked );
    run->dfSetRecordingEnabled( checked );
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
    if( e->modifiers() == Qt::ControlModifier ) {

        if( e->key() == Qt::Key_L ) {

            mainApp()->act.shwHidConsAct->trigger();
            e->accept();
            return;
        }
    }
    else if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
        return;
    }

    QWidget::keyPressEvent( e );
}


// Intercept the close box. Rather than close here,
// we ask the run manager if it's OK to close and if
// so, the run manager will delete us as part of the
// stopTask sequence.
//
void GraphsWindow::closeEvent( QCloseEvent *e )
{
    e->ignore();

    QMetaObject::invokeMethod(
        mainApp(), "file_AskStopRun",
        Qt::QueuedConnection );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void GraphsWindow::saveScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Graphs/geometry", saveGeometry() );
    settings.setValue( "WinLayout_Graphs/windowState", saveState() );
}


void GraphsWindow::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Graphs/geometry" ).toByteArray() )
        ||
        !restoreState(
        settings.value( "WinLayout_Graphs/windowState" ).toByteArray() ) ) {

        resize( 1280, 768 );
    }
}


