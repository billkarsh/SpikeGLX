
#include "Pixmaps/shanks.xpm"

#include "Util.h"
#include "CimCfg.h"
#include "SVGrafsM.h"
#include "MNavbar.h"
#include "SignalBlocker.h"

#include <QPushButton>
#include <QSpinBox>
#include <QLineEdit>
#include <QSlider>
#include <QLabel>




#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )


static const QIcon  *shankMapIcon = 0;

static void initIcons()
{
    if( !shankMapIcon )
        shankMapIcon = LOADICON( shanks_xpm );
}


MNavbar::MNavbar( SVGrafsM *gr ) : gr(gr)
{
    QPushButton     *B;
    QSpinBox        *S;
    QLineEdit       *E;
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

    addSeparator();

    L = new QLabel( "NChan", this );
    addWidget( L );

    S = new QSpinBox( this );
    S->setObjectName( "nchansb" );
    S->installEventFilter( gr->getGWWidget() );
    S->setMinimum( 1 );
    S->setMaximum( qMin( (int)IMROTbl::imType0Chan, gr->chanCount() ) );
    S->setValue( curNChan = gr->navNChan() );
    ConnectUI( S, SIGNAL(valueChanged(int)), this, SLOT(nchanChanged(int)) );
    addWidget( S );

// 1st

    L = new QLabel( "1st", this );
    addWidget( L );

    E = new QLineEdit( "0", this );
    E->setObjectName( "1stle" );
    E->setEnabled( false );
    E->setFixedWidth( 40 );
    addWidget( E );

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
    findChild<QSpinBox*>( "nchansb" )->setEnabled( enabled );
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
    SL->setValue( slpos );
    pageChanged( slpos, notify );

    if( notify )
        gr->nchanChanged( val, slpos*val );
}


void MNavbar::pageChanged( int val, bool notify )
{
    val *= findChild<QSpinBox*>( "nchansb" )->value();

    findChild<QLineEdit*>( "1stle" )->setText( QString::number( val ) );

    if( notify )
        gr->firstChanged( val );
}


