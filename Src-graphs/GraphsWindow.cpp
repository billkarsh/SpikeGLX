
#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "GraphsWindow.h"
#include "GWToolbar.h"
#include "GWLEDWidget.h"
#include "GWNiWidget.h"
#include "ConfigCtl.h"

#include <QAction>
#include <QKeyEvent>
#include <QStatusBar>
#include <QMessageBox>
#include <QSettings>




/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

const QColor    NeuGraphBGColor( 0x2f, 0x4f, 0x4f, 0xff ),
                AuxGraphBGColor( 0x4f, 0x4f, 0x4f, 0xff ),
                DigGraphBGColor( 0x1f, 0x1f, 0x1f, 0xff );

/* ---------------------------------------------------------------- */
/* class GraphsWindow --------------------------------------------- */
/* ---------------------------------------------------------------- */

GraphsWindow::GraphsWindow( DAQ::Params &p ) : QMainWindow(0), p(p)
{
    resize( 1024, 768 );

// Install widgets

    addToolBar( tbar = new GWToolbar( this, p ) );
    statusBar()->addPermanentWidget( LED = new GWLEDWidget( p ) );
    setCentralWidget( niW = new GWNiWidget( this, p ) );

// Init toolbar

    QString chanName;

    if( p.im.enabled ) {
    }
    else {
        selection.stream    = niStream;
        selection.ic        = niW->initialSelectedChan( chanName );
    }

    selection.maximized = false;

    tbar->setSelName( chanName );

// Init viewer states

    hipassClicked( tbar->getFltCheck() );   // assert filters
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
    setTrgEnable( on );
}


void GraphsWindow::remoteSetRunLE( const QString &name )
{
    tbar->setRunLE( name );
}


void GraphsWindow::showHideSaveChks()
{
    if( p.im.enabled ) {
    }

    if( p.ni.enabled )
        niW->showHideSaveChks();
}


void GraphsWindow::sortGraphs()
{
    tbar->updateSortButText();

    if( selection.maximized )
        toggleMaximized();

    if( p.im.enabled ) {
    }

    if( p.ni.enabled )
        niW->sortGraphs();

    ensureSelectionVisible();
}


void GraphsWindow::niPutScans( vec_i16 &data, quint64 firstSamp )
{
    niW->putScans( data, firstSamp );
}


void GraphsWindow::eraseGraphs()
{
    niW->eraseGraphs();
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
        ;
    else
        niW->ensureVisible( selection.ic );
}


void GraphsWindow::toggleMaximized()
{
    if( selection.stream == imStream )
        ;
    else
        niW->toggleMaximized( selection.ic, selection.maximized );

    selection.maximized = !selection.maximized;
    tbar->update();
}


void GraphsWindow::graphSecsChanged( double secs )
{
    if( selection.stream == imStream )
        ;
    else
        niW->graphSecsChanged( secs, selection.ic );
}


void GraphsWindow::graphYScaleChanged( double scale )
{
    if( selection.stream == imStream )
        ;
    else
        niW->graphYScaleChanged( scale, selection.ic );
}


void GraphsWindow::showColorDialog()
{
    QColor c = tbar->selectColor( getSelGraphColor() );

    if( c.isValid() ) {

        if( selection.stream == imStream )
            ;
        else
            niW->colorChanged( c, selection.ic );

        tbar->update();
    }
}


void GraphsWindow::applyAll()
{
    if( selection.stream == imStream )
        ;
    else
        niW->applyAll( selection.ic );
}


void GraphsWindow::hipassClicked( bool checked )
{
    niW->hipassChecked( checked );

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "DataOptions" );
    settings.setValue( "filter", checked );
}


void GraphsWindow::setTrgEnable( bool checked )
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
    mainApp()->file_AskStopRun();
    e->ignore();
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
            ;
        else
            niW->selectChan( selection.ic, false );
    }

// Select new
    selection.stream    = stream;
    selection.ic        = ic;

    if( stream == imStream )
        ;
    else
        niW->selectChan( ic, true );

    tbar->setSelName( name );
}


bool GraphsWindow::isSelGraphAnalog() const
{
    if( selection.stream == imStream )
        return true;
    else
        return niW->isChanAnalog( selection.ic );
}


void GraphsWindow::getSelGraphScales( double &xSpn, double &yScl ) const
{
    if( selection.stream == imStream )
        ;
    else
        niW->getGraphScales( xSpn, yScl, selection.ic );
}


QColor GraphsWindow::getSelGraphColor() const
{
//    if( selection.stream == imStream )
//        ;
//    else
        return niW->getGraphColor( selection.ic );
}


