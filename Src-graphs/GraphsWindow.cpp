
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "RunToolbar.h"
#include "GWLEDWidget.h"
#include "SView.h"
#include "SVGrafsM_Im.h"
#include "SVGrafsG_Ni.h"
#include "ConfigCtl.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QAction>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMessageBox>




/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QColor    NeuGraphBGColor( 0x2f, 0x4f, 0x4f, 0xff ),
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


GraphsWindow::GraphsWindow( DAQ::Params &p )
    : QMainWindow(0), p(p), imW(0), niW(0)
{
    resize( 1024, 768 );

// Install widgets

    addToolBar( tbar = new RunToolbar( this, p ) );
    statusBar()->addPermanentWidget( LED = new GWLEDWidget( p ) );

    QSplitter   *sp = new QSplitter;
    sp->setOrientation( Qt::Vertical );

    if( p.im.enabled )
        sp->addWidget( new SViewM_Im( imW, this, p ) );

    if( p.ni.enabled )
        sp->addWidget( new SViewG_Ni( niW, this, p ) );

    visibleGrabHandle( sp );
    setCentralWidget( sp );

// Equal size above and below

    if( sp->count() == 2 ) {

        sp->refresh();

        QList<int>  sz  = sp->sizes();
        int         ht  = (sz[0] + sz[1])/2;

        sz[0] = sz[1] = ht;
        sp->setSizes( sz );
    }
}


// Note:
// Destructor order: {GW, toolbar, LED, niW}.
//
GraphsWindow::~GraphsWindow()
{
    setUpdatesEnabled( false );
}


void GraphsWindow::showHideSaveChks()
{
    if( imW )
        imW->showHideSaveChks();

    if( niW )
        niW->showHideSaveChks();
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


void GraphsWindow::remoteSetTrgEnabled( bool on )
{
    tbar->setTrigEnable( on );
    tbSetTrgEnable( on );
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


void GraphsWindow::tbSetTrgEnable( bool checked )
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

                tbar->setTrigEnable( false );
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

    tbar->enableRunLE( !checked );
    run->dfSetTrgEnabled( checked );

// update graph checks

    if( imW )
        imW->enableAllChecks( !checked );

    if( niW )
        niW->enableAllChecks( !checked );
}


// Force Ctrl+A events to be treated as 'show AO-dialog',
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
    mainApp()->file_AskStopRun();
}


