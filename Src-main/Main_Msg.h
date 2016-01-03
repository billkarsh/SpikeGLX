#ifndef MAIN_MSG_H
#define MAIN_MSG_H

#include <QObject>
#include <QString>
#include <QColor>
#include <QMutex>

class MainApp;
class ConsoleWindow;
class QSystemTrayIcon;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Send messages to:
// - console window text log
// - console window status bar
// - Windows system tray
//
class Main_Msg : public QObject
{
    Q_OBJECT

    friend class MainApp;

private:
    ConsoleWindow   *cw;
    QSystemTrayIcon *sysTray;
    QColor          defLogColor;
    mutable QMutex  sb_Mtx;
    QString         sb_str,
                    sb_prev;
    int             sb_timeout;

public:
    Main_Msg();
    virtual ~Main_Msg();

    void initMessenger( ConsoleWindow *cw );
    void appQuiting();

    void logMsg( const QString &msg, const QColor &c = QColor() );

    void sysTrayMsg(
        const QString   &msg,
        int             timeout_msec = 0,
        bool            isError = false );

    void statusBarMsg(
        const QString   &msg,
        int             timeout_msec = 0 );

private slots:
    void timedStatusBarUpdate();

private:
    void updateSysTrayTitle( const QString &title );
};

#endif  // MAIN_MSG_H


