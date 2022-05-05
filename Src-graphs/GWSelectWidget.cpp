
#include "GWSelectWidget.h"
#include "Util.h"
#include "DAQ.h"
#include "GraphsWindow.h"

#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>




// Rules for initial left and right selections:
//
// Left: Select imec0 if present, else obx0, else item 0.
// R-Chk: Checked if more than one stream.
// Right: Select imec1, else item 0.
//
GWSelectWidget::GWSelectWidget( GraphsWindow *gw, const DAQ::Params &p )
    : gw(gw), p(p), lIdx(-1), rIdx(-1)
{
    QHBoxLayout *HBX = new QHBoxLayout;
    QLabel      *LBL;

// Left label

    LBL = new QLabel( "Left" );
    HBX->addWidget( LBL );

// Left selector

    lCB = new QComboBox( this );
    p.streamCB_fillRuntime( lCB );
    if( !p.streamCB_selItem( lCB, "imec0", true ) )
        p.streamCB_selItem( lCB, "obx0", true );
    ConnectUI( lCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( lCB );

// Right check

    rchk = new QCheckBox( "Right", this );
    rchk->setChecked( lCB->count() > 1 );
    ConnectUI( rchk, SIGNAL(clicked(bool)), gw, SLOT(initViews()) );
    HBX->addWidget( rchk );

// Right selector

    rCB = new QComboBox( this );
    p.streamCB_fillRuntime( rCB );
    p.streamCB_selItem( rCB, "imec1", true );
    ConnectUI( rCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( rCB );

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


bool GWSelectWidget::rChecked()
{
    return rchk->isChecked();
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


