
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
#include <QLabel>




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


static QLabel* midSep( QWidget *p )
{
    QLabel  *L = new QLabel( "|", p );
    L->setFont( QFont( "Ariel", 18, QFont::Bold ) );
    L->setStyleSheet(
        "padding-left: 2px; padding-right: 0px;"
        " padding-top: 0px; padding-bottom: 6px;"
        " color: rgb(0, 0, 170)" );
    return L;
}


/*  Toolbar layout:
    ---------------
    Stop Run
    time
    |sep|
    Trigger enable
    Next run name
    |sep|
    'Graphs'
    Pause
*/

RunToolbar::RunToolbar( GraphsWindow *gw, DAQ::Params &p )
    : gw(gw), p(p), paused(false)
{
    QPushButton *B;
    QLineEdit   *E;
    QAction     *A;
    QLabel      *L;

    initIcons();

// Stop

    B = new QPushButton( this );
    B->setText( "Stop Acquisition" );
    B->setToolTip(
        "Stop experiment, close files, close this window" );
    B->setAutoFillBackground( true );
    B->setStyleSheet(
        "padding-left: 8px; padding-right: 8px;"
        " padding-top: 4px; padding-bottom: 4px;"
        " color: rgb(170, 0, 0)" );
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.stopAcqAct, SLOT(trigger()) );
    addWidget( B );

// Time

    L = new QLabel( " ", this );
    L->setFont( QFont( "Courier", 12, QFont::Bold ) );
    addWidget( L );

    L = new QLabel( "00:00:00", this );
    L->setObjectName( "timelbl" );
    L->setFont( QFont( "Courier", 14, QFont::Bold ) );
    addWidget( L );

// Recording

    addWidget( midSep( this ) );

    if( p.mode.manOvShowBut ) {

        B = new QPushButton(
            (p.mode.manOvInitOff ? "Enable Recording" : "Disable Recording"),
            this );
        B->setObjectName( "recordbtn" );
        B->setCheckable( true );
        B->setChecked( !p.mode.manOvInitOff );
        B->setStyleSheet(
            "padding-left: 8px; padding-right: 8px;"
            " padding-top: 4px; padding-bottom: 4px;"
            " color: rgb(0, 0, 0)" );
        ConnectUI( B, SIGNAL(clicked(bool)), this, SLOT(recordButClicked(bool)) );
        addWidget( B );
    }

// Run name

    E = new QLineEdit( p.sns.runName, this );
    E->setObjectName( "runedit" );
    E->installEventFilter( gw );
    E->setToolTip( "<newName>, or, <curName_gxx_txx>" );
    E->setEnabled( p.mode.manOvInitOff );
    E->setMinimumWidth( 100 );
    addWidget( E );

// Graphs

    addWidget( midSep( this ) );

    L = new QLabel( "Graphs:", this );
    addWidget( L );

    A = addAction(
            *pauseIcon,
            "Pause/unpause visualization",
            this, SLOT(toggleFetcher()) );
    A->setObjectName( "pauseact" );
    A->setCheckable( true );
}


void RunToolbar::updateTime( const QString &s )
{
    findChild<QLabel*>( "timelbl" )->setText( s );
}


void RunToolbar::setRecordingEnabled( bool on, bool block )
{
    if( !p.mode.manOvShowBut )
        return;

    QPushButton *B = findChild<QPushButton*>( "recordbtn" );

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


void RunToolbar::recordButClicked( bool checked )
{
    setRecordingEnabled( checked, true );
    gw->tbSetRecordingEnabled( checked );
}


void RunToolbar::toggleFetcher()
{
    paused = !paused;
    mainApp()->getRun()->grfPause( paused );
    update();
}


