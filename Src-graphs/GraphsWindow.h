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

    friend class GWToolbar;
    friend class GWNiWidget;

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

    void remoteSetTrgEnabled( bool on );
    void remoteSetRunLE( const QString &name );
    void showHideSaveChks();
    void sortGraphs();

    void niPutScans( vec_i16 &scans, quint64 firstSamp );
    void eraseGraphs();

public slots:
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();

private slots:
    void ensureSelectionVisible();
    void toggleMaximized();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void showColorDialog();
    void applyAll();
    void hipassClicked( bool checked );
    void setTrgEnable( bool checked );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void setSelection(
        eStream         stream,
        int             ic,
        const QString   &name );
    bool isMaximized() const
        {return selection.maximized;}
    bool isSelGraphAnalog() const;
    void getSelGraphScales( double &xSpn, double &yScl ) const;
    QColor getSelGraphColor() const;
};

#endif  // GRAPHSWINDOW_H


