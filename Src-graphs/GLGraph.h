#ifndef GLGRAPH_H
#define GLGRAPH_H

#include "Vec2.h"
#include "WrapBuffer.h"

#include <QColor>
#include <QMap>

#ifdef OPENGL54
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#else
#include <QGLWidget>
#endif

class GLGraph;
class QMutex;

#undef max  // inherited from WinDef.h via QGLWidget

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class GLGraphState
{
    friend class GLGraph;

public:
    enum GrafCoordMode {
        grfReportXAve       = 0,
        grfReportXStream    = 1
    };

private:
    QVector<Vec2f>      verts;
// use setters for grid members
    QVector<Vec2f>      gridHs,
                        gridVs;
    int                 nHGridLines,
                        nVGridLines;

public:
    double              min_x,
                        max_x,
                        yscale;
    float               selectionBegin,
                        selectionEnd;
    GLGraph             *G;
    QMutex              *dataMtx;
    WrapT<float>        ydata;
    QColor              bkgnd_Color,
                        grid_Color,
                        trace_Color;
    int                 dwnSmp,
                        num;        // caller-defined
    ushort              gridStipplePat;
    GrafCoordMode       rptMode;
    bool                isDigChanType,
                        drawCursor,
                        hasSelection;

public:
    GLGraphState();
    virtual ~GLGraphState();

    void attach( GLGraph *newG );
    void detach()   {attach( 0 );}

    void initVerts( int n );
    void setSpanSecs( double t, double srate );
    double inline spanSecs() const  {return max_x - min_x;}

    void setHGridLines( int n );
    void setVGridLines( int n );
    void setVGridLinesAuto();

    void setSelRange( float begin_x, float end_x );
    void setSelEnabled( bool onoff );
    bool isSelVisible() const;
    void getSelVertices( float v[8] ) const;

    void applyGLBkgndClr() const;
    void applyGLGridClr() const;
    void applyGLTraceClr() const;

    QString toString() const;
    void fromString( const QString &s, double srate );
};


#ifdef OPENGL54
class GLGraph : public QOpenGLWidget, protected QOpenGLFunctions
#else
class GLGraph : public QGLWidget
#endif
{
    Q_OBJECT

private:
    struct shrRef {
        // this many graphs using this shared context
        GLGraph *gShr;
        int     nG;
        shrRef() : gShr(0), nG(1) {}
        shrRef( GLGraph *G ) : gShr(G), nG(1) {}
    };

private:
    static QMap<QString,shrRef>  usr2Ref;

    QString         usr;
    GLGraphState    *X;
    bool            ownsX,
                    immed_update,
                    need_update;

public:
    GLGraph( const QString &usr, QWidget *parent = 0, GLGraphState *X = 0 );
    virtual ~GLGraph();

    void attach( GLGraphState *newX );
    void detach();

    GLGraphState *getX()    {return X;}

    void setImmedUpdate( bool b ) {immed_update = b;}
    bool needsUpdateGL() const {return need_update;}

signals:
    // For all the below: x is a time value,
    // y is a graph Y-pos in range [-1,1].
    void cursorOver( double x, double y );
    void lbutClicked( double x, double y );
    void lbutReleased();
    void rbutClicked( double x, double y );
    void rbutReleased();
    void lbutDoubleClicked( double x, double y );

    // Similar, using window instead of graph coordinates.
    void cursorOverWindowCoords( int x, int y );
    void lbutClickedWindowCoords( int x, int y );
    void rbutClickedWindowCoords( int x, int y );
    void lbutDoubleClicked( int x, int y );

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
    const GLGraph *getShr( const QString &usr );
    void win2LogicalCoords( double &x, double &y );
    void drawGrid();
    void drawPointsDigital();
    void drawPointsWiping();
    void drawPointsMain();
    void drawSelection();
};

#endif  // GLGRAPH_H


