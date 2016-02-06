
#include "Util.h"
#include "MainApp.h"
#include "FramePool.h"
#include "GLGraph.h"

#include <QMessageBox>
#include <QFrame>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QTimer>




FramePool::FramePool()
    :   QObject(0), statusDlg(0), tPerGraph(0.0),
        maxGraphs(2*MAX_GRAPHS_PER_TAB)
{
// -----------
// These settings seem appropriate but make no apparent difference
//
//    fmt.setDepth( false );
//    fmt.setDepthBufferSize( 0 );
//    fmt.setStencilBufferSize( 0 );
//    fmt.setOverlay( false );
// -----------

// -----------
// Crucial settings
//
    fmt.setSwapInterval( 0 );   // disable syncing
// -----------

    frameParent     = new QWidget;
    creationTimer   = new QTimer( this );
    ConnectUI( creationTimer, SIGNAL(timeout()), this, SLOT(timerCreate1()) );
    creationTimer->start();
}


FramePool::~FramePool()
{
    if( creationTimer ) {
        delete creationTimer;
        creationTimer = 0;
    }

    if( statusDlg ) {
        delete statusDlg;
        statusDlg = 0;
    }

    if( frameParent ) {
        delete frameParent;
        frameParent = 0;
    }
}


bool FramePool::isStatusDlg( QObject *watched )
{
    return statusDlg && watched == statusDlg;
}


void FramePool::showStatusDlg()
{
    if( !statusDlg ) {

        MainApp *app = mainApp();

        statusDlg = new QMessageBox(
            QMessageBox::Information,
            "Precreating GL Graphs",
            QString(
            "Precreating %1 graphs, please wait...")
            .arg( maxGraphs ),
            QMessageBox::NoButton,
            (QWidget*)app->console() );

        statusDlg->addButton( "Quit", QMessageBox::RejectRole );
        ConnectUI( statusDlg, SIGNAL(rejected()), app, SLOT(quit()) ); // user requested
        statusDlg->show();
    }
}


// Front of the list has graphs, back not.
//
QFrame* FramePool::getFrame( bool nograph )
{
    QFrame  *f = 0;

    poolMtx.lock();

    if( !thePool.count() )
        create1( nograph );

    if( nograph ) {
        f = thePool.back();
        thePool.pop_back();
    }
    else {
        f = thePool.front();
        thePool.pop_front();
    }

    poolMtx.unlock();

    return f;
}


// The frames to put come in two flavors: with and w/o graphs.
// We'll push graph types to the front and empty ones in back.
//
void FramePool::allFramesToPool( const QVector<QFrame*> &vF )
{
    poolMtx.lock();

    for( int i = 0, n = vF.size(); i < n; ++i ) {

        QFrame  *f = vF[i];
        GLGraph *G = f->findChild<GLGraph*>();

        f->setParent( frameParent );

        if( G ) {
            G->detach();
            thePool.push_front( f );
        }
        else
            thePool.push_back( f );
    }

    poolMtx.unlock();
}


void FramePool::timerCreate1()
{
    if( !ready() ) {

        create1();

        if( ready() )
            allCreated();
    }
}


// Notes:
// Windows is very slow at reparenting a QGLWidget, but a common
// workaround is to make the QGLWidget a child of some interposed
// object, and reparent that.
//
void FramePool::create1( bool nograph )
{
    const double t0 = getTime();

// Create frame holding:
// - Save-checkbox above
// - graph below
// snug fit

    QFrame      *f		= new QFrame( frameParent );
    QVBoxLayout *bx_cg	= new QVBoxLayout( f );

    bx_cg->setSpacing( 0 );
    bx_cg->setContentsMargins( 0, 0, 0, 0 );

// Make/add checkbox

    QCheckBox	*chk = new QCheckBox( "Save enabled" );

    bx_cg->addWidget( chk );

// Make/add graph

    if( !nograph ) {

        QWidget     *tempParent = new QWidget;
        GLGraph     *G			= new GLGraph( "grf", tempParent );
        QVBoxLayout *sizer		= new QVBoxLayout( tempParent );

        sizer->setSpacing( 0 );
        sizer->setContentsMargins( 0, 0, 0, 0 );
        sizer->addWidget( G );

        bx_cg->addWidget( tempParent );
    }

// Update timing

    tPerGraph = tPerGraph * thePool.count();
    tPerGraph += (getTime() - t0);

    if( nograph )
        thePool.push_back( f );
    else
        thePool.push_front( f );

    tPerGraph /= thePool.count();
}


void FramePool::allCreated()
{
    if( creationTimer ) {

        Log()
            << "Pre-created "
            << thePool.count()
            << " GLGraphs, creation time "
            << tPerGraph * 1e3
            << " msec avg per graph, done.";

        if( creationTimer ) {
            delete creationTimer;
            creationTimer = 0;
        }

        if( statusDlg ) {
            delete statusDlg;
            statusDlg = 0;
        }
    }

    emit poolReady();
}


