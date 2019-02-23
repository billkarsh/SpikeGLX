#ifndef CALSRATE_H
#define CALSRATE_H

#include "DAQ.h"
#include "DFName.h"

#include <QMutex>
#include <QThread>

class DataFile;
class QProgressDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct CalSRStream {
    double  srate,  // from file
            av,
            se;
    QString err;
    int     ip;

    CalSRStream() : srate(0), av(0), se(0), ip(0)           {}
    CalSRStream( int ip ) : srate(0), av(0), se(0), ip(ip)  {}
};


class CalSRWorker : public QObject
{
    Q_OBJECT

private:
    struct Bin {
        QVector<double> C;
        qint64          bmin,
                        bmax;
        int             n;

        // We expect count variation of order +/- 1.
        // True glitches that split a counting window cause
        // large variation: +/- 1000 or more. We set nominal
        // width to +/- 20 to accommodate modest clock drift.
        Bin() : bmin(0), bmax(0), n(0)                  {}
        Bin( qint64 c ) : bmin(c-20), bmax(c+20), n(1)  {C.push_back(c);}
        bool isIn( qint64 c )
        {
            if( c >= bmin && c <= bmax ) {
                C.push_back( c );
                ++n;
                return true;
            }
            return false;
        }
    };

private:
    DFRunTag                &runTag;
    QVector<CalSRStream>    &vIM,
                            &vNI;
    mutable QMutex          runMtx;
    int                     pctCum,
                            pctMax,
                            pctRpt;
    bool                    _cancel;

public:
    CalSRWorker(
        DFRunTag                &runTag,
        QVector<CalSRStream>    &vIM,
        QVector<CalSRStream>    &vNI )
        :   runTag(runTag),
            vIM(vIM), vNI(vNI),
            _cancel(false)  {}
    virtual ~CalSRWorker()  {}

signals:
    void percent( int pct );
    void finished();

public slots:
    void run();
    void cancel()       {QMutexLocker ml( &runMtx ); _cancel=true;}

private:
    bool isCanceled()   {QMutexLocker ml( &runMtx ); return _cancel;}
    void reportTenth( int tenth );
    void calcRateIM( CalSRStream &S );
    void calcRateNI( CalSRStream &S );

    void scanDigital(
        CalSRStream     &S,
        DataFile        *df,
        double          syncPer,
        int             syncChan,
        int             dword );

    void scanAnalog(
        CalSRStream     &S,
        DataFile        *df,
        double          syncPer,
        double          syncThresh,
        int             syncChan );
};


class CalSRThread
{
public:
    QThread     *thread;
    CalSRWorker *worker;

public:
    CalSRThread(
        DFRunTag                &runTag,
        QVector<CalSRStream>    &vIM,
        QVector<CalSRStream>    &vNI );
    virtual ~CalSRThread();

    void startRun();
};


class CalSRRun : public QObject
{
    Q_OBJECT

private:
    DAQ::Params             oldParams;
    double                  runTZero;
    DFRunTag                runTag;
    QVector<CalSRStream>    vIM;
    QVector<CalSRStream>    vNI;
    CalSRThread             *thd;
    QProgressDialog         *prgDlg;

public:
    CalSRRun() : thd(0), prgDlg(0)  {}

    void initRun();
    void initTimer();
    int elapsedMS();

public slots:
    void finish();
    void finish_cleanup();

private:
    void createPrgDlg();
};

#endif  // CALSRATE_H


