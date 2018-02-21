#ifndef GWLEDWIDGET_H
#define GWLEDWIDGET_H

#include "QLED.h"

#include <QMutex>

namespace DAQ {
struct Params;
}

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWLEDWidget : public QWidget
{
    Q_OBJECT

private:
    mutable QMutex  LEDMtx;

public:
    GWLEDWidget( const DAQ::Params &p );

    void setOnColor( QLED::ledColor color );
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();
};

#endif  // GWLEDWIDGET_H


