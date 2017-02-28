#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

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

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct FVLink {
    FileViewerWindow*   win[3];     // ap, lf, ni
    QString             runName;    // subtype removed
    bool                linked;

    FVLink()        {zero();}
    FVLink( QString &s, FileViewerWindow *w, int fType )
        {zero(); runName=s; win[fType]=w;}
    int winCount()  {return (win[0]!=0) + (win[1]!=0) + (win[2]!=0);}
private:
    void zero()     {win[0]=0; win[1]=0; win[2]=0; linked=false;}
};


class FileViewerWindow : public QMainWindow
{
    Q_OBJECT

    friend class FVScanGrp;

private:
    struct SaveSet {
        double  fArrowKey,
                fPageKey,
                xSpan,
                ySclImAp,
                ySclImLf,
                ySclNiNeu,
                ySclAux;
        int     yPix,
                nDivs,
                sAveRadIm,
                sAveRadNi;
        bool    sortUserOrder,
                bp300HzNi,
                dcChkOnImAp,
                dcChkOnImLf,
                dcChkOnNi,
                binMaxOnIm,
                binMaxOnNi,
                manualUpdate;

        SaveSet() : fArrowKey(0.1), fPageKey(0.5)   {}
    };

    struct GraphParams {
        // Copiable to other graphs of same type
        double  gain;

        GraphParams() : gain(1.0)   {}
    };

    class DCAve {
    private:
        QVector<float>  sum;
        int             nC,
                        nN;
        bool            lvlOk;
    public:
        QVector<int>    lvl;
    public:
        void init( int nChannels, int nNeural );
        void updateLvl(
            const qint16    *d,
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
    QAction                 *linkAction,
                            *exportAction;
    TaggableLabel           *closeLbl;
    QTimer                  *hideCloseTimer;
    QVector<MGraphY>        grfY;
    QVector<GraphParams>    grfParams;          // per-graph params
    QVector<QAction*>       grfActShowHide;
    QVector<int>            order2ig,           // sort order
                            ig2AcqChan;
    QBitArray               grfVisBits;
    QVector<QVector<int> >  TSM;
    int                     fType,              // {0=imap, 1=imlf, 2=ni}
                            igSelected,         // if >= 0
                            igMaximized,        // if >= 0
                            igMouseOver,        // if >= 0
                            nSpikeChans,
                            nNeurChans;
    bool                    didLayout,
                            selDrag,
                            zoomDrag;

    static QVector<FVLink>  vlnk;

public:
    FileViewerWindow();
    virtual ~FileViewerWindow();

    bool viewFile( const QString &fname, QString *errMsg );

    // Return currently open (.bin) path or null
    QString file() const;

// Toolbar
    double tbGetfileSecs() const;
    double tbGetxSpanSecs() const   {return sav.xSpan;}
    double tbGetyScl() const
        {
            switch( fType ) {
                case 0:  return sav.ySclImAp;
                case 1:  return sav.ySclImLf;
                default: return sav.ySclNiNeu;
            }
        }
    int     tbGetyPix() const       {return sav.yPix;}
    int     tbGetNDivs() const      {return sav.nDivs;}
    int     tbGetSAveRad() const
        {
            switch( fType ) {
                case 0:  return sav.sAveRadIm;
                case 1:  return 0;
                default: return sav.sAveRadNi;
            }
        }
    bool    tbGet300HzOn() const
        {
            switch( fType ) {
                case 2:  return sav.bp300HzNi;
                default: return false;
            }
        }
    bool    tbGetDCChkOn() const
        {
            switch( fType ) {
                case 0:  return sav.dcChkOnImAp;
                case 1:  return sav.dcChkOnImLf;
                default: return sav.dcChkOnNi;
            }
        }
    bool    tbGetBinMaxOn() const
        {
            switch( fType ) {
                case 0:  return sav.binMaxOnIm;
                case 1:  return false;
                default: return sav.binMaxOnNi;
            }
        }

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
    void tbSAveRadChanged( int radius );
    void tbDcClicked( bool b );
    void tbBinMaxClicked( bool b );
    void tbApplyAll();

// FVW_MapDialog
    void cmDefaultBut();
    void cmMetaBut();
    void cmApplyBut();

private slots:
// Menu
    void file_Link();
    void file_ChanMap();
    void file_ZoomIn();
    void file_ZoomOut();
    void file_Options();
    void file_Notes();
    void channels_ShowAll();
    void channels_HideAll();
    void channels_Edit();

// CloseLabel
    void hideCloseLabel();
    void hideCloseTimeout();

// Export
    void doExport();

// Mouse
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

protected:
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

    void linkMenuChanged( bool linked );

private:
// Data-independent inits
    void initMenus();
    void initExport();
    void initCloseLbl();
    void initDataIndepStuff();

// Data-dependent inits
    bool openFile( const QString &fname, QString *errMsg );
    void initHipass();
    void killShowHideAction( int i );
    void killActions();
    void initGraphs();

    void loadSettings();
    void saveSettings() const;

    qint64 nScansPerGraph() const;
    void updateNDivText();

    double scalePlotValue( double v );

    QString nameGraph( int ig ) const;
    void hideGraph( int ig );
    void showGraph( int ig );
    void selectGraph( int ig, bool updateGraph = true );
    void toggleMaximized();
    void sAveTable( int radius );
    int s_t_Ave( const qint16 *d_ig, int ig );
    void updateXSel();
    void zoomTime();
    void updateGraphs();

    void printStatusMessage();
    bool queryCloseOK();

// Stream linking
    FVLink* linkFindMe();
    FVLink* linkFindRunName( const QString &runName );
    bool linkOpenName( const QString &name, QPoint &corner );
    void linkAddMe( QString runName );
    void linkRemoveMe();
    void linkSetLinked( FVLink *L, bool linked );
    void linkSendPos( int fChanged );
    void linkSendSel();
    void linkSendManualUpdate( bool manualUpdate );
};

#endif  // FILEVIEWERWINDOW_H


