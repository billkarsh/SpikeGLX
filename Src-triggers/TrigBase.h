#ifndef TRIGBASE_H
#define TRIGBASE_H

#include "DataFileIMAP.h"
#include "DataFileIMLF.h"
#include "DataFileNI.h"
#include "DataFileOB.h"
#include "Sync.h"

class GraphsWindow;

class QFileInfo;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class TrigBase : public QObject
{
    Q_OBJECT

private:
    struct ManOvr {
        int             usrG,
                        usrT;
        volatile bool   forceGT,
                        gateEnab;

        ManOvr( const DAQ::Params &p )
        :   gateEnab(!p.mode.manOvInitOff)
        {set( p.mode.initG, p.mode.initT );}

        void reset()                {usrG=-1; usrT=-1;  forceGT=false;}
        void set( int g, int t )    {usrG=g;  usrT=t;   forceGT=true;}
        void get( int &g, int &t )  {g=usrG;  t=usrT-1; reset();}
    };

private:
    const QVector<AIQ*>         &imQ;
    const QVector<AIQ*>         &obQ;
    const AIQ                   *niQ;
    std::vector<DataFileIMAP*>  dfImAp;
    std::vector<DataFileIMLF*>  dfImLf;
    std::vector<DataFileOB*>    dfOb;
    DataFileNI                  *dfNi;
    ManOvr                      ovr;
    mutable QMutex              dfMtx;
    mutable QMutex              startTMtx;
    mutable QMutex              haltMtx;
    QString                     lastRunDir,
                                forceName;
    QSet<int>                   iqhalt;
    QVector<QString>            svySBTT;
    KeyValMap                   kvmRmt;
    double                      startT,     // stream time
                                gateHiT,    // stream time
                                gateLoT,    // stream time
                                trigHiT,    // stream time
                                tLastReport;
    std::vector<double>         tLastProf;
    std::vector<quint64>        firstCtIm;
    std::vector<quint64>        firstCtOb;
    quint64                     firstCtNi;
    quint32                     offHertz,
                                offmsec,
                                onHertz,
                                onmsec;
    int                         nImQ,
                                nObQ,
                                nNiQ,
                                iGate,
                                iTrig,
                                loopPeriod_us;
    volatile bool               gateHi,
                                pleaseStop;

protected:
    const DAQ::Params       &p;
    GraphsWindow            *gw;
    std::vector<SyncStream> vS;
    mutable QMutex          runMtx;
    double                  statusT;    // wall time

public:
    TrigBase(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );
    virtual ~TrigBase()             {}

    bool allFilesClosed() const;
    bool isInUse( const QFileInfo &fi ) const;
    void setNextFileName( const QString &name )
        {QMutexLocker ml( &dfMtx ); forceName = name;}
    void setTriggerOffBeep( quint32 hertz, quint32 msec )
        {QMutexLocker ml( &dfMtx ); offHertz = hertz; offmsec = msec;}
    void setTriggerOnBeep( quint32 hertz, quint32 msec )
        {QMutexLocker ml( &dfMtx ); onHertz = hertz; onmsec = msec;}
    void setMetaData( const KeyValMap &kvm );
    void setSBTT( int ip, const QString &SBTT )
        {QMutexLocker ml( &dfMtx ); svySBTT[ip] = SBTT;}
    QString curNiFilename() const;
    quint64 curFileStart( int js, int ip ) const;

    void setStartT();
    void setGateEnabled( bool enabled );
    bool isGateHi() const   {QMutexLocker ml( &runMtx ); return gateHi;}

    void haltiq( int iq )
        {QMutexLocker ml( &haltMtx ); iqhalt.insert( iq );}
    bool isiqhalt( int iq ) const
        {QMutexLocker ml( &haltMtx ); return iqhalt.contains( iq );}

    void stop()             {QMutexLocker ml( &runMtx ); pleaseStop = true;}
    bool isStopped() const  {QMutexLocker ml( &runMtx ); return pleaseStop;}

    void setGate( bool hi );
    void forceGTCounters( int g, int t );

signals:
    void daqError( const QString &s );
    void finished();

public slots:
    virtual void run() = 0;

protected:
    double nowCalibrated() const;

    double getGateHiT() const   {QMutexLocker ml( &runMtx ); return gateHiT;}
    double getGateLoT() const   {QMutexLocker ml( &runMtx ); return gateLoT;}

    void getGT( int &ig, int &it ) const
        {
            QMutexLocker ml( &runMtx );
            ig = iGate;
            it = iTrig;
        }

    int incTrig( int &ig )
        {
            QMutexLocker ml( &runMtx );
            ig = iGate;
            return ++iTrig;
        }

    void endTrig();
    bool newTrig( int &ig, int &it, bool trigLED = true );
    void setSyncWriteMode();
    bool nSampsFromCt(
        vec_i16     &data,
        quint64     fromCt,
        int         nMax,
        int         js,
        int         ip );
    bool writeAndInvalData(
        int         js,
        int         ip,
        vec_i16     &data,
        quint64     headCt );
    quint64 sampCount( int js );
    void endRun( const QString &err );
    void statusOnSince( QString &s );
    void statusWrPerf( QString &s );
    void setYieldPeriod_ms( int loopPeriod_ms );
    void yield( double loopT );

private:
    bool openFile( DataFile *df, int ig, int it );
    bool writeDataLF(
        vec_i16     &data,
        quint64     headCt,
        int         ip,
        bool        inplace,
        bool        xtra );
    bool writeDataIM( vec_i16 &data, quint64 headCt, int ip );
    bool writeDataOB( vec_i16 &data, quint64 headCt, int ip );
    bool writeDataNI( vec_i16 &data, quint64 headCt );
    void getErrFlags( int ip );
};


class Trigger
{
public:
    QThread     *thread;
    TrigBase    *worker;

public:
    Trigger(
        const DAQ::Params   &p,
        GraphsWindow        *gw,
        const QVector<AIQ*> &imQ,
        const QVector<AIQ*> &obQ,
        const AIQ           *niQ );
    virtual ~Trigger();
};

#endif  // TRIGBASE_H


