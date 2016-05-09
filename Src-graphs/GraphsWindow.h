#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include "SGLTypes.h"

#include <QMainWindow>

namespace DAQ {
struct Params;
}

class RunToolbar;
class GWLEDWidget;
class SVGrafsM_Im;
class SVGrafsM_Ni;

/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

extern const QColor NeuGraphBGColor,
                    LfpGraphBGColor,
                    AuxGraphBGColor,
                    DigGraphBGColor;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GraphsWindow : public QMainWindow
{
    Q_OBJECT

private:
    DAQ::Params     &p;
    RunToolbar      *tbar;
    GWLEDWidget     *LED;
    SVGrafsM_Im     *imW;
    SVGrafsM_Ni     *niW;

public:
    GraphsWindow( DAQ::Params &p );
    virtual ~GraphsWindow();

// Run
    void eraseGraphs();

    void imPutScans( vec_i16 &data, quint64 headCt );
    void niPutScans( vec_i16 &data, quint64 headCt );

public slots:
// Remote
    bool remoteIsUsrOrderIm();
    bool remoteIsUsrOrderNi();
    void remoteSetRecordingEnabled( bool on );
    void remoteSetRunLE( const QString &name );

// Gates/triggers
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();
    void updateTime( const QString &s );

// Toolbar
    void tbSetRecordingEnabled( bool checked );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );
};

#endif  // GRAPHSWINDOW_H


