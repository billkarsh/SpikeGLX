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
    MainApp         *app;
    QVector<AIQ*>   imQ;            // guarded by runMtx
    AIQ*            niQ;            // guarded by runMtx
    QVector<GWPair> vGW;            // guarded by runMtx
    IMReader        *imReader;      // guarded by runMtx
    NIReader        *niReader;      // guarded by runMtx
    Gate            *gate;          // guarded by runMtx
    Trigger         *trg;           // guarded by runMtx
    mutable QMutex  runMtx;
    bool            running,        // guarded by runMtx
                    dumx[3];

public:
    Run( MainApp *app );

// Owned GraphsWindow ops
    bool grfIsUsrOrder( int ip );
    void grfRemoteSetsRunLE( const QString &fn );
    void grfSetStreams( std::vector<GFStream> &gfs, int igw );
    bool grfHardPause( bool pause, int igw = -1 );
    void grfWaitPaused( int igw );
    void grfSetFocusMain();
    void grfShowHideAll();
    void grfUpdateRHSFlagsAll();
    void grfUpdateWindowTitles();
    void grfClose( GraphsWindow *gw );

// Owned AIStream ops
    int streamSpanMax( const DAQ::Params &p, bool warn = true );
    quint64 getScanCount( int ip ) const;
    const AIQ* getImQ( uint ip ) const;
    const AIQ* getNiQ() const;
    double getStreamTime() const;

// Run control
    bool isRunning() const;
    bool startRun( QString &errTitle, QString &errMsg );
    void stopRun();
    bool askThenStopRun();
    void imecUpdate( int ip );

public slots:
// GraphFetcher ops
    void grfSoftPause( bool pause, int igw );

// Owned Datafile ops
    bool dfIsSaving() const;
    bool dfIsInUse( const QFileInfo &fi ) const;
    void dfSetRecordingEnabled( bool enabled, bool remote = false );
    bool dfIsRecordingEnabled();
    void dfForceGTCounters( int g, int t );
    QString dfGetCurNiName() const;
    quint64 dfGetFileStart( int ip ) const;

// Owned gate and trigger ops
    void rgtSetGate( bool hi );
    void rgtSetTrig( bool hi );
    void rgtSetMetaData( const KeyValMap &kvm );

// Audio ops
    void aoStart();
    void aoStop();

private slots:
    void gettingSamples();
    void workerStopsRun();

private:
    void aoStartDev();
    bool aoStopDev();
    void createGraphsWindow( const DAQ::Params &p );
};

#endif  // RUN_H


