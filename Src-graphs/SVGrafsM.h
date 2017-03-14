#ifndef SVGRAFSM_H
#define SVGRAFSM_H

#include "SGLTypes.h"
#include "MGraph.h"
#include "GraphStats.h"

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVToolsM;
class MNavbar;
class ShankCtl;
struct ShankMap;

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
                bandSel,
                sAveRadius;
        bool    filterChkOn,
                dcChkOn,
                binMaxOn,
                usrOrder;
    };

    class Joiner {
    private:
        vec_i16 residue;
        quint64 resNextCt;
        int     resDwnSmp;
    public:
        Joiner() : resNextCt(0), resDwnSmp(0) {}

        void reset();

        int addAndTrim(
            vec_i16*    &ptr,
            vec_i16     &cat,
            vec_i16     &data,
            quint64     &headCt,
            int         ntpts,
            int         nC,
            int         resDwnSmp );
    };

    class DCAve {
    private:
        QVector<float>  sum;
        QVector<int>    cnt;
        double          clock;
        int             nC,
                        nN;
    public:
        QVector<float>  lvl;
    public:
        void init( int nChannels, int nNeural );
        void setChecked( bool checked );
        void updateLvl(
            const qint16    *d,
            int             ntpts,
            int             dwnSmp );
    private:
        void updateSums(
            const qint16    *d,
            int             ntpts,
            int             dwnSmp );
    };

protected:
    GraphsWindow            *gw;
    SVToolsM                *tb;
    MNavbar                 *nv;
    ShankCtl                *shankCtl;
    const DAQ::Params       &p;
    MGraph                  *theM;
    MGraphX                 *theX;
    QAction                 *sortAction,
                            *saveAction,
                            *cTTLAction;
    QVector<MGraphY>        ic2Y;
    QVector<GraphStats>     ic2stat;
    QVector<int>            ic2iy,
                            ig2ic;
    QVector<QVector<int> >  TSM;
    mutable QMutex          drawMtx;
    UsrSettings             set;
    Joiner                  join;
    DCAve                   dc;
    int                     digitalType,
                            lastMouseOverChan,
                            selected,
                            maximized;
    bool                    externUpdateTimes;

public:
    SVGrafsM( GraphsWindow *gw, const DAQ::Params &p );
    void init( SVToolsM *tb );
    virtual ~SVGrafsM();

    QWidget *getGWWidget()  {return (QWidget*)gw;}
    MGraphX *getTheX()      {return theX;}

    void eraseGraphs();
    virtual void putScans( vec_i16 &data, quint64 headCt ) = 0;
    virtual void updateRHSFlags() = 0;

    virtual int chanCount()     const = 0;
    virtual int neurChanCount() const = 0;
    int  navNChan()         const   {return set.navNChan;}
    int  curSel()           const   {return selected;}
    virtual bool isBandpass()           const = 0;
    virtual QString filterChkTitle()    const = 0;
    int  curBandSel()       const   {return set.bandSel;}
    int  curSAveRadius()    const   {return set.sAveRadius;}
    bool isFilterChkOn()    const   {return set.filterChkOn;}
    bool isDcChkOn()        const   {return set.dcChkOn;}
    bool isBinMaxOn()       const   {return set.binMaxOn;}
    bool isUsrOrder()       const   {return set.usrOrder;}
    bool isMaximized()      const   {return maximized > -1;}
    void getSelScales( double &xSpn, double &yScl ) const;
    QColor getSelColor() const;
    virtual bool isSelAnalog() const = 0;
    virtual void setRecordingEnabled( bool checked ) = 0;

public slots:
    // Navbar
    void toggleSorting();
    void showShanks();
    void nchanChanged( int val, int first );
    void firstChanged( int first );
    // SVTools
    void ensureSelectionVisible();
    void toggleMaximized();
    void graphSecsChanged( double d );
    void graphYScaleChanged( double d );
    void showColorDialog();
    void applyAll();
    void dcChkClicked( bool checked );
    void binMaxChkClicked( bool checked );
    virtual void bandSelChanged( int sel ) = 0;
    virtual void filterChkClicked( bool checked ) = 0;
    virtual void sAveRadChanged( int radius ) = 0;

private slots:
    virtual void mySaveGraphClicked( bool checked ) = 0;

    virtual void myMouseOverGraph( double x, double y, int iy ) = 0;
    virtual void myClickGraph( double x, double y, int iy ) = 0;
    void dblClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy ) = 0;

protected:
    QString clrToString( QColor c ) const;
    QColor clrFromString( QString s ) const;

    virtual void myInit() = 0;
    virtual double mySampRate() const = 0;
    virtual void mySort_ig2ic() = 0;
    virtual QString myChanName( int ic ) const = 0;
    virtual const QBitArray& mySaveBits() const = 0;
    virtual int mySetUsrTypes() = 0;

    virtual void loadSettings() = 0;
    virtual void saveSettings() const = 0;

    void setSorting( bool userSorted );
    void selectChan( int ic );
    void ensureVisible();

    void sAveTable( const ShankMap &SM, int c0, int cLim, int radius );
    int s_t_Ave( const qint16 *d_ic, int ic );

private:
    void initGraphs();

    void pageChange( int first, bool internUpdateTimes = true );
    void setGraphTimeSecs();
    void update_ic2iy( int first );
};

#endif  // SVGRAFSM_H


