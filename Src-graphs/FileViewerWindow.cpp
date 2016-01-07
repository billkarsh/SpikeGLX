
#include "ui_FVW_OptionsDialog.h"

#include "Pixmaps/apply_all.xpm"
#include "Pixmaps/close_but_16px.xpm"

#include "FileViewerWindow.h"
#include "SpikeGL.h"
#include "Util.h"
#include "MainApp.h"
#include "GLGraph.h"
#include "Biquad.h"
#include "ExportCtl.h"
#include "ClickableLabel.h"
#include "Subset.h"

#include <QProgressDialog>
#include <QKeyEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QScrollBar>
#include <QScrollArea>
#include <QCheckBox>
#include <QSlider>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QPixmap>
#include <QCursor>
#include <QAction>
#include <QSettings>
#include <math.h>

/* ---------------------------------------------------------------- */
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QString FileViewerWindow::colorSchemeNames[] = {
    "Ice", "Fire", "Green", "BlackWhite", "Classic"
};

const QString FileViewerWindow::viewModeNames[] = {
    "Tiled", "Stacked", "Stacked Large", "Stacked Huge"
};

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TaggableLabel : public ClickableLabel
{
private:
    void    *tagPtr;
public:
    TaggableLabel( QWidget *parent, Qt::WindowFlags f = 0 )
    : ClickableLabel( parent, f ), tagPtr(0) {}
    void setTag( void *ptr )    {tagPtr = ptr;}
    void *tag() const           {return tagPtr;}
};

/* ---------------------------------------------------------------- */
/* FileViewerWindow ----------------------------------------------- */
/* ---------------------------------------------------------------- */

FileViewerWindow::FileViewerWindow()
    :   QMainWindow(0), tMouseOver(-1.0), yMouseOver(-1.0),
        pscale(1), hipass(0), igMouseOver(-1), didLayout(false),
        dragging(false)
{
    initDataIndepStuff();

    setAttribute( Qt::WA_DeleteOnClose, false );
    show();
}


FileViewerWindow::~FileViewerWindow()
{
    saveSettings();

    delete framePoolParent;

    if( hipass )
        delete hipass;
}


bool FileViewerWindow::viewFile( const QString &fname, QString *errMsg )
{
    pos             = 0;
    pscale          = 1;
    igMaximized     = -1;
    igSelected      = -1;
    dragL           = -1;
    dragR           = -1;
    tMouseOver      = -1.0;
    yMouseOver      = -1.0;
    igMouseOver     = -1;

// --------
// Get data
// --------

    if( !openFile( fname, errMsg ) )
        return false;

    loadSettings();

// --------------------
// Data-dependent inits
// --------------------

    applyStyles();
    setToolbarRanges();
    setSliderRanges();
    updateSliderTexts();
    initHipass();

// --------------------------
// Manage previous array data
// --------------------------

    if( !cacheFrames_killActions( errMsg ) )
        return false;

// ---------------------------
// Resize arrays for this file
// ---------------------------

    int nG = dataFile.numChans();

    grfFrames.resize( nG );
    grf.resize( nG );
    grfParams.resize( nG );
    grfActShowHide.resize( nG );
    order2ig.resize( nG );
    ig2AcqChan.resize( nG );

// -------------------
// Init new array data
// -------------------

    grfVisBits.fill( true, nG );

    if( !initFrames_initActions( errMsg ) )
        return false;

// ---------------
// Initialize view
// ---------------

    selectGraph( 0 );

    if( sav.sortUserOrder )
        view_sortUsr();
    else
        view_sortAcq();

    updateGraphs();

    return true;
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void FileViewerWindow::resizeEvent( QResizeEvent *e )
{
    QMainWindow::resizeEvent( e );

    if( didLayout && sav.viewMode > Tiled )
        QTimer::singleShot( 500, this, SLOT(layoutGraphs()) );
}


bool FileViewerWindow::eventFilter( QObject *obj, QEvent *e )
{
    if( (obj == scrollArea || obj == slider)
        && e->type() == QEvent::KeyPress ) {

        QKeyEvent   *keyEvent = static_cast<QKeyEvent*>(e);
        double      newPos = -1.0; // illegal

        switch( keyEvent->key() ) {

            case Qt::Key_Home:
                newPos = 0;
                break;
            case Qt::Key_End:
                newPos = maxPos();
                break;
            case Qt::Key_Left:
            case Qt::Key_Up:
                newPos = qMax( 0.0, pos - sav.fArrowKey * nScansPerGraph() );
                break;
            case Qt::Key_Right:
            case Qt::Key_Down:
                newPos = pos + sav.fArrowKey * nScansPerGraph();
                break;
            case Qt::Key_PageUp:
                newPos = qMax( 0.0, pos - sav.fPageKey * nScansPerGraph() );
                break;
            case Qt::Key_PageDown:
                newPos = pos + sav.fPageKey * nScansPerGraph();
                break;
        }

        if( newPos >= 0.0 ) {

            QDoubleSpinBox  *PS =
            sliderGrp->findChild<QDoubleSpinBox*>( "possb" );

            if( PS ) {
                PS->setValue( newPos );
                return true;
            }
        }
    }

    return QMainWindow::eventFilter( obj, e );
}


void FileViewerWindow::closeEvent( QCloseEvent *e )
{
    if( queryCloseOK() ) {

        QWidget::closeEvent( e );

        if( e->isAccepted() ) {
            mainApp()->win.removeFromMenu( this );
            this->deleteLater();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Menu items ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::file_Open()
{
    QMetaObject::invokeMethod(
        mainApp(), "fileOpen",
        Qt::QueuedConnection,
        Q_ARG(FileViewerWindow*, this) );
}


void FileViewerWindow::file_Options()
{
    QDialog                 dlg;
    Ui::FVW_OptionsDialog   ui;

    ui.setupUi( &dlg );
    ui.arrowSB->setValue( sav.fArrowKey );
    ui.pageSB->setValue( sav.fPageKey );

    if( QDialog::Accepted == dlg.exec() ) {

        sav.fArrowKey   = ui.arrowSB->value();
        sav.fPageKey    = ui.pageSB->value();
        saveSettings();
    }
}


void FileViewerWindow::color_SelectScheme()
{
    QAction     *a          = dynamic_cast<QAction*>(sender());
    ColorScheme oldScheme   = sav.colorScheme;

    if( !a )
        return;

    for( int is = 0; is < (int)N_ColorScheme; ++is ) {

        if( a == colorSchemeActions[is] ) {

            sav.colorScheme = (ColorScheme)is;
            colorSchemeActions[is]->setChecked( true );
        }
        else
            colorSchemeActions[is]->setChecked( false );
    }

    if( sav.colorScheme != oldScheme ) {

        saveSettings();

        for( int ig = 0, nG = grf.size(); ig < nG; ++ig )
            applyColorScheme( ig );

        updateGraphs();
    }
}


void FileViewerWindow::view_sortAcq()
{
    sortAcqAct->setChecked( true );
    sortUsrAct->setChecked( false );
    sav.sortUserOrder = false;
    saveSettings();
    chanMap.defaultOrder( order2ig );
    layoutGraphs();
}


void FileViewerWindow::view_sortUsr()
{
    sortAcqAct->setChecked( false );
    sortUsrAct->setChecked( true );
    sav.sortUserOrder = true;
    saveSettings();
    chanMap.userOrder( order2ig );
    layoutGraphs();
}


void FileViewerWindow::view_SelectMode()
{
    QAction     *a      = dynamic_cast<QAction*>(sender());
    ViewMode    oldMode = sav.viewMode;

    if( !a )
        return;

    for( int im = 0; im < (int)N_ViewMode; ++im ) {

        if( a == viewModeActions[im] ) {

            sav.viewMode = (ViewMode)im;
            viewModeActions[im]->setChecked( true );
        }
        else
            viewModeActions[im]->setChecked( false );
    }

    if( sav.viewMode != oldMode ) {

        saveSettings();
        layoutGraphs();
    }
}


void FileViewerWindow::channels_ShowAll()
{
    hideCloseLabel();

    igMaximized = -1;

    int nG = grf.size();

    grfVisBits.fill( true, nG );

    for( int ig = 0; ig < nG; ++ig ) {

        grfActShowHide[ig]->setChecked( true );
        grfFrames[ig]->show();
    }

    layoutGraphs();
}

/* ---------------------------------------------------------------- */
/* Toolbar -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::scrollToSelected()
{
    if( igSelected > -1 && sav.viewMode != Tiled )
        scrollArea->ensureWidgetVisible( grfFrames[igSelected] );
}


void FileViewerWindow::setXScale( double d )
{
    sav.xSpan = d;
    saveSettings();

    updateNDivText();

    setSliderRanges();
    updateSliderTexts();

    updateGraphs();
}


void FileViewerWindow::setYScale( double d )
{
    if( igSelected < 0 )
        return;

    grf[igSelected]->getX()->yscale = d;

    updateNDivText();
    updateGraphs();
}


void FileViewerWindow::setNDivs( int n )
{
    sav.nDivs = n;
    saveSettings();

    updateNDivText();

    for( int ig = 0, nG = grf.size(); ig < nG; ++ig ) {
        GLGraphX    *X = grf[ig]->getX();
        X->setHGridLines( sav.nDivs );
        X->setVGridLines( sav.nDivs );
    }

    updateGraphs();
}


void FileViewerWindow::setMuxGain( double d )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].gain = d;

    updateNDivText();
    printStatusMessage();

    updateGraphs();
}


void FileViewerWindow::hpfChk( bool b )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].filter300Hz = b;

    updateGraphs();
}


void FileViewerWindow::dcfChk( bool b )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].dcFilter = b;

    updateGraphs();
}


void FileViewerWindow::applyAll()
{
    if( igSelected < 0 )
        return;

    GraphParams &P      = grfParams[igSelected];
    double      yScale  = grf[igSelected]->getX()->yscale;

    if( P.niType == 0 )
        sav.ySclNeu = yScale;
    else if( P.niType == 1 )
        sav.ySclAux = yScale;

    if( P.niType < 2 )
        saveSettings();

    for( int ig = 0, nG = grfParams.size(); ig < nG; ++ig ) {

        if( ig != igSelected && grfParams[ig].niType == P.niType ) {

            grf[ig]->getX()->yscale  = yScale;
            grfParams[ig]            = P;
        }
    }

    if( didLayout )
        updateGraphs();
}

/* ---------------------------------------------------------------- */
/* Slider --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::sliderPosSBChanged( double p )
{
    QDoubleSpinBox  *SC = sliderGrp->findChild<QDoubleSpinBox*>( "secsb" );
    QSlider         *SR = sliderGrp->findChild<QSlider*>( "slider" );

    setFilePos64( p );

    SC->blockSignals( true );
    SR->blockSignals( true );

    SC->setValue( timeFromPos( pos ) );
    SR->setValue( pos / pscale );

    SC->blockSignals( false );
    SR->blockSignals( false );

    updateSliderTexts();
}


void FileViewerWindow::sliderSecSBChanged( double s )
{
    QDoubleSpinBox  *PS = sliderGrp->findChild<QDoubleSpinBox*>( "possb" );
    QSlider         *SR = sliderGrp->findChild<QSlider*>( "slider" );

    setFilePos64( posFromTime( s ) );

    PS->blockSignals( true );
    SR->blockSignals( true );

    PS->setValue( pos );
    SR->setValue( pos / pscale );

    PS->blockSignals( false );
    SR->blockSignals( false );

    updateSliderTexts();
}


void FileViewerWindow::sliderChanged( int i )
{
    QDoubleSpinBox  *PS = sliderGrp->findChild<QDoubleSpinBox*>( "possb" );
    QDoubleSpinBox  *SC = sliderGrp->findChild<QDoubleSpinBox*>( "secsb" );

    setFilePos64( i * pscale );

    PS->blockSignals( true );
    SC->blockSignals( true );

    PS->setValue( pos );
    SC->setValue( timeFromPos( pos ) );

    PS->blockSignals( false );
    SC->blockSignals( false );

    updateSliderTexts();
}

/* ---------------------------------------------------------------- */
/* CloseLabel ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::hideCloseLabel()
{
    closeLbl->hide();
    closeLbl->setTag( 0 );
    hideCloseTimer->stop();
}


void FileViewerWindow::hideCloseTimeout()
{
// underMouse() very unreliable
//    if( !closeLbl->underMouse() )
//        hideCloseLabel();

    if( !closeLbl->geometry().contains( QCursor::pos() ) )
        hideCloseLabel();
}

/* ---------------------------------------------------------------- */
/* Export --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::doExport()
{
// communicate settings across sessions/windows
    STDSETTINGS( settings, "cc_fileviewer" );
    exportCtl->loadSettings( settings );

    exportCtl->initDataFile( dataFile );
    exportCtl->initGrfRange( grfVisBits, igMouseOver );
    exportCtl->initScnRange( dragL, dragR );
    exportCtl->showExportDlg( this );
}

/* ---------------------------------------------------------------- */
/* Mouse ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Position readout and selection tracking.
//
// x in range [0,1].
// y in range [-1,1].
//
void FileViewerWindow::mouseOverGraph( double x, double y )
{
// ------------
// Which graph?
// ------------

    GLGraph *G = dynamic_cast<GLGraph*>(sender());

    if( !G ) {
        igMouseOver = -1;
        return;
    }

    igMouseOver = G->getX()->num;

    if( igMouseOver < 0 || igMouseOver >= (int)grf.size() ) {
        statusBar()->clearMessage();
        return;
    }

// ----------------
// Position readout
// ----------------

    tMouseOver = timeFromPos( pos ) + x * sav.xSpan;
    yMouseOver = scalePlotValue( y );

// ------------------
// Selection tracking
// ------------------

// Note:
// -----
// Each graph sets tracking true (GLGraph::attach) so we get
// mouseMove messages from the first click through the final
// mouse-up, all from the same graph, by the way.

    if( dragging ) {

        qint64  p = qBound( 0LL, posFromTime( tMouseOver ), dfCount - 1 );

        if( p < pos )
            setSliderPos( p );
        else if( p > pos + nScansPerGraph() )
            setSliderPos( qMax( 0LL, p - nScansPerGraph() ) );

        if( p >= dragAnchor ) {
            dragL   = dragAnchor;
            dragR   = p;
        }
        else {
            dragL   = p;
            dragR   = dragAnchor;
        }

        updateGraphs();
    }

    printStatusMessage();
}


void FileViewerWindow::clickGraph( double x, double y )
{
    mouseOverGraph( x, y );
    selectGraph( igMouseOver );

    dragging = QApplication::keyboardModifiers() & Qt::ControlModifier;

    if( dragging ) {

        dragAnchor  =
        dragL       =
        dragR       = pos + x * nScansPerGraph();
        dragging    = true;

        updateGraphs();
    }
}


void FileViewerWindow::dragDone()
{
    if( dragging ) {

        dragging = false;

        if( dragR <= dragL ) {
            dragL = dragR = -1;
            updateGraphs();
        }
    }
}


void FileViewerWindow::dblClickGraph( double x, double y )
{
    clickGraph( x, y );
    toggleMaximized();
}


void FileViewerWindow::mouseOverLabel( int x, int y )
{
    if( dragging )
        return;

    GLGraph *G = dynamic_cast<GLGraph*>(sender());

    if( !G || !didLayout || igMaximized > -1 )
        return;

    QSize   sz = closeLbl->size();

    if( x > G->width() - sz.width() && y < sz.height() ) {

        if( grfVisBits.count( false ) + 1 >= grf.size() )
            return; // leave at least one showing

        hideCloseTimer->stop();

        QPoint  p( G->width() - sz.width(), 0 );
        p = G->mapToGlobal( p );

        closeLbl->move( p.x(), p.y() );
        closeLbl->show();
        closeLbl->setTag( G );

        hideCloseTimer->start();
    }
}

/* ---------------------------------------------------------------- */
/* Actions -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::menuShowHideGraph()
{
    QAction *s = dynamic_cast<QAction*>(sender());

    if( !s )
        return;

    bool    ok = false;
    int     ig = s->objectName().toInt( &ok );

    if( !ok )
        return;

    if( ig >= 0 && ig < grf.size() ) {

        if( grfVisBits.testBit( ig ) )
            hideGraph( ig );
        else
            showGraph( ig );
    }
}


void FileViewerWindow::cursorHere( QPoint p )
{
    QCursor::setPos( p );
}


void FileViewerWindow::clickedCloseLbl()
{
    GLGraph *G = reinterpret_cast<GLGraph*>(closeLbl->tag());

    if( G )
        hideGraph( G->getX()->num );

    hideCloseLabel();

// -------------------------
// Force mouse move event(s)
// -------------------------

// This sequence causes status data to update after a graph
// is closed, yet the cursor stays still overall. Moreover,
// this let's the user hover over the close label and click
// a run of graphs closed. Unfortunately, setPos( curpos )
// is effectively a noop, or we'd just do that.

    // Here's where we are
    QPoint  p = QCursor::pos();

    // Immed: Jog to the left (cause a move event)
    QCursor::setPos( p.x() - 1, p.y() );

    // After: Jog back again (cause another move event)
    QMetaObject::invokeMethod(
        this, "cursorHere",
        Qt::QueuedConnection,
        Q_ARG(QPoint, p) );
}

/* ---------------------------------------------------------------- */
/* Timer targets -------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::layoutGraphs()
{
    if( igMaximized > -1 )
        return;

    const int padding   = 2;
    const int nG        = grfFrames.size();

    didLayout = false;

    hideCloseLabel();

    graphParent->hide();

// Fresh layout

    if( graphParent->layout() )
        delete graphParent->layout();

// Hide/show frames.
// Sizing the visible set requires them to be showing.

    for( int ig = 0; ig < nG; ++ig )
        grfFrames[ig]->setVisible( grfVisBits.testBit( ig ) );

// Layout

    if( sav.viewMode == Tiled ) {

        // As a Grid

        scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        scrollArea->setWidgetResizable( true );
        updateGeometry();

        QGridLayout *L = new QGridLayout( graphParent );

        L->setSpacing( padding );
        graphParent->setLayout( L );

        int nV  = grfVisBits.count( true ),
            nR  = int(sqrtf( nV )),
            nC  = nR;

        while( nR * nC < nV ) {

            if( nR > nC )
                ++nC;
            else
                ++nR;
        }

        for( int r = 0, is = -1, nAdded = 0; r < nR; ++r ) {

            for( int c = 0; c < nC; ++c ) {

                for(;;) { // find next visible graph

                    if( ++is >= nG )
                        goto insert_done;

                    int ig = order2ig[is];

                    if( grfVisBits.testBit( ig ) ) {

                        L->addWidget( grfFrames[ig], r, c );

                        if( ++nAdded >= nV )
                            goto insert_done;

                        break;
                    }
                }
            }
        }
    }
    else {

        // As a vert list

        scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        scrollArea->setWidgetResizable( false );
        updateGeometry();

        setStackSizing();

        QVBoxLayout *l = new QVBoxLayout( graphParent );

        l->setSpacing( padding );
        l->setContentsMargins( 0, 0, 0, 0 );
        graphParent->setLayout( l );

        for( int is = 0; is < nG; ++is ) {

            if( grfVisBits.testBit( order2ig[is] ) )
                l->addWidget( grfFrames[order2ig[is]] );
        }
    }

insert_done:
    graphParent->show();
    didLayout = true;
    scrollToSelected();
    updateGraphs();
}

/* ---------------------------------------------------------------- */
/* Data-independent inits ----------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::initMenus()
{
//#ifdef Q_OS_MACX
//	// shared menu bar on OSX
//	QMenuBar *mb = mainApp()->console()->menuBar();
//#else
//	// otherwise window-specific menu bar
    QMenuBar    *mb = menuBar();
//#endif

    QMenu   *m;

    m = mb->addMenu( "&File" );
    m->addAction( "&Open...", this, SLOT(file_Open()) );
    m->addAction( "O&ptions...", this, SLOT(file_Options()) );

    m = mb->addMenu( "Color &Scheme" );
    for( int is = 0; is < (int)N_ColorScheme; ++is ) {
        QAction *a;
        a = m->addAction( colorSchemeNames[is], this, SLOT(color_SelectScheme()) );
        a->setCheckable( true );
        colorSchemeActions[is] = a;
    }

    m = mb->addMenu( "&View Mode" );
    sortAcqAct = m->addAction( "Sort Graphs in Acq Order", this, SLOT(view_sortAcq()) );
    sortAcqAct->setCheckable( true );
    sortAcqAct->setChecked( true );
    sortUsrAct = m->addAction( "Sort Graphs in User Order", this, SLOT(view_sortUsr()) );
    sortUsrAct->setCheckable( true );
    m->addSeparator();
    for( int im = 0; im < (int)N_ViewMode; ++im ) {
        QAction *a;
        a = m->addAction( viewModeNames[im], this, SLOT(view_SelectMode()) );
        a->setCheckable( true );
        viewModeActions[im] = a;
    }
    viewMenu = m;

    m = mb->addMenu( "&Channels" );
    m->addAction( "Show All", this, SLOT(channels_ShowAll()) );
    m->addSeparator();
    channelsMenu = m;
}


void FileViewerWindow::initToolbar()
{
    QDoubleSpinBox  *S;
    QSpinBox        *V;
    QCheckBox       *C;
    QLabel          *L;

    toolBar = addToolBar( "Graph Controls" );

// Selected

    L = new ClickableLabel( "MN0C0;000", toolBar );
    L->setObjectName( "namelbl" );
    L->setToolTip( "Selected graph (click to find)" );
    L->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( L, SIGNAL(clicked()), this, SLOT(scrollToSelected()) );
    toolBar->addWidget( L );

// X-Scale

    toolBar->addSeparator();

    L = new QLabel( "Secs", toolBar );
    toolBar->addWidget( L );

    S = new QDoubleSpinBox( toolBar );
    S->setObjectName( "xscalesb" );
    S->setDecimals( 4 );
    S->setRange( 0.0001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(setXScale(double)) );
    toolBar->addWidget( S );

// YScale

    L = new QLabel( "YScale", toolBar );
    toolBar->addWidget( L );

    S = new QDoubleSpinBox( toolBar );
    S->setObjectName( "yscalesb" );
    S->setRange( 0.01, 100.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(setYScale(double)) );
    toolBar->addWidget( S );

// NDivs

    toolBar->addSeparator();

    L = new QLabel( "NDivs", toolBar );
    toolBar->addWidget( L );

    V = new QSpinBox( toolBar );
    V->setObjectName( "ndivssb" );
    V->setMinimum( 0 );
    V->setMaximum( 10 );
    ConnectUI( V, SIGNAL(valueChanged(int)), this, SLOT(setNDivs(int)) );
    toolBar->addWidget( V );

    L = new QLabel( " Each (-,-)", toolBar );
    L->setObjectName( "divlbl" );
    toolBar->addWidget( L );

// Gain

    toolBar->addSeparator();

    L = new QLabel( "Gain", toolBar );
    toolBar->addWidget( L );

    S = new QDoubleSpinBox( toolBar );
    S->setObjectName( "gainsb" );
    S->setDecimals( 3 );
    S->setRange( 0.001, 1e6 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(setMuxGain(double)) );
    toolBar->addWidget( S );

// Hipass

    toolBar->addSeparator();

    C = new QCheckBox( "Filter <300Hz", toolBar );
    C->setObjectName( "hpchk" );
    ConnectUI( C, SIGNAL(clicked(bool)), this, SLOT(hpfChk(bool)) );
    toolBar->addWidget( C );

    C = new QCheckBox( "DC Filter", toolBar );
    C->setObjectName( "dcchk" );
    ConnectUI( C, SIGNAL(clicked(bool)), this, SLOT(dcfChk(bool)) );
    toolBar->addWidget( C );

// Apply all

    toolBar->addSeparator();

    toolBar->addAction(
        QIcon( QPixmap( apply_all_xpm ) ),
        "Apply current graph settings to all graphs of like type",
        this, SLOT(applyAll()) );
}


// The 'slider group' is a trio of linked controls, with range texts:
//
// [pos^] to X (of Y) [secs^] to X (of Y) [---|------]
//
// The pos spinner allows single scan advance.
// The sec spinner allows millisecond advance.
// The slider allows chunked advance depending on pscale factor.
//
// User changes to one must update the other two, and any change
// requires updating the texts.
//
QWidget *FileViewerWindow::initSliderGrp()
{
    QDoubleSpinBox  *S;
    QLabel          *L;

    sliderGrp = new QWidget;

    QHBoxLayout *HL = new QHBoxLayout( sliderGrp );

// 'File position'

    L = new QLabel( "File position: ", sliderGrp );
    HL->addWidget( L );

// pos (scans)

    L = new QLabel( "scans", sliderGrp );
    HL->addWidget( L, 0, Qt::AlignRight );

    S = new QDoubleSpinBox( sliderGrp );
    S->setObjectName( "possb" );
    S->setDecimals( 0 );
    S->setSingleStep( 1.0 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(sliderPosSBChanged(double)) );
    HL->addWidget( S, 0, Qt::AlignLeft );

    L = new QLabel( "to X (of Y)", sliderGrp );
    L->setObjectName( "poslbl" );
    HL->addWidget( L, 0, Qt::AlignLeft );

// secs

    HL->addSpacing( 5 );

    L = new QLabel( "secs", sliderGrp );
    HL->addWidget( L, 0, Qt::AlignRight );

    S = new QDoubleSpinBox( sliderGrp );
    S->setObjectName( "secsb" );
    S->setDecimals( 3 );
    S->setSingleStep( 0.001 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(sliderSecSBChanged(double)) );
    HL->addWidget( S, 0, Qt::AlignLeft );

    L = new QLabel( "to X (of Y)", sliderGrp );
    L->setObjectName( "seclbl" );
    HL->addWidget( L, 0, Qt::AlignLeft );

// slider

    HL->addSpacing( 5 );

    slider = new QSlider( Qt::Horizontal, sliderGrp );
    slider->setObjectName( "slider" );
    slider->setMinimum( 0 );
    slider->setMaximum( 1000 );
    slider->installEventFilter( this );
    ConnectUI( slider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)) );
    HL->addWidget( slider );

    return sliderGrp;
}


void FileViewerWindow::initCloseLbl()
{
    QPixmap pm( close_but_16px );

    closeLbl = new TaggableLabel( graphParent, Qt::FramelessWindowHint | Qt::Popup );
    closeLbl->setPixmap( pm );
    closeLbl->resize( pm.size() );
    closeLbl->setTag( 0 );
    closeLbl->setToolTip( "Hide this graph" );
    closeLbl->setCursor( Qt::ArrowCursor );
    closeLbl->hide();
    closeLbl->move( -100, -100 );
    ConnectUI( closeLbl, SIGNAL(clicked()), this, SLOT(clickedCloseLbl()) );

    hideCloseTimer = new QTimer( this );
    hideCloseTimer->setInterval( 250 );
    ConnectUI( hideCloseTimer, SIGNAL(timeout()), this, SLOT(hideCloseTimeout()) );
}


void FileViewerWindow::initExport()
{
    exportAction = new QAction( "Export...", this );
    ConnectUI( exportAction, SIGNAL(triggered()), this, SLOT(doExport()) );

    exportCtl = new ExportCtl( this );
}


void FileViewerWindow::initDataIndepStuff()
{
    setCursor( Qt::ArrowCursor );
    resize( 1050, 640 );

// Top-most controls

    initMenus();
    initToolbar();

// CentralWidget gets vertical layout of two items:
// - graphs
// - 'slider' (position controls)

    QWidget     *cw = new QWidget;
    QVBoxLayout *VL = new QVBoxLayout( cw );
    setCentralWidget( cw );

    graphParent     = new QWidget;
    framePoolParent = new QWidget;  // unowned: manually delete

    scrollArea = new QScrollArea( cw );
    scrollArea->installEventFilter( this );
    scrollArea->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    scrollArea->setWidget( graphParent );
    scrollArea->setWidgetResizable( true );
    VL->addWidget( scrollArea, 1 );
    VL->addWidget( initSliderGrp() );

// Status bar below

    statusBar();

// Other one-time inits

    initCloseLbl();
    initExport();
}

/* ---------------------------------------------------------------- */
/* Data-dependent inits ------------------------------------------- */
/* ---------------------------------------------------------------- */

bool FileViewerWindow::openFile( const QString &fname, QString *errMsg )
{
    if( errMsg )
        errMsg->clear();

    QString fname_no_path = QFileInfo( fname ).fileName();

    if( !dataFile.openForRead( fname ) ) {

        QString err = QString("Error opening '%1'.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    if( !(dfCount = dataFile.scanCount()) ) {

        QString err = QString("'%1' is empty.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    chanMap = dataFile.chanMap();

    if( !chanMap.e.size() ) {

        QString err = QString("No channel map in '%1' meta data.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    setWindowTitle(
        QString(APPNAME " File Viewer: %1 [%2 chans @ %3 Hz, %4 scans]")
        .arg( fname_no_path )
        .arg( dataFile.numChans() )
        .arg( dataFile.samplingRateHz() )
        .arg( dfCount ) );

    return true;
}


void FileViewerWindow::applyStyles()
{
    for( int is = 0; is < (int)N_ColorScheme; ++is )
        colorSchemeActions[is]->setChecked( is == sav.colorScheme );

    for( int im = 0; im < (int)N_ViewMode; ++im )
        viewModeActions[im]->setChecked( im == sav.viewMode );

    if( sav.sortUserOrder ) {
        sortAcqAct->setChecked( false );
        sortUsrAct->setChecked( true );
    }
    else {
        sortAcqAct->setChecked( true );
        sortUsrAct->setChecked( false );
    }
}


void FileViewerWindow::setToolbarRanges()
{
    QDoubleSpinBox  *XS = toolBar->findChild<QDoubleSpinBox*>( "xscalesb" );
    QDoubleSpinBox  *YS = toolBar->findChild<QDoubleSpinBox*>( "yscalesb" );
    QSpinBox        *ND = toolBar->findChild<QSpinBox*>( "ndivssb" );

    XS->blockSignals( true );
    YS->blockSignals( true );
    ND->blockSignals( true );

    XS->setRange( 0.0001, qMin( XS->maximum(), dataFile.fileTimeSecs() ) );
    XS->setValue( sav.xSpan );
    YS->setValue( sav.ySclNeu );
    ND->setValue( sav.nDivs );

    XS->blockSignals( false );
    YS->blockSignals( false );
    ND->blockSignals( false );
}


void FileViewerWindow::initHipass()
{
    if( hipass )
        delete hipass;

    hipass =
    new Biquad( bq_type_highpass, 300.0 / dataFile.samplingRateHz() );
}


void FileViewerWindow::killShowHideAction( int i )
{
    QAction *A = grfActShowHide[i];

    channelsMenu->removeAction( A );
    delete A;
}


void FileViewerWindow::putFrameIntoPool( int i )
{
    QFrame  *f = grfFrames[i];

    f->setParent( framePoolParent );
    framePool.insert( f );
}


bool FileViewerWindow::getFrameFromPool(
    QFrame*     &f,
    GLGraph*    &G,
    GLGraphX*   &X )
{
    if( framePool.isEmpty() )
        return false;

    f = *framePool.begin();
    framePool.remove( f );
    f->setParent( graphParent );

    G = f->findChild<GLGraph*>();
    X = G->getX();

    return true;
}


void FileViewerWindow::create1NewFrame(
    QFrame*     &f,
    GLGraph*    &G,
    GLGraphX*   &X )
{
    f = new QFrame( graphParent );
    X = new GLGraphX;
    G = new GLGraph( "fvw", f, X );    // G owns X

    QVBoxLayout *vbl = new QVBoxLayout( f );
    vbl->setSpacing( 0 );
    vbl->setContentsMargins( 0, 0, 0, 0 );
    vbl->addWidget( G );

    G->setImmedUpdate( false );
    G->setMouseTracking( true );
    G->addAction( exportAction );
    G->setContextMenuPolicy( Qt::ActionsContextMenu );
    ConnectUI( G, SIGNAL(cursorOver(double,double)), this, SLOT(mouseOverGraph(double,double)) );
    ConnectUI( G, SIGNAL(lbutClicked(double,double)), this, SLOT(clickGraph(double,double)) );
    ConnectUI( G, SIGNAL(lbutReleased()), this, SLOT(dragDone()) );
    ConnectUI( G, SIGNAL(lbutDoubleClicked(double,double)), this, SLOT(dblClickGraph(double,double)) );
    ConnectUI( G, SIGNAL(cursorOverWindowCoords(int,int)), this, SLOT(mouseOverLabel(int,int)) );
}


bool FileViewerWindow::cacheFrames_killActions( QString *errMsg )
{
    int     nG  = grfFrames.size();
    bool    ok  = true;

    if( !nG )
        return true;

    graphParent->hide();

    QProgressDialog prog( "Caching graphs...", "Cancel", 0, nG, 0 );

    qApp->processEvents();
    prog.setWindowModality( Qt::WindowModal );
    prog.setWindowFlags( Qt::WindowStaysOnTopHint );
    prog.setMinimumDuration( 0 );
    prog.show();
    qApp->processEvents();

    for( int ig = 0; ig < nG; ++ig ) {

        killShowHideAction( ig );
        putFrameIntoPool( ig );

        prog.setValue( ig + 1 );
        qApp->processEvents();

        if( prog.wasCanceled() ) {

            if( errMsg )
                *errMsg = "User canceled.";

            ok = false;
            break;
        }
    }

    prog.setValue( nG );
    prog.close();
    qApp->processEvents();

    return ok;
}


// BK: If nG > ~100, widgets exist, but get openGL errors like this:
//
// createContext: wglCreateContextAttribsARB() failed (GL error code: 0x0)
// for format: QSurfaceFormat(version 2.0, options QFlags(),
// depthBufferSize -1, redBufferSize -1, greenBufferSize -1,
// blueBufferSize -1, alphaBufferSize -1, stencilBufferSize -1,
// samples -1, swapBehavior 0, swapInterval 1, profile  0),
// shared context: 0x30000 ()
// createContext: wglCreateContext failed. ()
// Unable to create a GL Context.
//
bool FileViewerWindow::initFrames_initActions( QString *errMsg )
{
    int     nG  = grfFrames.size();
    bool    ok  = true;

    QProgressDialog prog( "Initializing graphs...", "Cancel", 0, nG, 0 );

    qApp->processEvents();
    prog.setWindowModality( Qt::WindowModal );
    prog.setWindowFlags( Qt::WindowStaysOnTopHint );
    prog.setMinimumDuration( 0 );
    prog.show();
    qApp->processEvents();

    for( int ig = 0; ig < nG; ++ig ) {

        QFrame      *f;
        GLGraph     *G;
        GLGraphX    *X;
        QAction     *a;
        int         &C  = ig2AcqChan[ig];
        GraphParams &P  = grfParams[ig];

        if( !getFrameFromPool( f, G, X ) )
            create1NewFrame( f, G, X );

        f->setLineWidth( 0 );
        f->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );

        G->setToolTip( chanMap.e[ig].name );

        C = dataFile.channelIDs()[ig];

        P.niType        = dataFile.origID2Type( C );
        P.gain          = dataFile.origID2Gain( C );
        P.filter300Hz   = false;
        P.dcFilter      = P.niType == 0;

        X->num          = ig;
        X->yscale       = (P.niType == 0 ? sav.ySclNeu :
                            (P.niType == 1 ? sav.ySclAux : 1));
        X->isDigType    = P.niType == 2;
        X->drawCursor   = false;
        X->setHGridLines( sav.nDivs );
        X->setVGridLines( sav.nDivs );

        a = new QAction( QString::number( C ), this );
        a->setObjectName( QString::number( ig ) );
        a->setCheckable( true );
        a->setChecked( true );
        ConnectUI( a, SIGNAL(triggered()), this, SLOT(menuShowHideGraph()) );
        channelsMenu->addAction( a );

        grfFrames[ig]       = f;
        grf[ig]             = G;
        grfActShowHide[ig]  = a;
        order2ig[ig]        = ig; // default is acqsort

        applyColorScheme( ig );

        prog.setValue( ig + 1 );
        qApp->processEvents();

        if( prog.wasCanceled() ) {

            if( errMsg )
                *errMsg = "User canceled.";

            ok = false;
            break;
        }
    }

    prog.setValue( nG );
    prog.close();
    qApp->processEvents();

    return ok;
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::loadSettings()
{
    STDSETTINGS( settings, "cc_fileviewer" );
    settings.beginGroup( "FileViewerWindow" );

// -----------
// colorScheme
// -----------

    int cs = settings.value( "colorScheme", DefaultScheme ).toInt();

    if( cs < 0 || cs >= N_ColorScheme )
        cs = DefaultScheme;

    sav.colorScheme = (ColorScheme)cs;

// --------
// viewMode
// --------

    sav.viewMode = Tiled;

    int vm = settings.value( "viewMode", (int)Tiled ).toInt();

    if( vm >= 0 && vm < N_ViewMode )
        sav.viewMode = (ViewMode)vm;

// --------------
// arrowKeyFactor
// --------------

    sav.fArrowKey = settings.value( "fArrowKey", 0.1 ).toDouble();

    if( fabs( sav.fArrowKey ) < 0.0001 )
        sav.fArrowKey = 0.1;

// -----------
// pgKeyFactor
// -----------

    sav.fPageKey = settings.value( "fPageKey", 0.5 ).toDouble();

    if( fabs( sav.fPageKey ) < 0.0001 )
        sav.fPageKey = 0.5;

// ------
// Scales
// ------

    sav.xSpan   = settings.value( "xSpan", 4.0 ).toDouble();
    sav.ySclNeu = settings.value( "ySclNeu", 1.0 ).toDouble();
    sav.ySclAux = settings.value( "ySclAux", 1.0 ).toDouble();
    sav.nDivs   = settings.value( "nDivs", 4 ).toInt();

    sav.xSpan = qMin( sav.xSpan, dataFile.fileTimeSecs() );

// -------------
// sortUserOrder
// -------------

    sav.sortUserOrder = settings.value( "sortUserOrder", false ).toBool();

    settings.endGroup();

    exportCtl->loadSettings( settings );
}


void FileViewerWindow::saveSettings() const
{
    STDSETTINGS( settings, "cc_fileviewer" );
    settings.beginGroup( "FileViewerWindow" );

    settings.setValue( "colorScheme", (int)sav.colorScheme );
    settings.setValue( "viewMode", (int)sav.viewMode );
    settings.setValue( "fArrowKey", sav.fArrowKey );
    settings.setValue( "fPageKey", sav.fPageKey );
    settings.setValue( "xSpan", sav.xSpan );
    settings.setValue( "ySclNeu", sav.ySclNeu );
    settings.setValue( "ySclAux", sav.ySclAux );
    settings.setValue( "nDivs", sav.nDivs );
    settings.setValue( "sortUserOrder", sav.sortUserOrder );

    settings.endGroup();

    exportCtl->saveSettings( settings );
}


qint64 FileViewerWindow::nScansPerGraph() const
{
    return sav.xSpan * dataFile.samplingRateHz();
}


void FileViewerWindow::updateNDivText()
{
    QLabel  *DT = toolBar->findChild<QLabel*>( "divlbl" );

    if( sav.nDivs > 0 ) {

        double  X = sav.xSpan / sav.nDivs;

        if( igSelected > -1 && grfParams[igSelected].niType < 2 ) {

            double  Y = dataFile.niRng().span()
                        / (sav.nDivs
                            * grfParams[igSelected].gain
                            * grf[igSelected]->getX()->yscale);

            DT->setText( QString(" Each (%1s,%2V)").arg( X ).arg( Y ) );
        }
        else
            DT->setText( QString(" Each (%1s,-)").arg( X ) );
    }
    else
        DT->setText( " Each (-,-)" );
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double FileViewerWindow::scalePlotValue( double v )
{
    return dataFile.niRng().unityToVolts( (v+1)/2 )
            / grfParams[igMouseOver].gain;
}


double FileViewerWindow::timeFromPos( qint64 p ) const
{
    return p / dataFile.samplingRateHz();
}


qint64 FileViewerWindow::posFromTime( double s ) const
{
    return dataFile.samplingRateHz() * s;
}


qint64 FileViewerWindow::maxPos() const
{
    return qMax( 0LL, dfCount - nScansPerGraph() - 1 );
}


void FileViewerWindow::setFilePos64( qint64 newPos )
{
    pos = qBound( 0LL, newPos, maxPos() );

    updateGraphs();
}


void FileViewerWindow::setSliderPos( qint64 newPos )
{
    QDoubleSpinBox  *PS = sliderGrp->findChild<QDoubleSpinBox*>( "possb" );

    if( PS )
        PS->setValue( newPos );
}


void FileViewerWindow::setSliderRanges()
{
    QDoubleSpinBox  *PS = sliderGrp->findChild<QDoubleSpinBox*>( "possb" );
    QDoubleSpinBox  *SC = sliderGrp->findChild<QDoubleSpinBox*>( "secsb" );
    QSlider         *SR = sliderGrp->findChild<QSlider*>( "slider" );

    PS->blockSignals( true );
    SC->blockSignals( true );
    SR->blockSignals( true );

    qint64  maxVal = maxPos();

// Calculate slider scale factor

    pscale = 1;

    while( maxVal / pscale > qint64(INT_MAX) )
        pscale = (pscale == qint64(1) ? qint64(2) : pscale*pscale);

// Ranges

    PS->setMinimum( 0 );
    PS->setMaximum( maxVal );
    SC->setMinimum( 0 );
    SC->setMaximum( timeFromPos( maxVal ) );
    SR->setMaximum( maxVal / pscale );

// Values

    PS->setValue( pos );
    SC->setValue( timeFromPos( pos ) );
    SR->setValue( pos / pscale );

    PS->blockSignals( false );
    SC->blockSignals( false );
    SR->blockSignals( false );
}


void FileViewerWindow::updateSliderTexts()
{
    QLabel  *PL     = sliderGrp->findChild<QLabel*>( "poslbl" );
    QLabel  *SL     = sliderGrp->findChild<QLabel*>( "seclbl" );
    qint64  dfMax   = dfCount - 1,
            last    = qMin( dfMax, pos + nScansPerGraph() - 1 );
    int     fldW    = QString::number( dfMax ).size();

    PL->setText(
        QString("to %1 (of %2)")
        .arg( last, fldW, 10, QChar('0') )
        .arg( dfMax ) );

    SL->setText(
        QString("to %1 (of %2)")
        .arg( timeFromPos( last ), 0, 'f', 3 )
        .arg( timeFromPos( dfMax ), 0, 'f', 3 ) );
}


QString FileViewerWindow::nameGraph( int ig ) const
{
    if( ig < 0 || ig >= grf.size() )
        return QString::null;

    return chanMap.name( ig, ig2AcqChan[ig] == dataFile.triggerChan() );
}


void FileViewerWindow::hideGraph( int ig )
{
    int nG = grf.size();

    if( ig < 0 || ig >= nG )
        return;

// Don't hide last one

    if( grfVisBits.count( false ) + 1 >= nG )
        return;

// Mark it

    grfVisBits.clearBit( ig );
    grfActShowHide[ig]->setChecked( false );

// Adjust selection

    if( igSelected == ig ) {

        QVector<int>    ig2order( nG );

        for( int is = 0; is < nG; ++is )
            ig2order[order2ig[is]] = is;

        // try select next higher
        for( int is = ig2order[ig] + 1; is < nG; ++is ) {

            if( grfVisBits.testBit( order2ig[is] ) ) {

                selectGraph( order2ig[is] );
                goto doLayout;
            }
        }

        // try select next lower
        for( int is = ig2order[ig] - 1; is >= 0; --is ) {

            if( grfVisBits.testBit( order2ig[is] ) ) {

                selectGraph( order2ig[is] );
                goto doLayout;
            }
        }

        // select nothing
        selectGraph( -1 );
    }

doLayout:
    layoutGraphs();
}


void FileViewerWindow::showGraph( int ig )
{
    if( ig < 0 || ig >= grfFrames.size() )
        return;

    grfVisBits.setBit( ig );
    grfActShowHide[ig]->setChecked( true );
    layoutGraphs();
}


void FileViewerWindow::DrawSelected( int ig, bool selected )
{
    QFrame  *f = grfFrames[ig];

    f->setUpdatesEnabled( false );

    if( selected ) {
        f->setLineWidth( 3 );
        f->setFrameStyle( QFrame::Box | QFrame::Plain );
    }
    else {
        f->setLineWidth( 0 );
        f->setFrameStyle( QFrame::StyledPanel | QFrame::Plain );
    }

    f->setUpdatesEnabled( true );
}


void FileViewerWindow::selectGraph( int ig )
{
    if( igSelected == ig )
        return;

    if( igSelected > -1 )
        DrawSelected( igSelected, false );

    igSelected = ig;

    toolBar->findChild<QLabel*>( "namelbl" )->setText( nameGraph( ig ) );

    if( ig == -1 )
        return;

    DrawSelected( ig, true );

    QDoubleSpinBox  *YS = toolBar->findChild<QDoubleSpinBox*>( "yscalesb" );
    QDoubleSpinBox  *GN = toolBar->findChild<QDoubleSpinBox*>( "gainsb" );
    QCheckBox       *HP = toolBar->findChild<QCheckBox*>( "hpchk" );
    QCheckBox       *DC = toolBar->findChild<QCheckBox*>( "dcchk" );

    YS->blockSignals( true );
    GN->blockSignals( true );
    HP->blockSignals( true );
    DC->blockSignals( true );

    YS->setValue( grf[ig]->getX()->yscale );
    GN->setValue( grfParams[ig].gain );
    HP->setChecked( grfParams[ig].filter300Hz );
    DC->setChecked( grfParams[ig].dcFilter );

    int enabled;

    enabled = grfParams[ig].niType < 2;
    YS->setEnabled( enabled );
    GN->setEnabled( enabled );

    enabled = grfParams[ig].niType == 0;
    HP->setEnabled( enabled );
    DC->setEnabled( enabled );

    YS->blockSignals( false );
    GN->blockSignals( false );
    HP->blockSignals( false );
    DC->blockSignals( false );

    updateNDivText();
}


void FileViewerWindow::toggleMaximized()
{
    static int vPos = -1;

    hideCloseLabel();
    graphParent->hide();

// Adjust view scrollbars and sizing.
// Only need adjustment in stacked view modes.

    if( sav.viewMode != Tiled ) {

        if( igMaximized == -1 ) {
            // About to maximize -> need wide-open area
            vPos = scrollArea->verticalScrollBar()->sliderPosition();
            scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            scrollArea->setWidgetResizable( true );
        }
        else {
            // Restore previous scrolls/area
            scrollArea->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
            scrollArea->setWidgetResizable( false );
        }

        updateGeometry();
    }

// Hide or show target

    int nG = grfFrames.size();

    if( igMaximized == -1 ) {

        for( int ig = 0; ig < nG; ++ig ) {

            if( ig != igSelected )
                grfFrames[ig]->hide();
        }
    }
    else {

        if( sav.viewMode != Tiled )
            setStackSizing();

        for( int is = 0; is < nG; ++is ) {

            // prettier to reveal in sorted order

            int ig = order2ig[is];

            if( grfVisBits.testBit( ig ) )
                grfFrames[ig]->show();
        }
    }

// Restore view

    graphParent->show();

    if( igMaximized > -1 && sav.viewMode != Tiled ) {
        // Restore exact pos if can...
        scrollArea->verticalScrollBar()->setSliderPosition( vPos );
        // But always ensure visible if couldn't.
        scrollArea->ensureWidgetVisible( grfFrames[igMaximized] );
    }

// Update state

    if( igMaximized == -1 )
        igMaximized = igSelected;
    else
        igMaximized = -1;

    viewMenu->setEnabled( igMaximized == -1 );
    channelsMenu->setEnabled( igMaximized == -1 );

    updateGraphs();
}


void FileViewerWindow::updateXSel( int graphSpan )
{
    int nG = grf.size();

    if( dragL >= 0
        && dragR > dragL
        && dragR < dfCount ) {

        // transform selection from scans to range [0..1].

        float   gselbeg = (dragL - pos) / double(graphSpan),
                gselend = (dragR - pos) / double(graphSpan);

        for( int ig = 0; ig < nG; ++ig ) {

            GLGraphX    *X = grf[ig]->getX();

            X->setXSelEnabled( true );
            X->setXSelRange( gselbeg, gselend );
        }
    }
    else {
        for( int ig = 0; ig < nG; ++ig )
            grf[ig]->getX()->setXSelEnabled( false );
    }
}


// Notes:
//
// - User has random access to file data, and if filter is enabled,
// the first xflt data points of any block would ordinarily show a
// transient artifact. Therefore, we will always load an extra xflt
// scans, process the larger block, and trim the lead portion off
// when passing ybuf data to graph.
//
// - The code is simpler if we load a timespan, then process the
// data channel-by-channel. That is, we go over all the timepoints
// again and again. However, if the span is large this causes lots
// of memory paging and that's very slow.
//
// - Rather, we treat a long span as several short chunks. We have to
// retain state data for filters and DC calcs across chunks.
//
void FileViewerWindow::updateGraphs()
{
// -------------
// Channel setup
// -------------

    double  ysc     = 1.0 / 32768.0,
            srate   = dataFile.samplingRateHz();
    int     nC      = grfVisBits.count( true );

    QVector<uint>   onChans;

    Subset::bits2Vec( onChans, grfVisBits );

// -----------
// Scans setup
// -----------

    qint64  xpos, num2Read;
    int     xflt = qMin( 120LL, pos ),
            dwnSmp;

    xpos        = pos - xflt;
    num2Read    = xflt + sav.xSpan * srate;
    dwnSmp      = num2Read / (2 * scrollArea->width());

// Note: dwnSmp oversamples by 2X.

    if( dwnSmp < 1 )
        dwnSmp = 1;

// -----------
// Size graphs
// -----------

    if( xpos >= dfCount )
        return;

    qint64  ntpts   = qMin( num2Read, dfCount - xpos ),
            dtpts   = (ntpts + dwnSmp - 1) / dwnSmp,
            gtpts   = (ntpts - xflt + dwnSmp - 1) / dwnSmp,
            xoff    = dtpts - gtpts;

    for( int ic = 0; ic < nC; ++ic ) {

        GLGraphX    *X = grf[onChans[ic]]->getX();

        X->ydata.resizeAndErase( gtpts );
        X->initVerts( gtpts );
    }

// -----------------
// Pick a chunk size
// -----------------

// nominally 1 second's worth, but a multiple of dwnSmp

    qint64  chunk = int(srate/dwnSmp) * dwnSmp;

// ------------
// Filter setup
// ------------

    hipass->clearMem();

    QVector<double> dcSum( nC, 0.0 );
    QVector<qint64> dcN( nC, 0 );

// --------------
// Process chunks
// --------------

    qint64  nRem = num2Read;

    for(;;) {

        if( nRem <= 0 )
            break;

        // read this block

        vec_i16 data;
        qint64  nthis = qMin( chunk, nRem );

        ntpts = dataFile.readScans( data, xpos, nthis, grfVisBits );

        if( ntpts <= 0 )
            break;

        // advance for next block

        xpos    += ntpts;
        nRem    -= ntpts;

        // process this block

        dtpts = (ntpts + dwnSmp - 1) / dwnSmp;

        QVector<float>  ybuf( dtpts );

        // -------------------------
        // For each shown channel...
        // -------------------------

        for( int ic = 0; ic < nC; ++ic ) {

            qint16  *d      = &data[ic];
            int     ig      = onChans[ic],
                    dstep   = dwnSmp * nC,
                    ny      = 0;

            if( grfParams[ig].niType == 0 ) {

                // ---------------
                // Neural channels
                // ---------------

                // ------
                // Filter
                // ------

                if( grfParams[ig].filter300Hz )
                    hipass->apply1BlockwiseMemAll( &data[0], ntpts, nC, ic );

                // -----------
                // DC subtract
                // -----------

                // Subtract the average value over all
                // of the post-downsampling data points:
                // a sparse sampling of the whole graph.

                if( grfParams[ig].dcFilter ) {

                    for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                        dcSum[ic] += *d;

                    dcN[ic] += dtpts;

                    double  ave  = dcSum[ic] / dcN[ic];

                    d = &data[ic];

                    for( int it = 0; it < ntpts; ++it, d += nC )
                        *d -= ave;

                    d = &data[ic];
                }

                // -------------------
                // Neural downsampling
                // -------------------

                // Withing each bin, report the greatest
                // amplitude (pos or neg) extremum. This
                // ensures spikes are not missed. Result
                // in ybuf.

                if( dwnSmp <= 1 )
                    goto pickNth;

                int ndRem = ntpts;

                for( int it = 0; it < ntpts; it += dwnSmp ) {

                    int binMin = *d,
                        binMax = binMin,
                        binWid = dwnSmp;

                        d += nC;

                        if( ndRem < binWid )
                            binWid = ndRem;

                    for( int ib = 1; ib < binWid; ++ib, d += nC ) {

                        int	val = *d;

                        if( val < binMin )
                            binMin = val;

                        if( val > binMax )
                            binMax = val;
                    }

                    ndRem -= binWid;

                    if( abs( binMin ) > abs( binMax ) )
                        binMax = binMin;

                    ybuf[ny++] = binMax * ysc;
                }
            }
            else if( grfParams[ig].niType == 1 ) {

                // ------------
                // Aux channels
                // ------------

pickNth:
                for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                    ybuf[ny++] = *d * ysc;
            }
            else {

                // -------
                // Digital
                // -------

                for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                    ybuf[ny++] = *d;
            }

            // -------------
            // Copy to graph
            // -------------

            grf[ig]->getX()->ydata.putData( &ybuf[xoff], dtpts - xoff );
        }

        xoff = 0;   // only first chunk includes offset

    }   // end chunks

// ---------
// Selection
// ---------

    updateXSel( num2Read - xflt );

// ------
// Redraw
// ------

    for( int ic = 0; ic < nC; ++ic )
        grf[onChans[ic]]->updateNow();
}


void FileViewerWindow::setStackSizing()
{
    const int padding = 2;

    int w, h = 80, n = grfVisBits.count( true );

    if( sav.viewMode == StackedHuge )
        h = 320;
    else if( sav.viewMode == StackedLarge )
        h = 160;

    w = scrollArea->width()
        - scrollArea->verticalScrollBar()->width()
        - 2;

    graphParent->resize( w, n * (h + padding) );
}


static const QColor clrIce[3] = {
    QColor(0x87, 0xce, 0xfa, 0x7f),
    QColor(0x87, 0xce, 0xfa, 0x7f),
    QColor(0x87, 0xce, 0xfa, 0x7f)};

static const QColor clrFire[3] = {
    QColor(0xfa, 0x87, 0x37, 0x7f),
    QColor(0xfa, 0x87, 0x37, 0x7f),
    QColor(0xfa, 0x87, 0x37, 0x7f)};

static const QColor clrGreen[3] = {
    QColor(0x09, 0xca, 0x09, 0x7f),
    QColor(0x09, 0xca, 0x09, 0x7f),
    QColor(0x09, 0xca, 0x09, 0x7f)};

static const QColor clrBW[3] = {
    QColor(0xca, 0xca, 0xca, 0xc0),
    QColor(0xca, 0xca, 0xca, 0xc0),
    QColor(0xca, 0xca, 0xca, 0xc0)};

static const QColor clrClassic[3] = {
    QColor(0x2f, 0x4f, 0x4f, 0xff),
    QColor(0x4f, 0x4f, 0x4f, 0xff),
    QColor(0x1f, 0x1f, 0x1f, 0xff)};


void FileViewerWindow::applyColorScheme( int ig )
{
    QColor  bg, grid, fg;

    switch( sav.colorScheme ) {

        case Ice:
            fg = clrIce[grfParams[ig].niType];
            bg.setRgbF( .15, .15, .15 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Fire:
            fg = clrFire[grfParams[ig].niType];
            bg.setRgbF( .15, .15, .15 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Green:
            fg = clrGreen[grfParams[ig].niType];
            bg.setRgbF( .07, .07, .07 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case BlackWhite:
            fg = clrBW[grfParams[ig].niType];
            bg.setRgbF( .05, .05, .05 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Classic:
        default:
            bg      = clrClassic[grfParams[ig].niType];
            fg      = QColor(0xee, 0xdd, 0x82);
            grid    = QColor(0x87, 0xce, 0xfa, 0x7f);
            break;
    }

    GLGraphX    *X = grf[ig]->getX();

    X->bkgnd_Color  = bg;
    X->grid_Color   = grid;
    X->trace_Color  = fg;
}


void FileViewerWindow::printStatusMessage()
{
    int ig = igMouseOver;

    if( ig < 0 )
        return;

    double  t = tMouseOver,
            y = yMouseOver;

// check for millivolts

    const char  *unit = "V";

    if( dataFile.niRng().rmax / grfParams[ig].gain < 1.0 ) {

        unit = "mV";
        y   *= 1000.0;
    }

    QString msg;
    QString chStr = nameGraph( ig );

    msg = QString("Mouse tracking Graph %1 @ pos (%2 s, %3 %4)")
            .arg( STR2CHR( chStr ) )
            .arg( t, 0, 'f', 4 )
            .arg( y, 0, 'f', 4 )
            .arg( unit );

    if( dragL >= 0 && dragR >= 0 ) {

        msg += QString(" - Selection range: scans(%1,%2) secs(%3,%4)")
                .arg( dragL )
                .arg( dragR )
                .arg( timeFromPos( dragL ), 0, 'f', 4 )
                .arg( timeFromPos( dragR ), 0, 'f', 4 );
    }

    statusBar()->showMessage( msg );
}


bool FileViewerWindow::queryCloseOK()
{
// for now, always ok.
    return true;
}


