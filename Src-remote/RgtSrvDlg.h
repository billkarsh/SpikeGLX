#ifndef RGTSRVDLG_H
#define RGTSRVDLG_H

#include <QObject>
#include <QString>

namespace Ui {
class RgtSrvDialog;
}

namespace ns_RgtServer {
class RgtServer;
}

class QSettings;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class RgtSrvDlg : public QObject
{
    Q_OBJECT

private:
// Types
    struct RgtSrvParams {
        QString iface;
        int     timeout_ms;
        quint16 port;
    };
// Data
    RgtSrvParams            p;
    ns_RgtServer::RgtServer *rgtServer;
    Ui::RgtSrvDialog        *rgtUI;
public:
    RgtSrvDlg();
    virtual ~RgtSrvDlg();

    void loadSettings( QSettings &S );
    void saveSettings( QSettings &S ) const;

    bool startServer( bool isAppStrtup = false );
    void showStartupMessage();

public slots:
    void showOptionsDlg();

private slots:
    void ipBut();
    void okBut();
};

#endif  // RGTSRVDLG_H


