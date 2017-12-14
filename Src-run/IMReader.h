#ifndef IMREADER_H
#define IMREADER_H

#include "DAQ.h"
#include "AIQ.h"

#include <QObject>

class CimAcq;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class IMReaderWorker : public QObject
{
    Q_OBJECT

    friend class CimAcqImec;
    friend class CimAcqSim;

private:
    CimAcq  *imAcq;
    AIQ     *imQ;

public:
    IMReaderWorker( const DAQ::Params &p, AIQ *imQ );
    virtual ~IMReaderWorker();

    AIQ* getAIQ() const     {return imQ;}

    bool isReady() const;
    void start();
    void stayAwake();
    void wake()             {start();}
    void update();
    void stop();

signals:
    void daqError( const QString &s );
    void finished();

public slots:
    void run();
};


class IMReader
{
public:
    QThread         *thread;
    IMReaderWorker  *worker;

public:
    IMReader( const DAQ::Params &p, AIQ *imQ );
    virtual ~IMReader();

    void configure();
};

#endif  // IMREADER_H


