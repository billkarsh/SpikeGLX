#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include "SGLTypes.h"
#include "GLGraph.h"

#include <QMainWindow>
#include <QToolbar>
#include <QVector>
#include <QSet>
#include <QMutex>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class Biquad;
class QTabWidget;
class QFrame;
class QCheckBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWToolbar : public QToolBar
{
    Q_OBJECT

public:
    GraphsWindow    *gw;
    DAQ::Params     &p;
    bool            paused;

    GWToolbar( GraphsWindow *gw, DAQ::Params &p )
    : gw(gw), p(p), paused(false) {}

    void init();

    void updateSortButText();
    void setSelName( const QString &name );
    void updateMaximized()  {update();}
    bool getScales( double &xSpn, double &yScl ) const;
    QColor selectColor();
    bool getFltCheck() const;
    bool getTrigCheck() const;
    void setTrigCheck( bool on );
    QString getRunLE() const;
    void setRunLE( const QString &name );
    void enableRunLE( bool enabled );

public slots:
    void toggleFetcher();

private:
    void update();
};


class GraphsWindow : public QMainWindow
{
    Q_OBJECT

    friend class GWToolbar;

private:
    struct GraphStats
    {
    private:
        double  s1;
        double  s2;
        uint    num;
    public:
        GraphStats()  {clear();}
        void clear() {s1 = s2 = num = 0;}
        inline void add( double v ) {s1 += v, s2 += v*v, ++num;}
        double mean() const {return (num > 1 ? s1/num : s1) / 32768.0;}
        double rms() const;
        double stdDev() const;
    };

    GWToolbar               tbar;
    Vec2                    lastMousePos;
    DAQ::Params             &p;
    QVector<QWidget*>       graphTabs;
    QVector<GLGraph*>       ic2G;
    QVector<GLGraphX>       ic2X;
    QVector<GraphStats>     ic2stat;
    QVector<QFrame*>        ic2frame;
    QVector<QCheckBox*>     ic2chk;
    QVector<int>            ig2ic;
    QSet<GLGraph*>          extraGraphs;
    GLGraph                 *maximized;
    Biquad                  *hipass;
    mutable QMutex          hipassMtx,
                            LEDMtx,
                            drawMtx;
    int                     graphsPerTab,
                            trgChan,
                            lastMouseOverChan,
                            selChan;

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
    void setTriggerLED( bool on );
    void setGateLED( bool on );
    void blinkTrigger();

private slots:
    void blinkTrigger_Off();
    void toggleMaximize();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void doGraphColorDialog();
    void applyAll();
    void hpfChk( bool checked );
    void setTrgEnable( bool checked );
    void saveNiGraphChecked( bool checked );

    void selectChan( int ic );
    void selectSelChanTab();
    void tabChange( int itab );

    void timerUpdateGraphs();
    void timerUpdateMouseOver();

    void mouseOverGraph( double x, double y );
    void mouseClickGraph( double x, double y );
    void mouseDoubleClickGraph( double x, double y );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    QTabWidget *mainTabWidget() const
        {return dynamic_cast<QTabWidget*>(centralWidget());}

    int getNumGraphsPerTab() const;
    void initTabs();
    QWidget *initLEDWidget();
    void initStatusBar();
    bool initNiFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();
    void initAssertFilters();
    void initUpdateTimers();
    void initWindow();

    int graph2Chan( QObject *graphObj );
    void getSelGraphScales( double &xSpn, double &yScl ) const;
    QColor getSelGraphColor() const;
    bool isSelGraphAnalog() const;
    bool isMaximized() const
        {return maximized != 0;}
    void setGraphTimeSecs( int ic, double t );
    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );

    void setTabText( int itab, int igLast );
    void retileGraphsAccordingToSorting();

    void saveGraphSettings();
    void loadGraphSettings();
};

#endif  // GRAPHSWINDOW_H


