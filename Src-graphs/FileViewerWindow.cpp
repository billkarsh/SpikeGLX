
#include "ui_FVW_OptionsDialog.h"

#include "Pixmaps/apply_all.xpm"
#include "Pixmaps/close_but_16px.xpm"

#include "FileViewerWindow.h"
#include "Util.h"
#include "MainApp.h"
#include "MGraph.h"
#include "Biquad.h"
#include "ExportCtl.h"
#include "ClickableLabel.h"
#include "Subset.h"
#include "Version.h"

#include <QKeyEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QPushButton>
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

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TaggableLabel : public ClickableLabel
{
private:
    int mtag;
public:
    TaggableLabel( QWidget *parent, Qt::WindowFlags f = 0 )
    : ClickableLabel( parent, f ), mtag(-1) {}
    void noTag()            {mtag = -1;}
    void setTag( int tag )  {mtag = tag;}
    int tag() const         {return mtag;}
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

    killActions();

// ---------------------------
// Resize arrays for this file
// ---------------------------

    int nG = dataFile.numChans();

    grfY.resize( nG );
    grfParams.resize( nG );
    grfActShowHide.resize( nG );
    order2ig.resize( nG );
    ig2AcqChan.resize( nG );

// -------------------
// Init new array data
// -------------------

    grfVisBits.fill( true, nG );

    initGraphs();

// ---------------
// Initialize view
// ---------------

    selectGraph( 0, false );

    sav.sortUserOrder = !sav.sortUserOrder;
    toggleSort();

    return true;
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool FileViewerWindow::eventFilter( QObject *obj, QEvent *e )
{
    if( (obj == mscroll || obj == slider)
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


void FileViewerWindow::channels_ShowAll()
{
    hideCloseLabel();

    igMaximized = -1;

    int nG = grfY.size();

    grfVisBits.fill( true, nG );

    for( int ig = 0; ig < nG; ++ig )
        grfActShowHide[ig]->setChecked( true );

    layoutGraphs();
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

        for( int ig = 0, nG = grfY.size(); ig < nG; ++ig )
            applyColorScheme( ig );

        updateGraphs();
    }
}

/* ---------------------------------------------------------------- */
/* Toolbar -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::toggleSort()
{
    QPushButton *B = toolBar->findChild<QPushButton*>( "sortbtn" );

    if( !B )
        return;

    sav.sortUserOrder = !sav.sortUserOrder;
    saveSettings();

    if( sav.sortUserOrder ) {
        B->setText( "Usr Order" );
        chanMap.userOrder( order2ig );
    }
    else {
        B->setText( "Acq Order" );
        chanMap.defaultOrder( order2ig );
    }

    layoutGraphs();
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


// BK: Several of the spinboxes show a strange behavior:
// If window showing a lot of data so that layout or
// update is slow, then clicks in spinner arrows cause
// two steps of value change. This does not happen if
// activating arrows by keyboard. It does not help to
// defer drawing with a queued call. Using a timer can
// help if the delay is set very large (like a second).
//
void FileViewerWindow::setYPix( int n )
{
    sav.yPix = n;
    saveSettings();

    mscroll->theX->ypxPerGrf = n;

    layoutGraphs();
}


void FileViewerWindow::setYScale( double d )
{
    if( igSelected < 0 )
        return;

    grfY[igSelected].yscl = d;

    updateNDivText();
    updateGraphs();
}


void FileViewerWindow::setNDivs( int n )
{
    sav.nDivs = n;
    saveSettings();

    updateNDivText();
    mscroll->theX->setVGridLines( sav.nDivs );

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
    double      yScale  = grfY[igSelected].yscl;

    if( P.niType == 0 )
        sav.ySclNeu = yScale;
    else if( P.niType == 1 )
        sav.ySclAux = yScale;

    if( P.niType < 2 )
        saveSettings();

    for( int ig = 0, nG = grfParams.size(); ig < nG; ++ig ) {

        if( ig != igSelected && grfParams[ig].niType == P.niType ) {

            grfY[ig].yscl   = yScale;
            grfParams[ig]   = P;
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
    closeLbl->noTag();
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
void FileViewerWindow::mouseOverGraph( double x, double y, int iy )
{
// ------------
// Which graph?
// ------------

    igMouseOver = mscroll->theX->Y[iy]->num;

    if( igMouseOver < 0 || igMouseOver >= (int)grfY.size() ) {
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


void FileViewerWindow::clickGraph( double x, double y, int iy )
{
    mouseOverGraph( x, y, iy );
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


void FileViewerWindow::dblClickGraph( double x, double y, int iy )
{
    clickGraph( x, y, iy );
    toggleMaximized();
}


void FileViewerWindow::mouseOverLabel( int x, int y, int iy )
{
    if( dragging )
        return;

    if( !didLayout || igMaximized > -1 )
        return;

    QSize   sz      = closeLbl->size();
    int     hLbl    = sz.height(),
            xLbl    = mscroll->viewport()->width() - sz.width(),
            yLbl    = (sav.yPix - hLbl) / 2;

    if( x > xLbl && y > yLbl && y < yLbl + hLbl ) {

        if( grfVisBits.count( false ) + 1 >= grfY.size() )
            return; // leave at least one showing

        hideCloseTimer->stop();

        QPoint  p( xLbl, yLbl + iy*sav.yPix - mscroll->theX->clipTop );
        p = mscroll->theM->mapToGlobal( p );

        int ig = mscroll->theX->Y[iy]->num;

        closeLbl->setTag( ig );
        closeLbl->setToolTip( grfY[ig].label );
        closeLbl->move( p.x(), p.y() );
        closeLbl->show();

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

    if( ig >= 0 && ig < grfY.size() ) {

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
    int tag = closeLbl->tag();

    if( tag >= 0 )
        hideGraph( tag );

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
    didLayout = false;

    hideCloseLabel();

// ---------------------------------------
// Point to visible graphs in sorted order
// ---------------------------------------

    MGraphX *theX = mscroll->theX;

    theX->Y.clear();

    if( igMaximized > -1 )
        theX->Y.push_back( &grfY[igMaximized] );
    else {
        for( int is = 0, nG = grfY.size(); is < nG; ++is ) {

            int ig = order2ig[is];

            // BK: Analog graphs only, for now
            if( grfParams[ig].niType < 2 && grfVisBits.testBit( ig ) )
                theX->Y.push_back( &grfY[ig] );
        }
    }

// ------
// Layout
// ------

    mscroll->adjustLayout();
    theX->setYSelByNum( igSelected );
    didLayout = true;
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

    m = mb->addMenu( "&Channels" );
    m->addAction( "Show All", this, SLOT(channels_ShowAll()) );
    m->addSeparator();
    channelsMenu = m;

    m = mb->addMenu( "Color &Scheme" );
    for( int is = 0; is < (int)N_ColorScheme; ++is ) {
        QAction *a;
        a = m->addAction( colorSchemeNames[is], this, SLOT(color_SelectScheme()) );
        a->setCheckable( true );
        colorSchemeActions[is] = a;
    }
// BK: Disable color scheme for now
    m->setEnabled( false );
}


void FileViewerWindow::initToolbar()
{
    QDoubleSpinBox  *S;
    QSpinBox        *V;
    QPushButton     *B;
    QCheckBox       *C;
    QLabel          *L;

    toolBar = addToolBar( "Graph Controls" );

// Sort selector

    B = new QPushButton( toolBar );
    B->setObjectName( "sortbtn" );
    B->setToolTip(
        "Toggle graph sort order: user/acquired" );
    ConnectUI( B, SIGNAL(clicked()), this, SLOT(toggleSort()) );
    toolBar->addWidget( B );

// Selected

    L = new ClickableLabel( "MN0C0;000", toolBar );
    L->setObjectName( "namelbl" );
    L->setToolTip( "Selected graph (click to find)" );
    L->setMargin( 3 );
    L->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( L, SIGNAL(clicked()), mscroll, SLOT(scrollToSelected()) );
    toolBar->addWidget( L );

// X-Scale

    toolBar->addSeparator();

    L = new QLabel( "Secs", toolBar );
    toolBar->addWidget( L );

    S = new QDoubleSpinBox( toolBar );
    S->setObjectName( "xscalesb" );
    S->setToolTip( "Scan much faster with short span ~1sec" );
    S->setDecimals( 4 );
    S->setRange( 0.0001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(setXScale(double)) );
    toolBar->addWidget( S );

// YPix

    L = new QLabel( "YPix", toolBar );
    toolBar->addWidget( L );

    V = new QSpinBox( toolBar );
    V->setObjectName( "ypixsb" );
    V->setMinimum( 4 );
    V->setMaximum( 500 );
    ConnectUI( V, SIGNAL(valueChanged(int)), this, SLOT(setYPix(int)) );
    toolBar->addWidget( V );

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

    L = new QLabel( " Boxes - x -", toolBar );
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
    S->setSingleStep( 100.0 );
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
    S->setSingleStep( 0.01 );
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


void FileViewerWindow::initExport()
{
    exportAction = new QAction( "Export...", this );
    ConnectUI( exportAction, SIGNAL(triggered()), this, SLOT(doExport()) );

    exportCtl = new ExportCtl( this );
}


void FileViewerWindow::initCloseLbl()
{
    QPixmap pm( close_but_16px );

    closeLbl = new TaggableLabel( mscroll->theM, Qt::FramelessWindowHint | Qt::Popup );
    closeLbl->setPixmap( pm );
    closeLbl->resize( pm.size() );
    closeLbl->setCursor( Qt::ArrowCursor );
    closeLbl->hide();
    closeLbl->move( -100, -100 );
    ConnectUI( closeLbl, SIGNAL(clicked()), this, SLOT(clickedCloseLbl()) );

    hideCloseTimer = new QTimer( this );
    hideCloseTimer->setInterval( 250 );
    ConnectUI( hideCloseTimer, SIGNAL(timeout()), this, SLOT(hideCloseTimeout()) );
}


void FileViewerWindow::initDataIndepStuff()
{
    setCursor( Qt::ArrowCursor );
    resize( 1050, 640 );

// CentralWidget gets vertical layout of two items:
// - graphs
// - 'slider' (position controls)

    QWidget *cw = new QWidget;
    setCentralWidget( cw );

    mscroll = new MGScroll( "fvw", cw );
    mscroll->installEventFilter( this );

    QVBoxLayout *VL = new QVBoxLayout( cw );
    VL->addWidget( mscroll, 1 );
    VL->addWidget( initSliderGrp() );

// Top-most controls

    initMenus();
    initToolbar();

// Status bar below

    statusBar();

// Additional once-only

    initExport();

    // aux color
    mscroll->theX->yColor.push_back( QColor( 0x44, 0xee, 0xff ) );

    MGraph  *theM = mscroll->theM;
    theM->addAction( exportAction );
    theM->setContextMenuPolicy( Qt::ActionsContextMenu );
    ConnectUI( theM, SIGNAL(cursorOver(double,double,int)), this, SLOT(mouseOverGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutClicked(double,double,int)), this, SLOT(clickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(lbutReleased()), this, SLOT(dragDone()) );
    ConnectUI( theM, SIGNAL(lbutDoubleClicked(double,double,int)), this, SLOT(dblClickGraph(double,double,int)) );
    ConnectUI( theM, SIGNAL(cursorOverWindowCoords(int,int,int)), this, SLOT(mouseOverLabel(int,int,int)) );

    initCloseLbl();
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
}


void FileViewerWindow::setToolbarRanges()
{
    QDoubleSpinBox  *XS = toolBar->findChild<QDoubleSpinBox*>( "xscalesb" );
    QDoubleSpinBox  *YS = toolBar->findChild<QDoubleSpinBox*>( "yscalesb" );
    QSpinBox        *YP = toolBar->findChild<QSpinBox*>( "ypixsb" );
    QSpinBox        *ND = toolBar->findChild<QSpinBox*>( "ndivssb" );

    XS->blockSignals( true );
    YS->blockSignals( true );
    YP->blockSignals( true );
    ND->blockSignals( true );

    XS->setRange( 0.0001, qMin( 30.0, dataFile.fileTimeSecs() ) );
    XS->setValue( sav.xSpan );
    YS->setValue( sav.ySclNeu );
    YP->setValue( sav.yPix );
    ND->setValue( sav.nDivs );

    XS->blockSignals( false );
    YS->blockSignals( false );
    YP->blockSignals( false );
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


void FileViewerWindow::killActions()
{
    for( int ig = 0, nG = grfY.size(); ig < nG; ++ig )
        killShowHideAction( ig );
}


void FileViewerWindow::initGraphs()
{
    MGraphX *theX   = mscroll->theX;
    int     nG      = grfY.size();

    mscroll->scrollTo( 0 );

    theX->setVGridLines( sav.nDivs );
    theX->Y.clear();
    theX->ypxPerGrf     = sav.yPix;
    theX->drawCursor    = false;

    for( int ig = 0; ig < nG; ++ig ) {

        QAction     *a;
        int         &C  = ig2AcqChan[ig];
        GraphParams &P  = grfParams[ig];
        MGraphY     *Y  = &grfY[ig];

// BK: Want replacement for graphname tooltip
// Currently showing name as closeLabel-tooltip.
//        G->setToolTip( chanMap.e[ig].name );

        C = dataFile.channelIDs()[ig];

        P.niType        = dataFile.origID2Type( C );
        P.gain          = dataFile.origID2Gain( C );
        P.filter300Hz   = false;
        P.dcFilter      = P.niType == 0;

        Y->yscl         = (P.niType == 0 ? sav.ySclNeu :
                            (P.niType == 1 ? sav.ySclAux : 1));
        Y->label        = nameGraph( ig );
        Y->iclr         = (P.niType < 2 ? P.niType : 1);
        Y->num          = ig;
        Y->isDigType    = P.niType == 2;

        a = new QAction( QString::number( C ), this );
        a->setObjectName( QString::number( ig ) );
        a->setCheckable( true );
        a->setChecked( true );
        ConnectUI( a, SIGNAL(triggered()), this, SLOT(menuShowHideGraph()) );
        channelsMenu->addAction( a );

        grfActShowHide[ig]  = a;
        order2ig[ig]        = ig; // default is acqsort

// BK: ColorScheme need rethink
//        applyColorScheme( ig );
    }
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
    sav.yPix    = settings.value( "yPix", 100 ).toInt();
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
    settings.setValue( "fArrowKey", sav.fArrowKey );
    settings.setValue( "fPageKey", sav.fPageKey );
    settings.setValue( "xSpan", sav.xSpan );
    settings.setValue( "ySclNeu", sav.ySclNeu );
    settings.setValue( "ySclAux", sav.ySclAux );
    settings.setValue( "yPix", sav.yPix );
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

            const char  *unit   = "V";
            double      gain    = grfParams[igSelected].gain,
                        Y       = dataFile.niRng().span()
                                / (2 * gain * grfY[igSelected].yscl);

            if( dataFile.niRng().rmax / gain < 1.0 ) {

                unit = "mV";
                Y   *= 1000.0;
            }

            DT->setText( QString(" Boxes %1s x %2%3")
                        .arg( X ).arg( Y ).arg( unit ) );
        }
        else
            DT->setText( QString(" Boxes %1s x -").arg( X ) );
    }
    else
        DT->setText( " Boxes - x -" );
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
    if( ig < 0 || ig >= grfY.size() )
        return QString::null;

    return chanMap.name( ig, ig2AcqChan[ig] == dataFile.triggerChan() );
}


void FileViewerWindow::hideGraph( int ig )
{
    int nG = grfY.size();

    if( ig < 0 || ig >= nG )
        return;

// Don't hide last one

//    if( grfVisBits.count( false ) + 1 >= nG )
//        return;

// BK: Don't hide last <analog>, for now
    {
        int nA = 0;

        for( int i = 0; i < nG; ++i ) {

            if( grfParams[i].niType < 2 && grfVisBits.testBit( i ) ) {

                if( ++nA > 1 )
                    goto atLeast2;
            }
        }

        return;

atLeast2:;
    }

// Mark it

    grfVisBits.clearBit( ig );
    grfActShowHide[ig]->setChecked( false );

// Adjust selection
// BK: Only select <analog> channels, for now

    if( igSelected == ig ) {

        QVector<int>    ig2order( nG );

        for( int is = 0; is < nG; ++is )
            ig2order[order2ig[is]] = is;

        // try select next higher
        for( int is = ig2order[ig] + 1; is < nG; ++is ) {

            int ig2 = order2ig[is];

            if( grfParams[ig2].niType < 2 && grfVisBits.testBit( ig2 ) ) {

                selectGraph( ig2, false );
                goto doLayout;
            }
        }

        // try select next lower
        for( int is = ig2order[ig] - 1; is >= 0; --is ) {

            int ig2 = order2ig[is];

            if( grfParams[ig2].niType < 2 && grfVisBits.testBit( ig2 ) ) {

                selectGraph( ig2, false );
                goto doLayout;
            }
        }

        // select nothing
        selectGraph( -1, false );
    }

doLayout:
    layoutGraphs();
}


void FileViewerWindow::showGraph( int ig )
{
    if( ig < 0 || ig >= grfY.size() )
        return;

    grfVisBits.setBit( ig );
    grfActShowHide[ig]->setChecked( true );
    layoutGraphs();
}


void FileViewerWindow::selectGraph( int ig, bool updateGraph )
{
    if( igSelected == ig )
        return;

    igSelected = ig;

    if( updateGraph ) {
        mscroll->theX->setYSelByNum( ig );
        mscroll->theM->update();
    }

    toolBar->findChild<QLabel*>( "namelbl" )->setText( grfY[ig].label );

    if( ig == -1 )
        return;

    QDoubleSpinBox  *YS = toolBar->findChild<QDoubleSpinBox*>( "yscalesb" );
    QDoubleSpinBox  *GN = toolBar->findChild<QDoubleSpinBox*>( "gainsb" );
    QCheckBox       *HP = toolBar->findChild<QCheckBox*>( "hpchk" );
    QCheckBox       *DC = toolBar->findChild<QCheckBox*>( "dcchk" );

    YS->blockSignals( true );
    GN->blockSignals( true );
    HP->blockSignals( true );
    DC->blockSignals( true );

    YS->setValue( grfY[ig].yscl );
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
    hideCloseLabel();

    MGraphX *theX = mscroll->theX;

    if( igMaximized == -1 ) {

        igMaximized     = igSelected;
        theX->clipTop   = 0;
        theX->ypxPerGrf = mscroll->theM->height();
    }
    else {
        igMaximized     = -1;
        theX->ypxPerGrf = sav.yPix;
    }

    channelsMenu->setEnabled( igMaximized == -1 );
    toolBar->findChild<QSpinBox*>( "ypixsb" )->setEnabled( igMaximized == -1 );

    layoutGraphs();
    mscroll->scrollToSelected();
}


void FileViewerWindow::updateXSel( int graphSpan )
{
    MGraphX *theX = mscroll->theX;

    if( dragL >= 0
        && dragR > dragL
        && dragR < dfCount ) {

        // transform selection from scans to range [0..1].

        float   gselbeg = (dragL - pos) / double(graphSpan),
                gselend = (dragR - pos) / double(graphSpan);

        theX->setXSelEnabled( true );
        theX->setXSelRange( gselbeg, gselend );
    }
    else
        theX->setXSelEnabled( false );
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
    dwnSmp      = num2Read / (2 * mscroll->viewport()->width());

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

    for( int ic = 0; ic < nC; ++ic )
        grfY[onChans[ic]].resize( gtpts );

    mscroll->theX->initVerts( gtpts );

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

                // Subtract the average value over post-downsampling
                // data points from the first chunk. This is applied
                // to all chunks for smooth appearance.

                if( grfParams[ig].dcFilter ) {

                    if( !dcN[ic] ) {

                        for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                            dcSum[ic] += *d;

                        dcN[ic] += dtpts;
                    }

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

            grfY[ig].yval.putData( &ybuf[xoff], dtpts - xoff );
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

    mscroll->theM->update();
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

    MGraphX *theX = mscroll->theX;

    theX->bkgnd_Color  = bg;
    theX->grid_Color   = grid;
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

    msg = QString("Mouse tracking Graph %1 @ pos (%2 s, %3 %4)")
            .arg( STR2CHR( grfY[ig].label ) )
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


