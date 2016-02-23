
#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "Version.h"

#include <QMenu>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTimer>




/* ---------------------------------------------------------------- */
/* class Main_Msg ------------------------------------------------- */
/* ---------------------------------------------------------------- */

Main_Msg::Main_Msg() : QObject(0), sysTray(0), sb_timeout(0)
{
}


Main_Msg::~Main_Msg()
{
    if( sysTray ) {
        delete sysTray;
        sysTray = 0;
    }
}


void Main_Msg::initMessenger( ConsoleWindow *cw )
{
    MainApp *app = mainApp();

// -----------
// Console log
// -----------

    this->cw    = cw;
    defLogColor = cw->textEdit()->textColor();

// -----------
// System tray
// -----------

    QMenu   *m = new QMenu( cw );
    m->addAction( app->act.shwHidConsAct );
    m->addAction( app->act.shwHidGrfsAct );
    m->addSeparator();
    m->addAction( app->act.aboutAct );
    m->addSeparator();
    m->addAction( app->act.quitAct );

    sysTray = new QSystemTrayIcon( this );
    sysTray->setContextMenu( m );
    sysTray->setIcon( app->windowIcon() );
    sysTray->show();

// ----------
// Status bar
// ----------

    QTimer *timer = new QTimer( this );
    ConnectUI( timer, SIGNAL(timeout()), this, SLOT(timedStatusBarUpdate()) );
    timer->start( 500 );
}


void Main_Msg::appQuiting()
{
    cw = 0;
}


void Main_Msg::logMsg( const QString &msg, const QColor &c )
{
    if( cw ) {

        QMetaObject::invokeMethod(
            cw, "logAppendText",
            Qt::QueuedConnection,
            Q_ARG(QString, msg),
            Q_ARG(QColor, (c.isValid() ? c : defLogColor)) );

        // PROGRAMMER:
        // Enable following to get serialized posting for
        // debug purposes...DO NOT use in release builds.

//        guiBreathe();
    }
}


void Main_Msg::sysTrayMsg(
    const QString   &msg,
    int             timeout_msec,
    bool            isError )
{
    if( sysTray ) {

        sysTray->showMessage(
            APPNAME,
            msg,
            (isError ?
                QSystemTrayIcon::Critical :
                QSystemTrayIcon::Information),
            timeout_msec );
    }
}


void Main_Msg::statusBarMsg(
    const QString   &msg,
    int             timeout_msec )
{
    QMutexLocker    ml( &sb_Mtx );

    sb_str      = msg;
    sb_timeout  = timeout_msec;
}


// Note on status message handling:
// In the manner of log-lines, we might push each message directly
// to the status bar as it arrives. However, this places no bounds
// on the rate of message updates, making them unreadable. Instead,
// we buffer messages and update the status bar on a timer.
//
void Main_Msg::timedStatusBarUpdate()
{
    QMutexLocker    ml( &sb_Mtx );

    if( cw && sb_str != sb_prev ) {

        cw->statusBar()->showMessage( sb_str, sb_timeout );
        sysTray->setToolTip( sb_str );
        sb_prev = sb_str;
    }
}


void Main_Msg::updateSysTrayTitle( const QString &title )
{
    sysTray->contextMenu()->setTitle( title );
}


