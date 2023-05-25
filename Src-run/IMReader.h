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
    friend class ImAcqWorker;
    friend class CimAcqSim;

private:
    CimAcq          *imAcq;
    QVector<AIQ*>   &imQ,
                    &imQf,
                    &obQ;

public:
    IMReaderWorker(
        const DAQ::Params   &p,
        QVector<AIQ*>       &imQ,
        QVector<AIQ*>       &imQf,
        QVector<AIQ*>       &obQ );
    virtual ~IMReaderWorker();

    int nAIQ( int js ) const
        {return (js == jsIM ? imQ.size() : obQ.size());}

    const AIQ* getAIQ( int js, int ip ) const
        {return (js == jsIM ? imQ[ip] : obQ[ip]);}

    bool isReady() const;
    void start();
    void stayAwake();
    void wake()                         {start();}
    void update( int ip );
    QString opto_getAttens( int ip, int color );
    QString opto_emit( int ip, int color, int site );
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
    IMReader(
        const DAQ::Params   &p,
        QVector<AIQ*>       &imQ,
        QVector<AIQ*>       &imQf,
        QVector<AIQ*>       &obQ );
    virtual ~IMReader();

    void configure();
};

#endif  // IMREADER_H


