#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

#include "DataFile.h"

#include <QMainWindow>
#include <QSet>

class GLGraph;
class GLGraphX;
class Biquad;
class ExportCtl;
class TaggableLabel;

class QScrollArea;
class QSlider;
class QFrame;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FileViewerWindow : public QMainWindow
{
    Q_OBJECT

    friend class ExportCtl;

private:
    enum ColorScheme {
        Ice             = 0,
        Fire,
        Green,
        BlackWhite,
        Classic,
        N_ColorScheme,
        DefaultScheme   = Classic
    };

    enum ViewMode {
        Tiled           = 0,
        Stacked,
        StackedLarge,
        StackedHuge,
        N_ViewMode
    };

    struct SaveSet {
        double      fArrowKey,
                    fPageKey,
                    xSpan,
                    yScale;     // no GUI control for this
        int         nDivs;
        ColorScheme colorScheme;
        ViewMode    viewMode;
        bool        sortUserOrder;

        SaveSet()
        : fArrowKey(0.1), fPageKey(0.5) {}
    };

    struct GraphParams {
        // Copiable to other graphs of same type
        double  gain;
        int     niType;         // CniCfg::niTypeId
        bool    filter300Hz,
                dcFilter;

        GraphParams()
        : gain(1.0), filter300Hz(false), dcFilter(false) {}
    };

    static const QString    colorSchemeNames[];
    static const QString    viewModeNames[];

    SaveSet                 sav;
    DataFile                dataFile;
    double                  tMouseOver,
                            yMouseOver;
    qint64                  pos,                // (scans, up to 9E18)
                            pscale,             // QSlider scaling factor
                            dfCount,
                            dragAnchor,
                            dragL,              // or -1
                            dragR;
    Biquad                  *hipass;
    ExportCtl               *exportCtl;
    QMenu                   *viewMenu,
                            *channelsMenu;
    QToolBar                *toolBar;
    QScrollArea             *scrollArea;
    QWidget                 *graphParent,
                            *framePoolParent,   // owns hidden frames
                            *sliderGrp;
    QSlider                 *slider;
    QAction                 *colorSchemeActions[N_ColorScheme],
                            *viewModeActions[N_ViewMode],
                            *sortUsrAct,
                            *sortAcqAct,
                            *exportAction;
    TaggableLabel           *closeLbl;
    QTimer                  *hideCloseTimer;
    QVector<QFrame*>        grfFrames;
    QVector<GLGraph*>       grf;
    QVector<GraphParams>    grfParams;          // per-graph params
    QVector<QAction*>       grfActShowHide;
    QVector<int>            order2ig,           // sort order
                            ig2AcqChan;
    QSet<QFrame*>           framePool;          // frames from prev view
    ChanMap                 chanMap;
    QBitArray               grfVisBits;
    int                     igSelected,         // if >= 0
                            igMaximized,        // if >= 0
                            igMouseOver;        // if >= 0
    bool                    didLayout,
                            dragging;

public:
    FileViewerWindow();
    virtual ~FileViewerWindow();

    // Ok to call it multiple times to open new files using same window.
    bool viewFile( const QString &fileName, QString *errMsg_out = 0 );

    // Return currently open (.bin) path or null
    QString file() const
    {
        if( dataFile.isOpen() )
            return dataFile.fileName();

        return QString::null;
    }

protected:
    virtual void resizeEvent( QResizeEvent *e );
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private slots:
// Menu
    void file_Open();
    void file_Options();
    void color_SelectScheme();
    void view_sortAcq();
    void view_sortUsr();
    void view_SelectMode();
    void channels_ShowAll();

// Toolbar
    void scrollToSelected();
    void setXScale( double d );
    void setYScale( double d );
    void setNDivs( int n );
    void setMuxGain( double d );
    void hpfChk( bool b );
    void dcfChk( bool b );
    void applyAll();

// Slider
    void sliderPosSBChanged( double p );
    void sliderSecSBChanged( double s );
    void sliderChanged( int i );

// CloseLabel
    void hideCloseLabel();
    void hideCloseTimeout();

// Export
    void doExport();

// Mouse
    void mouseOverGraph( double x, double y );
    void clickGraph( double x, double y );
    void dragDone();
    void dblClickGraph( double x, double y );
    void mouseOverLabel( int x, int y );

// Actions
    void showHideGraphSlot();
    void cursorHere( QPoint p );
    void clickedCloseLbl();

// Timer targets
    void layoutGraphs();

private:
// Data-independent inits
    void initMenus();
    void initToolbar();
    QWidget *initSliderGrp();
    void initCloseLbl();
    void initExport();
    void initDataIndepStuff();

// Data-dependent inits
    bool openFile( const QString &fname, QString *errMsg );
    void applyStyles();
    void setToolbarRanges();
    void initHipass();
    void killShowHideAction( int i );
    void putFrameIntoPool( int i );
    bool getFrameFromPool(
        QFrame*     &f,
        GLGraph*    &G,
        GLGraphX*   &X );
    void create1NewFrame(
        QFrame*     &f,
        GLGraph*    &G,
        GLGraphX*   &X );
    bool cacheFrames_killActions( QString *errMsg );
    bool initFrames_initActions( QString *errMsg );

    void loadSettings();
    void saveSettings() const;

    qint64 nScansPerGraph() const;
    void updateNDivText();

    double scalePlotValue( double v );
    double timeFromPos( qint64 p ) const;
    qint64 posFromTime( double s ) const;
    qint64 maxPos() const;
    void setFilePos64( qint64 newPos );
    void setSliderPos( qint64 newPos );
    void setSliderRanges();
    void updateSliderTexts();

    QString nameGraph( int ig ) const;
    void hideGraph( int ig );
    void showGraph( int ig );
    void DrawSelected( int ig, bool selected );
    void selectGraph( int ig );
    void toggleMaximized();
    void updateSelection( int nG, int graphSpan );
    void updateGraphs();

    void setStackSizing();
    void applyColorScheme( int ig );
    void printStatusMessage();
    bool queryCloseOK();
};

#endif  // FILEVIEWERWINDOW_H


