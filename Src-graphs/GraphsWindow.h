#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include "SGLTypes.h"
#include "GLGraph.h"

#include <QMainWindow>
#include <QVector>
#include <QSet>
#include <QMutex>

namespace DAQ {
struct Params;
}

class Biquad;
class QTabWidget;
class QFrame;
class QCheckBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GraphsWindow : public QMainWindow
{
    Q_OBJECT

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

    Vec2                    lastMousePos;
    double                  tAvg,
                            tNum;
    DAQ::Params             &p;
    QVector<QWidget*>       graphTabs;
    QVector<GLGraph*>       ic2G;
    QVector<GLGraphX>       ic2X;
    QVector<GraphStats>     ic2stat;
    QVector<QFrame*>        ic2frame;
    QVector<QCheckBox*>     ic2chk;
    QVector<int>            ig2ic;
    QSet<GLGraph*>          extraGraphs;
    QToolBar                *graphCtls;
    GLGraph                 *maximized;
    Biquad                  *hipass;
    mutable QMutex          hipassMtx,
                            LEDMtx,
                            drawMtx;
    int                     graphsPerTab,
                            trgChan,
                            lastMouseOverChan,
                            selChan;
    bool                    paused;

public:
    GraphsWindow( DAQ::Params &p );
    virtual ~GraphsWindow();

    void remoteSetTrgEnabled( bool on );
    void remoteSetRunLE( const QString &name );
    void showHideSaveChks();
    void sortGraphs();

    void putScans( vec_i16 &scans, quint64 firstSamp );
    void eraseGraphs();

public slots:
    void setTriggerLED( bool on );
    void setGateLED( bool on );
    void blinkTrigger();

private slots:
    void blinkTrigger_Off();
    void toggleFetcher();
    void toggleMaximize();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void doGraphColorDialog();
    void applyAll();
    void hpfChk( bool checked );
    void setTrgEnable( bool checked );
    void saveGraphChecked( bool checked );

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
    void initToolbar();
    QWidget *initLEDWidget();
    void initStatusBar();
    bool initFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();
    void initAssertFilters();
    void initUpdateTimers();
    void initWindow();

    int graph2Chan( QObject *graphObj );
    void updateToolbar();
    void setGraphTimeSecs( int ic, double t );
    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ic,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );

    void setSortButText();
    void setTabText( int itab, int igLast );
    void retileGraphsAccordingToSorting();

    void saveGraphSettings();
    void loadGraphSettings();
};

#endif  // GRAPHSWINDOW_H


