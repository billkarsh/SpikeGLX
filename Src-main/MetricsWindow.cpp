
#include "ui_MetricsWindow.h"

#include "MetricsWindow.h"
#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"

#include <QFileDialog>
#include <QKeyEvent>
#include <QScrollBar>
#include <QSettings>


/* ---------------------------------------------------------------- */
/* MXDiskRec ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void MetricsWindow::MXDiskRec::init()
{
    lags.clear();

    niFull  = 0;
    obFull  = 0;
    imFull  = 0;
    wbps    = 0;
    rbps    = 0;
    g       = -1;
    t       = -1;
}

/* ---------------------------------------------------------------- */
/* MetricsWindow -------------------------------------------------- */
/* ---------------------------------------------------------------- */

MetricsWindow::MetricsWindow( QWidget *parent )
    :   QWidget(parent), mxTimer(this),
        erLines(0), erMaxLines(2000), isRun(false)
{
    mxUI = new Ui::MetricsWindow;
    mxUI->setupUi( this );
    ConnectUI( mxUI->helpBut, SIGNAL(clicked()), this, SLOT(help()) );
    ConnectUI( mxUI->saveBut, SIGNAL(clicked()), this, SLOT(save()) );

    mxTimer.setTimerType( Qt::CoarseTimer );
    mxTimer.setInterval( 2000 );
    ConnectUI( &mxTimer, SIGNAL(timeout()), this, SLOT(updateMx()) );

// Choices of monospaced fonts widely available:
// Consolas
// Lucida Console
// Lucida Sans Typewriter

    mxUI->mxTE->setFontFamily( "Consolas" );
    defSize     = 8;    // fontPointSize() returns zero
    defColor    = mxUI->erTE->textColor();
    defWeight   = mxUI->erTE->fontWeight();

    restoreScreenState();
}


MetricsWindow::~MetricsWindow()
{
    saveScreenState();

    if( mxUI ) {
        delete mxUI;
        mxUI = 0;
    }
}

/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void MetricsWindow::runInit()
{
    err.init();
    prf.init();
    dsk.init();

    setWindowTitle(
        QString("Metrics: %1")
        .arg( mainApp()->cfgCtl()->acceptedParams.sns.runName ) );

    mxUI->mxTE->clear();
    mxUI->erTE->clear();

    isRun = true;

    if( isVisible() )
        mxTimer.start();
}


void MetricsWindow::runStart()
{
// BK: Possibly reclear errors here
}


void MetricsWindow::runEnd()
{
    isRun = false;
    mxTimer.stop();
    updateMx();
}


void MetricsWindow::showDialog()
{
    showNormal();
}


QString MetricsWindow::getErrFlags( int js, int ip, int shank )
{
    QString stream;

    if( js == jsOB )
        stream = QString(" obx%1").arg( ip, 2, 10, QChar('0') );
    else if( shank < 0 )
        stream = QString("imec%1").arg( ip, 2, 10, QChar('0') );
    else
        stream = QString("quad%1-%2").arg( ip, 2, 10, QChar('0') ).arg( shank );

    QMutexLocker    ml( &err.erfMtx );

    if( err.flags.contains( stream ) ) {

        const MXErrFlags    &F = err.flags[stream];
        return QString("%1 %2 %3 %4 %5 %6 %7")
                .arg( F.errCOUNT || F.errSERDES ||
                        F.errLOCK || F.errPOP ||
                        F.errSYNC || F.errMISS )
                .arg( F.errCOUNT ).arg( F.errSERDES )
                .arg( F.errLOCK ).arg( F.errPOP )
                .arg( F.errSYNC ).arg( F.errMISS );
    }
    else
        return "0 0 0 0 0 0 0";
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Note that append automatically adds trailing '\n'.
//
void MetricsWindow::logAppendText( const QString &txt, const QColor &clr )
{
    QTextEdit   *te = mxUI->erTE;

// ------------
// Add new text
// ------------

    te->setTextColor( clr.isValid() ? clr : defColor );
    te->append( txt );

// ----------------------------------
// Limit growth - remove oldest lines
// ----------------------------------

    erLines += 1 + txt.count( '\n' );

    if( erLines >= erMaxLines ) {

        int         n2del   = MAX( erMaxLines / 10, erLines - erMaxLines );
        QTextCursor cursor  = te->textCursor();

        cursor.movePosition( QTextCursor::Start );
        cursor.movePosition(
            QTextCursor::Down,
            QTextCursor::KeepAnchor,
            n2del );

        cursor.removeSelectedText();
        erLines -= n2del;
    }

// --------------
// Normalize view
// --------------

    te->setTextColor( defColor );
    te->moveCursor( QTextCursor::End );
    te->ensureCursorVisible();
}


void MetricsWindow::updateMx()
{
    QTextEdit   *te = mxUI->mxTE;
    QScrollBar  *S;
    int         dx, dy, ledstate = 0;

// Remember user cursor position

    S = te->horizontalScrollBar();
    if( S )
        dx = S->value();
    else
        dx = 0;

    S = te->verticalScrollBar();
    if( S )
        dy = S->value();
    else
        dy = 0;

    te->clear();

// ------
// Errors
// ------

    te->setFontPointSize( 12 );
    te->setFontWeight( QFont::Bold );
    te->append( "Errors" );
    te->setFontPointSize( defSize );
    te->setFontWeight( defWeight );

// Flags

    err.erfMtx.lock();

    if( err.flags.size() ) {

        te->setTextColor( Qt::darkRed );
        te->append( "Stream-i {imec error flags}:" );
        ledstate = qMax( ledstate, 2 );

        QMap<QString,MXErrFlags>::iterator  it, end = err.flags.end();

        for( it = err.flags.begin(); it != end; ++it ) {

            MXErrFlags  F   = it.value();
            quint32     sum = F.errCOUNT + F.errSERDES
                                + F.errLOCK + F.errPOP
                                + F.errSYNC + F.errMISS;

            if( sum )
                te->setTextColor( Qt::darkRed );
            else
                te->setTextColor( Qt::darkGreen );

            te->append(
                QString("  %1 COUNT %2 SERDES %3 LOCK %4 POP %5 SYNC %6 MISS %7")
                .arg( it.key() )
                .arg( F.errCOUNT ).arg( F.errSERDES )
                .arg( F.errLOCK ).arg( F.errPOP )
                .arg( F.errSYNC ).arg( F.errMISS ) );
        }

        te->setTextColor( defColor );
    }

    err.erfMtx.unlock();

// -----------
// Performance
// -----------

    te->setFontPointSize( 12 );
    te->setFontWeight( QFont::Bold );
    te->append( "Acquisition" );
    te->setFontPointSize( defSize );
    te->setFontWeight( defWeight );

// Fifo

    if( prf.fifoPct.size() ) {

        // --------------------
        // Color title by worst
        // --------------------

        int maxPct = 0;

        QMap<QString,int>::iterator it, end = prf.fifoPct.end();

        for( it = prf.fifoPct.begin(); it != end; ++it ) {

            if( it.value() > maxPct )
                maxPct = it.value();
        }

        if( maxPct >= 20 ) {
            te->setTextColor( Qt::darkRed );
            ledstate = qMax( ledstate, 2 );
        }
        else if( maxPct >= 5 ) {
            te->setTextColor( Qt::darkMagenta );
            ledstate = qMax( ledstate, 1 );
        }
        else
            te->setTextColor( Qt::darkGreen );

        te->append( "Stream-i (fifo filling %):" );

        // --------------------
        // Color and write each
        // --------------------

        int nOnLine = 0;

        for( it = prf.fifoPct.begin(); it != end; ++it ) {

            if( !(nOnLine++ % 8) )
                te->insertPlainText( "\n" );

            te->moveCursor( QTextCursor::End );

            int pct = it.value();

            if( pct >= 20 )
                te->setTextColor( Qt::darkRed );
            else if( pct >= 5 )
                te->setTextColor( Qt::darkMagenta );
            else
                te->setTextColor( Qt::darkGreen );

            te->insertPlainText(
                QString("  %1(%2)")
                .arg( it.key() )
                .arg( pct, 2, 10, QChar(' ') ) );
        }

        te->setTextColor( defColor );
    }

// Awake

    if( prf.awakePct.size() ) {

        // --------------------
        // Color title by worst
        // --------------------

        int maxPct = 0;

        QMap<QString,int>::iterator it, end = prf.awakePct.end();

        for( it = prf.awakePct.begin(); it != end; ++it ) {

            if( it.value() > maxPct && !isActiveOK( it.key() ) )
                maxPct = it.value();
        }

        if( maxPct >= 90 ) {
            te->setTextColor( Qt::darkRed );
            ledstate = qMax( ledstate, 2 );
        }
        else if( maxPct >= 75 ) {
            te->setTextColor( Qt::darkMagenta );
            ledstate = qMax( ledstate, 1 );
        }
        else
            te->setTextColor( Qt::darkGreen );

        te->append( "Stream-i (worker-thread active %):" );

        // --------------------
        // Color and write each
        // --------------------

        int nOnLine = 0;

        for( it = prf.awakePct.begin(); it != end; ++it ) {

            if( !(nOnLine++ % 8) )
                te->insertPlainText( "\n" );

            te->moveCursor( QTextCursor::End );

            int pct = it.value();

            if( isActiveOK( it.key() ) )
                te->setTextColor( Qt::darkGreen );
            else if( pct >= 90 )
                te->setTextColor( Qt::darkRed );
            else if( pct >= 75 )
                te->setTextColor( Qt::darkMagenta );
            else
                te->setTextColor( Qt::darkGreen );

            te->insertPlainText(
                QString("  %1(%2)")
                .arg( it.key() )
                .arg( pct, 2, 10, QChar(' ') ) );
        }

        te->setTextColor( defColor );
    }

// ----
// Disk
// ----

    te->setFontPointSize( 12 );
    te->setFontWeight( QFont::Bold );
    te->append( QString("Disk <G%1 T%2>").arg( dsk.g ).arg( dsk.t ) );
    te->setFontPointSize( defSize );
    te->setFontWeight( defWeight );

// Buffer filling

    te->append(
        QString("Write buffer full (%); ni ob im:      %1   %2   %3")
        .arg( dsk.niFull, 0, 'f', 1 )
        .arg( dsk.obFull, 0, 'f', 1 )
        .arg( dsk.imFull, 0, 'f', 1 ) );

// Write rate

    te->append(
        QString("Actual (required) write rate (MB/s):  %1  (%2)")
        .arg( dsk.wbps, 0, 'f', 1 )
        .arg( dsk.rbps, 0, 'f', 1 ) );

// Lags

    if( dsk.lags.size() ) {

        // --------------------
        // Color title by worst
        // --------------------

        double  minPct = 100.0;

        QMap<QString,double>::iterator  it, end = dsk.lags.end();

        for( it = dsk.lags.begin(); it != end; ++it ) {

            if( it.value() < minPct )
                minPct = it.value();
        }

        if( minPct <= 10 ) {
            te->setTextColor( Qt::darkRed );
            ledstate = qMax( ledstate, 2 );
        }
        else if( minPct <= 50 ) {
            te->setTextColor( Qt::darkMagenta );
            ledstate = qMax( ledstate, 1 );
        }
        else
            te->setTextColor( Qt::darkGreen );

        te->append( "Stream-i fetch depth (old|--^-|new, as %):" );

        // --------------------
        // Color and write each
        // --------------------

        int nOnLine = 0;

        for( it = dsk.lags.begin(); it != end; ++it ) {

            if( !(nOnLine++ % 8) )
                te->insertPlainText( "\n" );

            te->moveCursor( QTextCursor::End );

            double  pct = it.value();

            if( pct <= 10 )
                te->setTextColor( Qt::darkRed );
            else if( pct <= 50 )
                te->setTextColor( Qt::darkMagenta );
            else
                te->setTextColor( Qt::darkGreen );

            te->insertPlainText(
                QString("  %1(%2)")
                .arg( it.key() )
                .arg( pct, 4, 'f', 1 ) );
        }

        te->setTextColor( defColor );
    }

// Restore user cursor

    S = te->horizontalScrollBar();
    if( S )
        S->setValue( dx );

    S = te->verticalScrollBar();
    if( S )
        S->setValue( dy );

// LED

    mxUI->LEDs->setState( ledstate );
}


void MetricsWindow::help()
{
    showHelp( "Metrics_Help" );
}


void MetricsWindow::save()
{
    QString fn = QFileDialog::getSaveFileName(
                    this,
                    "Save metrics as text file",
                    mainApp()->dataDir(),
                    "Text files (*.txt)" );

    if( fn.length() ) {

        QFile   f( fn );

        if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

            QTextStream ts( &f );

            ts << "Run: ";
            ts << mainApp()->cfgCtl()->acceptedParams.sns.runName;
            ts << "\n";
            ts << "Performance metrics --------\n";
            ts << mxUI->mxTE->toPlainText();
            ts << "\nErrors and warnings --------\n";
            ts << mxUI->erTE->toPlainText();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

void MetricsWindow::keyPressEvent( QKeyEvent *e )
{
    if( e->key() == Qt::Key_Escape ) {

        close();
        e->accept();
    }
    else
        QWidget::keyPressEvent( e );
}


// Detect show: update/start timer.
//
void MetricsWindow::showEvent( QShowEvent *e )
{
    QWidget::showEvent( e );

    updateMx();

    if( isRun )
        mxTimer.start();
}


// Detect hide: stop timer.
//
void MetricsWindow::hideEvent( QHideEvent *e )
{
    QWidget::hideEvent( e );

    mxTimer.stop();
}


// Detect window minimized: adjust update/timer.
//
void MetricsWindow::changeEvent( QEvent *e )
{
    if( e->type() == QEvent::WindowStateChange ) {

        QWindowStateChangeEvent *wsce =
            static_cast<QWindowStateChangeEvent*>( e );

        if( wsce->oldState() & Qt::WindowMinimized ) {

            if( !(windowState() & Qt::WindowMinimized) ) {

                updateMx();

                if( isRun )
                    mxTimer.start();
            }
        }
        else {
            if( windowState() & Qt::WindowMinimized )
                mxTimer.stop();
        }
    }

    QWidget::changeEvent( e );
}


void MetricsWindow::closeEvent( QCloseEvent *e )
{
    QWidget::closeEvent( e );

    if( e->isAccepted() )
        emit closed( this );
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

bool MetricsWindow::isActiveOK( const QString &stream ) const
{
    const DAQ::Params   &P = mainApp()->cfgCtl()->acceptedParams;

    if( P.im.prbAll.lowLatency )
        return true;

    int ip, js = P.stream2jsip( ip, stream );

    return js == jsIM && P.stream_nChans( js, ip ) > 384;
}


// Note:
// restoreScreenState() must be called after initializing
// a window's controls with setupUI().
//
void MetricsWindow::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Metrics/geometry" ).toByteArray() ) ) {

        // Get size from form, or do nothing.
    }
}


void MetricsWindow::saveScreenState() const
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Metrics/geometry", saveGeometry() );
}


