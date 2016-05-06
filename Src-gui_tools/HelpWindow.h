#ifndef HELPWINDOW_H
#define HELPWINDOW_H

#include <QDialog>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class HelpWindow : public QDialog
{
    Q_OBJECT

public:
    HelpWindow(
        const QString   &title,
        const QString   &filename,
        QWidget         *parent = 0 );

signals:
    void closed();

protected:
    virtual void closeEvent( QCloseEvent *e );
};

#endif  // HELPWINDOW_H


