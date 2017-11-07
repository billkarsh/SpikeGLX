
#include "ui_TextBrowser.h"

#include "HelpWindow.h"

#include <QCloseEvent>




HelpWindow::HelpWindow(
    const QString   &title,
    const QString   &filename,
    QWidget         *parent )
    :   QDialog(parent)
{
    Ui::TextBrowser uiTB;
    uiTB.setupUi( this );
    uiTB.textBrowser->setSearchPaths( QStringList( "qrc:/" ) );
    uiTB.textBrowser->setSource( QUrl(QString("qrc:/%1").arg(filename)) );

    setWindowFlags( windowFlags()
        & (~Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    setWindowTitle( title );
}


void HelpWindow::closeEvent( QCloseEvent *e )
{
    QDialog::closeEvent( e );

    if( e->isAccepted() )
        emit closed( this );
}


