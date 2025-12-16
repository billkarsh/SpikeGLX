
#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "GraphsWindow.h"
#include "SVGrafsM.h"
#include "SVToolsM.h"
#include "SignalBlocker.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QColorDialog>
#include <QPainter>




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


void SVToolsM::init()
{
    QDoubleSpinBox  *S;
    QPushButton     *B;
    QCheckBox       *C;
    QComboBox       *CB;
    QAction         *A;
    QLabel          *L;

    initIcons();

// Channel label

    A = new QAction( "", this );
    A->setObjectName( "chanact" );
    A->setToolTip( "Selected graph (click to find)" );
    A->setFont( QFont( QApplication::font().family(), 10, QFont::DemiBold ) );
    ConnectUI( A, SIGNAL(triggered(bool)), gr, SLOT(ensureSelectionVisible()) );
    addAction( A );

// Expand

    A = addAction(
            *graphMaxedIcon,
            "Maximize/restore graph",
            gr, SLOT(toggleMaximized()) );
    A->setObjectName( "maxact" );
    A->setCheckable( true );

// Seconds

    addSeparator();

    L = new QLabel( "Sec", this );
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

    L = new QLabel( "Yscl", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "yspin" );
    S->installEventFilter( gr->getGWWidget() );
    S->setRange( 0.0, 999.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), gr, SLOT(graphYScaleChanged(double)) );
    addWidget( S );

// Color

    L = new QLabel( "Clr", this );
    addWidget( L );

    B = new QPushButton( this );
    B->setObjectName( "colorbtn" );
    ConnectUI( B, SIGNAL(clicked(bool)), gr, SLOT(showColorDialog()) );
    addWidget( B );

// Apply all
// Retired October 2025 v 20250930

#if 0
    addAction(
        *applyAllIcon,
        "Apply Yscl to all graphs of like type",
        gr, SLOT(applyAll()) );
#endif

// ----------------
// Neural filtering
// ----------------

    if( gr->neurChanCount() > 0 ) {

        addSeparator();

        // Bandpass: Viewer customized

        CB = new QComboBox( this );

        if( gr->isImec() ) {
            CB->setToolTip( "Applied only to ap-band channels" );
            CB->addItem( "AP Native" );
            CB->addItem( "300 - INF" );
            CB->addItem( "0.5 - 500" );
            CB->addItem( "AP = AP+LF" );
        }
        else {
            CB->setToolTip( "Applied only to neural channels" );
            CB->addItem( "Pass All" );
            CB->addItem( "300 - INF" );
            CB->addItem( "0.1 - 300" );
        }

        CB->setCurrentIndex( gr->curBandSel() );
        ConnectUI( CB, SIGNAL(currentIndexChanged(int)), gr, SLOT(bandSelChanged(int)) );
        addWidget( CB );

        // -<Tn> (DC filter): Always

        C = new QCheckBox( "-<Tn>", this );
        C->setToolTip( "Temporally average neural channels" );
        C->setStyleSheet( "padding-left: 4px; padding-right: 4px" );
        C->setChecked( gr->isTnChkOn() );
        ConnectUI( C, SIGNAL(clicked(bool)), gr, SLOT(tnChkClicked(bool)) );
        addWidget( C );

        // -<S>: Always

        L = new QLabel( "-<S>", this );
        L->setTextFormat( Qt::PlainText );
        L->setAlignment( Qt::AlignCenter );
        L->setToolTip( "Spatially average spike channels" );
        L->setStyleSheet( "padding-bottom: 1px" );
        addWidget( L );

        CB = new QComboBox( this );
        CB->setToolTip( "Spatially average spike channels" );
        CB->addItem( "Off" );
        gr->nameLocalFilters( CB );
        CB->addItem( "Glb All" );
        CB->addItem( "Glb Dmx" );
        CB->setCurrentIndex( gr->curSAveSel() );
        ConnectUI( CB, SIGNAL(currentIndexChanged(int)), gr, SLOT(sAveSelChanged(int)) );
        addWidget( CB );
    }

// -------------
// Aux filtering
// -------------

    if( gr->analogChanCount() > gr->neurChanCount() ) {

        addSeparator();

        // -<Tx> (DC filter)

        C = new QCheckBox( "-<Tx>", this );
        C->setToolTip( "Temporally average auxiliary analog channels" );
        C->setStyleSheet( "padding-left: 4px; padding-right: 4px" );
        C->setChecked( gr->isTxChkOn() );
        ConnectUI( C, SIGNAL(clicked(bool)), gr, SLOT(txChkClicked(bool)) );
        addWidget( C );
    }

// ------
// BinMax
// ------

    addSeparator();

    C = new QCheckBox( "BinMax", this );
    C->setToolTip( "Draw peaks of each downsample bin" );
    C->setStyleSheet( "padding-left: 4px" );
    C->setChecked( gr->isBinMaxOn() );
    ConnectUI( C, SIGNAL(clicked(bool)), gr, SLOT(binMaxChkClicked(bool)) );
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


