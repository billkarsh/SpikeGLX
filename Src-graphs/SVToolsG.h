#ifndef SVTOOLSG_H
#define SVTOOLSG_H

#include <QToolBar>

class SVGrafsG;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SViewG:
//
// Sort Sel Max | [secs^] [scl^] Clr All | Flt
//
class SVToolsG : public QToolBar
{
    Q_OBJECT

private:
    SVGrafsG    *gr;

public:
    SVToolsG( SVGrafsG *gr );

    void setSelName( const QString &name );
    QColor selectColor( QColor inColor );
    void update();
};

#endif  // SVTOOLSG_H


