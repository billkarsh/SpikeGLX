
#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "SVToolsM.h"
#include "SignalBlocker.h"

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QAction>
#include <QLabel>
#include <QColorDialog>




#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )


static const QIcon  *graphMaxedIcon     = 0,
                    *graphNormalIcon    = 0,
                    *applyAllIcon       = 0;

static void initIcons()
{
    if( !graphMaxedIcon )
        graphMaxedIcon = LOADICON( window_fullscreen_xpm );

    if( !graphNormalIcon )
        graphNormalIcon = LOADICON( window_nofullscreen_xpm );

    if( !applyAllIcon )
        applyAllIcon = LOADICON( apply_all_xpm );
}


SVToolsM::SVToolsM( SVGrafsM *gr ) : gr(gr)
{
    QDoubleSpinBox  *S;
    QPushButton     *B;
    QCheckBox       *C;
    QAction         *A;
    QLabel          *L;

    initIcons();

// Sort selector

    B = new QPushButton( this );
    B->setObjectName( "sortbtn" );
    B->setToolTip( "Toggle graph sort order: user/acquired" );
    ConnectUI( B, SIGNAL(clicked()), gr, SLOT(toggleSorting()) );
    addWidget( B );

// Channel label

    A = new QAction( "", this );
    A->setObjectName( "chanact" );
    A->setToolTip( "Selected graph (click to find)" );
    A->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( A, SIGNAL(triggered(bool)), gr, SLOT(ensureSelectionVisible()) );
    addAction( A );

// Expand

    A = addAction(
            *graphMaxedIcon,
            "Maximize/Restore graph",
            gr, SLOT(toggleMaximized()) );
    A->setObjectName( "maxact" );
    A->setCheckable( true );

// Seconds

    addSeparator();

    L = new QLabel( "Secs", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "xspin" );
    S->installEventFilter( gr->getGWWidget() );
    S->setDecimals( 3 );
    S->setRange( .001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), gr, SLOT(graphSecsChanged(double)) );
    addWidget( S );

// Yscale

    L = new QLabel( "YScale", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "yspin" );
    S->installEventFilter( gr->getGWWidget() );
    S->setRange( 0.01, 9999.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), gr, SLOT(graphYScaleChanged(double)) );
    addWidget( S );

// Color

    L = new QLabel( "Color", this );
    addWidget( L );

    B = new QPushButton( this );
    B->setObjectName( "colorbtn" );
    ConnectUI( B, SIGNAL(clicked(bool)), gr, SLOT(showColorDialog()) );
    addWidget( B );

// Apply all

    addAction(
        *applyAllIcon,
        "Apply {secs,scale,color} to all graphs of like type",
        gr, SLOT(applyAll()) );

// Filter

    addSeparator();

    C = new QCheckBox( "Filter <300Hz", this );
    C->setObjectName( "filterchk" );
    C->setToolTip( "Applied only to neural channels" );
    C->setChecked( gr->isFiltered() );
    ConnectUI( C, SIGNAL(clicked(bool)), gr, SLOT(hipassClicked(bool)) );
    addWidget( C );
}


void SVToolsM::setSelName( const QString &name )
{
    findChild<QAction*>( "chanact" )->setText( name );
}


QColor SVToolsM::selectColor( QColor inColor )
{
    QColorDialog::setCustomColor( 0, NeuGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 1, LfpGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 2, AuxGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 3, DigGraphBGColor.rgb() );

    return QColorDialog::getColor( inColor, gr->getGWWidget() );
}


void SVToolsM::update()
{
    QAction         *maxmz;
    QDoubleSpinBox  *xspin, *yspin;
    QPushButton     *colorbtn;

    maxmz       = findChild<QAction*>( "maxact" );
    xspin       = findChild<QDoubleSpinBox*>( "xspin" );
    yspin       = findChild<QDoubleSpinBox*>( "yspin" );
    colorbtn    = findChild<QPushButton*>( "colorbtn" );

    SignalBlocker
            b0(maxmz),
            b1(xspin),
            b2(yspin),
            b3(colorbtn);

    findChild<QPushButton*>( "sortbtn" )->setText(
        (gr->isUsrOrder() ? "Usr Order" : "Acq Order") );

    if( gr->isMaximized() ) {
        maxmz->setChecked( true );
        maxmz->setIcon( *graphNormalIcon );
    }
    else {
        maxmz->setChecked( false );
        maxmz->setIcon( *graphMaxedIcon );
    }

// Analog and digital

    double  xSpn, yScl;
    gr->getSelScales( xSpn, yScl );

    xspin->setValue( xSpn );
    yspin->setValue( yScl );

    QPixmap     pm( 22, 22 );
    QPainter    pnt;

    pnt.begin( &pm );
    pnt.fillRect( 0, 0, 22, 22, QBrush( gr->getSelColor() ) );
    pnt.end();
    colorbtn->setIcon( QIcon( pm ) );

// Type-specific

    bool    enabled = gr->isSelAnalog();

    yspin->setEnabled( enabled );
    colorbtn->setEnabled( enabled );
}


