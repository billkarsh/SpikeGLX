
#include "GLGraph.h"
#include "Util.h"
#include "MainApp.h"
#include "GraphPool.h"

#include <QDesktopWidget>
#include <QMutex>
#include <QPoint>
#include <QMouseEvent>
#include <math.h>

#ifdef Q_WS_MACX
#include <gl.h>
#include <agl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


/* ---------------------------------------------------------------- */
/* GLGraphState --------------------------------------------------- */
/* ---------------------------------------------------------------- */

GLGraphState::GLGraphState()
{
    min_x           = 0.0;
    max_x           = 1.0;
    yscale          = 1.0;
    selectionBegin  = 0.0F;
    selectionEnd    = 0.0F;
    G               = 0;
    dataMtx         = new QMutex;
    bkgnd_Color     = QColor( 0x2f, 0x4f, 0x4f );
    grid_Color      = QColor( 0x87, 0xce, 0xfa, 0x7f );
    trace_Color     = QColor( 0xee, 0xdd, 0x82 );
    dwnSmp          = 1;
    num             = 0;
    gridStipplePat  = 0xf0f0; // 4pix on 4 off 4 on 4 off
    rptMode         = grfReportXAve;
    isDigChanType   = false;
    drawCursor      = true;
    hasSelection    = false;

    setHGridLines( 4 );
    setVGridLinesAuto();
}


GLGraphState::~GLGraphState()
{
    delete dataMtx;
}


void GLGraphState::attach( GLGraph *newG )
{
    G               = newG;
    hasSelection    = false;

    dataMtx->lock();

    ydata.erase();
    ydata.zeroFill();
    initVerts( ydata.capacity() );

    dataMtx->unlock();
}


// This needs to be called whenever the capacity of the data
// buffer is changed. setSpanSecs automatically calls this.
//
void GLGraphState::initVerts( int n )
{
    verts.resize( n );

    for( int i = 0; i < n; ++i )
        verts[i].x = i;
}


// A graph's raw span (in samples) is ordinarily (t * srate).
// However, no graph need store more than points than the
// largest screen dimension (2X oversampling is included).
// Each graph gets a downsample factor (dwnSmp) to moderate
// its storage according to current span.
//
void GLGraphState::setSpanSecs( double t, double srate )
{
    if( t <= 0.0 )
        return;

// force rounding to the GUI time span

    char	buf[64];
    sprintf( buf, "%g", t );
    sscanf( buf, "%lf", &t );

    max_x = min_x + t;

// Init points buffer.

    double  spanSmp = t * srate;
    QRect   rect    = QApplication::desktop()->screenGeometry();
    int     maxDim  = 2 * qMax( rect.width(), rect.height() );

    dwnSmp = int(spanSmp/maxDim);

    if( dwnSmp < 1 )
        dwnSmp = 1;

    uint    newSize = (uint)ceil( spanSmp/dwnSmp );

    dataMtx->lock();

    ydata.resizeAndErase( newSize );
    ydata.zeroFill();
    initVerts( newSize );

    dataMtx->unlock();
}


void GLGraphState::setHGridLines( int n )
{
    nHGridLines = n;

    gridHs.resize( 2 * n );

    for( int i = 0; i < n; ++i ) {

        int     k = 2 * i;
        float   y = float(k) / n - 1.0F;

        gridHs[k].x     = 0.0F;
        gridHs[k].y     = y;

        gridHs[k+1].x   = 1.0F;
        gridHs[k+1].y   = y;
    }
}


void GLGraphState::setVGridLines( int n )
{
    nVGridLines = n;

    gridVs.resize( 2 * n );

    for( int i = 0; i < n; ++i ) {

        float   x = float(i) / n;
        int     k = 2 * i;

        gridVs[k].x     = x;
        gridVs[k].y     = -1.0F;

        gridVs[k+1].x   = x;
        gridVs[k+1].y   = 1.0F;
    }
}


// If span >= 1 sec, set as many gridlines as the characteristic,
// that is, the integer part, so 12.31 gets 12 lines.
//
// For fractions < 1, set as many grid lines as leading digit
// in the fraction, so 0.043 would get 4 lines.
//
void GLGraphState::setVGridLinesAuto()
{
    double  t = spanSecs();

    while( t < 1.0 )
        t *= 10.0;

    setVGridLines( int(t) );
}


void GLGraphState::setSelRange( float begin_x, float end_x )
{
    if( begin_x <= end_x ) {
        selectionBegin   = begin_x;
        selectionEnd     = end_x;
    }
    else {
        selectionBegin   = end_x;
        selectionEnd     = begin_x;
    }
}


void GLGraphState::setSelEnabled( bool onoff )
{
    hasSelection = (rptMode == grfReportXStream) && onoff;
}


bool GLGraphState::isSelVisible() const
{
    return  hasSelection
            && selectionEnd   >= 0.0F
            && selectionBegin <= 1.0F;
}


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
void GLGraphState::getSelVertices( float v[] ) const
{
    v[0] = v[2] = selectionBegin;
    v[4] = v[6] = selectionEnd;
    v[1] = v[7] = -1.0F;
    v[3] = v[5] =  1.0F;
}


void GLGraphState::applyGLBkgndClr() const
{
    glClearColor(
        bkgnd_Color.redF(),  bkgnd_Color.greenF(),
        bkgnd_Color.blueF(), bkgnd_Color.alphaF() );
    glClear( GL_COLOR_BUFFER_BIT );
}


void GLGraphState::applyGLGridClr() const
{
    glColor4f(
        grid_Color.redF(),  grid_Color.greenF(),
        grid_Color.blueF(), grid_Color.alphaF() );
}


void GLGraphState::applyGLTraceClr() const
{
    glColor4f(
        trace_Color.redF(),  trace_Color.greenF(),
        trace_Color.blueF(), trace_Color.alphaF() );
}


QString GLGraphState::toString() const
{
    return QString("fg:%1 xsec:%2 yscl:%3")
            .arg( trace_Color.rgb(), 0, 16 )
            .arg( spanSecs(), 0, 'g' )
            .arg( yscale, 0, 'g' );
}


void GLGraphState::fromString( const QString &s, double srate )
{
    double  x, y;
    uint    c;

    if( 3 == sscanf( STR2CHR( s ),
        "fg:%x xsec:%lf yscl:%lf",
        &c, &x, &y ) ) {

        if( isDigChanType )
            y = 1.0;

        if( G )
            G->setUpdatesEnabled( false );

        trace_Color   = QColor::fromRgba( (QRgb)c );
        yscale        = y;

        setSpanSecs( x, srate );
        setVGridLinesAuto();

        if( G )
            G->setUpdatesEnabled( true );
    }
}

/* ---------------------------------------------------------------- */
/* GLGraph -------------------------------------------------------- */
/* ---------------------------------------------------------------- */


QMap<QString,GLGraph::shrRef>  GLGraph::usr2Ref;


// There are two types of graph clients, distinguished by ownsX:
//
// FileViewer:
// - Graphs own their GLGraphState (X) records.
// - Effectively, the time axis is ordered and correct.
// - Data appear to the user in strict time order.
// - Selection ranges are useable for export.
//
// GraphsWindow
// - Graphs swap GLGraphState records.
// - Time spans are correct but data within are pseudo-ordered.
// - Cursor readout is approximate (good to the epoch).
// - Data are displayed with a progressive wipe effect.
//
#ifdef OPENGL54
GLGraph::GLGraph( const QString &usr, QWidget *parent, GLGraphState *X )
    : QOpenGLWidget(parent), X(X), ownsX(false)
#elif 0
GLGraph::GLGraph( const QString &usr, QWidget *parent, GLGraphState *X )
    :   QGLWidget(mainApp()->pool->getFmt(), parent, getShr( usr )),
        usr(usr), X(X), ownsX(false)
#else
GLGraph::GLGraph( const QString &usr, QWidget *parent, GLGraphState *X )
    :   QGLWidget(mainApp()->pool->getFmt(), parent),
        usr(usr), X(X), ownsX(false)
#endif
{
#ifdef OPENGL54
    Q_UNUSED( usr );
#endif

    if( X ) {
        ownsX       = true;
        X->rptMode  = GLGraphState::grfReportXStream;
    }

    setCursor( Qt::CrossCursor );
}


GLGraph::~GLGraph()
{
    if( X && ownsX )
        delete X;

    QMap<QString,shrRef>::iterator  it;

    if( !usr.isEmpty() && (it = usr2Ref.find( usr )) != usr2Ref.end() ) {

        if( --it.value().nG <= 0 )
            usr2Ref.remove( usr );
    }
}


void GLGraph::attach( GLGraphState *newX )
{
    detach();

    if( newX ) {

        newX->attach( this );
        X = newX;

        immed_update    = false;
        need_update     = false;
        setMouseTracking( true );
        setUpdatesEnabled( true );
    }
}


void GLGraph::detach()
{
    setUpdatesEnabled( false );
    setMouseTracking( false );
    immed_update    = false;
    need_update     = false;

    if( X ) {
        X->detach();
        X = 0;
    }
}


// Note: makeCurrent() called automatically.
//
void GLGraph::initializeGL()
{
#ifdef OPENGL54
    initializeOpenGLFunctions();
#endif

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    //glEnable( GL_LINE_SMOOTH );
    //glEnable( GL_POINT_SMOOTH );
    glEnableClientState( GL_VERTEX_ARRAY );
}


// Note: makeCurrent() called automatically.
//
void GLGraph::resizeGL( int w, int h )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, w, h );
    gluOrtho2D( 0.0, 1.0, -1.0, 1.0 );
}


// Note: makeCurrent() called automatically.
//
void GLGraph::paintGL()
{
    if( !X || !isVisible() )
        return;

#ifdef OPENGL54
//    glClear(...);
#endif

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    X->applyGLBkgndClr();

//    glEnableClientState( GL_VERTEX_ARRAY );

    drawGrid();

    X->dataMtx->lock();

    int span = X->verts.size();

    if( span > 0 ) {

        glPushMatrix();
        glScalef( 1.0F/span, (float)X->yscale, 1.0F );
        drawPointsMain();
        glPopMatrix();
    }

    X->dataMtx->unlock();

    drawSelection();

//    glDisableClientState( GL_VERTEX_ARRAY );

    need_update = false;
}


// Window coords:
// [L,R] = [0,width()],
// [T,B] = [0,height()].
//
void GLGraph::mouseMoveEvent( QMouseEvent *evt )
{
    double  x = evt->x(), y = evt->y();

    emit( cursorOverWindowCoords( x, y ) );

    if( X ) {
        win2LogicalCoords( x, y );
        emit( cursorOver( x, y ) );
    }
}


void GLGraph::mousePressEvent( QMouseEvent *evt )
{
    double  x = evt->x(), y = evt->y();

    if( evt->buttons() & Qt::LeftButton ) {

        emit( lbutClickedWindowCoords( x, y ) );

        if( X ) {
            win2LogicalCoords( x, y );
            emit( lbutClicked( x, y ) );
        }
    }
    else if( evt->buttons() & Qt::RightButton ) {

        emit( rbutClickedWindowCoords( x, y ) );

        if( X ) {
            win2LogicalCoords( x, y );
            emit( rbutClicked( x, y ) );
        }
    }
}


void GLGraph::mouseReleaseEvent( QMouseEvent *evt )
{
    if( !(evt->buttons() & Qt::LeftButton) )
        emit( lbutReleased() );

    if( !(evt->buttons() & Qt::RightButton) )
        emit( rbutReleased() );
}


void GLGraph::mouseDoubleClickEvent( QMouseEvent *evt )
{
    if( !X || !(evt->buttons() & Qt::LeftButton) )
        return;

    double  x = evt->x(), y = evt->y();

    win2LogicalCoords( x, y );
    emit( lbutDoubleClicked( x, y ) );
}


const GLGraph *GLGraph::getShr( const QString &usr )
{
    QMap<QString,shrRef>::iterator  it;

    if( (it = usr2Ref.find( usr )) != usr2Ref.end() ) {

        shrRef  &S = it.value();

        ++S.nG;
        S.gShr->doneCurrent();

        return S.gShr;
    }
    else
        usr2Ref[usr] = shrRef( this );

    return 0;
}


// Input window coords:
// [L,R] = [0,width()],
// [T,B] = [0,height()].
//
// Output:
// [L,R] = [min_x, max_x]   (grfReportXStream)
// [L,R] = (min_x+max_x)/2  (grfReportXAve)
//
// [B,T] = [-1,1]/yscale
//
void GLGraph::win2LogicalCoords( double &x, double &y )
{
    if( X->rptMode == GLGraphState::grfReportXStream )
        x = x / width() * X->spanSecs() + X->min_x;
    else
        x = (X->min_x + X->max_x) / 2.0;

// Remap [B,T] from [H,0] to [-1,1] as follows:
// - Div by H:  [1,0]
// - Mul by -2: [-2,0]
// - Add 1:     [-1,1]

    y = (1.0 - 2.0 * y / height()) / X->yscale;
}


void GLGraph::drawGrid()
{
    bool wasEnabled = glIsEnabled( GL_LINE_STIPPLE );

    if( !wasEnabled )
        glEnable( GL_LINE_STIPPLE );

    GLfloat savedWidth;
    GLfloat savedClr[4];
    GLint   savedPat = 0, savedRepeat = 0;

// save attributes
    glGetFloatv( GL_LINE_WIDTH, &savedWidth );
    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    glGetIntegerv( GL_LINE_STIPPLE_PATTERN, &savedPat );
    glGetIntegerv( GL_LINE_STIPPLE_REPEAT, &savedRepeat );

// set attributes
    glLineWidth( 1.0F );
    X->applyGLGridClr();
    glLineStipple( 1, X->gridStipplePat );

// draw time-axis marks (the verticals)
    glVertexPointer( 2, GL_FLOAT, 0, &X->gridVs[0] );
    glDrawArrays( GL_LINES, 0, 2 * X->nVGridLines );

    if( X->isDigChanType ) {

        // Draw dashed baseline at bottom of each digital chart

        const int   nL  = 16;
        const float lo  = -1.0F,
                    hi  =  1.0F,
                    ht  = (hi-lo)/nL,
                    off = lo + ht*0.01F;

        for( int line = 0; line < nL; ++line ) {

            GLfloat y   = off + ht*line;
            GLfloat h[] = {0.0F, y, 1.0F, y};
            glVertexPointer( 2, GL_FLOAT, 0, h );
            glDrawArrays( GL_LINES, 0, 2 );
        }
    }
    else {

        // draw analog y-axis marks (the horizontals)
        glVertexPointer( 2, GL_FLOAT, 0, &X->gridHs[0] );
        glDrawArrays( GL_LINES, 0, 2 * X->nHGridLines );

        // draw dashed baseline at y = 0
        glLineStipple( 1, 0xffff );
        static const GLfloat h[] = {0.0F, 0.0F, 1.0F, 0.0F};
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }

// restore attributes
    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
    glLineStipple( savedRepeat, savedPat );

    if( !wasEnabled )
        glDisable( GL_LINE_STIPPLE );
}


// Define 6 fill patterns (cols) and 6 brightness levels (rows).
//
// Note that pure blues are hard to see on my gray digital background.
//
static QColor MakeColor36( int i )
{
    static quint8   bright[6] = { 0xFF, 0x80, 0xC0, 0x60, 0xA0, 0xE0 };

    const int   kcol = 6;

    int row = i / kcol,
        col = i - kcol*row,
        v   = bright[row];

    switch( col ) {
        case 0: return QColor( v, 0, 0 );
        case 1: return QColor( 0, v, 0 );
        case 2: return QColor( v, v, 0 );
        case 3: return QColor( v, 0, v );
        case 4: return QColor( 0, v, v );
        case 5: return QColor( v, v, v );
    };

    return QColor( 0xFF, 0xFF, 0xFF );
}


// Full graph span is [lo,hi] = [-1,1].
// Divide span into (nL = 16) lines/boxes/charts.
// Each chart has full box height ht.
// Shrink each chart's data to (scl = 0.88*ht) to fit in box.
// That is, each chart gets 0.12*ht of empty margin.
// Lower margin is at (off = 0.06*ht).
//
// Each y-value really a 16-bit word...each bit a chart line.
//
void GLGraph::drawPointsDigital()
{
    QVector<Vec2f>  &V = X->verts;
    const float     *Y;
    uint            len = X->ydata.all( (float* &)Y );
    const int       nL  = 16;
    const float     lo  = -1.0F,
                    hi  =  1.0F,
                    ht  = (hi-lo)/nL,
                    off = lo + ht * 0.06F,
                    scl = ht * 0.88F;

    for( int line = 0; line < nL; ++line ) {

        QColor  c   = MakeColor36( line );
        double  y0  = off + ht * line;

        glColor4f( c.redF(), c.greenF(), c.blueF(), c.alphaF() );

        for( uint i = 0; i < len; ++i )
            V[i].y = y0 + scl*((quint16(Y[i]) >> line) & 1);

        glVertexPointer( 2, GL_FLOAT, 0, &V[0] );
        glDrawArrays( GL_LINE_STRIP, 0, len );
    }
}


void GLGraph::drawPointsWiping()
{
    QVector<Vec2f>  &V = X->verts;
    const float     *Y;
    uint            len = X->ydata.all( (float* &)Y );

    for( uint i = 0; i < len; ++i )
        V[i].y = Y[i];

    glVertexPointer( 2, GL_FLOAT, 0, &V[0] );
    glDrawArrays( GL_LINE_STRIP, 0, len );
}


void GLGraph::drawPointsMain()
{
    GLfloat     savedWidth;
    GLfloat     savedClr[4];

// save attributes
    glGetFloatv( GL_LINE_WIDTH, &savedWidth );
    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    //glGetFloatv( GL_POINT_SIZE, &savedWidth );
    //glPointSize( 1.0F );

// new attributes
    glLineWidth( 1.0F );
    X->applyGLTraceClr();

// draw
    if( X->isDigChanType )
        drawPointsDigital();
    else
        drawPointsWiping();

// cursor
    if( X->drawCursor
        && X->ydata.size()
        && X->spanSecs() >= 0.329 ) {

        QColor  c( 0x80, 0x80, 0x80 );
        glColor4f( c.redF(), c.greenF(), c.blueF(), c.alphaF() );

        GLfloat    x   = X->ydata.cursor();
        GLfloat    h[] = {x, -(float)X->yscale, x, (float)X->yscale};
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }

// restore attributes
    //glPointSize( savedWidth );
    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
}


// GL_QUADS interprets array CCW fashion:
// A----D
// |    |
// B----C
//
void GLGraph::drawSelection()
{
    if( !X->isSelVisible() )
        return;

    float   vertices[8];
    int     saved_polygonmode[2];

    X->getSelVertices( vertices );

// invert color
    glGetIntegerv( GL_POLYGON_MODE, saved_polygonmode );
    glBlendFunc( GL_ONE_MINUS_DST_COLOR, GL_ZERO );

    glColor4f( 0.5F, 0.5F, 0.5F, 0.5F );
    glPolygonMode( GL_FRONT, GL_FILL ); // fill the polygon

    glVertexPointer( 2, GL_FLOAT, 0, vertices );
    glDrawArrays( GL_QUADS, 0, 4 );

// restore saved OpenGL state
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glPolygonMode( GL_FRONT, saved_polygonmode[0] );
}


