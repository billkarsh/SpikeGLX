
#include "GWSelectWidget.h"
#include "Util.h"
#include "DAQ.h"
#include "GraphsWindow.h"

#include <QHBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QPushButton>




// Rules for initial left and right selections:
//
// Left: Select imec0 if present, else obx0, else ni (item 0).
// Right: Select different than left, priority: probe, obx, ni.
// Grid: > 0 if more than one stream.
//
GWSelectWidget::GWSelectWidget( GraphsWindow *gw, const DAQ::Params &p )
    : gw(gw), p(p), lIdx(-1), rIdx(-1)
{
    QHBoxLayout *HBX = new QHBoxLayout;
    QPushButton *help;
    QLabel      *LBL;
    QString     lhs;

// Help

    help = new QPushButton( "Help" );
    help->setFixedWidth( 60 );
    ConnectUI( help, SIGNAL(clicked()), gw, SLOT(helpBut()) );
    HBX->addWidget( help );

// Left label

    LBL = new QLabel( "L" );
    HBX->addWidget( LBL );

// Left selector

    lCB = new QComboBox( this );
    p.streamCB_fillRuntime( lCB );

    if( !p.streamCB_selItem( lCB, (lhs = "imec0"), true ) ) {
        if( !p.streamCB_selItem( lCB, (lhs = "obx0"), true ) )
            lhs = "ni";
    }

    ConnectUI( lCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( lCB );

// Right label

    LBL = new QLabel( "R" );
    HBX->addWidget( LBL );

// Right selector

    rCB = new QComboBox( this );
    p.streamCB_fillRuntime( rCB );

    if( lhs == "imec0" ) {
        if( !p.streamCB_selItem( rCB, "imec1", true ) )
            p.streamCB_selItem( rCB, "obx0", true );
    }
    else if( lhs == "obx0" )
        p.streamCB_selItem( rCB, "obx1", true );

    ConnectUI( rCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( rCB );

// Grid selector

    gCB = new QComboBox( this );
    gCB->addItem( "L" );
    gCB->addItem( "L | R" );
    gCB->addItem( "L / R" );
    gCB->setCurrentIndex( lCB->count() > 1 );
    ConnectUI( gCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( gCB );

// insert items into widget

    setLayout( HBX );
}


bool GWSelectWidget::lChanged()
{
    return lIdx != lCB->currentIndex();
}


bool GWSelectWidget::rChanged()
{
    return rIdx != rCB->currentIndex();
}


int GWSelectWidget::grid()
{
    return gCB->currentIndex();
}


void GWSelectWidget::updateSelections()
{
    lIdx = lCB->currentIndex();
    rIdx = rCB->currentIndex();
}


int GWSelectWidget::ljsip( int &ip )
{
    return p.stream2jsip( ip, lCB->currentText() );
}


int GWSelectWidget::rjsip( int &ip )
{
    return p.stream2jsip( ip, rCB->currentText() );
}


QString GWSelectWidget::lStream()
{
    return lCB->currentText();
}


QString GWSelectWidget::rStream()
{
    return rCB->currentText();
}


void GWSelectWidget::setFocus( bool right )
{
    gw->raise();
    gw->activateWindow();

    if( right )
        rCB->setFocus( Qt::TabFocusReason );
    else
        lCB->setFocus( Qt::TabFocusReason );
}


