#ifndef GRAPHSWINDOW_H
#define GRAPHSWINDOW_H

#include <QMainWindow>

namespace DAQ {
struct Params;
}

class RunToolbar;
class GWSelectWidget;
class GWLEDWidget;
class SVGrafsM;
class ColorTTLCtl;
class SOCtl;

class QSplitter;

/* ---------------------------------------------------------------- */
/* Globals -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

extern const QColor NeuGraphBGColor,
                    LfpGraphBGColor,
                    AuxGraphBGColor,
                    DigGraphBGColor;

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GraphsWindow : public QMainWindow
{
    Q_OBJECT

private:
    static std::vector<QByteArray>  vShankGeom;

private:
    const DAQ::Params   &p;
    RunToolbar          *tbar;  // only main window 0
    GWSelectWidget      *SEL;
    GWLEDWidget         *LED;   // only main window 0
    SVGrafsM            *lW,
                        *rW;
    ColorTTLCtl         *TTLCC;
    SOCtl               *soctl; // only main window 0
    int                 igw;

public:
    GraphsWindow( const DAQ::Params &p, int igw );
    virtual ~GraphsWindow();

    ColorTTLCtl *getTTLColorCtl()   {return TTLCC;}
    SOCtl *getSOCtl()               {return soctl;}

// Panels
    static void setShankGeom( const QByteArray &geom, int jpanel )
        {vShankGeom[jpanel] = geom;}
    void shankCtlWantGeom( int jpanel ) const;

// Run
    void eraseGraphs();

public slots:
// View control
    void initViews();

// Appearance
    void updateRHSFlags();
    void updateProbe( int ip, bool shankMap, bool chanMap );

// Remote
    bool remoteIsUsrOrder( int js, int ip );
    void remoteSetRecordingEnabled( bool on );
    void remoteSetRunLE( const QString &name );
    void remoteSetAnatomyPP( const QString &elems, int ip, int sk );

// Gates/triggers
    void setGateLED( bool on );
    void setTriggerLED( bool on );
    void blinkTrigger();
    void updateOnTime( const QString &s );
    void updateRecTime( const QString &s );
    void updateGT( const QString &s );

// Toolbar
    void tbSetRecordingEnabled( bool checked );

protected:
    virtual bool eventFilter( QObject *watched, QEvent *event );
    virtual void keyPressEvent( QKeyEvent *e );
    virtual void showEvent( QShowEvent *e );
    virtual void hideEvent( QHideEvent *e );
    virtual void changeEvent( QEvent *e );
    virtual void closeEvent( QCloseEvent *e );

private:
    void installLeft( QSplitter *sp );
    bool installRight( QSplitter *sp );
    void initColorTTL();
    void initGFStreams();
    void loadShankScreenState();
    void saveShankScreenState() const;
    void restoreScreenState();
    void saveScreenState() const;
};

#endif  // GRAPHSWINDOW_H


