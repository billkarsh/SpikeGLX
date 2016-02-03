#ifndef GWLEDWIDGET_H
#define GWLEDWIDGET_H

#include "QLED.h"

#include <QWidget>
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
    GWLEDWidget( DAQ::Params &p );

    void setOnColor( QLED::ledColor color );
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();

private slots:
    void blinkTrigger_Off();
};

#endif  // GWLEDWIDGET_H


