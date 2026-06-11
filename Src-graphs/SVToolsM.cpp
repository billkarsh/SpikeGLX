
#include "Util.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "SVToolsM.h"

#include <QColorDialog>
#include <QTextBrowser>




QColor SVToolsM::selectColor( QColor inColor )
{
    QColorDialog::setCustomColor( 0, NeuGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 1, LfpGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 2, AuxGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 3, DigGraphBGColor.rgb() );

    return QColorDialog::getColor(
        inColor, gr->getGWWidget(), "Color This Trace" );
}


// Demonstrate online quick tips help.
//
void SVToolsM::qtips( const QString &fHelp )
{
    QTextBrowser    *B = new QTextBrowser( gr );

    B->setAttribute( Qt::WA_DeleteOnClose, true );
    B->setWindowFlag( Qt::WindowStaysOnTopHint, true );
    B->setWindowFlags( Qt::Tool );
    B->setSource(
        QUrl::fromUserInput(
            QString("%1/_Help/%2.html")
            .arg( appPath() )
            .arg( fHelp ) ) );
    B->resize(400, 300);
    B->show();
}


