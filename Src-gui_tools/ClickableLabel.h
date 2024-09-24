#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>
#include <QMouseEvent>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    ClickableLabel(
        QWidget         *parent = 0,
        Qt::WindowFlags f = Qt::WindowFlags() )
    :   QLabel(parent, f)           {}

    ClickableLabel(
        const QString   &txt,
        QWidget         *parent = 0,
        Qt::WindowFlags f = Qt::WindowFlags() )
    :   QLabel(txt, parent, f)      {}

    virtual ~ClickableLabel()       {}

signals:
    void clicked();

protected:
    void mouseReleaseEvent( QMouseEvent *e );
};

#endif  // CLICKABLELABEL_H


