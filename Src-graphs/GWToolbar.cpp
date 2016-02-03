
#include "Pixmaps/play.xpm"
#include "Pixmaps/pause.xpm"
#include "Pixmaps/window_fullscreen.xpm"
#include "Pixmaps/window_nofullscreen.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "DAQ.h"
#include "GraphsWindow.h"
#include "GWToolbar.h"
#include "ClickableLabel.h"
#include "SignalBlocker.h"

#include <QIcon>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QLineEdit>
#include <QPainter>
#include <QAction>
#include <QSettings>
#include <QColorDialog>




#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )


static const QIcon  *playIcon           = 0,
                    *pauseIcon          = 0,
                    *graphMaxedIcon     = 0,
                    *graphNormalIcon    = 0,
                    *applyAllIcon       = 0;

static void initIcons()
{
    if( !playIcon )
        playIcon = LOADICON( play_xpm );

    if( !pauseIcon )
        pauseIcon = LOADICON( pause_xpm );

    if( !graphMaxedIcon )
        graphMaxedIcon = LOADICON( window_fullscreen_xpm );

    if( !graphNormalIcon )
        graphNormalIcon = LOADICON( window_nofullscreen_xpm );

    if( !applyAllIcon )
        applyAllIcon = LOADICON( apply_all_xpm );
}


/*	Toolbar layout:
    ---------------
    Sort selector
    selected channel label
    |sep|
    Pause
    Expand
    |sep|
    Seconds
    Yscale
    Color
    Apply all
    |sep|
    Filter
    |sep|
    Trigger enable
    Next run name
    |sep|
    Stop Run
*/

GWToolbar::GWToolbar( GraphsWindow *gw, DAQ::Params &p )
    : gw(gw), p(p), paused(false)
{
    QAction         *A;
    QDoubleSpinBox  *S;
    QPushButton     *B;
    QCheckBox       *C;
    QLineEdit       *E;
    QLabel          *L;

    initIcons();

// Sort selector

    B = new QPushButton( this );
    B->setObjectName( "sortbtn" );
    B->setToolTip( "Toggle graph sort order: user/acquired" );
    updateSortButText();
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.srtUsrOrderAct, SLOT(trigger()) );
    addWidget( B );

// Channel label

    L = new ClickableLabel( this );
    L->setObjectName( "chanlbl" );
    L->setToolTip( "Selected graph (click to find)" );
    L->setMargin( 3 );
    L->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( L, SIGNAL(clicked()), gw, SLOT(ensureSelectionVisible()) );
    addWidget( L );

// Pause

    addSeparator();

    A = addAction(
            *pauseIcon,
            "Pause/unpause all graphs",
            this, SLOT(toggleFetcher()) );
    A->setObjectName( "pauseact" );
    A->setCheckable( true );

// Expand

    A = addAction(
            *graphMaxedIcon,
            "Maximize/Restore graph",
            gw, SLOT(toggleMaximized()) );
    A->setObjectName( "maxact" );
    A->setCheckable( true );

// Seconds

    addSeparator();

    L = new QLabel( "Secs", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "xspin" );
    S->installEventFilter( gw );
    S->setDecimals( 3 );
    S->setRange( .001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), gw, SLOT(graphSecsChanged(double)) );
    addWidget( S );

// Yscale

    L = new QLabel( "YScale", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "yspin" );
    S->installEventFilter( gw );
    S->setRange( 0.01, 9999.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), gw, SLOT(graphYScaleChanged(double)) );
    addWidget( S );

// Color

    L = new QLabel( "Color", this );
    addWidget( L );

    B = new QPushButton( this );
    B->setObjectName( "colorbtn" );
    ConnectUI( B, SIGNAL(clicked(bool)), gw, SLOT(showColorDialog()) );
    addWidget( B );

// Apply all

    addAction(
        *applyAllIcon,
        "Apply {secs,scale,color} to all graphs of like type",
        gw, SLOT(applyAll()) );

// Filter

    addSeparator();

    STDSETTINGS( settings, "cc_graphs" );
    settings.beginGroup( "DataOptions" );

    bool    setting_flt = settings.value( "filter", false ).toBool();

    C = new QCheckBox( "Filter <300Hz", this );
    C->setObjectName( "filterchk" );
    C->setToolTip( "Applied only to neural channels" );
    C->setChecked( setting_flt );
    ConnectUI( C, SIGNAL(clicked(bool)), gw, SLOT(hipassClicked(bool)) );
    addWidget( C );

// Trigger enable

    addSeparator();

    C = new QCheckBox( "Trig enabled", this );
    C->setObjectName( "trgchk" );
    C->setChecked( !p.mode.trgInitiallyOff );
    ConnectUI( C, SIGNAL(clicked(bool)), gw, SLOT(setTrgEnable(bool)) );
    addWidget( C );

// Run name

    E = new QLineEdit( p.sns.runName, this );
    E->setObjectName( "runedit" );
    E->installEventFilter( gw );
    E->setToolTip( "<newName>, or, <curName_gxx_txx>" );
    E->setEnabled( p.mode.trgInitiallyOff );
    E->setMinimumWidth( 100 );
    addWidget( E );

// Stop

    addSeparator();

    B = new QPushButton( this );
    B->setText( "Stop Run" );
    B->setToolTip(
        "Stop experiment, close files, close this window" );
    B->setAutoFillBackground( true );
    B->setStyleSheet( "color: rgb(170, 0, 0)" );    // brick red
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.stopAcqAct, SLOT(trigger()) );
    addWidget( B );
}


void GWToolbar::updateSortButText()
{
    QPushButton *B = findChild<QPushButton*>( "sortbtn" );

    if( mainApp()->isSortUserOrder() )
        B->setText( "Usr Order" );
    else
        B->setText( "Acq Order" );
}


void GWToolbar::setSelName( const QString &name )
{
    QLabel  *L = findChild<QLabel*>( "chanlbl" );

    L->setText( name );
    update();
}


bool GWToolbar::getScales( double &xSpn, double &yScl ) const
{
    QDoubleSpinBox  *X, *Y;

    X = findChild<QDoubleSpinBox*>( "xspin" );
    Y = findChild<QDoubleSpinBox*>( "yspin" );

    if( !X->hasAcceptableInput() || !Y->hasAcceptableInput() )
        return false;

    xSpn = X->value();
    yScl = Y->value();

    return true;
}


QColor GWToolbar::selectColor( QColor inColor )
{
    QColorDialog::setCustomColor( 0, NeuGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 1, AuxGraphBGColor.rgb() );
    QColorDialog::setCustomColor( 2, DigGraphBGColor.rgb() );

    return QColorDialog::getColor( inColor, gw );
}


bool GWToolbar::getFltCheck() const
{
    QCheckBox*  C = findChild<QCheckBox*>( "filterchk" );

    return C->isChecked();
}


bool GWToolbar::getTrigCheck() const
{
    QCheckBox   *C = findChild<QCheckBox*>( "trgchk" );

    return C->isChecked();
}


void GWToolbar::setTrigCheck( bool on )
{
    QCheckBox   *C = findChild<QCheckBox*>( "trgchk" );

    C->setChecked( on );
}


QString GWToolbar::getRunLE() const
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    return LE->text().trimmed();
}


void GWToolbar::setRunLE( const QString &name )
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    LE->setText( name );
}


void GWToolbar::enableRunLE( bool enabled )
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    LE->setEnabled( enabled );
}


void GWToolbar::update()
{
    QAction         *pause, *maxmz;
    QDoubleSpinBox  *xspin, *yspin;
    QPushButton     *colorbtn;

    pause       = findChild<QAction*>( "pauseact" );
    maxmz       = findChild<QAction*>( "maxact" );
    xspin       = findChild<QDoubleSpinBox*>( "xspin" );
    yspin       = findChild<QDoubleSpinBox*>( "yspin" );
    colorbtn    = findChild<QPushButton*>( "colorbtn" );

    SignalBlocker
            b0(pause),
            b1(maxmz),
            b2(xspin),
            b3(yspin),
            b4(colorbtn);

    pause->setChecked( paused );
    pause->setIcon( paused ? *playIcon : *pauseIcon );

    if( gw->isMaximized() ) {
        maxmz->setChecked( true );
        maxmz->setIcon( *graphNormalIcon );
    }
    else {
        maxmz->setChecked( false );
        maxmz->setIcon( *graphMaxedIcon );
    }

// Analog and digital

    double  xSpn, yScl;
    gw->getSelGraphScales( xSpn, yScl );

    xspin->setValue( xSpn );
    yspin->setValue( yScl );

    QPixmap     pm( 22, 22 );
    QPainter    pnt;

    pnt.begin( &pm );
    pnt.fillRect( 0, 0, 22, 22, QBrush( gw->getSelGraphColor() ) );
    pnt.end();
    colorbtn->setIcon( QIcon( pm ) );

// Type-specific

    bool    enabled = gw->isSelGraphAnalog();

    yspin->setEnabled( enabled );
    colorbtn->setEnabled( enabled );
}


void GWToolbar::toggleFetcher()
{
    paused = !paused;
    mainApp()->getRun()->grfPause( paused );
    update();
}


