#ifndef FILEVIEWERWINDOW_H
#define FILEVIEWERWINDOW_H

#include "DataFileNI.h"

#include <QMainWindow>
#include <QToolBar>

class FileViewerWindow;
class MGraphY;
class MGScroll;
class Biquad;
class ExportCtl;
class TaggableLabel;

class QSlider;
class QFrame;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class FVToolbar : public QToolBar
{
    Q_OBJECT

private:
    FileViewerWindow    *fv;

public:
    FVToolbar( FileViewerWindow *fv ) : fv(fv) {}

    void init();
    void setRanges();

    void setSortButText( const QString &name );
    void setSelName( const QString &name );
    void enableYPix( bool enabled );
    void setYSclAndGain( double &yScl, double &gain, bool enabled );
    void setFltChecks( bool hp, bool dc, bool enabled );
    void setNDivText( const QString &s );
};


// The 'slider group' is a trio of linked controls, with range texts:
//
// [pos^] to X (of Y) [secs^] to X (of Y) [---|------]
//
// The pos spinner allows single scan advance.
// The sec spinner allows millisecond advance.
// The slider allows chunked advance depending on pscale factor.
//
// User changes to one must update the other two, and any change
// requires updating the texts.
//
class FVSliderGrp : public QWidget
{
    Q_OBJECT

private:
    FileViewerWindow    *fv;
    QSlider             *slider;
    qint64              pos,    // range [0..9E18]
                        pscale; // QSlider scaling factor

public:
    FVSliderGrp( FileViewerWindow *fv )
    : fv(fv), pos(0), pscale(1) {init();}

    void init();
    void setRanges( bool newFile );

    const QObject* getSliderObj() const
        {return (QObject*)slider;}

    qint64 getPos() const
        {return pos;}
    double getTime() const
        {return timeFromPos( pos );}

    double timeFromPos( qint64 p ) const;
    qint64 posFromTime( double s ) const;
    qint64 maxPos() const;
    void setFilePos64( qint64 newPos );
    void guiSetPos( qint64 newPos );

private slots:
    void posSBChanged( double p );
    void secSBChanged( double s );
    void sliderChanged( int i );

private:
    void updateTexts();
};


class FileViewerWindow : public QMainWindow
{
    Q_OBJECT

    friend class FVToolbar;
    friend class FVSliderGrp;

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

    struct SaveSet {
        double      fArrowKey,
                    fPageKey,
                    xSpan,
                    ySclNeu,
                    ySclAux;
        int         yPix,
                    nDivs;
        ColorScheme colorScheme;
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

    FVToolbar               tbar;
    FVSliderGrp             *sliderGrp;
    SaveSet                 sav;
    DataFileNI              df;
    double                  tMouseOver,
                            yMouseOver;
    qint64                  dfCount,
                            dragAnchor,
                            dragL,              // or -1
                            dragR;
    Biquad                  *hipass;
    ExportCtl               *exportCtl;
    QMenu                   *channelsMenu;
    MGScroll                *mscroll;
    QAction                 *colorSchemeActions[N_ColorScheme],
                            *exportAction;
    TaggableLabel           *closeLbl;
    QTimer                  *hideCloseTimer;
    QVector<MGraphY>        grfY;
    QVector<GraphParams>    grfParams;          // per-graph params
    QVector<QAction*>       grfActShowHide;
    QVector<int>            order2ig,           // sort order
                            ig2AcqChan;
    ChanMapNI               chanMap;
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
        if( df.isOpen() )
            return df.binFileName();

        return QString::null;
    }

// Export
    void getInverseNiGains(
        std::vector<double> &invGain,
        const QBitArray     &exportBits ) const;

private slots:
// Menu
    void file_Open();
    void file_Options();
    void channels_ShowAll();
    void color_SelectScheme();

// Toolbar
    void toggleSort();
    void setXScale( double d );
    void setYPix( int n );
    void setYScale( double d );
    void setNDivs( int n );
    void setMuxGain( double d );
    void hipassClicked( bool b );
    void dcClicked( bool b );
    void applyAll();

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

protected:
    virtual bool eventFilter( QObject *obj, QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
// Data-independent inits
    void initMenus();
    void initExport();
    void initCloseLbl();
    void initDataIndepStuff();

// Data-dependent inits
    bool openFile( const QString &fname, QString *errMsg );
    void applyStyles();
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
    void updateXSel( int graphSpan );
    void updateGraphs();

    void applyColorScheme( int ig );
    void printStatusMessage();
    bool queryCloseOK();
};

#endif  // FILEVIEWERWINDOW_H


