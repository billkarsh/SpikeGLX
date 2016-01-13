#ifndef MGRAPH_H
#define MGRAPH_H

#include "Vec2.h"
#include "WrapBuffer.h"

#include <QString>
#include <QColor>
#include <QMap>
#include <QAbstractScrollArea>

#ifdef OPENGL54
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#else
#include <QGLWidget>
#endif

class MGraph;
class MGScroll;
class QMutex;

#undef max  // inherited from WinDef.h via QGLWidget

/* ---------------------------------------------------------------- */
/* MGraphY -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Generally, client owns these and the MGraph package just
// points to the active ones.
//
// BK: It may be that digital channel handling is quite
// different, and we really want MGraphX to have one list
// each of analog and digital chans; only one non-empty.
//
class MGraphY
{
public:
    double          yscl;
    WrapT<float>    yval;
    QString         label;
    int             iclr,
                    num;
    bool            isDigType;

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

public:
    enum GrafCoordMode {
        grfReportXAve       = 0,
        grfReportXStream    = 1
    };

private:
    QVector<Vec2f>      verts;
// use setters for grid members
    QVector<Vec2f>      gridVs;
    int                 nVGridLines;

public:
    QVector<MGraphY*>   Y;
    QVector<QColor>     yColor;
    double              min_x,
                        max_x;
    float               xSelBegin,
                        xSelEnd;
    MGraph              *G;
    QMutex              *dataMtx;
    QColor              bkgnd_Color,
                        grid_Color,
                        label_Color;
    int                 dwnSmp,
                        ySel,
                        ypxPerGrf,
                        clipTop,
                        clipHgt;
    ushort              gridStipplePat;
    GrafCoordMode       rptMode;
    bool                drawCursor,
                        isXsel;

public:
    MGraphX();
    virtual ~MGraphX();

    void attach( MGraph *newG );
    void detach()   {attach( 0 );}

    void initVerts( int n );
    void setSpanSecs( double t, double srate );
    double inline spanSecs() const          {return max_x - min_x;}

    void setVGridLines( int n );
    void setVGridLinesAuto();

    void setYClip( int top, int height )    {clipTop=top; clipHgt=height;}
    void setYSelByNum( int num );
    int getSelY0();

    void setXSelRange( float begin_x, float end_x );
    void setXSelEnabled( bool onoff );
    bool isXSelVisible() const;
    void getXSelVerts( float v[8] ) const;

    void applyGLBkgndClr() const;
    void applyGLGridClr() const;
    void applyGLLabelClr() const;
    void applyGLTraceClr( int iy ) const;
};

/* ---------------------------------------------------------------- */
/* MGraph --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#ifdef OPENGL54
class MGraph : public QOpenGLWidget, protected QOpenGLFunctions
#else
class MGraph : public QGLWidget
#endif
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
    bool        ownsX,
                inited,
                immed_update,
                need_update;

public:
    MGraph( const QString &usr, QWidget *parent = 0, MGraphX *X = 0 );
    virtual ~MGraph();

    void attach( MGraphX *newX );
    void detach();

    MGraphX *getX()     {return X;}

    void setImmedUpdate( bool b ) {immed_update = b;}
    bool needsUpdateGL() const {return need_update;}

signals:
    // For all the below: x is a time value,
    // y is a graph Y-pos in range [-1,1].
    // iy is the current graph index.
    void cursorOver( double x, double y, int iy );
    void lbutClicked( double x, double y, int iy );
    void lbutReleased();
    void rbutClicked( double x, double y, int iy );
    void rbutReleased();
    void lbutDoubleClicked( double x, double y, int iy );

    // Similar, using window instead of graph coordinates.
    void cursorOverWindowCoords( int x, int y, int iy );
    void lbutClickedWindowCoords( int x, int y, int iy );
    void rbutClickedWindowCoords( int x, int y, int iy );
    void lbutDoubleClicked( int x, int y, int iy );

public slots:
#ifdef OPENGL54
    void updateNow()    {update();}
#else
    void update() {if(immed_update) updateGL(); else need_update=true;}
    void updateNow()    {updateGL();}
#endif

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

    void mouseMoveEvent( QMouseEvent *evt );
    void mousePressEvent( QMouseEvent *evt );
    void mouseReleaseEvent( QMouseEvent *evt );
    void mouseDoubleClickEvent( QMouseEvent *evt );

private:
    const MGraph *getShr( const QString &usr );
    void win2LogicalCoords( double &x, double &y, int iy );
    void drawBaselines();
    void drawGrid();
    void drawLabels();
    void drawYSel();
    void drawXSel();
    void draw1Analog( int iy );
    void drawPointsMain();
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
    virtual void resizeEvent( QResizeEvent *e );
    virtual void scrollContentsBy( int dx, int dy );
    virtual bool viewportEvent( QEvent *e );
};

#endif  // MGRAPH_H


