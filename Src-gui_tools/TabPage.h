#ifndef TABPAGE_H
#define TABPAGE_H

#include <QWidget>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// TabPage solves the problem wherein Qt does not signal that
// a tab is about to change. Rather, QTabWidget only signals
// that a tab HAS ALREADY changed and the corresponding tab
// page is already showing. This doesn't allow us to configure
// it first, and that may look ugly.
//
// Instead for each QTabWidget, store a single persistent (void*),
// e.g. "curTab," which TabPage can access (by address). When you
// change tabs, configure the page first, and then set curTab =
// the new tab.
//
class TabPage : public QWidget
{
private:
    void **pCurTab;

public:
    TabPage( void **pCurTab ) : pCurTab(pCurTab) {*pCurTab=0;}

    void setVisible( bool visible )
    {
        if( (void*)this != *pCurTab )
            visible = false;

        QWidget::setVisible( visible );
    }
};

#endif  // TABPAGE_H


