
#include "Pixmaps/shanks.xpm"

#include "Util.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SignalBlocker.h"
#include "ToolBut.h"

#include <QPushButton>


#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )
#define LVBUT_STYLE     "font-size: 20px;"
#define LVLBL_STYLE     "font-size: 14px; color: #444444;"
#define LVVAL_STYLE     "color: #0078D7; font-weight: bold;"


static const QIcon  *shankMapIcon = 0;

static void initIcons()
{
    if( !shankMapIcon )
        shankMapIcon = LOADICON( shanks_xpm );
}


MNavbar::MNavbar( SVGrafsM *gr ) : gr(gr)
{
    LVBut           *LV;
    LVE_int         *LVS;
    QPushButton     *B;
    QSlider         *Z;
    QLabel          *L;
    int             curNChan;

    initIcons();

// Sort selector

    B = new QPushButton( this );
    B->setObjectName( "sortbtn" );
    B->setToolTip( "Toggle graph sort order: user/acquired" );
    ConnectUI( B, SIGNAL(clicked()), gr, SLOT(toggleSorting()) );
    addWidget( B );

// Shank map

    addAction(
        *shankMapIcon,
        "Show graphical shank map",
        gr, SLOT(showShanks()) );

// NChan

    LVS = new LVE_int(
                "Set number of channels per page; scroll pages using slider.",
                "1", this );

    LVS->m_spinBox->setObjectName( "nchansb" );
    LVS->m_spinBox->installEventFilter( gr->getGWWidget() );
    LVS->m_spinBox->setMinimum( 1 );
    LVS->m_spinBox->setMaximum( qMin( 384, gr->chanCount() ) );
    curNChan = gr->navNChan();
    LVS->setValue( QString("%1").arg( curNChan ) );
    ConnectUI( LVS->m_spinBox, SIGNAL(valueChanged(int)), this, SLOT(nchanChanged(int)) );

    LV = new LVBut(
            "Chan/page", QString("%1").arg( curNChan ),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVS, this );
    LV->setObjectName( "nchanbut" );
    addWidget( LV );

// slider

    L = new QLabel( " ", this );
    addWidget( L );

    Z = new QSlider( Qt::Horizontal, this );
    Z->setObjectName( "slider" );
    Z->setMinimum( 0 );
    Connect( Z, SIGNAL(valueChanged(int)), this, SLOT(pageChanged(int)), Qt::AutoConnection );
    addWidget( Z );

    nchanChanged( curNChan, false );
}


void MNavbar::setEnabled( bool enabled )
{
    findChild<LVBut*>( "nchanbut" )->setEnabled( enabled );
    findChild<QSlider*>("slider")->setEnabled( enabled );
}


void MNavbar::setPage( int page )
{
    findChild<QSlider*>("slider")->setValue( page );
}


int MNavbar::page() const
{
    return findChild<QSlider*>("slider")->value();
}


int MNavbar::first() const
{
    return page() * findChild<QSpinBox*>( "nchansb" )->value();
}


void MNavbar::update()
{
    findChild<QPushButton*>( "sortbtn" )->setText(
        (gr->isUsrOrder() ? "Usr Order" : "Acq Order") );
}


// Try to keep middle channel on current page visible.
//
void MNavbar::nchanChanged( int val, bool notify )
{
    QSlider *SL = findChild<QSlider*>("slider");

    SignalBlocker   b0(SL);

    int chans   = gr->chanCount(),
        preval  = gr->navNChan(),
        chmid   = (preval*SL->value() +
                    qMin( preval*(1 + SL->value()), chans )) / 2,
        slpos   = chmid / val;

    SL->setMaximum( (chans + val-1) / val - 1 );
    SL->setPageStep( SL->maximum() / 4 );
    SL->setValue( slpos );
    pageChanged( slpos, notify );

    if( notify )
        gr->nchanChanged( val, slpos*val );
}


void MNavbar::pageChanged( int val, bool notify )
{
    val *= findChild<QSpinBox*>( "nchansb" )->value();

    if( notify )
        gr->firstChanged( val );
}


