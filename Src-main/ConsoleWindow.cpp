
#include "ConsoleWindow.h"
#include "Util.h"
#include "MainApp.h"

#include <QMenuBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QCloseEvent>
#include <QSettings>
#include <QTextEdit>


/* ---------------------------------------------------------------- */
/* Public --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

ConsoleWindow::ConsoleWindow( QWidget *p, Qt::WindowFlags f )
    :   QMainWindow( p, f ), nLines(0), maxLines(2000)
{
    MainApp *app = mainApp();

// Central text

    QTextEdit *te = new QTextEdit( this );
    te->installEventFilter( this );
    te->setUndoRedoEnabled( false );
    te->setReadOnly( !app->isLogEditable() );
    setCentralWidget( te );

    defTextColor = te->textColor();

// Menus

    app->act.initMenus( this );

// Statusbar

    statusBar()->showMessage( QString::null );

// Screen state

    restoreScreenState();
}


ConsoleWindow::~ConsoleWindow()
{
    saveScreenState();
}


void ConsoleWindow::setReadOnly( bool readOnly )
{
    textEdit()->setReadOnly( readOnly );
}

/* ---------------------------------------------------------------- */
/* Slots ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Note that append automatically adds trailing '\n'.
//
void ConsoleWindow::logAppendText( const QString &txt, const QColor &clr )
{
    QTextEdit   *te = textEdit();

// ------------
// Add new text
// ------------

    te->setTextColor( clr.isValid() ? clr : defTextColor );
    te->append( txt );

// ----------------------------------
// Limit growth - remove oldest lines
// ----------------------------------

    nLines += 1 + txt.count( '\n' );

    if( nLines >= maxLines ) {

        int         n2del   = MAX( maxLines / 10, nLines - maxLines );
        QTextCursor cursor  = te->textCursor();

        cursor.movePosition( QTextCursor::Start );
        cursor.movePosition(
            QTextCursor::Down,
            QTextCursor::KeepAnchor,
            n2del );

        cursor.removeSelectedText();
        nLines -= n2del;
    }

// --------------
// Normalize view
// --------------

    te->setTextColor( defTextColor );
    te->moveCursor( QTextCursor::End );
    te->ensureCursorVisible();
}


void ConsoleWindow::saveToFile()
{
    QString fn = QFileDialog::getSaveFileName(
                    this,
                    "Save log as text file",
                    mainApp()->dataDir(),
                    "Text files (*.txt)" );

    if( fn.length() ) {

        QFile   f( fn );

        if( f.open( QIODevice::WriteOnly | QIODevice::Text ) ) {

            QTextStream ts( &f );

            ts << textEdit()->toPlainText();
        }
    }
}

/* ---------------------------------------------------------------- */
/* Protected ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

// Force Ctrl+A events to be treated as 'show audio dialog',
// instead of 'text-field select-all'.
//
bool ConsoleWindow::eventFilter( QObject *watched, QEvent *event )
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


// Console close box clicked.
//
void ConsoleWindow::closeEvent( QCloseEvent *e )
{
    e->ignore();
    mainApp()->act.quitAct->trigger();
}

/* ---------------------------------------------------------------- */
/* Private -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

QTextEdit* ConsoleWindow::textEdit() const
{
    return dynamic_cast<QTextEdit*>(centralWidget());
}


void ConsoleWindow::saveScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    settings.setValue( "WinLayout_Console/geometry", saveGeometry() );
    settings.setValue( "WinLayout_Console/windowState", saveState() );
}


void ConsoleWindow::restoreScreenState()
{
    STDSETTINGS( settings, "windowlayout" );

    if( !restoreGeometry(
        settings.value( "WinLayout_Console/geometry" ).toByteArray() )
        ||
        !restoreState(
        settings.value( "WinLayout_Console/windowState" ).toByteArray() ) ) {

        resize( 800, 300 );
    }
}


