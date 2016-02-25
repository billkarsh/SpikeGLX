#ifndef SVGRAFSG_H
#define SVGRAFSG_H

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
class SVToolsG;

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
class SVGrafsG : public QTabWidget
{
    Q_OBJECT

protected:
    struct UsrSettings {
        bool    filterChkOn;
        bool    usrOrder;
    };

protected:
    GraphsWindow            *gw;
    SVToolsG                *tb;
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
    UsrSettings             set;
    int                     graphsPerTab,
                            trgChan,
                            lastMouseOverChan,
                            selected,
                            maximized;

public:
    SVGrafsG( GraphsWindow *gw, DAQ::Params &p );
    void init( SVToolsG *tb );
    virtual ~SVGrafsG();

    QWidget *getGWWidget()  {return (QWidget*)gw;}

    virtual void putScans( vec_i16 &data, quint64 headCt ) = 0;
    void eraseGraphs();

    virtual QString filterChkTitle() const = 0;
    bool isFilterChkOn()    const   {return set.filterChkOn;}
    bool isUsrOrder()       const   {return set.usrOrder;}
    bool isMaximized()      const   {return maximized > -1;}
    void getSelScales( double &xSpn, double &yScl ) const;
    QColor getSelColor() const;
    virtual bool isSelAnalog() const = 0;

// BK: Need decide handling of save checks.
// BK: MainApp should lose save check option.

    void showHideSaveChks();
    void enableAllChecks( bool enabled );

public slots:
    void toggleSorting();
    void ensureSelectionVisible();
    void toggleMaximized();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void showColorDialog();
    void applyAll();
    virtual void filterChkClicked( bool checked ) = 0;

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

    int graph2Chan( QObject *graphObj );
    void selectChan( int ic );

    virtual void saveSettings() = 0;
    virtual void loadSettings() = 0;

private:
    void initTabs();
    bool initFrameCheckBox( QFrame* &f, int ic );
    void initFrameGraph( QFrame* &f, int ic );
    void initFrames();

    void ensureVisible();
    void setGraphTimeSecs( int ic, double t );

    void retileBySorting();

    void cacheAllGraphs();
    void returnFramesToPool();
};

#endif  // SVGRAFSG_H


