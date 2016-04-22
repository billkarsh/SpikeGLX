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
class MNavbar;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// This is a QWidget with a MNavbar above and a single MGraph below.
// As graph pages change we point the MGraphX.Y vector to the
// corresponding ic2Y entries.
//
class SVGrafsM : public QWidget
{
    Q_OBJECT

protected:
    struct UsrSettings {
    // Default settings:
    // All graphs get these settings initially.
    // Toolbar can change individual graphs.
    // Only applyAll saves new defaults.
    //
        double  secs,
                yscl0,     // primary neural (AP)
                yscl1,     // secondary neural (LF)
                yscl2;     // primary aux
        QColor  clr0,
                clr1,
                clr2;
        int     navNChan,
                bandSel;
        bool    filterChkOn,
                dcChkOn,
                usrOrder;
    };

protected:
    GraphsWindow            *gw;
    SVToolsM                *tb;
    MNavbar                 *nv;
    DAQ::Params             &p;
    MGraph                  *theM;
    MGraphX                 *theX;
    QVector<MGraphY>        ic2Y;
    QVector<GraphStats>     ic2stat;
    QVector<int>            ic2iy,
                            ig2ic;
    mutable QMutex          drawMtx;
    UsrSettings             set;
    int                     digitalType,
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

    virtual int chanCount() const = 0;
    int  navNChan()         const   {return set.navNChan;}
    int  curSel()           const   {return selected;}
    virtual bool isBandpass()           const = 0;
    virtual QString filterChkTitle()    const = 0;
    virtual QString dcChkTitle()        const = 0;
    int  curBandSel()       const   {return set.bandSel;}
    bool isFilterChkOn()    const   {return set.filterChkOn;}
    bool isDcChkOn()        const   {return set.dcChkOn;}
    bool isUsrOrder()       const   {return set.usrOrder;}
    bool isMaximized()      const   {return maximized > -1;}
    void getSelScales( double &xSpn, double &yScl ) const;
    QColor getSelColor() const;
    virtual bool isSelAnalog() const = 0;
    virtual void setRecordingEnabled( bool checked ) = 0;

public slots:
    void nchanChanged( int val, int first );
    void firstChanged( int first );
    void toggleSorting();
    void ensureSelectionVisible();
    void toggleMaximized();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void showColorDialog();
    void applyAll();
    virtual void bandSelChanged( int sel ) = 0;
    virtual void filterChkClicked( bool checked ) = 0;
    virtual void dcChkClicked( bool checked ) = 0;

private slots:
    virtual void mySaveGraphClicked( bool checked ) = 0;

    virtual void myMouseOverGraph( double x, double y, int iy ) = 0;
    virtual void myClickGraph( double x, double y, int iy ) = 0;
    void dblClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy ) = 0;

protected:
    QString clrToString( QColor c );
    QColor clrFromString( QString s );

    virtual void myInit() = 0;
    virtual double mySampRate() = 0;
    virtual void mySort_ig2ic() = 0;
    virtual QString myChanName( int ic ) const = 0;
    virtual QBitArray& mySaveBits() = 0;
    virtual int mySetUsrTypes() = 0;

    virtual void saveSettings() = 0;
    virtual void loadSettings() = 0;

    void selectChan( int ic );

private:
    void initGraphs();

    void pageChange( int first, bool internUpdateTimes = true );
    void ensureVisible();
    void setGraphTimeSecs();
    void update_ic2iy( int first );
};

#endif  // SVGRAFSM_H


