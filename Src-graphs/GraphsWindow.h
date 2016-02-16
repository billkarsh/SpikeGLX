#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include "SGLTypes.h"

#include <QMainWindow>

namespace DAQ {
struct Params;
}

class GWToolbar;
class GWLEDWidget;
class GWImWidgetM;
class GWNiWidgetG;

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
    enum eStream {
        imStream = 0,
        niStream = 1
    };

    struct SelState {
        int     ic;
        eStream stream;
    };

private:
    DAQ::Params     &p;
    GWToolbar       *tbar;
    GWLEDWidget     *LED;
    GWImWidgetM     *imW;
    GWNiWidgetG     *niW;
    SelState        selection;

public:
    GraphsWindow( DAQ::Params &p );
    virtual ~GraphsWindow();

// Run
    void showHideSaveChks();
    void sortGraphs();
    void eraseGraphs();

    void imPutScans( vec_i16 &data, quint64 headCt );
    void niPutScans( vec_i16 &data, quint64 headCt );

// Toolbar
    bool tbIsSelMaximized() const;
    bool tbIsSelGraphAnalog() const;
    void tbGetSelGraphScales( double &xSpn, double &yScl ) const;
    QColor tbGetSelGraphColor() const;

// GW widgets
    void imSetSelection( int ic, const QString &name );
    void niSetSelection( int ic, const QString &name );

public slots:
// Remote
    void remoteSetTrgEnabled( bool on );
    void remoteSetRunLE( const QString &name );

// Gates/triggers
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();

// GW widgets
    void ensureSelectionVisible();
    void toggledMaximized();

// Toolbar
    void tbToggleMaximized();
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


