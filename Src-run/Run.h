#ifndef RUN_H
#define RUN_H

#include "KVParams.h"

#include <QObject>
#include <QMutex>
#include <QVector>

#include <vector>

namespace DAQ {
struct Params;
}

class MainApp;
class GraphsWindow;
class GraphFetcher;
struct GFStream;
class IMReader;
class NIReader;
class Gate;
class Trigger;
class AIQ;

class QFileInfo;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class Run : public QObject
{
    Q_OBJECT

private:
    struct GWPair {
        GraphsWindow    *gw;
        GraphFetcher    *gf;

        GWPair() : gw(0), gf(0) {}
        GWPair( const DAQ::Params &p, int igw );

        void createWindow( const DAQ::Params &p, int igw );
        void setTitle( int igw );
        void kill();
        void startFetching( QMutex &runMtx );
        void stopFetching();
    };

private:
    MainApp             *app;
    QVector<AIQ*>       imQ;            // guarded by runMtx
    QVector<AIQ*>       imQf;           // guarded by runMtx
    QVector<AIQ*>       obQ;            // guarded by runMtx
    AIQ*                niQ;            // guarded by runMtx
    std::vector<GWPair> vGW;            // guarded by runMtx
    IMReader            *imReader;      // guarded by runMtx
    NIReader            *niReader;      // guarded by runMtx
    Gate                *gate;          // guarded by runMtx
    Trigger             *trg;           // guarded by runMtx
    mutable QMutex      runMtx;
    bool                running,        // guarded by runMtx
                        dumx[3];

public:
    Run( MainApp *app );

// Owned GraphsWindow ops
    bool grfIsUsrOrder( int js, int ip );
    void grfRemoteSetsRunLE( const QString &fn );
    void grfSetStreams( std::vector<GFStream> &gfs, int igw );
    bool grfHardPause( bool pause, int igw = -1 );
    void grfWaitPaused( int igw = -1 );
    void grfSetFocusMain();
    void grfShowSpikes( int gp, int ip, int ch );
    void grfShowColorTTL();
    void grfRefresh();
    void grfShowHideAll();
    void grfMoreTraces();
    void grfUpdateRHSFlagsAll();
    void grfUpdateProbe( int ip, bool shankMap, bool chanMap );
    void grfUpdateWindowTitles();
    void grfClose( GraphsWindow *gw );

// Owned AIStream ops
    int streamSpanMax( const DAQ::Params &p, bool warn = true );
    quint64 getSampleCount( int js, int ip ) const;
    const AIQ* getQ( int js, int ip ) const;
    double getStreamTime() const;

// Run control
    bool isRunning() const;
    bool startRun( QString &err );
    void stopRun();
    bool askThenStopRun();
    void imecUpdate( int ip );

public slots:
// GraphFetcher ops
    void grfSoftPause( bool pause, int igw );

// Owned Datafile ops
    bool dfIsSaving() const;
    bool dfIsInUse( const QFileInfo &fi ) const;
    void dfSetNextFileName( const QString &name );
    void dfSetTriggerOffBeep( quint32 hertz, quint32 msec );
    void dfSetTriggerOnBeep( quint32 hertz, quint32 msec );
    void dfSetSBTT( int ip, const QString &SBTT );
    void dfSetRecordingEnabled( bool enabled, bool remote = false );
    bool dfIsRecordingEnabled();
    void dfHaltiq( int iq );
    void dfForceGTCounters( int g, int t );
    QString dfGetCurNiName() const;
    quint64 dfGetFileStart( int js, int ip ) const;

// Owned gate and trigger ops
    void rgtSetGate( bool hi );
    void rgtSetTrig( bool hi );
    void rgtSetMetaData( const KeyValMap &kvm );

// Audio ops
    void aoStart();
    void aoStop();

// Anatomy ops
    QString setAnatomyPP( const QString &s );

// Opto ops
    QString opto_getAttens( int ip, int color );
    QString opto_emit( int ip, int color, int site );

private slots:
    void gettingSamples();
    void workerStopsRun();

private:
    void aoStartDev();
    bool aoStopDev();
    void createGraphsWindow( const DAQ::Params &p );
};

#endif  // RUN_H


