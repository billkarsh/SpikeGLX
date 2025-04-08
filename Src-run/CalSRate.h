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
    int     js,
            ip;

    CalSRStream()
        :   srate(0), av(0), se(0), js(0), ip(0)    {}
    CalSRStream( int js, int ip )
        :   srate(0), av(0), se(0), js(js), ip(ip)  {}
    QString result() const;
private:
    QString remark() const;
};


class CalSRWorker : public QObject
{
    Q_OBJECT

private:
    struct Bin {
        std::vector<double> C;
        qint64              bmin,
                            bmax;
        int                 n;

        // We expect count variation of order +/- 1.
        // True glitches that split a counting window cause
        // large variation: +/- 1000 or more. We set nominal
        // width to +/- 20/hr to accommodate modest clock drift.
        //
        // Derivation:
        // Tolerable variation (as measured) for 30KHz probe over 1 hour
        // is ~0.05Hz = ~200 samples, which is 20 samples per bin. But we
        // should be more strict at lower sample rates, so we scale 20
        // samples linearly w.r.t sample rate: 20*srate/30000. Further,
        // the error grows linearly with time, but should be no smaller
        // than 20. Therefore, we use: 20 (up to 1 hr) + 20/hr thereafter.
        //
        Bin() : bmin(0), bmax(0), n(0)  {}
        Bin( qint64 c, quint64 nCts, double srate ) : n(1)
        {
            int mrg = (20*srate/30000.0)*(1.0 + (nCts/srate/3600.0 - 1.0));
            bmin = c - mrg;
            bmax = c + mrg;
            C.push_back( c );
        }
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
    DFRunTag                    &runTag;
    std::vector<CalSRStream>    &vIM,
                                &vOB,
                                &vNI;
    mutable QMutex              runMtx;
    int                         pctCum,
                                pctMax,
                                pctRpt;
    bool                        _cancel;

public:
    CalSRWorker(
        DFRunTag                    &runTag,
        std::vector<CalSRStream>    &vIM,
        std::vector<CalSRStream>    &vOB,
        std::vector<CalSRStream>    &vNI )
        :   QObject(0), runTag(runTag),
            vIM(vIM), vOB(vOB), vNI(vNI),
            _cancel(false)  {}

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
    void calcRateOB( CalSRStream &S );
    void calcRateNI( CalSRStream &S );

    void scanDigital(
        CalSRStream     &S,
        DataFile        *df,
        double          syncPer,
        int             syncBit,
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
        DFRunTag                    &runTag,
        std::vector<CalSRStream>    &vIM,
        std::vector<CalSRStream>    &vOB,
        std::vector<CalSRStream>    &vNI );
    virtual ~CalSRThread();

    void startRun();
};


class CalSRRun : public QObject
{
    Q_OBJECT

private:
    DAQ::Params                 oldParams;
    double                      runTZero;
    DFRunTag                    runTag;
    std::vector<CalSRStream>    vIM;
    std::vector<CalSRStream>    vOB;
    std::vector<CalSRStream>    vNI;
    CalSRThread                 *thd;
    QProgressDialog             *prgDlg;

public:
    CalSRRun() : QObject(0), thd(0), prgDlg(0)  {}

    void initRun();
    void initTimer();
    int elapsedMS();

    static void fmtResults(
        QString                         &msg,
        const std::vector<CalSRStream>  vIM,
        const std::vector<CalSRStream>  vOB,
        const std::vector<CalSRStream>  vNI );

public slots:
    void finish();
    void finish_cleanup();

private:
    void createPrgDlg();
};

#endif  // CALSRATE_H


