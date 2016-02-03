
#include "DAQ.h"
#include "GWLEDWidget.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>




GWLEDWidget::GWLEDWidget( DAQ::Params &p )
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
    LED->setOnColor( p.mode.trgInitiallyOff ? QLED::Yellow : QLED::Green );
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
    setTriggerLED( true );
    QTimer::singleShot( 50, this, SLOT(blinkTrigger_Off()) );
}


void GWLEDWidget::blinkTrigger_Off()
{
    setTriggerLED( false );
}


