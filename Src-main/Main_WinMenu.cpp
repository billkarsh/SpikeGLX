
#include "Util.h"
#include "Main_WinMenu.h"

#include <QApplication>
#include <QMenu>




void Main_WinMenu::addToMenu( QWidget *w )
{
    int iw = wList.indexOf( w );

    if( iw >= 0 )
        return;

    QAction *a = new QAction( w->windowTitle(), this );
    a->setData( QVariant(reinterpret_cast<quint64>(w)) );
    ConnectUI( a, SIGNAL(triggered()), this, SLOT(activateWindow()) );

    act->windowMenu->addAction( a );
    win2Act[w] = a;

    wList.push_back( w );
}


void Main_WinMenu::removeFromMenu( QWidget *w )
{
    int iw = wList.indexOf( w );

    if( iw >= 0 ) {

        QAction *a = win2Act[w];

        act->windowMenu->removeAction( a );
        win2Act.remove( w );
        delete a;

        wList.removeAt( iw );
    }
}


void Main_WinMenu::closeAll()
{
    QList<QWidget*> wWasOpen = wList;

    foreach( QWidget *w, wWasOpen )
        w->close();
}


void Main_WinMenu::copyWindowTitleToAction( QWidget *w )
{
    QMap<QWidget*,QAction*>::const_iterator it;

    if( (it = win2Act.find( w )) != win2Act.end() )
        it.value()->setText( w->windowTitle() );
}


QWidget *Main_WinMenu::find_if( const WindowTester &test ) const
{
    foreach( QWidget* w, wList ) {

        if( test( w ) )
            return w;
    }

    return 0;
}


void Main_WinMenu::activateWindow( QWidget *w )
{
    if( !w ) {

        QAction *a = dynamic_cast<QAction*>(sender());

        if( !a )
            return;

        w = reinterpret_cast<QWidget*>(a->data().toULongLong());
    }

    if( w ) {
        QMetaObject::invokeMethod( w, "raise", Qt::AutoConnection );
        QMetaObject::invokeMethod( w, "showNormal", Qt::AutoConnection );
        w->activateWindow();
    }
}


void Main_WinMenu::checkMarkActiveWindow()
{
    QWidget                             *wTop = QApplication::activeWindow();
    QMap<QWidget*,QAction*>::iterator   it;

    for( it = win2Act.begin(); it != win2Act.end(); ++it )
        it.value()->setChecked( it.key() == wTop );
}


void Main_WinMenu::bringAllToFront()
{
    QWidget     *prevTop    = QApplication::activeWindow();
    QWidgetList all         = QApplication::topLevelWidgets();

    foreach( QWidget *w, all ) {

        if( w != prevTop && !w->isHidden() )
            QMetaObject::invokeMethod( w, "raise", Qt::AutoConnection );
    }

    if( prevTop )
        QMetaObject::invokeMethod( prevTop, "raise", Qt::AutoConnection );
}


