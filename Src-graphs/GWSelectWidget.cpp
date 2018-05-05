
#include "GWSelectWidget.h"
#include "Util.h"
#include "DAQ.h"
#include "GraphsWindow.h"
#include "GUIControls.h"

#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>




// Rules for initial left and right selections:
//
// Left: Select imec0 if present, else nidq.
// R-Chk: Checked if more than one stream.
// Right: Select imec1, else nidq, else item 0.
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
    FillExtantStreamCB( lCB, p.ni.enabled, p.im.get_nProbes() );
    SelStreamCBItem( lCB, "imec0" );    // Note: nidq if fails
    ConnectUI( lCB, SIGNAL(currentIndexChanged(int)), gw, SLOT(initViews()) );
    HBX->addWidget( lCB );

// Right check

    rchk = new QCheckBox( "Right", this );
    rchk->setChecked( lCB->count() > 1 );
    ConnectUI( rchk, SIGNAL(clicked(bool)), gw, SLOT(initViews()) );
    HBX->addWidget( rchk );

// Right selector

    rCB = new QComboBox( this );
    FillExtantStreamCB( rCB, p.ni.enabled, p.im.get_nProbes() );

    if( !SelStreamCBItem( rCB, "imec1" ) )
        SelStreamCBItem( rCB, "nidq" ); // Note: imec0 if fails

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


int GWSelectWidget::lType()
{
    QString s = lCB->currentText();

    return (s == "nidq" ? -1 : p.streamID( s ));
}


int GWSelectWidget::rType()
{
    QString s = rCB->currentText();

    return (s == "nidq" ? -1 : p.streamID( s ));
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


