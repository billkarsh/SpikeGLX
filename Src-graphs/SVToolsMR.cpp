
#include "ui_TSpikeEditor.h"

#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "SVGrafsM.h"
#include "SVToolsMR.h"
#include "SignalBlocker.h"

#include <QApplication>
#include <QButtonGroup>
#include <QPushButton>
#include <QPainter>


/* ---------------------------------------------------------------- */
/* Tspike --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

TSpikeEditor::TSpikeEditor( QWidget *parent, SVGrafsM *gr )
    :   DlgEditor(parent), gr(gr), tsUI(0)
{
    setWindowFlags( Qt::Popup );

    tsUI = new Ui::TSpikeEditor;
    tsUI->setupUi( this );

    gr->getTspike( T );
    tsUI->nFixT->setValue( T.nFixT );
    tsUI->pFixT->setValue( T.pFixT );
    tsUI->negGB->setChecked( T.nDo );
    tsUI->posGB->setChecked( T.pDo );
    tsUI->nFixRadio->setChecked( T.nFix );
    tsUI->pFixRadio->setChecked( T.pFix );

    QButtonGroup    *bg;

    bg = new QButtonGroup( this );
    bg->addButton( tsUI->nFixRadio );

    bg = new QButtonGroup( this );
    bg->addButton( tsUI->pFixRadio );

    connect( tsUI->negGB, &QGroupBox::clicked, this, [this]() {
        T.nDo = tsUI->negGB->isChecked();
        this->gr->setTspike( T );
    });

    connect( tsUI->posGB, &QGroupBox::clicked, this, [this]() {
        T.pDo = tsUI->posGB->isChecked();
        this->gr->setTspike( T );
    });

    // connect( tsUI->nFixRadio, &QRadioButton::clicked, this, [this]() {
    //     bool fixed = tsUI->nFixRadio->isChecked();
    //     tsUI->nFixT->setEnabled( fixed );
    // });

    // connect( tsUI->pFixRadio, &QRadioButton::clicked, this, [this]() {
    //     bool fixed = tsUI->pFixRadio->isChecked();
    //     tsUI->pFixT->setEnabled( fixed );
    // });

    connect( tsUI->nFixT, &QSpinBox::valueChanged, this, [this]() {
        T.nFixT = tsUI->nFixT->value();
        this->gr->setTspike( T );
    });

    connect( tsUI->pFixT, &QSpinBox::valueChanged, this, [this]() {
        T.pFixT = tsUI->pFixT->value();
        this->gr->setTspike( T );
    });

    connect( tsUI->doneBut, &QPushButton::clicked, this, [this]() {
        emit finished();
    });
}


TSpikeEditor::~TSpikeEditor()
{
    if( tsUI ) {
        delete tsUI;
        tsUI = 0;
    }
}


void TSpikeEditor::init()
{
    show();
}

/* ---------------------------------------------------------------- */
/* SVToolsMR ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )
#define LVBUT_STYLE     "font-size: 20px;"
#define LVLBL_STYLE     "font-size: 14px; color: #444444;"
#define LVVAL_STYLE     "color: #0078D7; font-weight: bold;"
#define DBUT_LBL_STYLE  "color: #444444; font-weight: bold;"


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


void SVToolsMR::init()
{
    LVBut           *LV;
    LVE_chk         *LVC;
    LVE_int         *LVI;
    LVE_dbl         *LVD;
    LVE_cb          *LVB;
    DlgBut          *DBUT;
    TSpikeEditor    *TS;
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
                "Set graph time span in seconds.",
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

// Baselines

    LVC = new LVE_chk(
                "Baselines",
                "Draw graph baselines.",
                (gr->isBlChkOn() ? "1" : "0"), this );
    ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), gr, SLOT(blChkClicked(bool)) );

    LV = new LVBut(
            "BL", (gr->isBlChkOn() ? "1" : "0"),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVC, this );
    addWidget( LV );

// ----------------
// Neural filtering
// ----------------

    if( gr->neurChanCount() > 0 ) {

        // -<S>: Always

        LVB = new LVE_cb(
                    "Spatially average AP-band channels"
                    " and subtract common signal.\n"
                    " - Loc I,O\t= local donut {in,out} radii (sites)\n"
                    " - Glb All\t= average all channels\n"
                    " - Gbl Dmx\t= ave chans with shared multiplex phase",
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

// ----------
// Thresholds
// ----------

    TS   = new TSpikeEditor( this, gr );
    DBUT = new DlgBut(
            "T", LVBUT_STYLE, DBUT_LBL_STYLE, TS, this );
    addWidget( DBUT );

// N

    TSpikeSettings  T;
    gr->getTspike( T );

    LVI = new LVE_int(
                "Brightest negative spikes are (-uV):",
                QString("%1").arg( T.nMax ), this );

    LVI->m_spinBox->installEventFilter( gr->getGWWidget() );
    LVI->m_spinBox->setRange( 0, 5000 );
    LVI->m_spinBox->setSingleStep( 10 );
    LVI->m_spinBox->setValue( T.nMax );
    ConnectUI( LVI->m_spinBox, SIGNAL(valueChanged(int)), gr, SLOT(rasNMaxChanged(int)) );

    LV = new LVBut(
            "N", QString("%1").arg( T.nMax ),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVI, this );
    addWidget( LV );

// P

    LVI = new LVE_int(
                "Brightest positive spikes are (uV):",
                QString("%1").arg( T.pMax ), this );

    LVI->m_spinBox->installEventFilter( gr->getGWWidget() );
    LVI->m_spinBox->setRange( 0, 5000 );
    LVI->m_spinBox->setSingleStep( 10 );
    LVI->m_spinBox->setValue( T.pMax );
    ConnectUI( LVI->m_spinBox, SIGNAL(valueChanged(int)), gr, SLOT(rasPMaxChanged(int)) );

    LV = new LVBut(
            "P", QString("%1").arg( T.pMax ),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVI, this );
    addWidget( LV );
}


void SVToolsMR::setSelName( const QString &name )
{
    findChild<QAction*>( "chanact" )->setText( name );
}


void SVToolsMR::update()
{
    QAction         *maxmz;
    LVBut           *xspin;
    QPushButton     *colorbtn;

    maxmz       = findChild<QAction*>( "maxact" );
    xspin       = findChild<LVBut*>( "xspin" );
    colorbtn    = findChild<QPushButton*>( "colorbtn" );

    SignalBlocker
            b0(maxmz),
            b1(xspin),
            b2(colorbtn);

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

    QPixmap     pm( 22, 22 );
    QPainter    pnt;

    pnt.begin( &pm );
    pnt.fillRect( 0, 0, 22, 22, QBrush( gr->getSelColor() ) );
    pnt.end();
    colorbtn->setIcon( QIcon( pm ) );

// Type-specific

    bool    enabled = gr->isSelAnalog();

    colorbtn->setEnabled( enabled );
}


