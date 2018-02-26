
#include "HelpButDialog.h"
#include "Util.h"

#include <QApplication>
#include <QEvent>
#include <QWhatsThis>




bool HelpButDialog::event( QEvent *e )
{
    if( e->type() == QEvent::EnterWhatsThisMode
        && this == QApplication::activeWindow() ) {

        QWhatsThis::leaveWhatsThisMode();

        showHelp( filename );

        e->accept();
        return true;
    }
    else
        return QDialog::event( e );
}


