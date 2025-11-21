#ifndef RUNTOOLBAR_H
#define RUNTOOLBAR_H

#include <QToolBar>

namespace DAQ {
struct Params;
}

namespace Ui {
class FVW_NotesDialog;
}

class GraphsWindow;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Main Graphs window toolbar:
//
// Stop T-on | Trg-enab runname <G T> T-rec | Notes 'Graphs' pause
//
class RunToolbar : public QToolBar
{
    Q_OBJECT

private:
    GraphsWindow        *gw;
    const DAQ::Params   &p;
    Ui::FVW_NotesDialog *notesui;
    bool                paused;

public:
    RunToolbar( GraphsWindow *gw, const DAQ::Params &p );

    void updateOnTime( const QString &s );
    void updateRecTime( const QString &s );
    void updateGT( const QString &s );
    void setRecordingEnabled( bool on, bool block = false );
    QString getRunLE() const;
    void setRunLE( const QString &name );
    void enableRunLE( bool enabled );
    void update();

public slots:
    void recordBut( bool checked );
    void notes();
    void notesAddStamp();
    void toggleFetcher();
    void remotePause( bool pause );
};

#endif  // RUNTOOLBAR_H


