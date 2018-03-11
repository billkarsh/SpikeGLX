#ifndef MAIN_WINMENU_H
#define MAIN_WINMENU_H

#include "Main_Actions.h"

#include <QList>
#include <QMap>

class QWidget;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// WindowTester base class for Main_WinMenu::find_if.
// Subclass this and implement your own test.
//
struct WindowTester {
    virtual bool operator()( QWidget* ) const = 0;
};


class Main_WinMenu : public QObject
{
    Q_OBJECT

private:
    Main_Actions            *act;
    QList<QWidget*>         wList;
    QMap<QWidget*,QAction*> win2Act;

public:
    Main_WinMenu() : QObject(0) {}

    void setActionPtr( Main_Actions *act ) {this->act = act;}

    void addToMenu( QWidget *w );
    void removeFromMenu( QWidget *w );

    void closeAll();

    void copyWindowTitleToAction( QWidget *w );

    QWidget *find_if( const WindowTester &test ) const;

public slots:
    void activateWindow( QWidget *w = 0 );
    void checkMarkActiveWindow();
    void bringAllToFront();
};

#endif  // MAIN_WINMENU_H


