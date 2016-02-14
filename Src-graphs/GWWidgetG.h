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

// This is a QTabWidget with true tab pages. Each page shows a grid
// of QFrames (one per channel). All pages contain frames and all
// frames contain a permanent 'save' checkbox. On the other hand,
// only the frames on the current page also have GLGraphs. On tab
// change, all graphs are unparented, moved to the local graphCache,
// drawn again from the cache, and then reparented into the current
// page's frames.
//
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
    void applyAll( int ichan );
    virtual void hipassChecked( bool checked ) = 0;
    void showHideSaveChks();
    void enableAllChecks( bool enabled );

private slots:
    void tabChange( int itab );
    virtual void mySaveGraphClicked( bool checked ) = 0;

    virtual void myMouseOverGraph( double x, double y ) = 0;
    virtual void myClickGraph( double x, double y ) = 0;
    void dblClickGraph( double x, double y );

protected:
    virtual int myChanCount() = 0;
    virtual double mySampRate() = 0;
    virtual void mySort_ig2ic() = 0;
    virtual int myGrfPerTab() const = 0;
    virtual QString myChanName( int ic ) const = 0;
    virtual QBitArray& mySaveBits() = 0;
    virtual void myCustomXSettings( int ic ) = 0;
    virtual QString mySettingsGrpName() = 0;

    int graph2Chan( QObject *graphObj );

    void saveSettings();
    void loadSettings();

private:
    void initTabs();
    bool initFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();

    void setGraphTimeSecs( int ic, double t );

    void retileBySorting();

    void cacheAllGraphs();
    void returnFramesToPool();
};

#endif  // GWWIDGETG_H


