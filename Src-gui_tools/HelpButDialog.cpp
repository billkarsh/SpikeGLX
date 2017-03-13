
#include "HelpButDialog.h"
#include "HelpWindow.h"

#include <QApplication>
#include <QEvent>
#include <QWhatsThis>




HelpButDialog::~HelpButDialog()
{
    killHelp();
}


bool HelpButDialog::event( QEvent *e )
{
    if( e->type() == QEvent::EnterWhatsThisMode
        && this == QApplication::activeWindow() ) {

        QWhatsThis::leaveWhatsThisMode();

        if( !helpDlg )
            helpDlg = new HelpWindow( title, filename, this );

        helpDlg->show();
        helpDlg->activateWindow();
        e->accept();
        return true;
    }
    else
        return QDialog::event( e );
}


void HelpButDialog::closeEvent( QCloseEvent* )
{
    killHelp();
}


void HelpButDialog::killHelp()
{
    if( helpDlg ) {
        delete helpDlg;
        helpDlg = 0;
    }
}


