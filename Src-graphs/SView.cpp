
#include "SVToolsM.h"
#include "SVGrafsM_Im.h"
#include "SVGrafsM_Ni.h"
#include "SView.h"

#include <QVBoxLayout>




SViewM_Im::SViewM_Im(
    SVGrafsM*           &vw,
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip )
{
    vw = new SVGrafsM_Im( gw, p, ip );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
    adjustSize();
}


SViewM_Ni::SViewM_Ni(
    SVGrafsM*           &vw,
    GraphsWindow        *gw,
    const DAQ::Params   &p )
{
    vw = new SVGrafsM_Ni( gw, p );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
    adjustSize();
}


