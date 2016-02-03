#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include "SGLTypes.h"

#include <QMainWindow>
#include <QSet>

namespace DAQ {
struct Params;
}

class GWToolbar;
class GWLEDWidget;
class GWNiWidget;
class GLGraph;

/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

extern const QColor NeuGraphBGColor,
                    AuxGraphBGColor,
                    DigGraphBGColor;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GraphsWindow : public QMainWindow
{
    Q_OBJECT

private:
    enum eStream {
        imStream = 0,
        niStream = 1
    };

    struct SelState {
        int     ic;
        eStream stream;
        bool    maximized;
    };

private:
    DAQ::Params     &p;
    GWToolbar       *tbar;
    GWLEDWidget     *LED;
    GWNiWidget      *niW;
    QSet<GLGraph*>  extraGraphs;
    SelState        selection;

public:
    GraphsWindow( DAQ::Params &p );
    virtual ~GraphsWindow();

// Remote
    void remoteSetTrgEnabled( bool on );
    void remoteSetRunLE( const QString &name );

// Run
    void showHideSaveChks();
    void sortGraphs();
    void eraseGraphs();

    void niPutScans( vec_i16 &scans, quint64 firstSamp );

// Toolbar
    bool tbIsMaximized() const      {return selection.maximized;}
    bool tbIsSelGraphAnalog() const;
    void tbGetSelGraphScales( double &xSpn, double &yScl ) const;
    QColor tbGetSelGraphColor() const;

// GW widgets
    QSet<GLGraph*> &gwGraphPool()   {return extraGraphs;}
    void niSetSelection( int ic, const QString &name );

public slots:
// Gates/triggers
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();

// GW widgets
    void ensureSelectionVisible();
    void toggleMaximized();

// Toolbar
    void tbGraphSecsChanged( double d );
    void tbGraphYScaleChanged( double d );
    void tbShowColorDialog();
    void tgApplyAll();
    void tbHipassClicked( bool checked );
    void tbSetTrgEnable( bool checked );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void setSelection(
        eStream         stream,
        int             ic,
        const QString   &name );
};

#endif  // GRAPHSWINDOW_H


