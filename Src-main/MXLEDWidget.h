#ifndef MXLEDWIDGET_H
#define MXLEDWIDGET_H

#include <QWidget>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class MXLEDWidget : public QWidget
{
    Q_OBJECT

public:
    MXLEDWidget( QWidget *parent = 0 );

    // -1=ini, 0=grn, 1=ylw, 2=red
    void setState( int state );
};

#endif  // MXLEDWIDGET_H


