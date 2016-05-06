
#include "HelpButDialog.h"
#include "HelpWindow.h"

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
    if( e->type() == QEvent::EnterWhatsThisMode ) {

        QWhatsThis::leaveWhatsThisMode();

        if( !helpDlg )
            helpDlg = new HelpWindow( title, filename, this );

        helpDlg->show();
        helpDlg->activateWindow();
        return false;
    }
    else
        return QDialog::event( e );
}


