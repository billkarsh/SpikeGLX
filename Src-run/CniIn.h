#ifndef CNIIN_H
#define CNIIN_H

#include "NIReader.h"

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Base class for NI-DAQ input
//
class CniIn : public QObject
{
    Q_OBJECT

protected:
    NIReaderWorker  *owner;
    const Params    &p;
    quint64         totalTPts;
    mutable QMutex  runMtx;
    volatile bool   pleaseStop;

public:
    CniIn( NIReaderWorker *owner, const Params &p )
    :   QObject(0), owner(owner), p(p),
        totalTPts(0ULL), pleaseStop(false)  {}

    virtual void run() = 0;

    void stop()         {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped()    {QMutexLocker ml( &runMtx ); return pleaseStop;}
};

#endif  // CNIIN_H


