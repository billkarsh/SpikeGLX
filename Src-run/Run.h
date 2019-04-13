#ifndef RUN_H
#define RUN_H

#include "KVParams.h"

#include <QObject>
#include <QMutex>

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
    MainApp         *app;
    AIQ             *imQ,           // guarded by runMtx
                    *niQ;           // guarded by runMtx
    GraphsWindow    *graphsWindow;  // guarded by runMtx
    GraphFetcher    *graphFetcher;  // guarded by runMtx
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
    bool grfIsUsrOrderIm();
    bool grfIsUsrOrderNi();
    void grfRemoteSetsRunLE( const QString &fn );
    void grfSetStreams( std::vector<GFStream> &gfs );
    void grfHardPause( bool pause );
    void grfSetFocus();
    void grfShowHide();
    void grfUpdateRHSFlags();
    void grfUpdateWindowTitles();

// Owned AIStream ops
    int streamSpanMax( const DAQ::Params &p, bool warn = true );
    quint64 getImScanCount() const;
    quint64 getNiScanCount() const;
    const AIQ* getImQ() const;
    const AIQ* getNiQ() const;

// Run control
    bool isRunning() const;
    bool startRun( QString &errTitle, QString &errMsg );
    void stopRun();
    bool askThenStopRun();
    void imecUpdate();

public slots:
// GraphFetcher ops
    void grfSoftPause( bool pause );

// Owned Datafile ops
    bool dfIsSaving() const;
    bool dfIsInUse( const QFileInfo &fi ) const;
    void dfSetNextFileName( const QString &name );
    void dfSetTriggerOffBeep( quint32 hertz, quint32 msec );
    void dfSetTriggerOnBeep( quint32 hertz, quint32 msec );
    void dfSetRecordingEnabled( bool enabled, bool remote = false );
    void dfForceGTCounters( int g, int t );
    QString dfGetCurNiName() const;
    quint64 dfGetImFileStart() const;
    quint64 dfGetNiFileStart() const;

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


