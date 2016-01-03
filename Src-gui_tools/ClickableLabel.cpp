
#include "ClickableLabel.h"




void ClickableLabel::mouseReleaseEvent( QMouseEvent *e )
{
    if( e->button() == Qt::LeftButton
        && rect().contains( e->pos() ) ) {

        emit clicked();
        e->accept();
    }
    else
        QLabel::mouseReleaseEvent( e );
}


