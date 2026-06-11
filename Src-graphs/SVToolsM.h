#ifndef SVTOOLSM_H
#define SVTOOLSM_H

#include <QToolBar>

class SVGrafsM;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar base class for SViewM:
//
// Sel Max | [secs^] [scl^] Clr All | [band^] -<Tn> -<S> | -<Tx> | BinMax
//
class SVToolsM : public QToolBar
{
    Q_OBJECT

protected:
    SVGrafsM    *gr;

public:
    SVToolsM( SVGrafsM *gr ) : gr(gr)   {}
    virtual ~SVToolsM()                 {}
    virtual void init() = 0;

    virtual void setSelName( const QString &name ) = 0;
    virtual void update() = 0;

    QColor selectColor( QColor inColor );

protected:
    void qtips( const QString &fHelp );
};

#endif  // SVTOOLSM_H


