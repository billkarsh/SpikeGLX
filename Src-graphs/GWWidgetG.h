#ifndef GWWIDGETG_H
#define GWWIDGETG_H

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

class QFrame;
class QCheckBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GWWidgetG : public QTabWidget
{
    Q_OBJECT

protected:
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
    mutable QMutex          drawMtx;
    int                     graphsPerTab,
                            trgChan,
                            lastMouseOverChan,
                            maximized;

public:
    GWWidgetG( GraphsWindow *gw, DAQ::Params &p );
    void init();
    virtual ~GWWidgetG();

    virtual void putScans( vec_i16 &data, quint64 headCt ) = 0;
    void eraseGraphs();

    void sortGraphs();
    virtual bool isChanAnalog( int ic ) const = 0;
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
    virtual void hipassChecked( bool checked ) = 0;
    void showHideSaveChks();
    void enableAllChecks( bool enabled );

private slots:
    void tabChange( int itab );
    virtual void saveGraphClicked( bool checked ) = 0;

    virtual void mouseOverGraph( double x, double y ) = 0;
    virtual void mouseClickGraph( double x, double y ) = 0;
    void mouseDoubleClickGraph( double x, double y );

protected:
    virtual int myChanCount() = 0;
    virtual void sort_ig2ic() = 0;
    virtual int getNumGraphsPerTab() const = 0;
    virtual QString chanName( int ic ) const = 0;
    virtual bool indexRangeThisType( int &c0, int &cLim, int ic ) = 0;
    virtual QBitArray& mySaveBits() = 0;
    virtual void customXSettings( int ic ) = 0;
    virtual QString settingsGrpName() = 0;

    int graph2Chan( QObject *graphObj );

    void saveSettings();
    void loadSettings();

private:
    void initTabs();
    bool initFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();

    void setGraphTimeSecs( int ic, double t );

    void setTabText( int itab, int igLast );
    void retileBySorting();

    void cacheAllGraphs();
    void returnFramesToPool();
};

#endif  // GWWIDGETG_H


