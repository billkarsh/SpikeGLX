
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "FileViewerWindow.h"
#include "FVToolbar.h"
#include "SignalBlocker.h"

#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QAction>
#include <QLabel>




FVToolbar::FVToolbar( FileViewerWindow *fv, int fType ) : fv(fv)
{
    QDoubleSpinBox  *S;
    QSpinBox        *V;
    QPushButton     *B;
    QCheckBox       *C;
    QAction         *A;
    QLabel          *L;

// Sort selector

    B = new QPushButton( this );
    B->setObjectName( "sortbtn" );
    B->setToolTip( "Toggle graph sort order: user/acquired" );
    ConnectUI( B, SIGNAL(clicked()), fv, SLOT(tbToggleSort()) );
    addWidget( B );

// Selected

    A = new QAction( "", this );
    A->setObjectName( "nameact" );
    A->setToolTip( "Selected graph (click to find)" );
    A->setFont( QFont( "Courier", 10, QFont::Bold ) );
    ConnectUI( A, SIGNAL(triggered(bool)), fv, SLOT(tbScrollToSelected()) );
    addAction( A );

// X-Scale

    addSeparator();

    L = new QLabel( "Secs", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "xscalesb" );
    S->setToolTip( "Scan much faster with short span ~1sec" );
    S->setDecimals( 4 );
    S->setRange( 0.0001, 30.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), fv, SLOT(tbSetXScale(double)) );
    addWidget( S );

// YPix

    addSeparator();

    L = new QLabel( "YPix", this );
    addWidget( L );

    V = new QSpinBox( this );
    V->setObjectName( "ypixsb" );
    V->setToolTip( "Height on screen (all graphs)" );
    V->setMinimum( 4 );
    V->setMaximum( 500 );
    ConnectUI( V, SIGNAL(valueChanged(int)), fv, SLOT(tbSetYPix(int)) );
    addWidget( V );

// YScale

    L = new QLabel( "YScale", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "yscalesb" );
    S->setToolTip( "Y magnifier (sel graph)" );
    S->setRange( 0.0, 100.0 );
    S->setSingleStep( 0.25 );
    ConnectUI( S, SIGNAL(valueChanged(double)), fv, SLOT(tbSetYScale(double)) );
    addWidget( S );

// Gain

    L = new QLabel( "Gain", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "gainsb" );
    S->setToolTip( "Amplifier gain (sel graph)" );
    S->setDecimals( 3 );
    S->setRange( 0.001, 1e6 );
    ConnectUI( S, SIGNAL(valueChanged(double)), fv, SLOT(tbSetMuxGain(double)) );
    addWidget( S );

// NDivs

    addSeparator();

    L = new QLabel( "NDivs", this );
    addWidget( L );

    V = new QSpinBox( this );
    V->setObjectName( "ndivssb" );
    V->setToolTip( "Ruler (all graphs)" );
    V->setMinimum( 0 );
    V->setMaximum( 10 );
    ConnectUI( V, SIGNAL(valueChanged(int)), fv, SLOT(tbSetNDivs(int)) );
    addWidget( V );

    L = new QLabel( " Boxes - x -", this );
    L->setObjectName( "divlbl" );
    addWidget( L );

// Hipass

    addSeparator();

    if( fType == 2 ) {

        C = new QCheckBox( "300 - INF", this );
        C->setObjectName( "hpchk" );
        C->setToolTip( "Neural bandpass (sel graph)" );
        ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbHipassClicked(bool)) );
        addWidget( C );
    }

// -<T> (DC filter)

    C = new QCheckBox( "-<T>", this );
    C->setObjectName( "dcchk" );
    C->setToolTip( "Temporal average (sel graph)" );
    ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbDcClicked(bool)) );
    addWidget( C );

// BinMax

    if( fType != 1 ) {

        C = new QCheckBox( "BinMax", this );
        C->setObjectName( "bmchk" );
        C->setToolTip( "Graph extremum in each downsample bin (sel graph)" );
        ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbBinMaxClicked(bool)) );
        addWidget( C );
    }

// Apply all

    addSeparator();

    addAction(
        QIcon( QPixmap( apply_all_xpm ) ),
        "Apply selected graph settings to all graphs of like type",
        fv, SLOT(tbApplyAll()) );
}


void FVToolbar::setRanges()
{
    QDoubleSpinBox  *XS = findChild<QDoubleSpinBox*>( "xscalesb" );
    QDoubleSpinBox  *YS = findChild<QDoubleSpinBox*>( "yscalesb" );
    QSpinBox        *YP = findChild<QSpinBox*>( "ypixsb" );
    QSpinBox        *ND = findChild<QSpinBox*>( "ndivssb" );

    SignalBlocker
        b0(XS),
        b1(YS),
        b2(YP),
        b3(ND);

    XS->setRange( 0.0001, qMin( 30.0, fv->tbGetfileSecs() ) );
    XS->setValue( fv->tbGetxSpanSecs() );
    YS->setValue( fv->tbGetyScl() );
    YP->setValue( fv->tbGetyPix() );
    ND->setValue( fv->tbGetNDivs() );
}


void FVToolbar::setSortButText( const QString &name )
{
    QPushButton *B = findChild<QPushButton*>( "sortbtn" );

    B->setText( name );
}


void FVToolbar::setSelName( const QString &name )
{
    QAction *A = findChild<QAction*>( "nameact" );

    A->setText( name );
}


void FVToolbar::setXScale( double secs )
{
    QDoubleSpinBox  *XS = findChild<QDoubleSpinBox*>( "xscalesb" );

    SignalBlocker   b0(XS);

    XS->setValue( secs );
}


void FVToolbar::enableYPix( bool enabled )
{
    QSpinBox    *V = findChild<QSpinBox*>( "ypixsb" );

    V->setEnabled( enabled );
}


void FVToolbar::setYSclAndGain( double yScl, double gain, bool enabled )
{
    QDoubleSpinBox  *YS = findChild<QDoubleSpinBox*>( "yscalesb" );
    QDoubleSpinBox  *GN = findChild<QDoubleSpinBox*>( "gainsb" );

    SignalBlocker   b0(YS), b1(GN);

    YS->setValue( yScl );
    GN->setValue( gain );

    YS->setEnabled( enabled );
    GN->setEnabled( enabled );
}


void FVToolbar::setFltChecks(
    bool    hp,
    bool    dc,
    bool    bm,
    bool    enabHP,
    bool    enabDC,
    bool    enabBM )
{
    QCheckBox   *HP = findChild<QCheckBox*>( "hpchk" );
    QCheckBox   *DC = findChild<QCheckBox*>( "dcchk" );
    QCheckBox   *BM = findChild<QCheckBox*>( "bmchk" );

    if( HP ) {
        SignalBlocker   b(HP);
        HP->setChecked( hp );
        HP->setEnabled( enabHP );
    }

    if( BM ) {
        SignalBlocker   b(BM);
        BM->setChecked( bm );
        BM->setEnabled( enabBM );
    }

    SignalBlocker   b(DC);
    DC->setChecked( dc );
    DC->setEnabled( enabDC );
}


void FVToolbar::setNDivText( const QString &s )
{
    QLabel  *L = findChild<QLabel*>( "divlbl" );

    L->setText( s );
}


