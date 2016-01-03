
#include "ui_CmdSrvDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "ConsoleWindow.h"
#include "CmdSrvDlg.h"
#include "CmdServer.h"

#include <QMessageBox>
#include <QSettings>




CmdSrvDlg::CmdSrvDlg() : QObject(0), cmdServer(0)
{
}


CmdSrvDlg::~CmdSrvDlg()
{
    CmdServer::deleteAllActiveConnections();

    if( cmdServer ) {
        delete cmdServer;
        cmdServer = 0;
    }
}


void CmdSrvDlg::loadSettings( QSettings &S )
{
    S.beginGroup( "CmdSrv" );

    p.iface         = S.value( "iface", getMyIPAddress() ).toString();
    p.port          = S.value( "port", CMD_DEF_PORT ).toUInt();
    p.timeout_ms    = S.value( "timeoutMS", CMD_TOUT_MS ).toInt();
    p.enabled       = S.value( "enabled", false ).toBool();

    S.endGroup();
}


void CmdSrvDlg::saveSettings( QSettings &S ) const
{
    S.beginGroup( "CmdSrv" );

    S.setValue( "iface", p.iface );
    S.setValue( "port",  p.port );
    S.setValue( "timeoutMS", p.timeout_ms );
    S.setValue( "enabled", p.enabled );

    S.endGroup();
}


bool CmdSrvDlg::startServer( bool isAppStartup )
{
    if( cmdServer ) {
        delete cmdServer;
        cmdServer = 0;
    }

    if( p.enabled ) {

        MainApp *app = mainApp();

        cmdServer = new CmdServer( app );

        if( !cmdServer->beginListening( p.iface, p.port, p.timeout_ms ) ) {

            if( !isAppStartup ) {

                QMessageBox::critical(
                    0,
                    "CmdSrv Listen Error",
                    QString(
                    "Command server could not listen on (%1:%2).")
                    .arg( p.iface )
                    .arg( p.port ) );
            }

            Error() << "Failed starting command server!";
            delete cmdServer;
            cmdServer = 0;
            return false;
        }
    }
    else
        Log() << "CmdSrv currently disabled.";

    return true;
}


void CmdSrvDlg::showStartupMessage()
{
    if( cmdServer || !p.enabled )
        return;

    int but = QMessageBox::critical(
        0,
        "CmdSrv Listen Error",
        QString(
        "Command server could not listen on (%1:%2).\n\n"
        "You can ignore this for now and select different settings\n"
        "later using the Options menu.\n\n"
        "(If mouse not working, actuate these buttons with keyboard.)"
        "            ")
        .arg( p.iface )
        .arg( p.port ),
        QMessageBox::Abort,
        QMessageBox::Ignore );

    if( but == QMessageBox::Abort )
        QMetaObject::invokeMethod( mainApp(), "quit", Qt::QueuedConnection );
}


void CmdSrvDlg::showOptionsDlg()
{
    QDialog dlg;

    cmdUI = new Ui::CmdSrvDialog;
    cmdUI->setupUi( &dlg );
    cmdUI->ipLE->setText( p.iface );
    cmdUI->portSB->setValue( p.port );
    cmdUI->toSB->setValue( p.timeout_ms );
    cmdUI->enabledGB->setChecked( p.enabled );
    ConnectUI( cmdUI->ipBut, SIGNAL(clicked()), this, SLOT(ipBut()) );
    ConnectUI( cmdUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );

    if( QDialog::Accepted != dlg.exec() && !cmdServer ) {

        // If failed to set new listener...restore former.
        startServer();
    }

    cmdUI->buttonBox->disconnect();
    delete cmdUI;
}


void CmdSrvDlg::ipBut()
{
    cmdUI->ipLE->setText( getMyIPAddress() );
}


void CmdSrvDlg::okBut()
{
    CmdSrvParams    pSave = p;

    p.iface         = cmdUI->ipLE->text();
    p.port          = cmdUI->portSB->value();
    p.timeout_ms    = cmdUI->toSB->value();
    p.enabled       = cmdUI->enabledGB->isChecked();

    if( startServer() ) {
        mainApp()->saveSettings();
        dynamic_cast<QDialog*>(cmdUI->buttonBox->parent())->accept();
    }
    else
        p = pSave;
}


