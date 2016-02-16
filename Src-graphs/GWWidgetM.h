#ifndef GWWIDGETM_H
#define GWWIDGETM_H

#include "SGLTypes.h"
#include "MGraph.h"
#include "GraphStats.h"

#include <QWidget>
#include <QMutex>

namespace DAQ {
struct Params;
}

class GraphsWindow;

class QTabBar;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This is a QWidget with a QTabBar above and a single MGraph below.
// We don't use tab pages here. Rather, as tabs change we point the
// MGraphX.Y vector to the corresponding ic2Y entries.
//
class GWWidgetM : public QWidget
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
                ysclNa,     // primary neural (AP)
                ysclNb,     // secondary neural (LF)
                ysclAa;     // primary aux
        QColor  clrNa,
                clrNb,
                clrAa;
        int     grfPerTab;
    };

protected:
    GraphsWindow            *gw;
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
                            maximized;

public:
    GWWidgetM( GraphsWindow *gw, DAQ::Params &p );
    void init();
    virtual ~GWWidgetM();

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
    void tabChange( int itab, bool updateSecs = true );
    virtual void mySaveGraphClicked( bool checked ) = 0;

    virtual void myMouseOverGraph( double x, double y, int iy ) = 0;
    virtual void myClickGraph( double x, double y, int iy ) = 0;
    void dblClickGraph( double x, double y, int iy );

protected:
    virtual int myChanCount() = 0;
    virtual double mySampRate() = 0;
    virtual void mySort_ig2ic() = 0;
    virtual int myGrfPerTab() const = 0;
    virtual QString myChanName( int ic ) const = 0;
    virtual QBitArray& mySaveBits() = 0;
    virtual int mySetUsrTypes() = 0;
    virtual QString mySettingsGrpName() = 0;

    void saveSettings();
    void loadSettings();

private:
    void initTabs();
    void initGraphs();

    void setGraphTimeSecs( int ic, double t );
    void update_ic2iy( int itab );
};

#endif  // GWWIDGETM_H


