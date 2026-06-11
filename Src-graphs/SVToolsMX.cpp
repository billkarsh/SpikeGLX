
#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "SVGrafsM.h"
#include "SVToolsMX.h"
#include "SignalBlocker.h"
#include "ToolBut.h"

#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QPainter>




#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )
#define LVBUT_STYLE     "font-size: 20px;"
#define LVLBL_STYLE     "font-size: 14px; color: #444444;"
#define LVVAL_STYLE     "color: #0078D7; font-weight: bold;"


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


void SVToolsMX::init()
{
    LVBut           *LV;
    LVE_chk         *LVC;
    LVE_dbl         *LVD;
    LVE_cb          *LVB;
    QPushButton     *B;
    QAction         *A;

    initIcons();

    // setObjectName( "SVToolsM" );
    // setStyleSheet( "#SVToolsM { background-color: #4a4a4a; }" );

// Channel label

    A = new QAction( "", this );
    A->setObjectName( "chanact" );
    A->setToolTip( "Selected graph (click to find)" );
    A->setFont( QFont( QApplication::font().family(), 12, QFont::DemiBold ) );
    ConnectUI( A, SIGNAL(triggered(bool)), gr, SLOT(ensureSelectionVisible()) );
    addAction( A );

    QWidget *wA = widgetForAction( A );
    if( wA ) {
        wA->setStyleSheet(
            "QToolButton { padding-left: 0px; padding-right: 0px; margin: 0px; }" );
        wA->setSizePolicy( QSizePolicy::Minimum, wA->sizePolicy().verticalPolicy() );
    }

    // Color

    B = new QPushButton( this );
    B->setObjectName( "colorbtn" );
    B->setToolTip( "Apply color to selected trace" );
    B->setFixedSize( 24, 24 );
    ConnectUI( B, SIGNAL(clicked(bool)), gr, SLOT(showColorDialog()) );
    addWidget( B );

// Expand

    A = addAction(
            *graphMaxedIcon,
            "Maximize/restore graph (dbl-click graph)",
            gr, SLOT(toggleMaximized()) );
    A->setObjectName( "maxact" );
    A->setCheckable( true );

// Seconds

    LVD = new LVE_dbl(
                "Set the graph window time span in seconds.",
                "1", this );

    LVD->m_spinBox->installEventFilter( gr->getGWWidget() );
    LVD->m_spinBox->setDecimals( 3 );
    LVD->m_spinBox->setRange( .001, 30.0 );
    LVD->m_spinBox->setSingleStep( 0.25 );
    ConnectUI( LVD->m_spinBox, SIGNAL(valueChanged(double)), gr, SLOT(graphSecsChanged(double)) );

    LV = new LVBut(
            "X", "1",
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVD, this );
    LV->setObjectName( "xspin" );
    addWidget( LV );

// Yscale

    LVD = new LVE_dbl(
                "Set the graph vertical magnification factor.",
                "1", this );

    LVD->m_spinBox->installEventFilter( gr->getGWWidget() );
    LVD->m_spinBox->setRange( 0.0, 999.0 );
    LVD->m_spinBox->setSingleStep( 0.25 );
    ConnectUI( LVD->m_spinBox, SIGNAL(valueChanged(double)), gr, SLOT(graphYScaleChanged(double)) );

    LV = new LVBut(
            "Y", "1",
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVD, this );
    LV->setObjectName( "yspin" );
    addWidget( LV );

// ----------------
// Neural filtering
// ----------------

    if( gr->neurChanCount() > 0 ) {

        if( gr->isImec() ) {
            LVB = new LVE_cb(
                        "Band-pass filter applied to AP-band channels.",
                        this );
            LVB->m_comboBox->addItem( "Off" );
            LVB->m_comboBox->addItem( "300-INF" );
            LVB->m_comboBox->addItem( "0.5-500" );
            LVB->m_comboBox->addItem( "+LF" );
        }
        else {
            LVB = new LVE_cb(
                        "Band-pass filter applied to neural channels.",
                        this );
            LVB->m_comboBox->setToolTip( "Applied only to neural channels" );
            LVB->m_comboBox->addItem( "Off" );
            LVB->m_comboBox->addItem( "300-INF" );
            LVB->m_comboBox->addItem( "0.1-300" );
        }
        LVB->m_comboBox->setCurrentIndex( gr->curBandSel() );
        LVB->setValue( LVB->m_comboBox->currentText() );
        ConnectUI( LVB->m_comboBox, SIGNAL(currentIndexChanged(int)), gr, SLOT(bandSelChanged(int)) );

        LV = new LVBut(
                (gr->isImec() ? "AP" : "BP"), LVB->m_comboBox->currentText(),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVB, this );
        addWidget( LV );

        // -<Tn> (DC filter): Always

        LVC = new LVE_chk(
                    "DCneural",
                    "Subtract time-averaged DC level of neural channels;"
                    " updates every 5 seconds.",
                    (gr->isTnChkOn() ? "1" : "0"), this );
        ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), gr, SLOT(tnChkClicked(bool)) );

        LV = new LVBut(
                "DCn", (gr->isTnChkOn() ? "1" : "0"),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVC, this );
        addWidget( LV );

        // -<S>: Always

        LVB = new LVE_cb(
                    "Spatially average AP-band channels"
                    " and subtract out common signal.\n"
                    " - Loc I,O = local donut {in,out} radii (sites)\n"
                    " - Glb All = average all channels\n"
                    " - Gbl Dmx = ave chans with shared multiplex phase",
                    this );
        LVB->m_comboBox->addItem( "Off" );
        gr->nameLocalFilters( LVB->m_comboBox );
        LVB->m_comboBox->addItem( "Glb All" );
        LVB->m_comboBox->addItem( "Glb Dmx" );
        LVB->m_comboBox->setCurrentIndex( gr->curSAveSel() );
        LVB->setValue( LVB->m_comboBox->currentText() );
        ConnectUI( LVB->m_comboBox, SIGNAL(currentIndexChanged(int)), gr, SLOT(sAveSelChanged(int)) );

        LV = new LVBut(
                "CAR", LVB->m_comboBox->currentText(),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVB, this );
        addWidget( LV );
    }

// -------------
// Aux filtering
// -------------

    if( gr->analogChanCount() > gr->neurChanCount() ) {

        LVC = new LVE_chk(
                    "DCaux",
                    "Subtract time-averaged DC level of AUX channels;"
                    " updates every 5 seconds.",
                    (gr->isTxChkOn() ? "1" : "0"), this );
        ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), gr, SLOT(txChkClicked(bool)) );

        LV = new LVBut(
                "DCx", (gr->isTxChkOn() ? "1" : "0"),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVC, this );
        addWidget( LV );
    }

// ------
// BinMax
// ------

    LVC = new LVE_chk(
                "BinMax",
                "Downsample by selecting extreme values in each bin;"
                " helps catch rare signals.",
                (gr->isBinMaxOn() ? "1" : "0"), this );
    ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), gr, SLOT(binMaxChkClicked(bool)) );

    LV = new LVBut(
            "BM", (gr->isBinMaxOn() ? "1" : "0"),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVC, this );
    addWidget( LV );
}


void SVToolsMX::setSelName( const QString &name )
{
    findChild<QAction*>( "chanact" )->setText( name );
}


void SVToolsMX::update()
{
    QAction         *maxmz;
    LVBut           *xspin, *yspin;
    QPushButton     *colorbtn;

    maxmz       = findChild<QAction*>( "maxact" );
    xspin       = findChild<LVBut*>( "xspin" );
    yspin       = findChild<LVBut*>( "yspin" );
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

    xspin->setValue( QString("%1").arg( xSpn ) );
    yspin->setValue( QString("%1").arg( yScl ) );

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


