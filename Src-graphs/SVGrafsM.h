#ifndef SVGRAFSM_H
#define SVGRAFSM_H

#include "SGLTypes.h"
#include "MGraph.h"
#include "GraphStats.h"
#include "TimedTextUpdate.h"

#include <QWidget>

namespace DAQ {
struct Params;
}

class GraphsWindow;
class SVToolsM;
class MNavbar;
class SVShankCtl;
struct ShankMap;
class Biquad;

class QComboBox;

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
                yscl0,      // primary neural (AP)
                yscl1,      // secondary neural (LF)
                yscl2;      // primary aux
        QColor  clr0,
                clr1,
                clr2;
        int     navNChan,
                bandSel,
                sAveSel;    // {0=Off, 1=Local, 2=Global}
        bool    dcChkOn,
                binMaxOn,
                usrOrder;
    };

    class DCAve {
    private:
        std::vector<float>  sum;
        std::vector<int>    cnt;
        double              clock;
        int                 nC,
                            nN;
    public:
        std::vector<int>    lvl;
    public:
        void init( int nChannels, int nNeural );
        void setChecked( bool checked );
        void updateLvl(
            const qint16    *d,
            int             ntpts,
            int             dwnSmp );
        void apply(
            qint16          *d,
            int             ntpts,
            int             c0,
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
    SVShankCtl              *shankCtl;
    const DAQ::Params       &p;
    MGraph                  *theM;
    MGraphX                 *theX;
    QAction                 *audioLAction,
                            *audioRAction,
                            *sortAction,
                            *saveAction,
                            *refreshAction,
                            *cTTLAction;
    Biquad                  *hipass,
                            *lopass;
    std::vector<MGraphY>    ic2Y;
    std::vector<GraphStats> ic2stat;
    QVector<int>            ic2iy,
                            ig2ic;
    std::vector<std::vector<int> >  TSM;
    mutable QMutex          drawMtx,
                            fltMtx;
    UsrSettings             set;
    DCAve                   dc;
    TimedTextUpdate         timStatBar;
    int                     js,
                            ip,
                            jpanel,
                            digitalType,
                            lastMouseOverChan,
                            selected,
                            maximized;
    bool                    externUpdateTimes,
                            inConstructor;

public:
    SVGrafsM(
        GraphsWindow        *gw,
        const DAQ::Params   &p,
        int                 js,
        int                 ip,
        int                 jpanel );
    void init( SVToolsM *tb );
    virtual ~SVGrafsM();

    QWidget *getGWWidget()  {return (QWidget*)gw;}
    MGraphX *getTheX()      {return theX;}

    bool shankCtlGeomGet( QByteArray &geom ) const;
    void shankCtlGeomSet( const QByteArray &geom, bool show );

    void eraseGraphs();
    virtual void putSamps( vec_i16 &data, quint64 headCt ) = 0;
    virtual void updateRHSFlags() = 0;
    virtual void updateProbe( bool shankMap, bool chanMap )     {}
    virtual void setAnatomyPP( const QString &elems, int sk )   {}

    virtual int chanCount()     const = 0;
    virtual int neurChanCount() const = 0;
    virtual int analogChanCount() const = 0;
    virtual bool isImec()       const = 0;
    int  navNChan()         const   {return set.navNChan;}
    int  curSel()           const   {return selected;}
    int  curBandSel()       const   {return set.bandSel;}
    int  curSAveSel()       const   {return set.sAveSel;}
    bool isDcChkOn()        const   {return set.dcChkOn;}
    bool isBinMaxOn()       const   {return set.binMaxOn;}
    bool isUsrOrder()       const   {return set.usrOrder;}
    bool isMaximized()      const   {return maximized > -1;}
    void getSelScales( double &xSpn, double &yScl ) const;
    QColor getSelColor() const;
    virtual bool isSelAnalog() const = 0;
    virtual void setRecordingEnabled( bool checked ) = 0;
    virtual void nameLocalFilters( QComboBox *CB ) const    {}
    virtual void setLocalFilters( int &rin, int &rout, int iflt ) = 0;

public slots:
    // Navbar
    void toggleSorting();
    void showShanks( bool getGeom = true );
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
    virtual void bandSelChanged( int sel )  {}
    virtual void sAveSelChanged( int sel )  {}
    // Right-click
    void refresh();
    void colorTTL();

private slots:
    virtual void myMouseOverGraph( double x, double y, int iy ) = 0;
    virtual void myClickGraph( double x, double y, int iy ) = 0;
    void dblClickGraph( double x, double y, int iy );
    virtual void myRClickGraph( double x, double y, int iy ) = 0;
    void statBarDraw( QString s );

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

    void sAveTable( const ShankMap &SM, int nSpikeChans, int sel );
    int sAveApplyLocal( const qint16 *d_ic, int ic );
    void sAveApplyGlobal(
        const ShankMap  &SM,
        qint16          *d,
        int             ntpts,
        int             nC,
        int             nAP,
        int             dwnSmp );
    void sAveApplyGlobalStride(
        const ShankMap  &SM,
        qint16          *d,
        int             ntpts,
        int             nC,
        int             nAP,
        int             stride,
        int             dwnSmp );

private:
    void initGraphs();

    void pageChange( int first, bool internUpdateTimes = true );
    void setGraphTimeSecs();
    void update_ic2iy( int first );
};

#endif  // SVGRAFSM_H


