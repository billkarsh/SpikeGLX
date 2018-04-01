
#include "GWLEDWidget.h"
#include "DAQ.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QThread>




GWLEDWidget::GWLEDWidget( const DAQ::Params &p )
{
    QMutexLocker    ml( &LEDMtx );
    QHBoxLayout     *HBX = new QHBoxLayout;
    QLabel          *LBL;
    QLED            *LED;

// gate LED label

    LBL = new QLabel( "Gate:" );
    HBX->addWidget( LBL );

// gate LED

    LED = new QLED;
    LED->setObjectName( "gateLED" );
    LED->setOffColor( QLED::Red );
    LED->setOnColor( QLED::Green );
    LED->setMinimumSize( 20, 20 );
    HBX->addWidget( LED );

// trigger LED label

    LBL = new QLabel( "Trigger:" );
    HBX->addWidget( LBL );

// trigger LED

    LED = new QLED;
    LED->setObjectName( "trigLED" );
    LED->setOffColor( QLED::Red );
    LED->setOnColor( p.mode.manOvInitOff ? QLED::Yellow : QLED::Green );
    LED->setMinimumSize( 20, 20 );
    HBX->addWidget( LED );

// insert LEDs into widget

    setLayout( HBX );
}


void GWLEDWidget::setOnColor( QLED::ledColor color )
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = findChild<QLED*>( "trigLED" );

    led->setOnColor( color );
}


void GWLEDWidget::setGateLED( bool on )
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = findChild<QLED*>( "gateLED" );

    led->setValue( on );
}


void GWLEDWidget::setTriggerLED( bool on )
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = findChild<QLED*>( "trigLED" );

    led->setValue( on );
}


void GWLEDWidget::blinkTrigger()
{
    QMutexLocker    ml( &LEDMtx );
    QLED            *led = findChild<QLED*>( "trigLED" );

    led->setValueNow( true );
    QThread::msleep( 50 );
    led->setValueNow( false );
}


