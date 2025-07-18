#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

#include "DFName.h"
#include "CAR.h"
#include "GraphStats.h"
#include "SvyPrb.h"

#include <QMainWindow>
#include <QMutex>

class FileViewerWindow;
class FVToolbar;
class FVScanGrp;
class DataFile;
class DataSource;
class FVShankCtl;
struct ShankMap;
struct ChanMap;
class MGraphY;
class MGScroll;
class Biquad;
class ExportCtl;
class TaggableLabel;

class QComboBox;
class QThread;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// The set of DSXXX classes are experiments seeking
// faster data loading, especially over a network.
// To date, the original DSDirect method is fatest,
// smallest mem, simplest.

//#define DSMapAll
//#define DSMapped
//#define DSBuffered
#define DSDirect

#ifdef DSMapAll
class DataSource {
private:
    qint64      flen;
    uchar*      b;
    DataFile    *df;
    QFile       f;
    int         nC;
public:
    DataSource() : b(0)     {}
    virtual ~DataSource()   {if( b ) f.unmap( b );}
    void set_df( DataFile *df );
    int read( vec_i16 &dst, qint64 smp0, int nsmp );
};
#endif

#ifdef DSMapped
class DataSource {
private:
    qint64      buf0,
                flen,
                sprv;
    uchar*      b;
    DataFile    *df;
    QFile       f;
    int         chunk,
                gran,
                mapx,
                blen,
                trig,   // when to reload
                nC;
public:
    DataSource() : b(0)     {}
    virtual ~DataSource()   {if( b ) f.unmap( b );}
    void set_df( DataFile *df );
    int read( vec_i16 &dst, qint64 smp0, int nsmp );
private:
    void map( qint64 s );
};
#endif

#ifdef DSBuffered
class DSWorker : public QObject
{
    Q_OBJECT
private:
    DataSource  &DS;
    qint64     smp0;
public:
    DSWorker( DataSource &DS, qint64 smp0 )
        :   QObject(0), DS(DS), smp0(smp0)  {}
signals:
    void finished();
public slots:
    void run();
};

class DSThread
{
public:
    QThread     *thread;
    DSWorker    *worker;
public:
    DSThread( DataSource &DS, qint64 smp0 );
    virtual ~DSThread();
};

class DataSource {
    friend class DSWorker;
private:
    qint64      buf0,
                flen,
                sprv;
    DataFile    *df;
    DSThread    *T;
    QFile       f;
    QMutex      bmMtx;
    vec_i16     bufm,   // mem shared
                buff;   // file
    int         blen,
                trig,   // when to reload
                nC;
public:
    DataSource() : T(0)     {}
    virtual ~DataSource()   {if( T ) delete T;}
    void set_df( DataFile *df );
    int read( vec_i16 &dst, qint64 smp0, int nsmp );
private:
    void load( qint64 newbuf0 );
};
#endif

#ifdef DSDirect
class DataSource {
private:
    QBitArray   BA;
    DataFile    *df;
public:
    void set_df( DataFile *df ) {this->df = df;}
    int read( vec_i16 &dst, qint64 smp0, int nsmp );
};
#endif

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

enum fvTypeID {
    fvAP    = 0,
    fvLF    = 1,
    fvOB    = 2,
    fvNI    = 3
};

class FileViewerWindow : public QMainWindow
{
    Q_OBJECT

    friend class FVScanGrp;

private:
    struct SaveAll {
        double  fArrowKey,
                fPageKey,
                fOffset,
                xSpan,
                ySclAux;
        int     nDivs;
        bool    sortUserOrder,
                manualUpdate;

        SaveAll() : fArrowKey(0.1), fPageKey(0.5), fOffset(0)   {}

        void loadSettings( QSettings &S, double minSpan );
        void saveSettings( QSettings &S ) const;
    };

    struct SaveIm {
        double  ySclAp,
                ySclLf;
        int     yPixAp,
                yPixLf,
                bandSel,    // {0=off, 1=AP, 2=LF}
                sAveSel,    // {0=off, 1=lcl, 2=lcl', 3=gbl, 4=dmx}
                binMax;     // {0=Off, 1=slow, 2=fast, 3=faster}
        bool    tnChkOnAp,
                tnChkOnLf;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S, int fType ) const;
    };

    struct SaveOb {
        int     yPix;
        bool    txChkOn;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
    };

    struct SaveNi {
        double  ySclNeu;
        int     yPix,
                bandSel,    // {0=off, 1=AP, 2=LF}
                sAveSel,    // {0=off, 1=lcl, 2=lcl', 3=gbl, 4=dmx}
                binMax;     // {0=Off, 1=slow, 2=fast, 3=faster}
        bool    tnChkOn,
                txChkOn;

        void loadSettings( QSettings &S );
        void saveSettings( QSettings &S ) const;
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
                            i0,
                            nI,
                            maxInt;
    public:
        std::vector<int>    lvl;
    public:
        void init( int nChannels, int c0, int cLim, int maxInt );
        void updateLvl(
            const DataFile  *df,
            Biquad          *hipass,
            qint64          xpos,
            int             nRem,
            int             chunk,
            int             dwnSmp );
        void apply(
            qint16          *d,
            int             ntpts,
            int             dwnSmp );
    };

    FVToolbar               *tbar;
    FVScanGrp               *scanGrp;
    SaveSet                 sav;
    DCAve                   Tn,
                            Tx;
    QString                 sTitleNoTime,
                            cmChanStr;
    double                  tMouseOver,
                            yMouseOver;
    qint64                  dfCount,
                            dragAnchor,
                            dragL,              // or -1
                            dragR,
                            savedDragL,         // zoom: temp save sel
                            savedDragR;
    DataSource              DS;
    SvyVSBTT                SVY;
    DataFile                *df;
    FVShankCtl              *shankCtl;
    ShankMap                *shankMap;
    ChanMap                 *chanMap;
    Biquad                  *hipass,
                            *lopass;
    ExportCtl               *exportCtl;
    QMenu                   *channelsMenu;
    MGScroll                *mscroll;
    TaggableLabel           *closeLbl;
    QTimer                  *hideCloseTimer;
    CAR                     car;
    std::vector<MGraphY>    grfY;
    std::vector<GraphParams>grfParams;          // per-graph params
    std::vector<GraphStats> grfStats;           // per-graph voltage stats
    std::vector<QMenu*>     chanSubMenus;
    std::vector<QAction*>   grfActShowHide;
    std::vector<double>     svySpikes,
                            svyAPPkPk,
                            svyLFPkPk;
    QVector<int>            order2ig,           // sort order
                            ig2ic,              // saved to acquired
                            ic2ig;              // acq to saved or -1
    QBitArray               grfVisBits;
    int                     fType,              // {0=AP, 1=LF, 2=OB, 3=NI}
                            igSelected,         // if >= 0
                            igMaximized,        // if >= 0
                            igMouseOver,        // if >= 0
                            nSpikeChans,
                            nNeurChans,
                            nAnaChans,
                            curSMap;
    bool                    didLayout,
                            sortingDisabled,
                            selDrag,
                            zoomDrag;

    static std::vector<FVOpen>  vOpen;
    static QSet<QString>        linkedRuns;

public:
    FileViewerWindow();
    virtual ~FileViewerWindow();

    bool viewFile( QString &error, const QString &fname );

    // Return currently open (.bin) path or null
    QString file() const;

// Toolbar
    int tbGetNeurChans() const      {return nNeurChans;}
    int tbGetAnaChans() const       {return nAnaChans;}
    double tbGetfileSecs() const;
    double tbGetxSpanSecs() const   {return sav.all.xSpan;}
    int    tbGetyPix() const
        {
            switch( fType ) {
                case fvAP: return sav.im.yPixAp;
                case fvLF: return sav.im.yPixLf;
                case fvOB: return sav.ob.yPix;
                case fvNI: return sav.ni.yPix;
            }
        }
    double tbGetyScl() const
        {
            switch( fType ) {
                case fvAP: return sav.im.ySclAp;
                case fvLF: return sav.im.ySclLf;
                default:   return sav.all.ySclAux;
            }
        }
    int     tbGetNDivs() const      {return sav.all.nDivs;}
    int     tbGetBandSel() const
        {
            switch( fType ) {
                case fvAP: return sav.im.bandSel;
                case fvNI: return sav.ni.bandSel;
                default:   return 0;
            }
        }
    int     tbGetSAveSel() const
        {
            switch( fType ) {
                case fvAP: return sav.im.sAveSel;
                case fvNI: return sav.ni.sAveSel;
                default:   return 0;
            }
        }
    bool    tbGetTnChkOn() const
        {
            switch( fType ) {
                case fvAP: return sav.im.tnChkOnAp;
                case fvLF: return sav.im.tnChkOnLf;
                case fvOB: return false;
                case fvNI: return sav.ni.tnChkOn;
            }
        }
    bool    tbGetTxChkOn() const
        {
            switch( fType ) {
                case fvAP:
                case fvLF: return false;
                case fvOB: return sav.ob.txChkOn;
                case fvNI: return sav.ni.txChkOn;
            }
        }
    int     tbGetBinMax() const
        {
            switch( fType ) {
                case fvAP: return sav.im.binMax;
                case fvNI: return sav.ni.binMax;
                default:   return 0;
            }
        }
    void tbNameLocalFilters( QComboBox *CB );

// Export
    double getFOffset() const   {return sav.all.fOffset;}
    void getInverseGains(
        std::vector<double> &invGain,
        const QBitArray     &exportBits ) const;

// ShankView
    bool isSvy() const      {return SVY.nmaps > 0;}
    int svyMaxbank() const  {return SVY.maxbank;}
    const double* svyAllBanks(
        int     what,
        int     T,
        int     inarow,
        int     maxrow,
        bool    gbldmx );
    void svyScrollToShankBank( int shank, int bank );

public slots:
// Toolbar
    void tbToggleSort();
    void tbShowShanks();
    void tbScrollToSelected();
    void tbSetXScale( double d );
    void tbSetYPix( int n );
    void tbSetYScale( double d );
    void tbSetMuxGain( double d );
    void tbSetNDivs( int n );
    void tbTnClicked( bool b );
    void tbTxClicked( bool b );
    void tbBandSelChanged( int sel, bool update = true );
    void tbSAveSelChanged( int sel );
    void tbBinMaxChanged( int sel );
    void tbApplyAll();

// FVW_MapDialog
    void cmDefaultBut();
    void cmMetaBut();
    void cmApplyBut();

// ShankView
    void editSave( QString sInit );
    void gbldmxChanged();
    void maxrowChanged();
    void feedShankCtl( bool needMap = false );
    void colorTraces();
    void externSelectChan( int ig );

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
    void file_Meta();
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
    bool openFile( QString &error, const QString &fname );
    void killActions();
    void initGraphs();

    void loadSettings();
    void saveSettings() const;

    int nSampsPerGraph() const;
    void updateNDivText();

    QString nameGraph( int ig ) const;
    void hideGraph( int ig );
    void showGraph( int ig );
    void selectGraph( int ig, bool updateGraph = true );
    void selectShankMap( qint64 pos );
    void shankMapChanged();
    void toggleMaximized();
    void sAveTable( int sel );
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


