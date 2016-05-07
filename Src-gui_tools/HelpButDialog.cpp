
#include "HelpButDialog.h"
#include "HelpWindow.h"

#include <QApplication>
#include <QEvent>
#include <QWhatsThis>




HelpButDialog::~HelpButDialog()
{
    if( helpDlg ) {
        delete helpDlg;
        helpDlg = 0;
    }
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


