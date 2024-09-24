
#include "SVToolsM.h"
#include "SVGrafsM_Im.h"
#include "SVGrafsM_Ob.h"
#include "SVGrafsM_Ni.h"
#include "SView.h"

#include <QVBoxLayout>




SViewM_Im::SViewM_Im(
    SVGrafsM*           &vw,
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel )
{
    vw = new SVGrafsM_Im( gw, p, ip, jpanel );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setContentsMargins( 0, 0, 0, 0 );
    L->addWidget( tb );
    L->addWidget( vw );
    adjustSize();
}


SViewM_Ob::SViewM_Ob(
    SVGrafsM*           &vw,
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 ip,
    int                 jpanel )
{
    vw = new SVGrafsM_Ob( gw, p, ip, jpanel );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setContentsMargins( 0, 0, 0, 0 );
    L->addWidget( tb );
    L->addWidget( vw );
    adjustSize();
}


SViewM_Ni::SViewM_Ni(
    SVGrafsM*           &vw,
    GraphsWindow        *gw,
    const DAQ::Params   &p,
    int                 jpanel )
{
    vw = new SVGrafsM_Ni( gw, p, jpanel );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setContentsMargins( 0, 0, 0, 0 );
    L->addWidget( tb );
    L->addWidget( vw );
    adjustSize();
}


