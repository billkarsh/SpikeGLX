#ifndef GWNIWIDGETG_H
#define GWNIWIDGETG_H

#include "SGLTypes.h"
#include "GLGraph.h"
#include "GraphStats.h"

#include <QTabWidget>
#include <QSet>
#include <QMutex>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class Biquad;

class QFrame;
class QCheckBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWNiWidgetG : public QTabWidget
{
    Q_OBJECT

private:
    GraphsWindow            *gw;
    DAQ::Params             &p;
    void                    *curTab;
    QVector<QWidget*>       graphTabs;
    QVector<GLGraph*>       ic2G;
    QVector<GLGraphX>       ic2X;
    QVector<GraphStats>     ic2stat;
    QVector<QFrame*>        ic2frame;
    QVector<QCheckBox*>     ic2chk;
    QVector<int>            ig2ic;
    QSet<GLGraph*>          graphCache;
    Biquad                  *hipass;
    mutable QMutex          hipassMtx,
                            drawMtx;
    int                     graphsPerTab,
                            trgChan,
                            lastMouseOverChan,
                            maximized;

public:
    GWNiWidgetG( GraphsWindow *gw, DAQ::Params &p );
    virtual ~GWNiWidgetG();

    void putScans( vec_i16 &scans, quint64 firstSamp );
    void eraseGraphs();

    void sortGraphs();
    bool isChanAnalog( int ic ) const;
    int  initialSelectedChan( QString &name ) const;
    void selectChan( int ic, bool selected );
    void ensureVisible( int ic );
    int  getMaximized() const {return maximized;}
    void toggleMaximized( int newMaximized );
    void getGraphScales( double &xSpn, double &yScl, int ic ) const;
    void graphSecsChanged( double d, int ic );
    void graphYScaleChanged( double d, int ic );
    QColor getGraphColor( int ic ) const;
    void colorChanged( QColor c, int ic );
    void applyAll( int ic );
    void hipassChecked( bool checked );
    void showHideSaveChks();
    void enableAllChecks( bool enabled );

private slots:
    void tabChange( int itab );
    void saveGraphClicked( bool checked );

    void mouseOverGraph( double x, double y );
    void mouseClickGraph( double x, double y );
    void mouseDoubleClickGraph( double x, double y );

private:
    int getNumGraphsPerTab() const;
    void initTabs();
    bool initFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();

    int graph2Chan( QObject *graphObj );
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
    void retileBySorting();

    void cacheAllGraphs();
    void returnFramesToPool();

    void saveSettings();
    void loadSettings();
};

#endif  // GWNIWIDGETG_H


