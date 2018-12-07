
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

Main_Msg::Main_Msg() : QObject(0), sysTray(0), timStatBar(500, this)
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

    this->cw = cw;

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

    ConnectUI( &timStatBar, SIGNAL(draw(QString,int)), this, SLOT(statBarDraw(QString,int)) );
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
            Q_ARG(QColor, c) );

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
    timStatBar.latestString( msg, timeout_msec );
}


void Main_Msg::statBarDraw( QString s, int textPersistMS )
{
    cw->statusBar()->showMessage( s, textPersistMS );
    sysTray->setToolTip( s );
}


void Main_Msg::updateSysTrayTitle( const QString &title )
{
    sysTray->contextMenu()->setTitle( title );
}


