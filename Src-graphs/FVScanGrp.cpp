
#include "Util.h"
#include "FileViewerWindow.h"
#include "FVScanGrp.h"
#include "DataFile.h"
#include "SignalBlocker.h"

#include <QDoubleSpinBox>
#include <QLabel>
#include <QSlider>
#include <QHBoxLayout>




FVScanGrp::FVScanGrp( FileViewerWindow *fv )
    : fv(fv), pos(0), pscale(1)
{
    QDoubleSpinBox  *S;
    QLabel          *L;

    QHBoxLayout *HL = new QHBoxLayout( this );

// 'File position'

    L = new QLabel( "File position: ", this );
    HL->addWidget( L );

// pos (scans)

    L = new QLabel( "scans", this );
    HL->addWidget( L, 0, Qt::AlignRight );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "possb" );
    S->setDecimals( 0 );
    S->setSingleStep( 100.0 );
    ConnectUI( S, SIGNAL(valueChanged(double)), this, SLOT(posSBChanged(double)) );
    HL->addWidget( S, 0, Qt::AlignLeft );

    L = new QLabel( "to X (of Y)", this );
    L->setObjectName( "poslbl" );
    HL->addWidget( L, 0, Qt::AlignLeft );

// secs

    HL->addSpacing( 5 );

    L = new QLabel( "secs", this );
    HL->addWidget( L, 0, Qt::AlignRight );

    S = new QDoubleSpinBox( this );
    S->setObjectName( "secsb" );
    S->setDecimals( 3 );
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
    HL->addWidget( slider );
}


void FVScanGrp::setRanges( bool newFile )
{
    if( newFile ) {
        pos     = 0;
        pscale  = 1;
    }

    QDoubleSpinBox  *PS = findChild<QDoubleSpinBox*>( "possb" );
    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );
    QSlider         *SR = findChild<QSlider*>( "slider" );

    {
        SignalBlocker   b0(PS), b1(SC), b2(SR);

        qint64  maxVal = maxPos();

        // Calculate slider scale factor

        while( maxVal / pscale > qint64(INT_MAX) )
            pscale = (pscale == qint64(1) ? qint64(2) : pscale*pscale);

        // Ranges

        PS->setMinimum( 0 );
        PS->setMaximum( maxVal );
        SC->setMinimum( 0 );
        SC->setMaximum( timeFromPos( maxVal ) );
        SR->setMaximum( maxVal / pscale );

        // Values

        PS->setValue( pos );
        SC->setValue( getTime() );
        SR->setValue( pos / pscale );
    }

    updateTexts();
}


double FVScanGrp::timeFromPos( qint64 p ) const
{
    return p / fv->df->samplingRateHz();
}


qint64 FVScanGrp::posFromTime( double s ) const
{
    return fv->df->samplingRateHz() * s;
}


qint64 FVScanGrp::maxPos() const
{
    return qMax( 0LL, fv->dfCount - fv->nScansPerGraph() - 1 );
}


void FVScanGrp::setFilePos64( qint64 newPos )
{
    pos = qBound( 0LL, newPos, maxPos() );

    fv->updateGraphs();
}


void FVScanGrp::guiSetPos( qint64 newPos )
{
    QDoubleSpinBox  *PS = findChild<QDoubleSpinBox*>( "possb" );

    PS->setValue( newPos );
}


void FVScanGrp::posSBChanged( double p )
{
    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );
    QSlider         *SR = findChild<QSlider*>( "slider" );

    setFilePos64( p );

    {
        SignalBlocker   b0(SC), b1(SR);

        SC->setValue( getTime() );
        SR->setValue( pos / pscale );
    }

    updateTexts();
}


void FVScanGrp::secSBChanged( double s )
{
    QDoubleSpinBox  *PS = findChild<QDoubleSpinBox*>( "possb" );
    QSlider         *SR = findChild<QSlider*>( "slider" );

    setFilePos64( posFromTime( s ) );

    {
        SignalBlocker   b0(PS), b1(SR);

        PS->setValue( pos );
        SR->setValue( pos / pscale );
    }

    updateTexts();
}


void FVScanGrp::sliderChanged( int i )
{
    QDoubleSpinBox  *PS = findChild<QDoubleSpinBox*>( "possb" );
    QDoubleSpinBox  *SC = findChild<QDoubleSpinBox*>( "secsb" );

    setFilePos64( i * pscale );

    {
        SignalBlocker   b0(PS), b1(SC);

        PS->setValue( pos );
        SC->setValue( getTime() );
    }

    updateTexts();
}


void FVScanGrp::updateTexts()
{
    QLabel  *PL     = findChild<QLabel*>( "poslbl" );
    QLabel  *SL     = findChild<QLabel*>( "seclbl" );
    qint64  dfMax   = fv->dfCount - 1,
            last    = qMin( dfMax, pos + fv->nScansPerGraph() );
    int     fldW    = QString::number( dfMax ).size();

    PL->setText(
        QString("to %1 (of %2)")
        .arg( last, fldW, 10, QChar('0') )
        .arg( dfMax ) );

    SL->setText(
        QString("to %1 (of %2)")
        .arg( timeFromPos( last ), 0, 'f', 3 )
        .arg( timeFromPos( dfMax ), 0, 'f', 3 ) );
}


