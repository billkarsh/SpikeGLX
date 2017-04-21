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
class ColorTTLCtl;

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
    const DAQ::Params   &p;
    RunToolbar          *tbar;
    GWLEDWidget         *LED;
    SVGrafsM_Im         *imW;
    SVGrafsM_Ni         *niW;
    ColorTTLCtl         *TTLCC;

public:
    GraphsWindow( const DAQ::Params &p );
    virtual ~GraphsWindow();

    ColorTTLCtl *getTTLColorCtl()   {return TTLCC;}

// Run
    void eraseGraphs();

    void imPutScans( vec_i16 &data, quint64 headCt );
    void niPutScans( vec_i16 &data, quint64 headCt );

public slots:
// Graph flags
    void updateRHSFlags();

// Remote
    bool remoteIsUsrOrderIm();
    bool remoteIsUsrOrderNi();
    void remoteSetRecordingEnabled( bool on );
    void remoteSetRunLE( const QString &name );

// Gates/triggers
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();
    void updateOnTime( const QString &s );
    void updateRecTime( const QString &s );
    void updateGT( const QString &s );

// Toolbar
    void tbSetRecordingEnabled( bool checked );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void saveScreenState();
    void restoreScreenState();
};

#endif  // GRAPHSWINDOW_H


