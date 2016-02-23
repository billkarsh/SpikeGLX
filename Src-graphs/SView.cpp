
#include "SVToolsG.h"
#include "SVToolsM.h"
#include "SVGrafsG_Im.h"
#include "SVGrafsG_Ni.h"
#include "SVGrafsM_Im.h"
#include "SVGrafsM_Ni.h"
#include "SView.h"

#include <QVBoxLayout>




SViewG_Im::SViewG_Im( SVGrafsG_Im *&vw, GraphsWindow *gw, DAQ::Params &p )
{
    vw = new SVGrafsG_Im( gw, p );
    SVToolsG    *tb = new SVToolsG( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
}


SViewG_Ni::SViewG_Ni( SVGrafsG_Ni *&vw, GraphsWindow *gw, DAQ::Params &p )
{
    vw = new SVGrafsG_Ni( gw, p );
    SVToolsG    *tb = new SVToolsG( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
}


SViewM_Im::SViewM_Im( SVGrafsM_Im *&vw, GraphsWindow *gw, DAQ::Params &p )
{
    vw = new SVGrafsM_Im( gw, p );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
}


SViewM_Ni::SViewM_Ni( SVGrafsM_Ni* &vw, GraphsWindow *gw, DAQ::Params &p )
{
    vw = new SVGrafsM_Ni( gw, p );
    SVToolsM    *tb = new SVToolsM( vw );

    vw->init( tb );

    QVBoxLayout *L = new QVBoxLayout( this );
    L->setSpacing( 0 );
    L->setMargin( 0 );
    L->addWidget( tb );
    L->addWidget( vw );
}


