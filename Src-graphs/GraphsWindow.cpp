
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "GWToolbar.h"
#include "GWLEDWidget.h"
#include "GWImWidgetG.h"
#include "GWNiWidgetG.h"
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


GraphsWindow::GraphsWindow( DAQ::Params &p ) : QMainWindow(0), p(p)
{
    resize( 1024, 768 );

// Install widgets

    addToolBar( tbar = new GWToolbar( this, p ) );
    statusBar()->addPermanentWidget( LED = new GWLEDWidget( p ) );

    QSplitter   *sp = new QSplitter;
    sp->setOrientation( Qt::Vertical );

    if( p.im.enabled )
        sp->addWidget( imW = new GWImWidgetG( this, p ) );

    if( p.ni.enabled )
        sp->addWidget( niW = new GWNiWidgetG( this, p ) );

    visibleGrabHandle( sp );
    setCentralWidget( sp );

// Init toolbar

    QString chanName;
    int     ic;
    eStream st;

    selection.ic = -1;   // force initialization

    if( p.im.enabled ) {
        st = imStream;
        ic = imW->initialSelectedChan( chanName );
    }
    else {
        st = niStream;
        ic = niW->initialSelectedChan( chanName );
    }

    setSelection( st, ic, chanName );

// Init viewer states

    tbHipassClicked( tbar->getFltCheck() );   // assert filters
}


// Note:
// Destructor order: {GW, toolbar, LED, niW}.
//
GraphsWindow::~GraphsWindow()
{
    setUpdatesEnabled( false );
}


void GraphsWindow::remoteSetTrgEnabled( bool on )
{
    tbar->setTrigCheck( on );
    tbSetTrgEnable( on );
}


void GraphsWindow::remoteSetRunLE( const QString &name )
{
    tbar->setRunLE( name );
}


void GraphsWindow::showHideSaveChks()
{
    if( p.im.enabled )
        imW->showHideSaveChks();

    if( p.ni.enabled )
        niW->showHideSaveChks();
}


void GraphsWindow::sortGraphs()
{
    tbar->updateSortButText();

    QString dum;

    if( p.im.enabled ) {

        if( imW->getMaximized() >= 0 )
            imW->toggleMaximized( -1 );

        imW->sortGraphs();

        if( selection.stream == imStream )
            imW->ensureVisible( selection.ic );
        else
            imW->ensureVisible( imW->initialSelectedChan( dum ) );
    }

    if( p.ni.enabled ) {

        if( niW->getMaximized() >= 0 )
            niW->toggleMaximized( -1 );

        niW->sortGraphs();

        if( selection.stream == niStream )
            niW->ensureVisible( selection.ic );
        else
            niW->ensureVisible( niW->initialSelectedChan( dum ) );
    }

    tbar->update();
}


void GraphsWindow::eraseGraphs()
{
    if( p.im.enabled )
        imW->eraseGraphs();

    if( p.ni.enabled )
        niW->eraseGraphs();
}


void GraphsWindow::imPutScans( vec_i16 &data, quint64 firstSamp )
{
    imW->putScans( data, firstSamp );
}


void GraphsWindow::niPutScans( vec_i16 &data, quint64 firstSamp )
{
    niW->putScans( data, firstSamp );
}


bool GraphsWindow::tbIsSelMaximized() const
{
    if( selection.ic < 0 )
        return false;

    if( selection.stream == imStream )
        return imW->getMaximized() == selection.ic;
    else
        return niW->getMaximized() == selection.ic;
}


bool GraphsWindow::tbIsSelGraphAnalog() const
{
    if( selection.stream == imStream )
        return imW->isChanAnalog( selection.ic );
    else
        return niW->isChanAnalog( selection.ic );
}


void GraphsWindow::tbGetSelGraphScales( double &xSpn, double &yScl ) const
{
    if( selection.stream == imStream )
        imW->getGraphScales( xSpn, yScl, selection.ic );
    else
        niW->getGraphScales( xSpn, yScl, selection.ic );
}


QColor GraphsWindow::tbGetSelGraphColor() const
{
    if( selection.stream == imStream )
        return imW->getGraphColor( selection.ic );
    else
        return niW->getGraphColor( selection.ic );
}


void GraphsWindow::imSetSelection( int ic, const QString &name )
{
    setSelection( imStream, ic, name );
}


void GraphsWindow::niSetSelection( int ic, const QString &name )
{
    setSelection( niStream, ic, name );
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


void GraphsWindow::ensureSelectionVisible()
{
    if( selection.stream == imStream )
        imW->ensureVisible( selection.ic );
    else
        niW->ensureVisible( selection.ic );
}


void GraphsWindow::toggledMaximized()
{
    tbar->update();
}


void GraphsWindow::tbToggleMaximized()
{
    if( selection.stream == imStream )
        imW->toggleMaximized( selection.ic );
    else
        niW->toggleMaximized( selection.ic );

    tbar->update();
}


void GraphsWindow::tbGraphSecsChanged( double secs )
{
    if( selection.stream == imStream )
        imW->graphSecsChanged( secs, selection.ic );
    else
        niW->graphSecsChanged( secs, selection.ic );
}


void GraphsWindow::tbGraphYScaleChanged( double scale )
{
    if( selection.stream == imStream )
        imW->graphYScaleChanged( scale, selection.ic );
    else
        niW->graphYScaleChanged( scale, selection.ic );
}


void GraphsWindow::tbShowColorDialog()
{
    QColor c = tbar->selectColor( tbGetSelGraphColor() );

    if( c.isValid() ) {

        if( selection.stream == imStream )
            imW->colorChanged( c, selection.ic );
        else
            niW->colorChanged( c, selection.ic );

        tbar->update();
    }
}


void GraphsWindow::tgApplyAll()
{
    if( selection.stream == imStream )
        imW->applyAll( selection.ic );
    else
        niW->applyAll( selection.ic );
}


void GraphsWindow::tbHipassClicked( bool checked )
{
    if( p.im.enabled )
        imW->hipassChecked( checked );

    if( p.ni.enabled )
        niW->hipassChecked( checked );

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "DataOptions" );
    settings.setValue( "filter", checked );
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

        if( sender() ) {

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

                    tbar->setTrigCheck( false );
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
        }

        LED->setOnColor( QLED::Green );
    }
    else
        LED->setOnColor( QLED::Yellow );

    tbar->enableRunLE( !checked );
    run->dfSetTrgEnabled( checked );

// update graph checks

    if( p.im.enabled )
        imW->enableAllChecks( !checked );

    if( p.ni.enabled )
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


void GraphsWindow::setSelection(
    eStream         stream,
    int             ic,
    const QString   &name )
{
// Changed?
    if( stream == selection.stream && ic == selection.ic )
        return;

// Deselect previous
    if( selection.ic >= 0 ) {

        if( selection.stream == imStream )
            imW->selectChan( selection.ic, false );
        else
            niW->selectChan( selection.ic, false );
    }

// Select new
    selection.stream    = stream;
    selection.ic        = ic;

    if( stream == imStream )
        imW->selectChan( ic, true );
    else
        niW->selectChan( ic, true );

    tbar->setSelName( name );
}


