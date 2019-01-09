
#include "MXLEDWidget.h"

#include <QHBoxLayout>
#include <QLabel>




MXLEDWidget::MXLEDWidget( QWidget *parent )
    :   QWidget(parent)
{
    QHBoxLayout *HBX = new QHBoxLayout;
    QLabel      *LBL;
    QLED        *LED;

// green

    LBL = new QLabel( "Normal:" );
    HBX->addWidget( LBL );

    LED = new QLED( ":/QLEDResources" );
    LED->setObjectName( "LED0" );
    LED->setOffColor( QLED::Grey );
    LED->setOnColor( QLED::Green );
    LED->setMinimumSize( 40, 30 );
    HBX->addWidget( LED );

// yellow

    LBL = new QLabel( "Stressed:" );
    HBX->addWidget( LBL );

    LED = new QLED( ":/QLEDResources" );
    LED->setObjectName( "LED1" );
    LED->setOffColor( QLED::Grey );
    LED->setOnColor( QLED::Yellow );
    LED->setMinimumSize( 40, 30 );
    HBX->addWidget( LED );

// red

    LBL = new QLabel( "Failing:" );
    HBX->addWidget( LBL );

    LED = new QLED( ":/QLEDResources" );
    LED->setObjectName( "LED2" );
    LED->setOffColor( QLED::Grey );
    LED->setOnColor( QLED::Red );
    LED->setMinimumSize( 40, 30 );
    HBX->addWidget( LED );

// insert items into widget

    setLayout( HBX );

    setState( -1 );
}


void MXLEDWidget::setState( int state )
{
    for( int i = 0; i < 3; ++i ) {

        QLED    *led = findChild<QLED*>( QString("LED%1").arg( i ) );

        led->setValue( state == i );
    }
}


