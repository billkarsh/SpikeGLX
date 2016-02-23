
#include "Pixmaps/play.xpm"
#include "Pixmaps/pause.xpm"

#include "Util.h"
#include "MainApp.h"
#include "Run.h"
#include "DAQ.h"
#include "GraphsWindow.h"
#include "RunToolbar.h"
#include "SignalBlocker.h"

#include <QPushButton>
#include <QLineEdit>
#include <QAction>




#define LOADICON( xpm ) new QIcon( QPixmap( xpm ) )


static const QIcon  *playIcon   = 0,
                    *pauseIcon  = 0;

static void initIcons()
{
    if( !playIcon )
        playIcon = LOADICON( play_xpm );

    if( !pauseIcon )
        pauseIcon = LOADICON( pause_xpm );
}


/*	Toolbar layout:
    ---------------
    Pause
    |sep|
    Trigger enable
    Next run name
    |sep|
    Stop Run
*/

RunToolbar::RunToolbar( GraphsWindow *gw, DAQ::Params &p )
    : gw(gw), p(p), paused(false)
{
    QPushButton *B;
    QLineEdit   *E;
    QAction     *A;

    initIcons();

// Pause

    A = addAction(
            *pauseIcon,
            "Pause/unpause all graphs",
            this, SLOT(toggleFetcher()) );
    A->setObjectName( "pauseact" );
    A->setCheckable( true );

// Trigger enable

    addSeparator();

    B = new QPushButton(
        (p.mode.trgInitiallyOff ? "Enable Recording" : "Disable Recording"),
        this );
    B->setObjectName( "trgbtn" );
    B->setCheckable( true );
    B->setChecked( !p.mode.trgInitiallyOff );
    B->setStyleSheet(
        "padding-left: 8px; padding-right: 8px;"
        " padding-top: 5px; padding-bottom: 5px" );
    ConnectUI( B, SIGNAL(clicked(bool)), this, SLOT(trigButClicked(bool)) );
    addWidget( B );

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
    B->setStyleSheet( "color: rgb(170, 0, 0)" ); // brick red
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.stopAcqAct, SLOT(trigger()) );
    addWidget( B );
}


void RunToolbar::setTrigEnable( bool on, bool block )
{
    QPushButton *B = findChild<QPushButton*>( "trgbtn" );

    B->setText( !on ? "Enable Recording" : "Disable Recording" );

    if( block )
        B->blockSignals( true );

    B->setChecked( on );
    B->blockSignals( false );
}


QString RunToolbar::getRunLE() const
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    return LE->text().trimmed();
}


void RunToolbar::setRunLE( const QString &name )
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    LE->setText( name );
}


void RunToolbar::enableRunLE( bool enabled )
{
    QLineEdit   *LE = findChild<QLineEdit*>( "runedit" );

    LE->setEnabled( enabled );
}


void RunToolbar::update()
{
    QAction *pause;

    pause   = findChild<QAction*>( "pauseact" );

    SignalBlocker   b0(pause);

    pause->setChecked( paused );
    pause->setIcon( paused ? *playIcon : *pauseIcon );
}


void RunToolbar::trigButClicked( bool checked )
{
    setTrigEnable( checked, true );
    gw->tbSetTrgEnable( checked );
}


void RunToolbar::toggleFetcher()
{
    paused = !paused;
    mainApp()->getRun()->grfPause( paused );
    update();
}


