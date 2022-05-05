
#include "Util.h"
#include "FileViewerWindow.h"
#include "FVScanGrp.h"
#include "DataFile.h"
#include "SignalBlocker.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QHBoxLayout>




FVScanGrp::FVScanGrp( FileViewerWindow *fv )
    :   fv(fv), pos(0), pscale(1)
{
    QDoubleSpinBox  *S;
    QPushButton     *B;
    QLabel          *L;

    QHBoxLayout *HL = new QHBoxLayout( this );

// 'File position'

    L = new QLabel( "File position: secs", this );
    HL->addWidget( L );

// secs

    S = new QDoubleSpinBox( this );
    S->setObjectName( "secsb" );
    S->setDecimals( 4 );
    S->setSingleStep( 0.01 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(secSBChanged(double)) );
    HL->addWidget( S, 0, Qt::AlignLeft );

    L = new QLabel( "to X (of Y)", this );
    L->setObjectName( "seclbl" );
    HL->addWidget( L, 0, Qt::AlignLeft );

// slider

    HL->addSpacing( 5 );

    slider = new QSlider( Qt::Horizontal, this );
    slider->setObjectName( "slider" );
    slider->setMinimum( 0 );
    slider->setMaximum( 1000 );
    slider->installEventFilter( fv );
    ConnectUI( slider, SIGNAL(valueChanged(int)), this, SLOT(sliderChanged(int)) );
    ConnectUI( slider, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()) );
    HL->addWidget( slider );

// Manual updater

    B = new QPushButton( "Update", this );
    B->setObjectName( "manupbtn" );
    B->setToolTip( "Redraw graphs now" );
    ConnectUI( B, SIGNAL(clicked()), this, SLOT(manualUpdateClicked()) );
    HL->addWidget( B );
}


void FVScanGrp::setRanges( bool newFile )
{
    if( newFile ) {
        pos     = 0;
        pscale  = 1;
    }

    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );
    QSlider         *SR = findChild<QSlider*>( "slider" );

    {
        SignalBlocker   b0(SC), b1(SR);

        qint64  maxVal = maxPos();

        // Calculate slider scale factor

        while( maxVal / pscale > qint64(INT_MAX) )
            pscale = (pscale == qint64(1) ? qint64(2) : pscale*pscale);

        // Ranges

        SC->setMinimum( 0 );
        SC->setMaximum( timeFromPos( maxVal ) );
        SR->setMaximum( maxVal / pscale );

        // Values

        SC->setValue( curTime() );
        SR->setValue( pos / pscale );
    }

    updateText();
}


double FVScanGrp::timeFromPos( qint64 p ) const
{
    return p / fv->df->samplingRateHz();
}


qint64 FVScanGrp::posFromTime( double s ) const
{
    return qRound64( fv->df->samplingRateHz() * s );
}


qint64 FVScanGrp::maxPos() const
{
    return qMax( 0LL, fv->dfCount - fv->nScansPerGraph() - 1 );
}


void FVScanGrp::setFilePos64( qint64 newPos )
{
    pos = qBound( 0LL, newPos, maxPos() );

    if( !fv->sav.all.manualUpdate ) {

        if( fv->isActiveWindow() )
            fv->linkSendPos( 1 );

        fv->updateGraphs();
    }
}


// Always return true as a convenience.
//
bool FVScanGrp::guiSetPos( qint64 newPos )
{
    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );

    double  oldVal = SC->value();

    SC->setValue( timeFromPos( newPos ) );

// Always want to redraw, even if value unchanged

    if( SC->value() == oldVal )
        secSBChanged( timeFromPos( newPos ) );

    return true;
}


void FVScanGrp::enableManualUpdate( bool enable )
{
    findChild<QPushButton*>( "manupbtn" )->setEnabled( enable );
}


void FVScanGrp::secSBChanged( double s )
{
    QSlider *SR = findChild<QSlider*>( "slider" );

    setFilePos64( posFromTime( s ) );

    {
        SignalBlocker   b0(SR);

        SR->setValue( pos / pscale );
    }

    updateText();
}


void FVScanGrp::sliderChanged( int i )
{
    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );

    setFilePos64( i * pscale );

    {
        SignalBlocker   b0(SC);

        SC->setValue( curTime() );
    }

    updateText();
}


void FVScanGrp::sliderReleased()
{
    QSlider *SR = findChild<QSlider*>( "slider" );
    sliderChanged( SR->value() );
}


void FVScanGrp::manualUpdateClicked()
{
    if( fv->sav.all.manualUpdate ) {

        if( fv->isActiveWindow() ) {

            // On manual update push changes to linked windows

            fv->linkSendManualUpdate( false );
            guiBreathe();
            fv->linkSendPos( 1 );
            guiBreathe();
            fv->linkSendManualUpdate( true );
        }

        fv->updateGraphs();
    }
}


void FVScanGrp::updateText()
{
    QLabel  *SL     = findChild<QLabel*>( "seclbl" );
    qint64  dfMax   = fv->dfCount - 1,
            last    = qMin( dfMax, pos + fv->nScansPerGraph() );

    SL->setText(
        QString("to %1 (of %2)")
        .arg( timeFromPos( last ), 0, 'f', 4 )
        .arg( timeFromPos( dfMax ), 0, 'f', 3 ) );
}


