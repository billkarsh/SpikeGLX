#ifndef SVYPRB_H
#define SVYPRB_H

#include "DAQ.h"
#include "DFName.h"

#include <QMutex>
#include <QThread>

class DataFile;
class QProgressDialog;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SvyPrbStream {
    double  srate,  // from file
            av,
            se;
    QString err;
    int     ip;

    SvyPrbStream() : srate(0), av(0), se(0), ip(0)          {}
    SvyPrbStream( int ip ) : srate(0), av(0), se(0), ip(ip) {}
};


class SvyPrbWorker : public QObject
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
    DFRunTag                    &runTag;
    std::vector<SvyPrbStream>   &vIM,
                                &vOB,
                                &vNI;
    mutable QMutex              runMtx;
    int                         pctCum,
                                pctMax,
                                pctRpt;
    bool                        _cancel;

public:
    SvyPrbWorker(
        DFRunTag                    &runTag,
        std::vector<SvyPrbStream>   &vIM,
        std::vector<SvyPrbStream>   &vOB,
        std::vector<SvyPrbStream>   &vNI )
        :   runTag(runTag),
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
    void calcRateIM( SvyPrbStream &S );
    void calcRateOB( SvyPrbStream &S );
    void calcRateNI( SvyPrbStream &S );

    void scanDigital(
        SvyPrbStream    &S,
        DataFile        *df,
        double          syncPer,
        int             syncBit,
        int             dword );

    void scanAnalog(
        SvyPrbStream    &S,
        DataFile        *df,
        double          syncPer,
        double          syncThresh,
        int             syncChan );
};


class SvyPrbThread
{
public:
    QThread         *thread;
    SvyPrbWorker    *worker;

public:
    SvyPrbThread(
        DFRunTag                    &runTag,
        std::vector<SvyPrbStream>   &vIM,
        std::vector<SvyPrbStream>   &vOB,
        std::vector<SvyPrbStream>   &vNI );
    virtual ~SvyPrbThread();

    void startRun();
};


class SvyPrbRun : public QObject
{
    Q_OBJECT

private:
    DAQ::Params                 oldParams;
    DFRunTag                    runTag;
    std::vector<SvyPrbStream>   vIM;
    std::vector<SvyPrbStream>   vOB;
    std::vector<SvyPrbStream>   vNI;
    std::vector<int>            vCurShnk,
                                vCurBank;
    SvyPrbThread                *thd;
    QProgressDialog             *prgDlg;
    int                         irunbank,
                                nrunbank;

public:
    SvyPrbRun() : thd(0), prgDlg(0), irunbank(0), nrunbank(0)   {}

    void initRun();
    int msPerBnk();
    bool nextBank();

public slots:
    void finish();
    void finish_cleanup();

private:
    void createPrgDlg();
};

#endif  // SVYPRB_H

