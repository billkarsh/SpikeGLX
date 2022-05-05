#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

#include "DFName.h"
#include "GraphStats.h"

#include <QMainWindow>
#include <QBitArray>

class FileViewerWindow;
class FVToolbar;
class FVScanGrp;
class DataFile;
struct ShankMap;
struct ChanMap;
class MGraphY;
class MGScroll;
class Biquad;
class ExportCtl;
class TaggableLabel;

class QComboBox;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct FVOpen {
    FileViewerWindow*   fvw;
    DFRunTag            runTag;

    FVOpen()
    :   fvw(0)                  {}
    FVOpen( FileViewerWindow *fvw, const QString &fname )
    :   fvw(fvw), runTag(fname) {}
};

struct FVLinkRec {
    DFRunTag    runTag;
    QBitArray   apBits,
                lfBits,
                obBits;
    int         nProbe,
                nOb;
    bool        openNI,
                close,
                tile;
};


class FileViewerWindow : public QMainWindow
{
    Q_OBJECT

    friend class FVScanGrp;

private:
    struct SaveAll {
        double  fArrowKey,
                fPageKey,
                xSpan,
                ySclAux;
        int     yPix,
                nDivs;
        bool    sortUserOrder,
                manualUpdate;

        SaveAll() : fArrowKey(0.1), fPageKey(0.5)   {}
    };

    struct SaveIm {
        double  ySclAp,
                ySclLf;
        int     sAveSel,    // {0=Off, 1,2=Local, 3,4=Global}
                binMax;
        bool    bp300Hz,
                dcChkOnAp,
                dcChkOnLf;
    };

    struct SaveOb {
        bool    dcChkOn;
    };

    struct SaveNi {
        double  ySclNeu;
        int     sAveSel,    // {0=Off, 1,2=Local, 3,4=Global}
                binMax;
        bool    bp300Hz,
                dcChkOn;
    };

    struct SaveSet {
        SaveAll all;
        SaveIm  im;
        SaveOb  ob;
        SaveNi  ni;
    };

    struct GraphParams {
        // Copiable to other graphs of same type
        double  gain;

        GraphParams() : gain(1.0)   {}
    };

    class DCAve {
    private:
        int                 nC,
                            nN;
    public:
        std::vector<int>    lvl;
    public:
        void init( int nChannels, int nNeural );
        void updateLvl(
            const DataFile  *df,
            qint64          xpos,
            qint64          nRem,
            qint64          chunk,
            int             dwnSmp );
        void apply(
            qint16          *d,
            int             ntpts,
            int             dwnSmp );
    };

    FVToolbar               *tbar;
    FVScanGrp               *scanGrp;
    SaveSet                 sav;
    DCAve                   dc;
    QString                 cmChanStr;
    double                  tMouseOver,
                            yMouseOver;
    qint64                  dfCount,
                            dragAnchor,
                            dragL,              // or -1
                            dragR,
                            savedDragL,         // zoom: temp save sel
                            savedDragR;
    DataFile                *df;
    ShankMap                *shankMap;
    ChanMap                 *chanMap;
    Biquad                  *hipass;
    ExportCtl               *exportCtl;
    QMenu                   *channelsMenu;
    MGScroll                *mscroll;
    TaggableLabel           *closeLbl;
    QTimer                  *hideCloseTimer;
    std::vector<MGraphY>    grfY;
    std::vector<GraphParams>grfParams;          // per-graph params
    std::vector<GraphStats> grfStats;           // per-graph voltage stats
    std::vector<QMenu*>     chanSubMenus;
    std::vector<QAction*>   grfActShowHide;
    QVector<int>            order2ig,           // sort order
                            ig2ic,              // saved to acquired
                            ic2ig;              // acq to saved or -1
    QBitArray               grfVisBits;
    std::vector<std::vector<int> >  TSM;
    std::vector<int>        muxTbl;
    int                     nADC,
                            nGrp,
                            fType,              // {0=AP, 1=LF, 2=OB, 3=NI}
                            igSelected,         // if >= 0
                            igMaximized,        // if >= 0
                            igMouseOver,        // if >= 0
                            nSpikeChans,
                            nNeurChans;
    bool                    didLayout,
                            selDrag,
                            zoomDrag;

    static std::vector<FVOpen>  vOpen;
    static QSet<QString>        linkedRuns;

public:
    FileViewerWindow();
    virtual ~FileViewerWindow();

    bool viewFile( const QString &fname, QString *errMsg );

    // Return currently open (.bin) path or null
    QString file() const;

// Toolbar
    double tbGetfileSecs() const;
    double tbGetxSpanSecs() const   {return sav.all.xSpan;}
    double tbGetyScl() const
        {
            switch( fType ) {
                case 0: return sav.im.ySclAp;
                case 1: return sav.im.ySclLf;
                case 2: return sav.all.ySclAux;
                case 3: return sav.all.ySclAux;
            }
        }
    int     tbGetyPix() const       {return sav.all.yPix;}
    int     tbGetNDivs() const      {return sav.all.nDivs;}
    int     tbGetSAveSel() const
        {
            switch( fType ) {
                case 0: return sav.im.sAveSel;
                case 3: return sav.ni.sAveSel;
                default: return 0;
            }
        }
    bool    tbGet300HzOn() const
        {
            switch( fType ) {
                case 0: return sav.im.bp300Hz;
                case 3: return sav.ni.bp300Hz;
                default: return false;
            }
        }
    bool    tbGetDCChkOn() const
        {
            switch( fType ) {
                case 0: return sav.im.dcChkOnAp;
                case 1: return sav.im.dcChkOnLf;
                case 2: return sav.ob.dcChkOn;
                case 3: return sav.ni.dcChkOn;
            }
        }
    int     tbGetBinMax() const
        {
            switch( fType ) {
                case 0: return sav.im.binMax;
                case 3: return sav.ni.binMax;
                default: return 0;
            }
        }
    void tbNameLocalFilters( QComboBox *CB );

// Export
    void getInverseGains(
        std::vector<double> &invGain,
        const QBitArray     &exportBits ) const;

public slots:
// Toolbar
    void tbToggleSort();
    void tbScrollToSelected();
    void tbSetXScale( double d );
    void tbSetYPix( int n );
    void tbSetYScale( double d );
    void tbSetMuxGain( double d );
    void tbSetNDivs( int n );
    void tbHipassClicked( bool b );
    void tbDcClicked( bool b );
    void tbSAveSelChanged( int sel );
    void tbBinMaxChanged( int n );
    void tbApplyAll();

// FVW_MapDialog
    void cmDefaultBut();
    void cmMetaBut();
    void cmApplyBut();

private slots:
// Menu
    void file_Link();
    void file_Unlink();
    void file_Export();
    void file_ChanMap();
    void file_ZoomIn();
    void file_ZoomOut();
    void file_Options();
    void file_Notes();
    void channels_ShowAll();
    void channels_HideAll();
    void channels_Edit();
    void help_ShowHelp();

// CloseLabel
    void hideCloseLabel();
    void hideCloseTimeout();

// Context menu
    void shankmap_Tog();
    void shankmap_Edit();
    void shankmap_Restore();

// Mouse
    void mouseOutside();
    void mouseOverGraph( double x, double y, int iy );
    void clickGraph( double x, double y, int iy );
    void dragDone();
    void dblClickGraph( double x, double y, int iy );
    void mouseOverLabel( int x, int y, int iy );

// Actions
    void menuShowHideGraph();
    void cursorHere( QPoint p );
    void clickedCloseLbl();

// Timer targets
    void layoutGraphs();

// Stream linking
    void linkRecvPos( double t0, double tSpan, int fChanged );
    void linkRecvSel( double tL, double tR );
    void linkRecvManualUpdate( bool manualUpdate );
    void linkRecvDraw();

protected:
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
// Data-independent inits
    void initMenus();
    void initContextMenu();
    void initCloseLbl();
    void initDataIndepStuff();

// Data-dependent inits
    bool openFile( const QString &fname, QString *errMsg );
    void initHipass();
    void killActions();
    void initGraphs();

    void loadSettings();
    void saveSettings() const;

    qint64 nScansPerGraph() const;
    void updateNDivText();

    QString nameGraph( int ig ) const;
    void hideGraph( int ig );
    void showGraph( int ig );
    void selectGraph( int ig, bool updateGraph = true );
    void toggleMaximized();
    void sAveTable( int sel );
    int sAveApplyLocal( const qint16 *d_ig, int ig );
    void sAveApplyGlobal(
        qint16  *d,
        int     ntpts,
        int     nC,
        int     nAP,
        int     dwnSmp );
    void sAveApplyGlobalStride(
        qint16  *d,
        int     ntpts,
        int     nC,
        int     nAP,
        int     stride,
        int     dwnSmp );
    void sAveApplyDmxTbl(
        qint16  *d,
        int     ntpts,
        int     nC,
        int     nAP,
        int     dwnSmp );
    void updateXSel();
    void zoomTime();
    void updateGraphs();

    double scalePlotValue( double v, double gain );
    void computeGraphMouseOverVars(
        int         ig,
        double      &y,
        double      &mean,
        double      &stdev,
        double      &rms,
        const char* &unit );
    void printStatusMessage();
    bool queryCloseOK();

// Stream linking
    FVOpen* linkFindMe();
    FVOpen* linkFindName(
        const DFRunTag  &runTag,
        int             ip,
        int             fType );
    bool linkIsLinked( const FVOpen *me );
    bool linkIsSameRun( const FVOpen *W, const FVOpen *me );
    bool linkIsSibling( const FVOpen *W, const FVOpen *me );
    int linkNSameRun( const FVOpen *me );
    bool linkOpenName(
        const DFRunTag  &runTag,
        int             ip,
        int             fType,
        QPoint          &corner );
    void linkAddMe( const QString &fname );
    void linkRemoveMe();
    void linkSetLinked( FVOpen *me, bool linked );
    void linkSendPos( int fChanged );
    void linkSendSel();
    void linkSendManualUpdate( bool manualUpdate );
    void linkWhosOpen( FVLinkRec &L );
    bool linkShowDialog( FVLinkRec &L );
    void linkTile( FVLinkRec &L );
    void linkStaticSave();
    void linkStaticRestore( const DFRunTag &runTag );
};

#endif  // FILEVIEWERWINDOW_H


