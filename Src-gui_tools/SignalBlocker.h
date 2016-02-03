#ifndef SIGNALBLOCKER_H
#define SIGNALBLOCKER_H

#include <QObject>

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SignalBlocker
{
    QObject *obj;
    bool    wasBlocked;

    SignalBlocker( QObject *obj ) : obj(obj)
    {
        wasBlocked = obj->signalsBlocked();
        obj->blockSignals( true );
    }

    virtual ~SignalBlocker() {obj->blockSignals(wasBlocked);}
};

#endif  // SIGNALBLOCKER_H


