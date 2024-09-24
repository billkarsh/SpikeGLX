#ifndef MGRAPH_H
#define MGRAPH_H

#include "Vec2.h"
#include "WrapBuffer.h"

#include <QMutex>
#include <QAbstractScrollArea>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

#include <deque>

class MGraph;
class MGScroll;

#undef max  // inherited from WinDef.h via QGLWidget

/* ---------------------------------------------------------------- */
/* EvtSpan -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct EvtSpan {
    double  start,
            end;
    EvtSpan() : start(0), end(-1) {}
    EvtSpan( double start, double end ) : start(start), end(end) {}
};

/* ---------------------------------------------------------------- */
/* MGraphY -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Generally, client owns these and the MGraph package just
// points to the active ones.
//
class MGraphY
{
public:
    double          yscl;
    WrapT<float>    yval,
                    yval2;          // used for binMax
    QString         lhsLabel,
                    rhsLabel;
    int             usrChan,
                    anarow,
                    anaclr,
                    iclr;
    qint16          usrType,
                    anashank;
    bool            drawBinMax,
                    isDigType;

public:
    MGraphY();

    void erase();
    void resize( int n );
};

/* ---------------------------------------------------------------- */
/* MGraphX -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class MGraphX
{
    friend class MGraph;

private:
    std::vector<Vec2f>      verts,
                            verts2x;    // used for binMax
    std::vector<quint8>     vdigclr;
// use setters for grid members
    std::vector<Vec2f>      gridVs;
    int                     nVGridLines,
                            dwnSmp;

public:
    std::vector<MGraphY*>   Y;
    std::vector<QColor>     yColor;
    std::deque<EvtSpan>     evQ[4];
    double                  min_x,
                            max_x;
    float                   xSelBegin,
                            xSelEnd;
    MGraph                  *G;
    mutable QMutex          dataMtx,
                            spanMtx,
                            dsmpMtx;
    QColor                  bkgnd_Color,
                            grid_Color,
                            label_Color;
    int                     ySel,
                            fixedNGrf,
                            ypxPerGrf,
                            clipTop;
    ushort                  gridStipplePat;
    bool                    drawCursor,
                            isXsel;

public:
    MGraphX();

    void attach( MGraph *newG );
    void detach()   {attach( 0 );}

    void initVerts( int n );
    void setSpanSecs( double t, double srate );
    double inline spanSecs() const  {return max_x - min_x;}
    int nDwnSmp()   {QMutexLocker ml( &dsmpMtx ); return dwnSmp;}

    void setVGridLines( int n );
    void setVGridLinesAuto();

    void setClipTop( int top )      {clipTop=top;}
    void calcYpxPerGrf();
    void setYSelByUsrChan( int usrChan );
    int getSelY0();

    void setXSelRange( float begin_x, float end_x );
    void setXSelEnabled( bool onoff );
    bool isXSelVisible() const;
    void getXSelVerts( float v[8] ) const;

    void evQReset();
    void evQExtendLast( double end, double minSecs, int clr );

    void applyGLBkgndClr() const;
    void applyGLGridClr() const;
    void applyGLLabelClr() const;
    void applyGLTraceClr( int iy ) const;
};

/* ---------------------------------------------------------------- */
/* MGraph --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class MGraph : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    friend class MGScroll;

private:
    struct shrRef {
        // this many graphs using this shared context
        MGraph  *gShr;
        int     nG;
        shrRef() : gShr(0), nG(1) {}
        shrRef( MGraph *G ) : gShr(G), nG(1) {}
    };

private:
    static QMap<QString,shrRef>  usr2Ref;

    QString     usr;
    MGraphX     *X;
    bool        ownsX;

public:
    MGraph( const QString &usr, QWidget *parent = 0, MGraphX *X = 0 );
    virtual ~MGraph();

    void attach( MGraphX *newX );
    void detach();

    MGraphX *getX()     {return X;}

signals:
    // For these:
    // x  is a time value,
    // y  is a graph Y-pos in range [-1,1],
    // iy is the current graph index.
    void cursorOutside();
    void cursorOver( double x, double y, int iy );
    void lbutClicked( double x, double y, int iy );
    void lbutReleased();
    void rbutClicked( double x, double y, int iy );
    void rbutReleased();
    void lbutDoubleClicked( double x, double y, int iy );

    // For these:
    // x  is in range [0,width()],
    // y  is in range [0,ypxPerGrf],
    // iy is the current graph index.
    void cursorOverWindowCoords( int x, int y, int iy );
    void lbutClickedWindowCoords( int x, int y, int iy );
    void rbutClickedWindowCoords( int x, int y, int iy );
    void lbutDoubleClicked( int x, int y, int iy );

public slots:
    void updateNow()    {update();}

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

    void leaveEvent( QEvent *evt );
    void mouseMoveEvent( QMouseEvent *evt );
    void mousePressEvent( QMouseEvent *evt );
    void mouseReleaseEvent( QMouseEvent *evt );
    void mouseDoubleClickEvent( QMouseEvent *evt );

private:
    const MGraph *getShr( const QString &usr );
    int evtCoords( double &x, double &y, QMouseEvent *evt, int ny );
    void win2LogicalCoords( double &x, double &y, int iy );
    void drawBaselines();
    void drawGrid();
    void drawLabels();
    void drawYSel();
    void drawXSel();
    void drawEvents();
    void draw1Digital( int iy );
    void draw1BinMax( int iy );
    void draw1Analog( int iy );
    void drawPointsMain();

    void clipToView( int *view );

    void renderTextWin(
        int             x,
        int             y,
        const QString   &str,
        const QFont     &font = QFont() );

    void renderTextMdl(
        double          x,
        double          y,
        double          z,
        const QString   &str,
        const QFont     &font = QFont() );
};

/* ---------------------------------------------------------------- */
/* MGScroll ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class MGScroll : public QAbstractScrollArea
{
    Q_OBJECT

public:
    MGraphX *theX;
    MGraph  *theM;

public:
    MGScroll( const QString &usr, QWidget *parent = 0 );

    void scrollTo( int y );
    void adjustLayout();

public slots:
    void scrollToSelected();

protected:
    virtual void scrollContentsBy( int dx, int dy );
    virtual bool viewportEvent( QEvent *e );
};

#endif  // MGRAPH_H


