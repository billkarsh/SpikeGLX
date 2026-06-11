#ifndef SVTOOLSM0_H
#define SVTOOLSM0_H

#include "SVToolsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SViewM:
//
// Sel Max | [secs^] [scl^] Clr All | [band^] -<Tn> -<S> | -<Tx> | BinMax
//
class SVToolsM0 : public SVToolsM
{
    Q_OBJECT

public:
    SVToolsM0( SVGrafsM *gr ) : SVToolsM(gr)    {}
    virtual void init();

    virtual void setSelName( const QString &name );
    virtual void update();
};

#endif  // SVTOOLSM0_H


