#ifndef SVTOOLSM_H
#define SVTOOLSM_H

#include <QToolBar>

class SVGrafsM;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SViewM:
//
// Sel Max | [secs^] [scl^] Clr All | Flt
//
class SVToolsM : public QToolBar
{
    Q_OBJECT

private:
    SVGrafsM    *gr;

public:
    SVToolsM( SVGrafsM *gr ) : gr(gr) {}
    void init();

    void setSelName( const QString &name );
    QColor selectColor( QColor inColor );
    void update();
};

#endif  // SVTOOLSM_H


