
#include "ui_FVW_OptionsDialog.h"

#include "Pixmaps/close_but_16px.xpm"

#include "Util.h"
#include "MainApp.h"
#include "FileViewerWindow.h"
#include "FVToolbar.h"
#include "FVScanGrp.h"
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
#include <QMenuBar>
#include <QStatusBar>
#include <QSlider>
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
/* class TaggableLabel -------------------------------------------- */
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
        hipass(0), igMouseOver(-1), didLayout(false), dragging(false)
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
    tbar->setRanges();
    scanGrp->setRanges( true );
    initHipass();

// --------------------------
// Manage previous array data
// --------------------------

    killActions();

// ---------------------------
// Resize arrays for this file
// ---------------------------

    int nG = df.numChans();

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
    tbToggleSort();

    return true;
}


void FileViewerWindow::getInverseGains(
    std::vector<double> &invGain,
    const QBitArray     &exportBits ) const
{
    invGain.clear();

    for( int i = 0, nC = exportBits.size(); i < nC; ++i ) {

        if( exportBits.testBit( i ) )
            invGain.push_back( 1.0 / grfParams[i].gain );
    }
}


void FileViewerWindow::tbToggleSort()
{
    sav.sortUserOrder = !sav.sortUserOrder;
    saveSettings();

    if( sav.sortUserOrder ) {
        tbar->setSortButText( "Usr Order" );
        chanMap.userOrder( order2ig );
    }
    else {
        tbar->setSortButText( "Acq Order" );
        chanMap.defaultOrder( order2ig );
    }

    layoutGraphs();
}


void FileViewerWindow::tbScrollToSelected()
{
    mscroll->scrollToSelected();
}


void FileViewerWindow::tbSetXScale( double d )
{
    sav.xSpan = d;
    saveSettings();

    updateNDivText();

    scanGrp->setRanges( false );

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
void FileViewerWindow::tbSetYPix( int n )
{
    sav.yPix = n;
    saveSettings();

    mscroll->theX->ypxPerGrf = n;

    layoutGraphs();
}


void FileViewerWindow::tbSetYScale( double d )
{
    if( igSelected < 0 )
        return;

    grfY[igSelected].yscl = d;

    updateNDivText();
    updateGraphs();
}


void FileViewerWindow::tbSetMuxGain( double d )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].gain = d;

    updateNDivText();
    printStatusMessage();

    updateGraphs();
}


void FileViewerWindow::tbSetNDivs( int n )
{
    sav.nDivs = n;
    saveSettings();

    updateNDivText();
    mscroll->theX->setVGridLines( sav.nDivs );

    updateGraphs();
}


void FileViewerWindow::tbHipassClicked( bool b )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].filter300Hz = b;

    updateGraphs();
}


void FileViewerWindow::tbDcClicked( bool b )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].dcFilter = b;

    updateGraphs();
}


void FileViewerWindow::tbApplyAll()
{
    if( igSelected < 0 )
        return;

    GraphParams &P      = grfParams[igSelected];
    double      yScale  = grfY[igSelected].yscl;
    int         type    = grfY[igSelected].usrType;

    if( type == 0 )
        sav.ySclNeu = yScale;
    else if( type == 1 )
        sav.ySclAux = yScale;

    if( type < 2 )
        saveSettings();

    for( int ig = 0, nG = grfParams.size(); ig < nG; ++ig ) {

        if( ig != igSelected && grfY[ig].usrType == type ) {

            grfY[ig].yscl   = yScale;
            grfParams[ig]   = P;
        }
    }

    if( didLayout )
        updateGraphs();
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

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

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

    exportCtl->initDataFile( df );
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

    igMouseOver = mscroll->theX->Y[iy]->usrChan;

    if( igMouseOver < 0 || igMouseOver >= (int)grfY.size() ) {
        statusBar()->clearMessage();
        return;
    }

// ----------------
// Position readout
// ----------------

    tMouseOver = scanGrp->getTime() + x * sav.xSpan;
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

        qint64  pos = scanGrp->getPos(),
                p   = qBound(
                        0LL,
                        scanGrp->posFromTime( tMouseOver ),
                        dfCount - 1 );

        if( p < pos )
            scanGrp->guiSetPos( p );
        else if( p > pos + nScansPerGraph() )
            scanGrp->guiSetPos( qMax( 0LL, p - nScansPerGraph() ) );

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
        dragR       = scanGrp->getPos() + x * nScansPerGraph();
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

        int ig = mscroll->theX->Y[iy]->usrChan;

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

            if( grfVisBits.testBit( ig ) )
                theX->Y.push_back( &grfY[ig] );
        }
    }

// ------
// Layout
// ------

    mscroll->adjustLayout();
    theX->setYSelByUsrChan( igSelected );
    didLayout = true;
    updateGraphs();
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

bool FileViewerWindow::eventFilter( QObject *obj, QEvent *e )
{
    if( (obj == mscroll || obj == scanGrp->getSliderObj())
        && e->type() == QEvent::KeyPress ) {

        QKeyEvent   *keyEvent   = static_cast<QKeyEvent*>(e);
        double      newPos      = -1.0; // illegal
        qint64      pos         = scanGrp->getPos();

        switch( keyEvent->key() ) {

            case Qt::Key_Home:
                newPos = 0;
                break;
            case Qt::Key_End:
                newPos = scanGrp->maxPos();
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

            scanGrp->guiSetPos( newPos );
            return true;
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
            deleteLater();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Data-independent inits ----------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::initMenus()
{
//#ifdef Q_OS_MACX
//    // shared menu bar on OSX
//  QMenuBar *mb = mainApp()->console()->menuBar();
//#else
//    // otherwise window-specific menu bar
    QMenuBar    *mb = menuBar();
//#endif

    QMenu   *m;

    m = mb->addMenu( "&File" );
    m->addAction( "&Open In This Viewer...", this, SLOT(file_Open()) );
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


void FileViewerWindow::initExport()
{
    exportAction = new QAction( "Export...", this );
    ConnectUI( exportAction, SIGNAL(triggered()), this, SLOT(doExport()) );

    exportCtl = new ExportCtl( this );
}


void FileViewerWindow::initCloseLbl()
{
    QPixmap pm( close_but_16px );

// BK: In Qt 5.3, setting Qt::Popup for a child window issues a warning
// that the flag is for top-level windows...but it behaves perfectly in
// all Qt versions.

    closeLbl = new TaggableLabel(
                    mscroll->theM,
                    Qt::Popup | Qt::FramelessWindowHint );
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

// Top-most controls

    initMenus();
    addToolBar( tbar = new FVToolbar( this ) );

// CentralWidget gets vertical layout of two items:
// - graphs
// - 'slider' (position controls)

    QWidget *cw = new QWidget;
    setCentralWidget( cw );

    mscroll = new MGScroll( "fvw", cw );
    mscroll->installEventFilter( this );

    QVBoxLayout *VL = new QVBoxLayout( cw );
    VL->addWidget( mscroll, 1 );
    VL->addWidget( scanGrp = new FVScanGrp( this ) );

// Status bar below

    statusBar();

// Additional once-only

    initExport();

    // aux color
    mscroll->theX->yColor.push_back( QColor( 0x44, 0xee, 0xff ) );

    MGraph  *theM = mscroll->theM;
    theM->setImmedUpdate( true );
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

    if( !df.openForRead( fname ) ) {

        QString err = QString("Error opening '%1'.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    if( !(dfCount = df.scanCount()) ) {

        QString err = QString("'%1' is empty.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    chanMap = df.chanMap();

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
        .arg( df.numChans() )
        .arg( df.samplingRateHz() )
        .arg( dfCount ) );

    return true;
}


void FileViewerWindow::applyStyles()
{
    for( int is = 0; is < (int)N_ColorScheme; ++is )
        colorSchemeActions[is]->setChecked( is == sav.colorScheme );
}


void FileViewerWindow::initHipass()
{
    if( hipass )
        delete hipass;

    hipass =
    new Biquad( bq_type_highpass, 300.0 / df.samplingRateHz() );
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
        MGraphY     &Y  = grfY[ig];
        GraphParams &P  = grfParams[ig];

        C = df.channelIDs()[ig];

        Y.usrType       = df.origID2Type( C );
        Y.yscl          = (Y.usrType == 0 ? sav.ySclNeu :
                            (Y.usrType == 1 ? sav.ySclAux : 1));
        Y.label         = nameGraph( ig );
        Y.usrChan       = ig;
        Y.iclr          = (Y.usrType < 2 ? Y.usrType : 1);
        Y.isDigType     = Y.usrType == 2;

        P.gain          = df.origID2Gain( C );
        P.filter300Hz   = false;
        P.dcFilter      = Y.usrType == 0;

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

    sav.xSpan = qMin( sav.xSpan, df.fileTimeSecs() );

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
    return sav.xSpan * df.samplingRateHz();
}


void FileViewerWindow::updateNDivText()
{
    if( sav.nDivs > 0 ) {

        double  X = sav.xSpan / sav.nDivs;

        if( igSelected > -1 && grfY[igSelected].usrType < 2 ) {

            const char  *unit   = "V";
            double      gain    = grfParams[igSelected].gain,
                        Y       = df.vRange().span()
                                / (2 * gain * grfY[igSelected].yscl);

            if( Y < 0.001 ) {
                Y   *= 1e6;
                unit = "uV";
            }
            else if( Y < 1.0 ) {
                Y   *= 1e3;
                unit = "mV";
            }

            tbar->setNDivText( QString(" Boxes %1s x %2%3")
                                .arg( X ).arg( Y ).arg( unit ) );
        }
        else
            tbar->setNDivText( QString(" Boxes %1s x -").arg( X ) );
    }
    else
        tbar->setNDivText( " Boxes - x -" );
}


// Values (v) are in range [-1,1].
// (v+1)/2 is in range [0,1].
// This is mapped to range [rmin,rmax].
//
double FileViewerWindow::scalePlotValue( double v )
{
    return df.vRange().unityToVolts( (v+1)/2 )
            / grfParams[igMouseOver].gain;
}


QString FileViewerWindow::nameGraph( int ig ) const
{
    if( ig < 0 || ig >= grfY.size() )
        return QString::null;

    return chanMap.name( ig, df.isTrigChan( ig2AcqChan[ig] ) );
}


void FileViewerWindow::hideGraph( int ig )
{
    int nG = grfY.size();

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

            int ig2 = order2ig[is];

            if( grfVisBits.testBit( ig2 ) ) {

                selectGraph( ig2, false );
                goto doLayout;
            }
        }

        // try select next lower
        for( int is = ig2order[ig] - 1; is >= 0; --is ) {

            int ig2 = order2ig[is];

            if( grfVisBits.testBit( ig2 ) ) {

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
        mscroll->theX->setYSelByUsrChan( ig );
        mscroll->theM->update();
    }

    tbar->setSelName( grfY[ig].label );

    if( ig == -1 )
        return;

    tbar->setYSclAndGain(
            grfY[ig].yscl,
            grfParams[ig].gain,
            grfY[ig].usrType < 2 );

    tbar->setFltChecks(
        grfParams[ig].filter300Hz,
        grfParams[ig].dcFilter,
        grfY[ig].usrType == 0 );

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
    tbar->enableYPix( igMaximized == -1 );

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

        qint64  pos     = scanGrp->getPos();
        float   gselbeg = (dragL - pos) / double(graphSpan),
                gselend = (dragR - pos) / double(graphSpan);

        theX->setXSelEnabled( true );
        theX->setXSelRange( gselbeg, gselend );
    }
    else
        theX->setXSelEnabled( false );
}


#define MAX16BIT    32768


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

    double  ysc     = 1.0 / MAX16BIT,
            srate   = df.samplingRateHz();
    int     nC      = grfVisBits.count( true );

    QVector<uint>   onChans;

    Subset::bits2Vec( onChans, grfVisBits );

// -----------
// Scans setup
// -----------

    qint64  pos     = scanGrp->getPos(),
            xpos, num2Read;
    int     xflt    = qMin( 120LL, pos ),
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

        ntpts = df.readScans( data, xpos, nthis, grfVisBits );

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

            if( grfY[ig].usrType == 0 ) {

                // ---------------
                // Neural channels
                // ---------------

                // ------
                // Filter
                // ------

                if( grfParams[ig].filter300Hz ) {
                    hipass->apply1BlockwiseMemAll(
                                &data[0], MAX16BIT, ntpts, nC, ic );
                }

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

                        int val = *d;

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
            else if( grfY[ig].usrType == 1 ) {

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
    QColor(0x20, 0x3c, 0x3c, 0xff),
    QColor(0x4f, 0x4f, 0x4f, 0xff),
    QColor(0x1f, 0x1f, 0x1f, 0xff)};


void FileViewerWindow::applyColorScheme( int ig )
{
    QColor  bg, grid, fg;

    switch( sav.colorScheme ) {

        case Ice:
            fg = clrIce[grfY[ig].usrType];
            bg.setRgbF( .15, .15, .15 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Fire:
            fg = clrFire[grfY[ig].usrType];
            bg.setRgbF( .15, .15, .15 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Green:
            fg = clrGreen[grfY[ig].usrType];
            bg.setRgbF( .07, .07, .07 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case BlackWhite:
            fg = clrBW[grfY[ig].usrType];
            bg.setRgbF( .05, .05, .05 );
            grid.setRgbF( 0.4, 0.4, 0.4 );
            break;
        case Classic:
        default:
            bg      = clrClassic[grfY[ig].usrType];
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
    QString msg;

    if( grfY[ig].usrType <= 1 ) {

        // ---------------
        // Analog channels
        // ---------------

        const char  *unit   = "V";
        double      gain    = grfParams[ig].gain,
                    Y       = df.vRange().span()
                            / (2 * gain * grfY[ig].yscl);

        if( Y < 0.001 ) {
            y   *= 1e6;
            unit = "uV";
        }
        else if( Y < 1.0 ) {
            y   *= 1e3;
            unit = "mV";
        }

        msg = QString("Mouse tracking Graph %1 @ pos (%2 s, %3 %4)")
                .arg( STR2CHR( grfY[ig].label ) )
                .arg( t, 0, 'f', 4 )
                .arg( y, 0, 'f', 4 )
                .arg( unit );
    }
    else {

        // -------
        // Digital
        // -------

        msg = QString("Mouse tracking Graph %1 @ pos (%2 s)")
                .arg( STR2CHR( grfY[ig].label ) )
                .arg( t, 0, 'f', 4 );
    }

    if( dragL >= 0 && dragR >= 0 ) {

        msg += QString(" - Selection range: scans(%1,%2) secs(%3,%4)")
                .arg( dragL )
                .arg( dragR )
                .arg( scanGrp->timeFromPos( dragL ), 0, 'f', 4 )
                .arg( scanGrp->timeFromPos( dragR ), 0, 'f', 4 );
    }

    statusBar()->showMessage( msg );
}


bool FileViewerWindow::queryCloseOK()
{
// for now, always ok.
    return true;
}


