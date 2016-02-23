#ifndef SVTOOLSM_H
#define SVTOOLSM_H

#include <QToolBar>

class SVGrafsM;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SViewM:
//
// Sel Max | [secs^] [scl^] Clr All | [gpt^] [tab^] [---|-----]
//
class SVToolsM : public QToolBar
{
    Q_OBJECT

private:
    SVGrafsM    *gr;

public:
    SVToolsM( SVGrafsM *gr );

    void setSelName( const QString &name );
    QColor selectColor( QColor inColor );
    void update();
};

#endif  // SVTOOLSM_H


