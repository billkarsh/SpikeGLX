#ifndef SVTOOLSMX_H
#define SVTOOLSMX_H

#include "SVToolsM.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Toolbar for SViewM:
//
// Sel Max | [secs^] [scl^] Clr | [band^] -<Tn> -<S> | -<Tx> | BinMax
//
class SVToolsMX : public SVToolsM
{
    Q_OBJECT

public:
    SVToolsMX( SVGrafsM *gr ) : SVToolsM(gr)    {}
    virtual void init();

    virtual void setSelName( const QString &name );
    virtual void update();
};

#endif  // SVTOOLSMX_H


