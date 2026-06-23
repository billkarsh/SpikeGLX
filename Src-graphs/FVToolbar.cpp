
#include "Pixmaps/shanks.xpm"
// #include "Pixmaps/apply_all.xpm"

#include "Util.h"
#include "FileViewerWindow.h"
#include "FVToolbar.h"
#include "SignalBlocker.h"
#include "ToolBut.h"

#include <QApplication>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>


#define LVBUT_STYLE     "font-size: 20px;"
#define LVLBL_STYLE     "font-size: 14px; color: #444444;"
#define LVVAL_STYLE     "color: #0078D7; font-weight: bold;"


FVToolbar::FVToolbar( FileViewerWindow *fv, int fType ) : fv(fv)
{
    LVBut           *LV;
    LVE_chk         *LVC;
    LVE_int         *LVI;
    LVE_dbl         *LVD;
    LVE_cb          *LVB;
    QPushButton     *B;
    QAction         *A;

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

    LVD = new LVE_dbl(
                "Set window time span in seconds.\n"
                "Tip: Scan faster with short span.",
                QString("%1").arg( fv->tbGetxSpanSecs() ), this );

    LVD->m_spinBox->setDecimals( 3 );
    LVD->m_spinBox->setRange( .001, qMin( 30.0, fv->tbGetfileSecs() ) );
    LVD->m_spinBox->setSingleStep( 0.25 );
    ConnectUI( LVD->m_spinBox, SIGNAL(valueChanged(double)), fv, SLOT(tbSetXScale(double)) );

    LV = new LVBut(
            "X", QString("%1").arg( fv->tbGetxSpanSecs() ),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVD, this );
    LV->setObjectName( "xspin" );
    addWidget( LV );

// YPix

    LVI = new LVE_int(
                "Set graph height in pixels.",
                QString("%1").arg( fv->tbGetyPix() ), this );

    LVI->m_spinBox->setMinimum( 4 );
    LVI->m_spinBox->setMaximum( 500 );
    ConnectUI( LVI->m_spinBox, SIGNAL(valueChanged(int)), fv, SLOT(tbSetYPix(int)) );

    LV = new LVBut(
            "Ypix", QString("%1").arg( fv->tbGetyPix() ),
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVI, this );
    LV->setObjectName( "ypix" );
    addWidget( LV );

// YScale

    LVD = new LVE_dbl(
                "Set vertical magnification factor.",
                "1", this );

    LVD->m_spinBox->setRange( 0.0, 999.0 );
    LVD->m_spinBox->setSingleStep( 0.25 );
    ConnectUI( LVD->m_spinBox, SIGNAL(valueChanged(double)), fv, SLOT(tbSetYScale(double)) );

    LV = new LVBut(
            "Ymag", "1",
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVD, this );
    LV->setObjectName( "ymag" );
    addWidget( LV );

// Gain

    LVD = new LVE_dbl(
                "Amplifier gain (read-only).",
                "1", this );

    LVD->m_spinBox->setEnabled( false );
    LVD->m_spinBox->setDecimals( 3 );
    LVD->m_spinBox->setRange( 0.001, 1e6 );
    ConnectUI( LVD->m_spinBox, SIGNAL(valueChanged(double)), fv, SLOT(tbSetMuxGain(double)) );

    LV = new LVBut(
            "Gn", "1",
            LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
            LVD, this );
    LV->setObjectName( "gain" );
    addWidget( LV );

// NDivs
// Retired v 20260115

#if 0
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
#endif

// Neural

    if( fv->tbGetNeurChans() ) {

        addSeparator();

        // Bandpass

        if( fType != fvLF ) {

            if( fType == fvAP ) {
                LVB = new LVE_cb(
                            "Band-pass filter applied to AP-band channels.",
                            this );
                LVB->m_comboBox->addItem( "Off" );
                LVB->m_comboBox->addItem( "300-INF" );
                LVB->m_comboBox->addItem( "0.5-500" );
            }
            else {
                LVB = new LVE_cb(
                            "Band-pass filter applied to neural channels.",
                            this );
                LVB->m_comboBox->addItem( "Off" );
                LVB->m_comboBox->addItem( "300-INF" );
                LVB->m_comboBox->addItem( "0.1-300" );
            }
            LVB->m_comboBox->setCurrentIndex( fv->tbGetBandSel() );
            LVB->setValue( LVB->m_comboBox->currentText() );
            ConnectUI( LVB->m_comboBox, SIGNAL(currentIndexChanged(int)), fv, SLOT(tbBandSelChanged(int)) );

            LV = new LVBut(
                    (fType == fvAP ? "AP" : "BP"), LVB->m_comboBox->currentText(),
                    LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                    LVB, this );
            addWidget( LV );
        }

        // -<Tn> (DC filter)

        LVC = new LVE_chk(
                    "DCneural",
                    "Subtract window-averaged DC level of neural channels.",
                    (fv->tbGetTnChkOn() ? "1" : "0"), this );
        ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), fv, SLOT(tbTnClicked(bool)) );

        LV = new LVBut(
                "DCn", (fv->tbGetTnChkOn() ? "1" : "0"),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVC, this );
        addWidget( LV );

        // -<S> (spatial average)

        if( fType != fvLF ) {

            LVB = new LVE_cb(
                        "Spatially average AP-band channels"
                        " and subtract common signal.\n"
                        " - Loc I,O\t= local donut {in,out} radii (sites)\n"
                        " - Glb All\t= average all channels\n"
                        " - Gbl Dmx\t= ave chans with shared multiplex phase",
                        this );
            LVB->m_comboBox->addItem( "Off" );
            fv->tbNameLocalFilters( LVB->m_comboBox );
            LVB->m_comboBox->addItem( "Glb All" );
            LVB->m_comboBox->addItem( "Glb Dmx" );
            LVB->m_comboBox->setCurrentIndex( fv->tbGetSAveSel() );
            LVB->setValue( LVB->m_comboBox->currentText() );
            ConnectUI( LVB->m_comboBox, SIGNAL(currentIndexChanged(int)), fv, SLOT(tbSAveSelChanged(int)) );

            LV = new LVBut(
                    "CAR", LVB->m_comboBox->currentText(),
                    LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                    LVB, this );
            addWidget( LV );
        }
    }

// Aux analog

    if( fv->tbGetAnaChans() > fv->tbGetNeurChans() ) {

        // -<Tx> (DC filter)

        LVC = new LVE_chk(
                    "DCaux",
                    "Subtract window-averaged DC level of AUX channels.",
                    (fv->tbGetTxChkOn() ? "1" : "0"), this );
        ConnectUI( LVC->m_checkBox, SIGNAL(clicked(bool)), fv, SLOT(tbTxClicked(bool)) );

        LV = new LVBut(
                "DCx", (fv->tbGetTxChkOn() ? "1" : "0"),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVC, this );
        addWidget( LV );
    }

// BinMax

    if( fType != fvLF ) {

        LVB = new LVE_cb(
                    "Draw extreme values in each downsample bin;"
                    " helps catch rare signals.\n"
                    " - 0 = Off\n"
                    " - 1 = Slow\t: Examine every sample\n"
                    " - 2 = Fast\t: Examine every 2nd sample\n"
                    " - 3 = Faster\t: Examine every 3rd sample",
                    this );
        LVB->m_comboBox->addItem( "0" );
        LVB->m_comboBox->addItem( "1" );
        LVB->m_comboBox->addItem( "2" );
        LVB->m_comboBox->addItem( "3" );
        LVB->m_comboBox->setCurrentIndex( fv->tbGetBinMax() );
        LVB->setValue( LVB->m_comboBox->currentText() );
        ConnectUI( LVB->m_comboBox, SIGNAL(currentIndexChanged(int)), fv, SLOT(tbBinMaxChanged(int)) );

        LV = new LVBut(
                "BM", LVB->m_comboBox->currentText(),
                LVBUT_STYLE, LVLBL_STYLE, LVVAL_STYLE,
                LVB, this );
        addWidget( LV );
    }

// Apply all
// Retired October 2025 v 20250930

#if 0
    addSeparator();

    addAction(
        QIcon( QPixmap( apply_all_xpm ) ),
        "Apply selected graph settings to all graphs of like type",
        fv, SLOT(tbApplyAll()) );
#endif
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
    LVBut   *xspin = findChild<LVBut*>( "xspin" );

    SignalBlocker   b0(xspin);

    xspin->setValue( QString("%1").arg( secs ) );
}


void FVToolbar::enableYPix( bool enabled )
{
    LVBut   *V = findChild<LVBut*>( "ypix" );

    V->setEnabled( enabled );
}


void FVToolbar::setYSclAndGain( double yScl, double gain, bool enabled )
{
    LVBut   *YS = findChild<LVBut*>( "ymag" );
    LVBut   *GN = findChild<LVBut*>( "gain" );

    SignalBlocker   b0(YS), b1(GN);

    YS->setValue( QString("%1").arg( yScl ) );
    GN->setValue( QString("%1").arg( gain ) );

    YS->setEnabled( enabled );
//    GN->setEnabled( enabled );
}


void FVToolbar::setNDivText( const QString &s )
{
    QLabel  *L = findChild<QLabel*>( "divlbl" );

    if( L )
        L->setText( s );
}


