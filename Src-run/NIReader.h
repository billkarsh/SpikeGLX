#ifndef NIREADER_H
#define NIREADER_H

#include "DAQ.h"
#include "AIQ.h"

#include <QObject>
using namespace DAQ;

class CniIn;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class NIReaderWorker : public QObject
{
    Q_OBJECT

    friend class CniInDmx;
    friend class CniInSim;

private:
    CniIn   *niin;
    AIQ     *niQ;

public:
    NIReaderWorker( const Params &p, AIQ *niQ );
    virtual ~NIReaderWorker();

    void stop();

signals:
    void runStarted();
    void daqError( const QString &s );
    void finished();

public slots:
    void run();
};


class NIReader
{
public:
    QThread         *thread;
    NIReaderWorker  *worker;

public:
    NIReader( const Params &p, AIQ *niQ );
    virtual ~NIReader();

    void start();
};

#endif  // NIREADER_H


