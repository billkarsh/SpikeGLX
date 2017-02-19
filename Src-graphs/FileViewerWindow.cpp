
#include "ui_ChanListDialog.h"
#include "ui_FVW_MapDialog.h"
#include "ui_FVW_OptionsDialog.h"
#include "ui_FVW_NotesDialog.h"

#include "Pixmaps/close_but_16px.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "FileViewerWindow.h"
#include "FVToolbar.h"
#include "FVScanGrp.h"
#include "DataFileIMAP.h"
#include "DataFileIMLF.h"
#include "DataFileNI.h"
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
#include <QMessageBox>
#include <math.h>

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
/* Statics -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QVector<FVLink> FileViewerWindow::vlnk;

/* ---------------------------------------------------------------- */
/* class DCAve ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::DCAve::init( int nChannels, int nNeural )
{
    nC      = nChannels;
    nN      = nNeural;
    lvlOk   = false;
    lvl.fill( 0, nN );
}


void FileViewerWindow::DCAve::updateLvl(
    const qint16    *d,
    int             ntpts,
    int             dwnSmp )
{
    if( lvlOk || !nN )
        return;

    sum.fill( 0.0F, nN );

    int     *L      = &lvl[0];
    float   *S      = &sum[0];
    int     dStep   = nC * dwnSmp,
            dtpts   = (ntpts + dwnSmp - 1) / dwnSmp;

    for( int it = 0; it < ntpts; it += dwnSmp, d += dStep ) {

        for( int ic = 0; ic < nN; ++ic )
            S[ic] += d[ic];
    }

    for( int ic = 0; ic < nN; ++ic )
        L[ic] = S[ic]/dtpts;

    lvlOk = true;
}

/* ---------------------------------------------------------------- */
/* FileViewerWindow ----------------------------------------------- */
/* ---------------------------------------------------------------- */

FileViewerWindow::FileViewerWindow()
    :   QMainWindow(0), tMouseOver(-1.0), yMouseOver(-1.0),
        df(0), shankMap(0), chanMap(0), hipass(0),
        igSelected(-1), igMaximized(-1), igMouseOver(-1),
        didLayout(false), dragging(false)
{
    initDataIndepStuff();

    setAttribute( Qt::WA_DeleteOnClose, false );
    show();
}


FileViewerWindow::~FileViewerWindow()
{
    saveSettings();

    if( df )
        delete df;

    if( shankMap )
        delete shankMap;

    if( chanMap )
        delete chanMap;

    if( hipass )
        delete hipass;
}


bool FileViewerWindow::viewFile( const QString &fname, QString *errMsg )
{
    if( igMaximized != -1 )
        toggleMaximized();

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

    addToolBar( tbar = new FVToolbar( this, fType ) );
    scanGrp->setRanges( true );
    initHipass();

// --------------------------
// Manage previous array data
// --------------------------

    killActions();

// ---------------------------
// Resize arrays for this file
// ---------------------------

    int nG = df->numChans();

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

    sAveTable( tbGetSAveRad() );

// ---------------
// Initialize view
// ---------------

    selectGraph( 0, false );

    sav.sortUserOrder = !sav.sortUserOrder;
    tbToggleSort();

    return true;
}


QString FileViewerWindow::file() const
{
    if( df && df->isOpen() )
        return df->binFileName();

    return QString::null;
}


double FileViewerWindow::tbGetfileSecs() const
{
    return (df ? df->fileTimeSecs() : 0);
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

/* ---------------------------------------------------------------- */
/* Toolbar -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::tbToggleSort()
{
    sav.sortUserOrder = !sav.sortUserOrder;
    saveSettings();

    if( sav.sortUserOrder ) {
        tbar->setSortButText( "Usr Order" );
        chanMap->userOrder( order2ig );
    }
    else {
        tbar->setSortButText( "Acq Order" );
        chanMap->defaultOrder( order2ig );
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

    if( isActiveWindow() )
        linkSendPos( 2 );
}


// BK: Spinboxes invoking updateGraphs show strange behavior:
// If window showing a lot of data so update is slow, then
// clicks in spinner arrows cause two steps of value change.
// This doesn't happen if activating arrows by keyboard. It
// doesn't help to defer drawing with a queued call. Using
// a timer can help if the delay is a second or more. Very
// annoying.
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
    mscroll->theM->update();
}


void FileViewerWindow::tbSetMuxGain( double d )
{
    if( igSelected < 0 )
        return;

    grfParams[igSelected].gain = d;

    updateNDivText();
    printStatusMessage();
}


void FileViewerWindow::tbSetNDivs( int n )
{
    sav.nDivs = n;
    saveSettings();

    updateNDivText();
    mscroll->theX->setVGridLines( sav.nDivs );
    mscroll->theM->update();
}


void FileViewerWindow::tbHipassClicked( bool b )
{
    sav.bp300HzNi = b;
    saveSettings();

    updateGraphs();
}


void FileViewerWindow::tbSAveRadChanged( int radius )
{
    if( fType < 2 )
        sav.sAveRadIm = radius;
    else
        sav.sAveRadNi = radius;

    saveSettings();

    sAveTable( radius );
    updateGraphs();
}


void FileViewerWindow::tbDcClicked( bool b )
{
    if( fType == 0 )
        sav.dcChkOnImAp = b;
    else if( fType == 1 )
        sav.dcChkOnImLf = b;
    else
        sav.dcChkOnNi = b;

    saveSettings();

    updateGraphs();
}


void FileViewerWindow::tbBinMaxClicked( bool b )
{
    if( fType < 2 )
        sav.binMaxOnIm = b;
    else
        sav.binMaxOnNi = b;

    saveSettings();

    updateGraphs();
}


void FileViewerWindow::tbApplyAll()
{
    if( igSelected < 0 )
        return;

    GraphParams &P      = grfParams[igSelected];
    double      yScale  = grfY[igSelected].yscl;
    int         type    = grfY[igSelected].usrType;

    if( type == 0 ) {

        switch( fType ) {
            case 0:  sav.ySclImAp  = yScale; break;
            case 1:  sav.ySclImLf  = yScale; break;
            default: sav.ySclNiNeu = yScale;
        }
    }
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
/* FVW_MapDialog -------------------------------------------------- */
/* ---------------------------------------------------------------- */

static Ui::FVW_MapDialog*   mapUI;


void FileViewerWindow::cmDefaultBut()
{
    int ne = chanMap->e.size();

    for( int i = 0; i < ne; ++i )
        chanMap->e[i].order = i;

    chanMap->userOrder( order2ig );
    layoutGraphs();
}


void FileViewerWindow::cmMetaBut()
{
    if( chanMap )
        delete chanMap;

    chanMap = df->chanMap();

    chanMap->userOrder( order2ig );
    layoutGraphs();
}


void FileViewerWindow::cmApplyBut()
{
// Make mapping from user channel name to chanMap entry index
// Make mapping from table order to entry index

    QMap<int,int>   nam2Idx;
    QMap<int,int>   ord2Idx;
    QRegExp         re(";(\\d+)");
    int             ne = chanMap->e.size();

    for( int i = 0; i < ne; ++i ) {

        chanMap->e[i].name.contains( re );
        nam2Idx[re.cap(1).toInt()]      = i;
        ord2Idx[chanMap->e[i].order]    = i;
    }

// Initialize new order array with -1
// Mark all entry indices initially unused

    QVector<int>    newo( ne, -1 );
    QVector<bool>   used( ne, false );

// Parse user list and assign named chans to newo array

    QMap<int,int>::iterator it;
    int onext = 0;

    QString     s       = mapUI->listTE->toPlainText();
    QStringList terms   = s.split(
                            QRegExp("[,;]"),
                            QString::SkipEmptyParts );

    foreach( const QString &t, terms ) {

        QStringList rng = t.split(
                            QRegExp("[:-]"),
                            QString::SkipEmptyParts );
        int         n   = rng.count(),
                    r1, r2, id;
        bool        ok1, ok2;

        if( n > 2 ) {
            QMessageBox::critical(
                this,
                "Format Error",
                QString("Bad format: %1").arg( t ) );
            return;
        }

        if( n == 2 ) {

            r1  = rng[0].toUInt( &ok1 ),
            r2  = rng[1].toUInt( &ok2 );

            if( !ok1 || !ok2 )
                continue;

            if( r1 == r2 )
                goto justR1;

            if( r1 < r2 ) {

                for( int r = r1; r <= r2; ++r ) {

                    it = nam2Idx.find( r );

                    if( it == nam2Idx.end() )
                        continue;

                    id = it.value();

                    if( used[id] )
                        continue;

                    newo[id] = onext++;
                    used[id] = true;
                }
            }
            else {

                for( int r = r1; r >= r2; --r ) {

                    it = nam2Idx.find( r );

                    if( it == nam2Idx.end() )
                        continue;

                    id = it.value();

                    if( used[id] )
                        continue;

                    newo[id] = onext++;
                    used[id] = true;
                }
            }
        }
        else if( n == 1 ) {

justR1:
            int r1 = rng[0].toUInt( &ok1 );

            if( !ok1 )
                continue;

            it = nam2Idx.find( r1 );

            if( it == nam2Idx.end() )
                continue;

            id = it.value();

            if( used[id] )
                continue;

            newo[id] = onext++;
            used[id] = true;
        }
    }

// Walk all entries in order, appending if unused

    for( it = ord2Idx.begin(); it != ord2Idx.end(); ++it ) {

        int id = it.value();

        if( !used[id] ) {
            newo[id] = onext++;
            used[id] = true;
        }
    }

// Update

    for( int i = 0; i < ne; ++i )
        chanMap->e[i].order = newo[i];

    chanMap->userOrder( order2ig );
    layoutGraphs();
}

/* ---------------------------------------------------------------- */
/* Menu items ----------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::file_Link()
{
    FVLink* L = linkFindMe();

    if( !L )
        return;

// ----------------
// Turn linking off
// ----------------

    if( L->linked ) {
        linkSetLinked( L, false );
        return;
    }

// ---------------
// Turn linking on
// ---------------

// Test if others streams opened yet

    bool opened;

    L->win[fType] = 0;
    opened = L->winCount() > 0;
    L->win[fType] = this;


// If not, open them now

    if( !opened ) {

        QString path = df->binFileName();

        if( fType < 2 )
            path.remove( QRegExp("\\.imec.*") );
        else
            path.remove( QRegExp("\\.nidq.*") );

        QPoint  corner = pos();

        if( fType != 0 )
            opened |= linkOpenName( path + ".imec.ap.bin", corner );

        if( fType != 1 )
            opened |= linkOpenName( path + ".imec.lf.bin", corner );

        if( fType != 2 )
            opened |= linkOpenName( path + ".nidq.bin", corner );

        if( !opened )
            return;
    }

    linkSetLinked( L, true );
    linkSendPos( 3 );
    linkSendSel();
}


void FileViewerWindow::file_ChanMap()
{
// Show effects

    if( !sav.sortUserOrder )
        tbToggleSort();

// Make a backup chanMap

    ChanMap *cmBack;

    if( chanMap->type() == "nidq" )
        cmBack  = new ChanMapNI( *dynamic_cast<ChanMapNI*>(chanMap) );
    else
        cmBack  = new ChanMapIM( *dynamic_cast<ChanMapIM*>(chanMap) );

// Dialog setup

    QDialog             dlg;
    Ui::FVW_MapDialog   ui;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    mapUI = &ui;

    ui.setupUi( &dlg );
    ui.listTE->appendPlainText( cmChanStr );
    ConnectUI( ui.defaultBut, SIGNAL(clicked()), this, SLOT(cmDefaultBut()) );
    ConnectUI( ui.metaBut, SIGNAL(clicked()), this, SLOT(cmMetaBut()) );
    ConnectUI( ui.applyBut, SIGNAL(clicked()), this, SLOT(cmApplyBut()) );

// Run dialog

    if( QDialog::Accepted == dlg.exec() ) {
    }
    else {

        if( *chanMap != *cmBack ) {

            delete chanMap;
            chanMap = cmBack;
            cmBack  = 0;

            chanMap->userOrder( order2ig );
            layoutGraphs();
        }
    }

// Cleanup

    cmChanStr = ui.listTE->toPlainText();

    if( cmBack )
        delete cmBack;
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


void FileViewerWindow::file_Notes()
{
    QDialog             dlg;
    Ui::FVW_NotesDialog ui;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    ui.notesTE->setText( df->notes() );

    dlg.exec();
}


void FileViewerWindow::channels_ShowAll()
{
    hideCloseLabel();

    igMaximized = -1;

    int nG = grfY.size();

    grfVisBits.fill( true, nG );

    for( int ig = 0; ig < nG; ++ig )
        grfActShowHide[ig]->setChecked( true );

    if( igSelected == -1 )
        selectGraph( 0, false );

    mscroll->theX->ypxPerGrf = sav.yPix;

    layoutGraphs();
}


void FileViewerWindow::channels_HideAll()
{
    hideCloseLabel();

    igMaximized = -1;

    int nG = grfY.size();

    grfVisBits.fill( false, nG );

    for( int ig = 0; ig < nG; ++ig )
        grfActShowHide[ig]->setChecked( false );

    selectGraph( -1, false );

    layoutGraphs();
}


void FileViewerWindow::channels_Edit()
{
    QDialog             dlg;
    Ui::ChanListDialog  ui;

    dlg.setWindowFlags( dlg.windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    dlg.setWindowTitle( "Show These Channels" );

// List shown channels

    const QVector<uint> &src = df->channelIDs();

    QVector<uint>   chans;
    int             nG = grfY.size();

    for( int i = 0; i < nG; ++i ) {

        if( grfVisBits.testBit( i ) )
            chans.push_back( src[i] );
    }

    QString s = Subset::vec2RngStr( chans );

    ui.curLbl->setText( s );
    ui.chansLE->setText( s );

// Run dialog

    if( QDialog::Accepted == dlg.exec() ) {

        hideCloseLabel();

        igMaximized = -1;

        // Translate string to bits

        if( Subset::isAllChansStr( ui.chansLE->text() ) )
            Subset::defaultBits( grfVisBits, nG );
        else {

            QVector<uint>   chans;

            if( !Subset::rngStr2Vec( chans, ui.chansLE->text() ) ) {

                QMessageBox::critical(
                    this,
                    "Channels Error",
                    "Graphs list has bad format, view not changed." );
                return;
            }

            grfVisBits.fill( false, nG );

            int nC = chans.size();

            for( int ic = 0; ic < nC; ++ic ) {

                int idx = src.indexOf( chans[ic] );

                if( idx >= 0 )
                    grfVisBits.setBit( idx );
            }
        }

        // Adjust menu items

        for( int ig = 0; ig < nG; ++ig )
            grfActShowHide[ig]->setChecked( grfVisBits.testBit( ig ) );

        // Select lowest graph or -1

        if( igSelected == -1 || !grfVisBits.testBit( igSelected ) ) {

            int lowest = -1;

            for( int ig = 0; ig < nG; ++ig ) {

                if( grfVisBits.testBit( ig ) ) {
                    lowest = ig;
                    break;
                }
            }

            selectGraph( lowest, false );
        }
        else if( !grfVisBits.count( true ) )
            selectGraph( -1, false );

        mscroll->theX->ypxPerGrf = sav.yPix;

        layoutGraphs();
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
    exportCtl->initGrfRange( grfVisBits, igSelected );
    exportCtl->initTimeRange( dragL, dragR );
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

    tMouseOver = scanGrp->curTime() + x * sav.xSpan;
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

        qint64  pos = scanGrp->curPos(),
                p   = qBound(
                        0LL,
                        scanGrp->posFromTime( tMouseOver ),
                        dfCount - 1 );
        bool    moved = false;

        if( p < pos )
            moved = scanGrp->guiSetPos( p );
        else if( p > pos + nScansPerGraph() )
            moved = scanGrp->guiSetPos( qMax( 0LL, p - nScansPerGraph() ) );

        if( p >= dragAnchor ) {
            dragL   = dragAnchor;
            dragR   = p;
        }
        else {
            dragL   = p;
            dragR   = dragAnchor;
        }

        if( moved )
            updateGraphs();
        else
            updateXSel();

        linkSendSel();
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
        dragR       = scanGrp->curPos() + x * nScansPerGraph();
        dragging    = true;

        updateXSel();
    }
}


void FileViewerWindow::dragDone()
{
    if( dragging ) {

        dragging = false;

        if( dragR <= dragL ) {
            dragL = dragR = -1;
            updateXSel();
        }

        linkSendSel();
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

    int nV = grfVisBits.count( true );

    if( !nV )
        return;

    QSize   sz      = closeLbl->size();
    int     hLbl    = sz.height(),
            xLbl    = mscroll->viewport()->width() - sz.width(),
            yGrf    = (nV > 1 ? sav.yPix : mscroll->viewport()->height()),
            yLbl    = (yGrf - hLbl) / 2;

    if( x > xLbl && y > yLbl && y < yLbl + hLbl ) {

        hideCloseTimer->stop();

        QPoint  p( xLbl, yLbl + iy*sav.yPix - mscroll->theX->clipTop );
        p = mscroll->theM->mapToGlobal( p );

        int ig = mscroll->theX->Y[iy]->usrChan;

        closeLbl->setTag( ig );
        closeLbl->setToolTip( grfY[ig].lhsLabel );
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
/* Stream linking ------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::linkRecvPos( double t0, double tSpan, int fChanged )
{
    if( fChanged & 2 ) {

        sav.xSpan = qBound( 0.0001, tSpan, qMin( 30.0, tbGetfileSecs() ) );

        tbar->setXScale( sav.xSpan );
        updateNDivText();

        if( !(fChanged & 1) )
            updateGraphs();
    }

    scanGrp->setRanges( false );
    scanGrp->guiSetPos( scanGrp->posFromTime( t0 ) );
}


void FileViewerWindow::linkRecvSel( double tL, double tR )
{
    if( tR > tL ) {

        dragR = qBound( 0LL, scanGrp->posFromTime( tR ), dfCount - 1 );
        dragL = qBound( 0LL, scanGrp->posFromTime( tL ), dragR );
    }
    else
        dragL = dragR = -1;

    updateXSel();
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
        qint64      pos         = scanGrp->curPos();

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

        if( newPos >= 0.0 )
            return scanGrp->guiSetPos( newPos );
    }

    return QMainWindow::eventFilter( obj, e );
}


void FileViewerWindow::closeEvent( QCloseEvent *e )
{
    if( queryCloseOK() ) {

        QWidget::closeEvent( e );

        if( e->isAccepted() ) {
            linkRemoveMe();
            mainApp()->modelessClosed( this );
            deleteLater();
        }
    }
}


void FileViewerWindow::linkMenuChanged( bool linked )
{
    linkAction->setText( linked ? "Un&link" : "&Link" );
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
    linkAction = m->addAction( "&Link", this, SLOT(file_Link()), QKeySequence( tr("Ctrl+L") ) );
    m->addAction( "&Export...", this, SLOT(doExport()), QKeySequence( tr("Ctrl+E") ) );
    m->addSeparator();
    m->addAction( "&Channel Mapping...", this, SLOT(file_ChanMap()), QKeySequence( tr("Ctrl+M") ) );
    m->addAction( "&Time Scrolling...", this, SLOT(file_Options()) );
    m->addSeparator();
    m->addAction( "&View Notes", this, SLOT(file_Notes()) );

    m = mb->addMenu( "&Channels" );
    m->addAction( "&Show All", this, SLOT(channels_ShowAll()) );
    m->addAction( "&Hide All", this, SLOT(channels_HideAll()) );
    m->addAction( "&Edit List...", this, SLOT(channels_Edit()) );
    m->addSeparator();
    channelsMenu = m;

    m = mb->addMenu( "&Help" );
    m->addAction( "File Viewer &Help", mainApp(), SLOT(showFVWHelpWin()), QKeySequence( tr("Ctrl+H") ) );
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
// Toolbar added after file type known

    initMenus();

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

    if( fname_no_path.contains( ".imec.ap.", Qt::CaseInsensitive ) )
        fType = 0;
    else if( fname_no_path.contains( ".imec.lf.", Qt::CaseInsensitive ) )
        fType = 1;
    else if( fname_no_path.contains( ".nidq.", Qt::CaseInsensitive ) )
        fType = 2;
    else {

        QString err = QString("Missing type key in file name '%1'.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    if( df )
        delete df;

    switch( fType ) {
        case 0:  df = new DataFileIMAP; break;
        case 1:  df = new DataFileIMLF; break;
        default: df = new DataFileNI;
    }

    if( !df->openForRead( fname ) ) {

        QString err = QString("Error opening '%1'.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    if( !(dfCount = df->scanCount()) ) {

        QString err = QString("'%1' is empty.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    if( shankMap )
        delete shankMap;

    shankMap = df->shankMap();

    if( chanMap )
        delete chanMap;

    chanMap = df->chanMap();

    if( !chanMap->e.size() ) {

        QString err = QString("No channel map in '%1' meta data.")
                        .arg( fname_no_path );

        if( errMsg )
            *errMsg = err;

        Error() << err;
        return false;
    }

    double  srate   = df->samplingRateHz(),
            t0      = df->firstCt() / srate,
            dt      = dfCount / srate;

    setWindowTitle(
        QString(APPNAME " File Viewer: %1 [%2 chans @ %3 Hz] (t0, dt)=(%4, %5)")
        .arg( fname_no_path )
        .arg( df->numChans() )
        .arg( srate )
        .arg( t0, 0, 'f', 3 )
        .arg( dt, 0, 'f', 3 ) );

    mainApp()->modelessOpened( this );
    linkAddMe( fname_no_path );

    return true;
}


void FileViewerWindow::initHipass()
{
    if( hipass )
        delete hipass;

    hipass =
    new Biquad( bq_type_highpass, 300.0 / df->samplingRateHz() );
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

    // std neural
    theX->yColor.resize( 1 );

    // add lfp color if imec
    if( fType < 2 )
        theX->yColor.push_back( QColor( 0xff, 0x55, 0x00 ) );

    // add aux color
    theX->yColor.push_back( QColor( 0x44, 0xee, 0xff ) );

    theX->setVGridLines( sav.nDivs );
    theX->Y.clear();
    theX->ypxPerGrf     = sav.yPix;
    theX->drawCursor    = false;

    nSpikeChans = 0;
    nNeurChans  = 0;

    for( int ig = 0; ig < nG; ++ig ) {

        QAction     *a;
        int         &C  = ig2AcqChan[ig];
        MGraphY     &Y  = grfY[ig];
        GraphParams &P  = grfParams[ig];

        C = df->channelIDs()[ig];

        switch( fType ) {
            case 0:
                Y.usrType   = ((DataFileIMAP*)df)->origID2Type( C );
                Y.yscl      = (!Y.usrType ? sav.ySclImAp : sav.ySclAux);

                if( Y.usrType == 0 ) {
                    ++nSpikeChans;
                    ++nNeurChans;
                }
            break;
            case 1:
                Y.usrType   = ((DataFileIMLF*)df)->origID2Type( C );
                Y.yscl      = (!Y.usrType ? sav.ySclImLf : sav.ySclAux);

                if( Y.usrType == 1 )
                    ++nNeurChans;
            break;
            default:
                Y.usrType   = ((DataFileNI*)df)->origID2Type( C );
                Y.yscl      = (!Y.usrType ? sav.ySclNiNeu : sav.ySclAux);

                if( Y.usrType == 0 ) {
                    ++nSpikeChans;
                    ++nNeurChans;
                }
        }

        if( Y.usrType == 2 )
            Y.yscl = 1;

        Y.lhsLabel      = nameGraph( ig );
        Y.usrChan       = ig;
        Y.iclr          = (Y.usrType < 2 ? Y.usrType : 1);
        Y.isDigType     = Y.usrType == 2;

        P.gain          = df->origID2Gain( C );

        a = new QAction( QString::number( C ), this );
        a->setObjectName( QString::number( ig ) );
        a->setCheckable( true );
        a->setChecked( true );
        ConnectUI( a, SIGNAL(triggered()), this, SLOT(menuShowHideGraph()) );
        channelsMenu->addAction( a );

        grfActShowHide[ig]  = a;
        order2ig[ig]        = ig; // default is acqsort
    }
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void FileViewerWindow::loadSettings()
{
    STDSETTINGS( settings, "cc_fileviewer" );
    settings.beginGroup( "FileViewerWindow" );

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

    sav.xSpan       = settings.value( "xSpan", 4.0 ).toDouble();
    sav.ySclImAp    = settings.value( "ySclImAp", 1.0 ).toDouble();
    sav.ySclImLf    = settings.value( "ySclImLf", 1.0 ).toDouble();
    sav.ySclNiNeu   = settings.value( "ySclNiNeu", 1.0 ).toDouble();
    sav.ySclAux     = settings.value( "ySclAux", 1.0 ).toDouble();
    sav.yPix        = settings.value( "yPix", 100 ).toInt();
    sav.nDivs       = settings.value( "nDivs", 4 ).toInt();
    sav.sAveRadIm   = settings.value( "sAveRadIm", 0 ).toInt();
    sav.sAveRadNi   = settings.value( "sAveRadNi", 0 ).toInt();

    sav.xSpan = qMin( sav.xSpan, df->fileTimeSecs() );

// -------------
// sortUserOrder
// -------------

    sav.sortUserOrder = settings.value( "sortUserOrder", false ).toBool();

// -------
// Filters
// -------

    sav.bp300HzNi   = settings.value( "bp300HzNi", true ).toBool();

    sav.dcChkOnImAp = settings.value( "dcChkOnImAp", true ).toBool();
    sav.dcChkOnImLf = settings.value( "dcChkOnImLf", true ).toBool();
    sav.dcChkOnNi   = settings.value( "dcChkOnNi", true ).toBool();

    sav.binMaxOnIm = settings.value( "binMaxOnIm", false ).toBool();
    sav.binMaxOnNi = settings.value( "binMaxOnNi", false ).toBool();

    settings.endGroup();

    exportCtl->loadSettings( settings );
}


void FileViewerWindow::saveSettings() const
{
    STDSETTINGS( settings, "cc_fileviewer" );
    settings.beginGroup( "FileViewerWindow" );

    settings.setValue( "fArrowKey", sav.fArrowKey );
    settings.setValue( "fPageKey", sav.fPageKey );
    settings.setValue( "xSpan", sav.xSpan );
    settings.setValue( "ySclAux", sav.ySclAux );
    settings.setValue( "yPix", sav.yPix );
    settings.setValue( "nDivs", sav.nDivs );
    settings.setValue( "sortUserOrder", sav.sortUserOrder );

    if( fType < 2 ) {

        if( fType == 0 ) {
            settings.setValue( "ySclImAp", sav.ySclImAp );
            settings.setValue( "sAveRadIm", sav.sAveRadIm );
            settings.setValue( "dcChkOnImAp", sav.dcChkOnImAp );
            settings.setValue( "binMaxOnIm", sav.binMaxOnIm );
        }
        else {
            settings.setValue( "ySclImLf", sav.ySclImLf );
            settings.setValue( "dcChkOnImLf", sav.dcChkOnImLf );
        }
    }
    else {
        settings.setValue( "ySclNiNeu", sav.ySclNiNeu );
        settings.setValue( "sAveRadNi", sav.sAveRadNi );
        settings.setValue( "bp300HzNi", sav.bp300HzNi );
        settings.setValue( "dcChkOnNi", sav.dcChkOnNi );
        settings.setValue( "binMaxOnNi", sav.binMaxOnNi );
    }

    settings.endGroup();

    exportCtl->saveSettings( settings );
}


qint64 FileViewerWindow::nScansPerGraph() const
{
    return sav.xSpan * df->samplingRateHz();
}


void FileViewerWindow::updateNDivText()
{
    if( sav.nDivs > 0 ) {

        double  X = sav.xSpan / sav.nDivs;

        if( igSelected > -1 && grfY[igSelected].usrType < 2 ) {

            const char  *unit   = "V";
            double      gain    = grfParams[igSelected].gain,
                        Y       = df->vRange().span()
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
    return df->vRange().unityToVolts( (v+1)/2 )
            / grfParams[igMouseOver].gain;
}


QString FileViewerWindow::nameGraph( int ig ) const
{
    if( ig < 0 || ig >= grfY.size() )
        return QString::null;

    return chanMap->name( ig, df->isTrigChan( ig2AcqChan[ig] ) );
}


void FileViewerWindow::hideGraph( int ig )
{
    int nG = grfY.size();

    if( ig < 0 || ig >= nG )
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

    int nV = grfVisBits.count( true );

    if( nV == 1 )
        selectGraph( ig, false );
    else if( nV == 2 )
        mscroll->theX->ypxPerGrf = sav.yPix;

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

    if( ig >= 0 ) {

        tbar->setSelName( grfY[ig].lhsLabel );

        tbar->setYSclAndGain(
                grfY[ig].yscl,
                grfParams[ig].gain,
                grfY[ig].usrType < 2 );
    }
    else {

        tbar->setSelName( "None" );
        tbar->setYSclAndGain( 1, 1, false );
    }

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


// For each channel [0,nSpikeChan), calculate an 8-way
// neighborhood of indices into a timepoint's channels.
// - The index list excludes the central channel.
// - The list is sorted for cache friendliness.
//
void FileViewerWindow::sAveTable( int radius )
{
    TSM.clear();

    if( !nSpikeChans )
        return;

    TSM.resize( nSpikeChans );

    QMap<ShankMapDesc,uint> ISM;
    shankMap->inverseMap( ISM );

    for( int ic = 0; ic < nSpikeChans; ++ic ) {

        const ShankMapDesc  &E = shankMap->e[ic];

        if( !E.u )
            continue;

        QVector<int>    &V = TSM[ic];

        int xL  = qMax( int(E.c)  - radius, 0 ),
            xH  = qMin( uint(E.c) + radius + 1, shankMap->nc ),
            yL  = qMax( int(E.r)  - radius, 0 ),
            yH  = qMin( uint(E.r) + radius + 1, shankMap->nr );

        for( int ix = xL; ix < xH; ++ix ) {

            for( int iy = yL; iy < yH; ++iy ) {

                QMap<ShankMapDesc,uint>::iterator   it;

                it = ISM.find( ShankMapDesc( E.s, ix, iy, 1 ) );

                if( it == ISM.end() )
                    continue;

                int i = it.value();

                // Exclude self
                // Make zero-based

                if( i != ic )
                    V.push_back( i );
            }
        }

        qSort( V );
    }
}


// Space and time averaging for value: d_ig = &data[ig].
//
int FileViewerWindow::s_t_Ave( const qint16 *d_ig, int ig )
{
    const QVector<int>  &V = TSM[ig];
    const int           *L = &dc.lvl[0];

    int sum = 0,
        nv  = V.size();

    if( nv ) {

        const qint16    *d = d_ig - ig;

        for( int iv = 0; iv < nv; ++iv ) {

            int k = V[iv];

            sum += d[k] - L[k];
        }

        return *d_ig - sum/nv - L[ig];
    }

    return *d_ig - L[ig];
}


void FileViewerWindow::updateXSel()
{
    MGraphX *theX = mscroll->theX;

    if( dragL >= 0
        && dragR > dragL
        && dragR < dfCount ) {

        // transform selection from scans to range [0..1].

        double  span    = nScansPerGraph();
        qint64  pos     = scanGrp->curPos();
        float   gselbeg = (dragL - pos) / span,
                gselend = (dragR - pos) / span;

        theX->setXSelEnabled( true );
        theX->setXSelRange( gselbeg, gselend );
    }
    else
        theX->setXSelEnabled( false );

    mscroll->theM->update();
}

/* ---------------------------------------------------------------- */
/* updateGraphs --------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define MAX10BIT    512
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

    double  ysc,
            srate   = df->samplingRateHz();
    int     maxInt  = (fType < 2 ? MAX10BIT : MAX16BIT),
            nG      = df->numChans(),
            nVis    = grfVisBits.count( true );

    ysc = 1.0 / maxInt;

    QVector<uint>   iv2ig;

    Subset::bits2Vec( iv2ig, grfVisBits );

// -----------
// Scans setup
// -----------

    qint64  pos     = scanGrp->curPos(),
            xpos, num2Read;
    int     xflt    = qMin( (qint64)BIQUAD_TRANS_WIDE, pos ),
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

    for( int iv = 0; iv < nVis; ++iv )
        grfY[iv2ig[iv]].resize( gtpts );

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

    dc.init( nG, nNeurChans );

// --------------
// Process chunks
// --------------

    qint64  nRem = num2Read;

    for(;;) {

        if( nRem <= 0 )
            break;

        // ---------------
        // Read this block
        // ---------------

        vec_i16 data;
        qint64  nthis = qMin( chunk, nRem );

        ntpts = df->readScans( data, xpos, nthis, QBitArray() );

        if( ntpts <= 0 )
            break;

        // advance for next block

        xpos    += ntpts;
        nRem    -= ntpts;

        // ------
        // Filter
        // ------

        if( tbGet300HzOn() ) {
            hipass->applyBlockwiseMem(
                    &data[0], maxInt, ntpts, nG, 0, nSpikeChans );
        }

        if( tbGetDCChkOn() )
            dc.updateLvl( &data[0], ntpts, dwnSmp );

        // -------------
        // Result buffer
        // -------------

        dtpts = (ntpts + dwnSmp - 1) / dwnSmp;

        QVector<float>  ybuf( dtpts );

        // -------------------------
        // For each shown channel...
        // -------------------------

        for( int iv = 0; iv < nVis; ++iv ) {

            int     ig      = iv2ig[iv],
                    dstep   = dwnSmp * nG,
                    ny      = 0;
            qint16  *d      = &data[ig];

            if( grfY[ig].usrType == 0 ) {

                // ---------------
                // Neural channels
                // ---------------

                // -------------------
                // Neural downsampling
                // -------------------

                // Withing each bin, report the greatest
                // amplitude (pos or neg) extremum. This
                // ensures spikes are not missed. Result
                // in ybuf.

                if( tbGetBinMaxOn() && dwnSmp > 1 ) {

                    int ndRem = ntpts;

                    for( int it = 0; it < ntpts; it += dwnSmp ) {

                        qint16  *D      = d;
                        int     maxSqr  = *d * *d,
                                binWid  = dwnSmp;

                        d += nG;

                        if( ndRem < binWid )
                            binWid = ndRem;

                        for( int ib = 1; ib < binWid; ++ib, d += nG ) {

                            int sqr = *d * *d;

                            if( sqr > maxSqr ) {
                                maxSqr  = sqr;
                                D       = d;
                            }
                        }

                        ndRem -= binWid;

                        if( tbGetSAveRad() )
                            ybuf[ny++] = s_t_Ave( D, ig ) * ysc;
                        else
                            ybuf[ny++] = (*D - dc.lvl[ig]) * ysc;
                    }
                }
                else if( tbGetSAveRad() ) {

                    for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                        ybuf[ny++] = s_t_Ave( d, ig ) * ysc;
                }
                else {

                    for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                        ybuf[ny++] = (*d - dc.lvl[ig]) * ysc;
                }
            }
            else if( grfY[ig].usrType == 1 ) {

                // --------------------
                // Aux (or LF) channels
                // --------------------

                for( int it = 0; it < ntpts; it += dwnSmp, d += dstep )
                    ybuf[ny++] = (*d - dc.lvl[ig]) * ysc;
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

// -----------------
// Select and redraw
// -----------------

    updateXSel();
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
                    Y       = df->vRange().span()
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
                .arg( STR2CHR( grfY[ig].lhsLabel ) )
                .arg( t, 0, 'f', 4 )
                .arg( y, 0, 'f', 4 )
                .arg( unit );
    }
    else {

        // -------
        // Digital
        // -------

        msg = QString("Mouse tracking Graph %1 @ pos (%2 s)")
                .arg( STR2CHR( grfY[ig].lhsLabel ) )
                .arg( t, 0, 'f', 4 );
    }

    if( dragR > dragL ) {

        msg += QString("   Selection: [%1, %2]")
                .arg( scanGrp->timeFromPos( dragL ), 0, 'f', 4 )
                .arg( scanGrp->timeFromPos( dragR ), 0, 'f', 4 );
    }
    else
        msg += "   Ctrl+click to select time span";

    statusBar()->showMessage( msg );
}


bool FileViewerWindow::queryCloseOK()
{
// for now, always ok.
    return true;
}


FVLink* FileViewerWindow::linkFindMe()
{
    int nL = vlnk.size();

    for( int iL = 0; iL < nL; ++iL ) {

        if( vlnk[iL].win[fType] == this )
            return &vlnk[iL];
    }

    return 0;
}


FVLink* FileViewerWindow::linkFindRunName( const QString &runName )
{
    int nL = vlnk.size();

    for( int iL = 0; iL < nL; ++iL ) {

        if( vlnk[iL].runName == runName )
            return &vlnk[iL];
    }

    return 0;
}


// Return true if opened.
//
bool FileViewerWindow::linkOpenName( const QString &name, QPoint &corner )
{
    if( !QFile( name ).exists() )
        return false;

    QString         errorMsg;
    ConsoleWindow*  cons = mainApp()->console();

    if( !DataFile::isValidInputFile( name, &errorMsg ) ) {

        QMessageBox::critical(
            cons,
            "Error Opening File",
            QString("%1 cannot be used for input:\n[%2]")
            .arg( QFileInfo( name ).fileName() )
            .arg( errorMsg ) );
        return false;
    }

    FileViewerWindow    *fvw = new FileViewerWindow;

    corner += QPoint( 100, 100 );
    fvw->move( corner );

    if( !fvw->viewFile( name, &errorMsg ) ) {

        QMessageBox::critical(
            cons,
            "Error Opening File",
            errorMsg );

        Error() << "FileViewer open error: " << errorMsg;
        delete fvw;
        return false;
    }

    return true;
}


void FileViewerWindow::linkAddMe( QString runName )
{
// name -> base name

    if( fType < 2 )
        runName.remove( QRegExp("\\.imec.*") );
    else
        runName.remove( QRegExp("\\.nidq.*") );

// Add to existing named record, or add new record.

    FVLink* L = linkFindRunName( runName );

    if( L )
        L->win[fType] = this;
    else
        vlnk.push_back( FVLink( runName, this, fType ) );
}


void FileViewerWindow::linkRemoveMe()
{
    FVLink* L = linkFindMe();

    if( L ) {

        L->win[fType] = 0;

        int nW = L->winCount();

        if( nW == 1 )
            linkSetLinked( L, false );
        else if( !nW )
            vlnk.remove( L - &vlnk[0] );
    }
}


void FileViewerWindow::linkSetLinked( FVLink *L, bool linked )
{
    L->linked = linked;

    for( int iw = 0; iw < 3; ++iw ) {

        if( L->win[iw] )
            L->win[iw]->linkMenuChanged( linked );
    }
}


// Change-flags: {1=pos, 2=span, 3=both}
//
void FileViewerWindow::linkSendPos( int fChanged )
{
    FVLink* L = linkFindMe();

    if( !L || !L->linked )
        return;

    double  t0      = scanGrp->curTime(),
            tSpan   = sav.xSpan;

    for( int iw = 0; iw < 3; ++iw ) {

        FileViewerWindow    *fvw = L->win[iw];

        if( fvw && fvw != this ) {

            QMetaObject::invokeMethod(
                fvw, "linkRecvPos",
                Qt::QueuedConnection,
                Q_ARG(double, t0),
                Q_ARG(double, tSpan),
                Q_ARG(int, fChanged) );
        }
    }
}


void FileViewerWindow::linkSendSel()
{
    FVLink* L = linkFindMe();

    if( !L || !L->linked )
        return;

    double  tL, tR;

    if( dragL < dragR ) {
        tL = scanGrp->timeFromPos( dragL );
        tR = scanGrp->timeFromPos( dragR );
    }
    else
        tL = tR = -1;

    for( int iw = 0; iw < 3; ++iw ) {

        FileViewerWindow    *fvw = L->win[iw];

        if( fvw && fvw != this ) {

            QMetaObject::invokeMethod(
                fvw, "linkRecvSel",
                Qt::QueuedConnection,
                Q_ARG(double, tL),
                Q_ARG(double, tR) );
        }
    }
}


