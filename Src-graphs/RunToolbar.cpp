
#include "ui_FVW_NotesDialog.h"

#include "Pixmaps/play.xpm"
#include "Pixmaps/pause.xpm"

#include "Util.h"
#include "MainApp.h"
#include "ConfigCtl.h"
#include "Run.h"
#include "DAQ.h"
#include "GraphsWindow.h"
#include "RunToolbar.h"
#include "SignalBlocker.h"

#include <QMessageBox>
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


RunToolbar::RunToolbar( GraphsWindow *gw, const DAQ::Params &p )
    :   gw(gw), p(p), paused(false)
{
    QPushButton *B;
    QLineEdit   *E;
    QAction     *A;
    QLabel      *L;

    initIcons();

    setObjectName( "runtoolbar" );
    toggleViewAction()->setEnabled( false );    // can't hide toolbar

// Stop

    B = new QPushButton( this );
    B->setText( "Stop Acquisition" );
    B->setAutoFillBackground( true );
    B->setStyleSheet(
        "padding-left: 8px; padding-right: 8px;"
        " padding-top: 4px; padding-bottom: 4px;"
        " color: rgb(170, 0, 0)" );
    ConnectUI( B, SIGNAL(clicked()), mainApp()->act.stopAcqAct, SLOT(trigger()) );
    addWidget( B );

// On time

    L = new QLabel( " ", this );
    L->setFont( QFont( "Courier", 12, QFont::Bold ) );
    addWidget( L );

    L = new QLabel( "--:--:--", this );
    L->setObjectName( "ontimelbl" );
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
        ConnectUI( B, SIGNAL(clicked(bool)), this, SLOT(recordBut(bool)) );
        addWidget( B );
    }

// Run name

    E = new QLineEdit(
                (p.mode.initG == -1 ?
                 p.sns.runName :
                 QString("%1_g%2_t%3")
                    .arg( p.sns.runName )
                    .arg( p.mode.initG )
                    .arg( p.mode.initT ) ),
                this );
    E->setObjectName( "runedit" );
    E->installEventFilter( gw );
    E->setToolTip( "<newName>, or, <curName_gxx_txx>" );
    E->setEnabled( p.mode.manOvInitOff );
    E->setMinimumWidth( 100 );
    addWidget( E );

// <G T>

    L = new QLabel( " ", this );
    L->setFont( QFont( "Courier", 12, QFont::Bold ) );
    addWidget( L );

    L = new QLabel( "<G-1 T-1>", this );
    L->setObjectName( "gtlbl" );
    L->setFont( QFont( "Courier", 14, QFont::Bold ) );
    addWidget( L );

// Rec time

    L = new QLabel( " ", this );
    L->setFont( QFont( "Courier", 12, QFont::Bold ) );
    addWidget( L );

    L = new QLabel( "--:--:--", this );
    L->setObjectName( "rectimelbl" );
    L->setFont( QFont( "Courier", 14, QFont::Bold ) );
    addWidget( L );

// Notes

    addWidget( midSep( this ) );

    B = new QPushButton( this );
    B->setObjectName( "notesbtn" );
    B->setText( "Notes" );
    B->setStyleSheet(
        "padding-left: 8px; padding-right: 8px;"
        " padding-top: 4px; padding-bottom: 4px;"
        " color: rgb(0, 0, 0)" );
    ConnectUI( B, SIGNAL(clicked()), this, SLOT(notes()) );
    addWidget( B );

// Graphs

    L = new QLabel( " ", this );
    L->setFont( QFont( "Courier", 12, QFont::Bold ) );
    addWidget( L );

    L = new QLabel( "Graphs:", this );
    addWidget( L );

    A = addAction(
            *pauseIcon,
            "Pause/unpause visualization",
            this, SLOT(toggleFetcher()) );
    A->setObjectName( "pauseact" );
    A->setCheckable( true );
}


void RunToolbar::updateOnTime( const QString &s )
{
    findChild<QLabel*>( "ontimelbl" )->setText( s );
}


void RunToolbar::updateRecTime( const QString &s )
{
    findChild<QLabel*>( "rectimelbl" )->setText( s );
}


void RunToolbar::updateGT( const QString &s )
{
    findChild<QLabel*>( "gtlbl" )->setText( s );
}


void RunToolbar::setRecordingEnabled( bool on, bool block )
{
// Notes button

    QPushButton *B = findChild<QPushButton*>( "notesbtn" );
    B->setEnabled( !on );

// Recording button

    if( !p.mode.manOvShowBut )
        return;

    B = findChild<QPushButton*>( "recordbtn" );

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


void RunToolbar::recordBut( bool checked )
{
    if( !checked && p.mode.manOvConfirm ) {

        int yesNo = QMessageBox::question(
            0,
            "Confirm: Stop Recording",
            "Recording in progress.\n"
            "Are you sure you want to stop?",
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No );

        if( yesNo != QMessageBox::Yes ) {

            setRecordingEnabled( true, true );
            return;
        }
    }

    setRecordingEnabled( checked, true );
    gw->tbSetRecordingEnabled( checked );
}


void RunToolbar::notes()
{
    QDialog             dlg;
    Ui::FVW_NotesDialog ui;

    dlg.setWindowFlags( dlg.windowFlags()
        & ~(Qt::WindowContextHelpButtonHint
            | Qt::WindowCloseButtonHint) );

    ui.setupUi( &dlg );
    ui.notesTE->setText( p.sns.notes );
    ui.buttonBox->setStandardButtons(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel );

    if( QDialog::Accepted == dlg.exec() ) {
        QString err;
        mainApp()->cfgCtl()->externSetsNotes(
            err, ui.notesTE->toPlainText().trimmed() );
    }
}


void RunToolbar::toggleFetcher()
{
    paused = !paused;
    mainApp()->getRun()->grfHardPause( paused );
    update();
}


