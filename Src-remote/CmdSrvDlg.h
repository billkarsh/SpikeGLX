#ifndef CMDSRVDLG_H
#define CMDSRVDLG_H

#include <QObject>
#include <QString>

namespace Ui {
class CmdSrvDialog;
}

class CmdServer;

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class CmdSrvDlg : public QObject
{
    Q_OBJECT

private:
// Types
    struct CmdSrvParams {
        QString iface;
        int     timeout_ms;
        quint16 port;
        bool    enabled;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };
// Data
    CmdSrvParams        p;
    CmdServer           *cmdServer;
    Ui::CmdSrvDialog    *cmdUI;

public:
    CmdSrvDlg();
    virtual ~CmdSrvDlg();

    void loadSettings( QSettings &S )       {p.loadSettings( S );}
    void saveSettings( QSettings &S ) const {p.saveSettings( S );}

    bool startServer( bool isAppStartup = false );
    void showStartupMessage();

public slots:
    void showOptionsDlg();

private slots:
    void ipBut();
    void okBut();
};

#endif  // CMDSRVDLG_H


