#ifndef SVGRAFSM_H
#define SVGRAFSM_H

#include "SGLTypes.h"
#include "MGraph.h"
#include "GraphStats.h"

#include <QWidget>
#include <QMutex>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVToolsM;

class QTabBar;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This is a QWidget with a QTabBar above and a single MGraph below.
// We don't use tab pages here. Rather, as tabs change we point the
// MGraphX.Y vector to the corresponding ic2Y entries.
//
class SVGrafsM : public QWidget
{
    Q_OBJECT

protected:
    struct UsrSettings {
    // Default settings:
    // All graphs get these settings initially.
    // Toolbar can set individual tabs, graphs.
    // Only applyAll saves new defaults.
    //
        double  secs,
                yscl0,     // primary neural (AP)
                yscl1,     // secondary neural (LF)
                yscl2;     // primary aux
        QColor  clr0,
                clr1,
                clr2;
        int     grfPerTab;
        bool    filter;
        bool    usrOrder;
    };

protected:
    GraphsWindow            *gw;
    SVToolsM                *tb;
    DAQ::Params             &p;
    QTabBar                 *tabs;
    MGraph                  *theM;
    MGraphX                 *theX;
    QVector<MGraphY>        ic2Y;
    QVector<GraphStats>     ic2stat;
    QVector<int>            ic2iy,
                            ig2ic;
    mutable QMutex          drawMtx;
    UsrSettings             set;
    int                     trgChan,
                            digitalType,
                            lastMouseOverChan,
                            selected,
                            maximized;
    bool                    externUpdateTimes;

public:
    SVGrafsM( GraphsWindow *gw, DAQ::Params &p );
    void init( SVToolsM *tb );
    virtual ~SVGrafsM();

    QWidget *getGWWidget()  {return (QWidget*)gw;}

    virtual void putScans( vec_i16 &data, quint64 headCt ) = 0;
    void eraseGraphs();

    bool isFiltered()   const   {return set.filter;}
    bool isUsrOrder()   const   {return set.usrOrder;}
    bool isMaximized()  const   {return maximized > -1;}
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
    virtual void hipassClicked( bool checked ) = 0;

private slots:
    void tabChange( int itab, bool internUpdateTimes = true );
    virtual void mySaveGraphClicked( bool checked ) = 0;

    virtual void myMouseOverGraph( double x, double y, int iy ) = 0;
    virtual void myClickGraph( double x, double y, int iy ) = 0;
    void dblClickGraph( double x, double y, int iy );

protected:
    QString clrToString( QColor c );
    QColor clrFromString( QString s );

    virtual int myChanCount() = 0;
    virtual double mySampRate() = 0;
    virtual void mySort_ig2ic() = 0;
    virtual int myGrfPerTab() const = 0;
    virtual QString myChanName( int ic ) const = 0;
    virtual QBitArray& mySaveBits() = 0;
    virtual int mySetUsrTypes() = 0;

    virtual void saveSettings() = 0;
    virtual void loadSettings() = 0;

    void selectChan( int ic );

private:
    void initTabs();
    void initGraphs();

    void ensureVisible();
    void setGraphTimeSecs();
    void update_ic2iy( int itab );
};

#endif  // SVGRAFSM_H


