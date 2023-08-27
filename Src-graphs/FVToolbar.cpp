
#include "Pixmaps/shanks.xpm"
#include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "FileViewerWindow.h"
#include "FVToolbar.h"
#include "SignalBlocker.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>




FVToolbar::FVToolbar( FileViewerWindow *fv, int fType ) : fv(fv)
{
    QDoubleSpinBox  *S;
    QSpinBox        *V;
    QPushButton     *B;
    QCheckBox       *C;
    QComboBox       *CB;
    QAction         *A;
    QLabel          *L;

    toggleViewAction()->setEnabled( false );    // can't hide toolbar

// Sort selector

    B = new QPushButton( this );
    B->setObjectName( "sortbtn" );
    B->setToolTip( "Toggle graph sort order: user/acquired" );
    ConnectUI( B, SIGNAL(clicked()), fv, SLOT(tbToggleSort()) );
    addWidget( B );

// Shank map

    if( fType != fvOB ) {

        addAction(
            QIcon( QPixmap( shanks_xpm ) ),
            "Show graphical shank map",
            fv, SLOT(tbShowShanks()) );
    }

// Selected

    A = new QAction( "", this );
    A->setObjectName( "nameact" );
    A->setToolTip( "Selected graph (click to find)" );
    A->setFont( QFont( QApplication::font().family(), 10, QFont::DemiBold ) );
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
    S->setRange( 0.0001, qMin( 30.0, fv->tbGetfileSecs() ) );
    S->setSingleStep( 0.25 );
    S->setValue( fv->tbGetxSpanSecs() );
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
    V->setValue( fv->tbGetyPix() );
    ConnectUI( V, SIGNAL(valueChanged(int)), fv, SLOT(tbSetYPix(int)) );
    addWidget( V );

// YScale

    L = new QLabel( "YScale", this );
    addWidget( L );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "yscalesb" );
    S->setToolTip( "Y magnifier (sel graph)" );
    S->setRange( 0.0, 999.0 );
    S->setSingleStep( 0.25 );
    S->setValue( fv->tbGetyScl() );
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
    V->setToolTip( "Ruler (all graphs)" );
    V->setMinimum( 1 );
    V->setMaximum( 10 );
    V->setValue( fv->tbGetNDivs() );
    ConnectUI( V, SIGNAL(valueChanged(int)), fv, SLOT(tbSetNDivs(int)) );
    addWidget( V );

    L = new QLabel( " Boxes - x -", this );
    L->setObjectName( "divlbl" );
    addWidget( L );

// Neural

    if( fv->tbGetNeurChans() ) {

        // Hipass

        addSeparator();

        if( fType != fvLF ) {

            C = new QCheckBox( "300 - INF", this );
            C->setToolTip( "Applied only to neural channels" );
            C->setChecked( fv->tbGet300HzOn() );
            ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbHipassClicked(bool)) );
            addWidget( C );
        }

        // -<Tn> (DC filter)

        C = new QCheckBox( "-<Tn>", this );
        C->setToolTip( "Temporally average neural channels" );
        C->setChecked( fv->tbGetTnChkOn() );
        ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbTnClicked(bool)) );
        addWidget( C );

        // -<S> (spatial average)

        if( fType != fvLF ) {

            L = new QLabel( "-<S>", this );
            L->setTextFormat( Qt::PlainText );
            L->setToolTip( "Spatially average spike channels" );
            L->setStyleSheet( "padding-bottom: 1px" );
            addWidget( L );

            CB = new QComboBox( this );
            CB->setToolTip( "Spatially average spike channels" );
            CB->addItem( "Off" );
            fv->tbNameLocalFilters( CB );
            CB->addItem( "Glb All" );
            CB->addItem( "Glb Dmx" );
            CB->setCurrentIndex( fv->tbGetSAveSel() );
            ConnectUI( CB, SIGNAL(currentIndexChanged(int)), fv, SLOT(tbSAveSelChanged(int)) );
            addWidget( CB );
        }

        // BinMax

        if( fType != fvLF ) {

            L = new QLabel( "BinMax", this );
            L->setTextFormat( Qt::PlainText );
            L->setToolTip( "Draw peaks of each downsample bin" );
            L->setStyleSheet( "padding-bottom: 1px" );
            addWidget( L );

            CB = new QComboBox( this );
            CB->setToolTip( "Draw peaks of each downsample bin" );
            CB->addItem( "Off" );
            CB->addItem( "Slow" );
            CB->addItem( "Fast" );
            CB->addItem( "Faster" );
            CB->setCurrentIndex( fv->tbGetBinMax() );
            ConnectUI( CB, SIGNAL(currentIndexChanged(int)), fv, SLOT(tbBinMaxChanged(int)) );
            addWidget( CB );
        }
    }

// Aux analog

    if( fv->tbGetAnaChans() > fv->tbGetNeurChans() ) {

        // -<Tn> (DC filter)

        addSeparator();

        C = new QCheckBox( "-<Tx>", this );
        C->setToolTip( "Temporally average auxiliary analog channels" );
        C->setChecked( fv->tbGetTxChkOn() );
        ConnectUI( C, SIGNAL(clicked(bool)), fv, SLOT(tbTxClicked(bool)) );
        addWidget( C );
    }

// Apply all

    addSeparator();

    addAction(
        QIcon( QPixmap( apply_all_xpm ) ),
        "Apply selected graph settings to all graphs of like type",
        fv, SLOT(tbApplyAll()) );
}


void FVToolbar::disableSorting()
{
    QPushButton *B = findChild<QPushButton*>( "sortbtn" );

    B->setEnabled( false );
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


void FVToolbar::setNDivText( const QString &s )
{
    QLabel  *L = findChild<QLabel*>( "divlbl" );

    L->setText( s );
}


