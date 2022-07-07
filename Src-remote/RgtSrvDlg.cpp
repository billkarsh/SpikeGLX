
#include "ui_RgtSrvDialog.h"

#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "ConsoleWindow.h"
#include "RgtServer.h"
#include "RgtSrvDlg.h"

#include <QMessageBox>
#include <QSettings>

/* ---------------------------------------------------------------- */
/* RgtSrvParams --------------------------------------------------- */
/* ---------------------------------------------------------------- */

void RgtSrvDlg::RgtSrvParams::loadSettings( QSettings &S )
{
    S.beginGroup( "RgtSrv" );

    iface       = S.value( "iface",
                    QHostAddress( QHostAddress::LocalHost ).toString() )
                    .toString();
    port        = S.value( "port", RGT_DEF_PORT ).toUInt();
    timeout_ms  = S.value( "timeoutMS", RGT_TOUT_MS ).toInt();
    enabled     = S.value( "enabled", false ).toBool();

    S.endGroup();
}


void RgtSrvDlg::RgtSrvParams::saveSettings( QSettings &S ) const
{
    S.beginGroup( "RgtSrv" );

    S.setValue( "iface", iface );
    S.setValue( "port",  port );
    S.setValue( "timeoutMS", timeout_ms );
    S.setValue( "enabled", enabled );

    S.endGroup();
}

/* ---------------------------------------------------------------- */
/* RgtSrvDlg ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

RgtSrvDlg::RgtSrvDlg() : QObject(0), rgtServer(0)
{
}


RgtSrvDlg::~RgtSrvDlg()
{
    if( rgtServer ) {
        delete rgtServer;
        rgtServer = 0;
    }
}


bool RgtSrvDlg::startServer( bool isAppStrtup )
{
    if( rgtServer ) {
        delete rgtServer;
        rgtServer = 0;
    }

    if( p.enabled ) {

        MainApp *app = mainApp();
        Run     *run = app->getRun();

        rgtServer = new ns_RgtServer::RgtServer( app );

        if( !rgtServer->beginListening( p.iface, p.port, p.timeout_ms ) ) {

            if( !isAppStrtup ) {

                QMessageBox::critical(
                    0,
                    "Gate/Trigger Listen Error",
                    QString(
                    "Gate/Trigger server could not listen on (%1:%2).")
                    .arg( p.iface )
                    .arg( p.port ) );
            }

            Error() << "Failed starting gate/trigger server.";
            delete rgtServer;
            rgtServer = 0;
            return false;
        }

        Connect( rgtServer, SIGNAL(rgtSetGate(bool)), run, SLOT(rgtSetGate(bool)) );
        Connect( rgtServer, SIGNAL(rgtSetTrig(bool)), run, SLOT(rgtSetTrig(bool)) );
        Connect( rgtServer, SIGNAL(rgtSetMetaData(KeyValMap)), run, SLOT(rgtSetMetaData(KeyValMap)) );
    }
    else
        Log() << "Gate/Trigger server currently disabled.";

    return true;
}


void RgtSrvDlg::showStartupMessage()
{
    if( rgtServer || !p.enabled )
        return;

    int but = QMessageBox::critical(
        0,
        "Gate/Trigger Listen Error",
        QString(
        "Gate/Trigger server could not listen on (%1:%2).\n\n"
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


void RgtSrvDlg::showOptionsDlg()
{
    QDialog dlg;

    dlg.setWindowFlags( dlg.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    rgtUI = new Ui::RgtSrvDialog;
    rgtUI->setupUi( &dlg );
    rgtUI->ipLE->setText( p.iface );
    rgtUI->portSB->setValue( p.port );
    rgtUI->toSB->setValue( p.timeout_ms );
    rgtUI->enabledGB->setChecked( p.enabled );
    ConnectUI( rgtUI->ipBut, SIGNAL(clicked()), this, SLOT(ipBut()) );
    ConnectUI( rgtUI->buttonBox, SIGNAL(accepted()), this, SLOT(okBut()) );

    if( QDialog::Accepted != dlg.exec() && !rgtServer ) {

        // If failed to set new listener...restore former.
        startServer();
    }

    rgtUI->buttonBox->disconnect();
    delete rgtUI;
}


void RgtSrvDlg::ipBut()
{
    rgtUI->ipLE->setText( getMyIPAddress() );
}


void RgtSrvDlg::okBut()
{
    RgtSrvParams    pSave = p;

    p.iface         = rgtUI->ipLE->text();
    p.port          = rgtUI->portSB->value();
    p.timeout_ms    = rgtUI->toSB->value();
    p.enabled       = rgtUI->enabledGB->isChecked();

    if( startServer() ) {
        mainApp()->saveSettings();
        dynamic_cast<QDialog*>(rgtUI->buttonBox->parent())->accept();
    }
    else
        p = pSave;
}


