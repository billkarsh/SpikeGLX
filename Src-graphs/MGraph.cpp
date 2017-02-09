
#include "MGraph.h"
#include "Util.h"
#include "MainApp.h"

#include <QDesktopWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QScrollBar>
#include <math.h>

#ifdef Q_WS_MACX
#include <gl.h>
#include <agl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


/* ---------------------------------------------------------------- */
/* MGraphY -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

MGraphY::MGraphY()
{
    yscl        = 1.0;
    usrChan     = 0;
    iclr        = 0,
    isDigType   = false;
}


void MGraphY::erase()
{
    yval.erase();
    yval.zeroFill();
}


void MGraphY::resize( int n )
{
    yval.resizeAndErase( n );
    yval.zeroFill();
}

/* ---------------------------------------------------------------- */
/* MGraphX -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

// Notes
// -----
// fixedNGrf: If > 0, resize() uses this to set ypxPerGrf.
//
MGraphX::MGraphX()
{
    yColor.push_back( QColor( 0xee, 0xdd, 0x82 ) );

    min_x           = 0.0;
    max_x           = 1.0;
    xSelBegin       = 0.0F;
    xSelEnd         = 0.0F;
    G               = 0;
    bkgnd_Color     = QColor( 0x20, 0x3c, 0x3c );
    grid_Color      = QColor( 0x87, 0xce, 0xfa, 0x7f );
    label_Color     = QColor( 0xff, 0xff, 0xff );
    dwnSmp          = 1;
    ySel            = -1;
    fixedNGrf       = -1;
    ypxPerGrf       = 10;
    clipTop         = 0;
    gridStipplePat  = 0xf0f0; // 4pix on 4 off 4 on 4 off
    drawCursor      = true;
    isXsel          = false;

    setVGridLinesAuto();
}


void MGraphX::attach( MGraph *newG )
{
    G       = newG;
    isXsel  = false;

    dataMtx.lock();

    foreach( MGraphY *y, Y )
        y->erase();

    if( Y.size() )
        initVerts( Y[0]->yval.capacity() );
    else
        initVerts( 0 );

    dataMtx.unlock();
}


// This needs to be called whenever the capacity of the data
// buffer is changed. setSpanSecs automatically calls this.
//
void MGraphX::initVerts( int n )
{
    verts.resize( n );
    vdigclr.resize( 3 * n );

    for( int i = 0; i < n; ++i )
        verts[i].x = i;
}


// A graph's raw span (in samples) is ordinarily (t * srate).
// However, no graph need store more than points than the
// largest screen dimension (2X oversampling is included).
// Each graph gets a downsample factor (dwnSmp) to moderate
// its storage according to current span.
//
void MGraphX::setSpanSecs( double t, double srate )
{
    if( t <= 0.0 )
        return;

// ----------------------
// Round to GUI precision
// ----------------------

    char    buf[64];
    sprintf( buf, "%g", t );
    sscanf( buf, "%lf", &t );

    max_x = min_x + t;

// -------------------
// Init points buffers
// -------------------

    double  spanSmp = t * srate;
    QRect   rect    = QApplication::desktop()->screenGeometry();
    int     maxDim  = 2 * qMax( rect.width(), rect.height() );

    dwnSmp = int(spanSmp/maxDim);

    if( dwnSmp < 1 )
        dwnSmp = 1;

    uint    newSize = (uint)ceil( spanSmp/dwnSmp );

    dataMtx.lock();

    foreach( MGraphY *y, Y )
        y->resize( newSize );

    initVerts( newSize );

    dataMtx.unlock();
}


void MGraphX::setVGridLines( int n )
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
void MGraphX::setVGridLinesAuto()
{
    double  t = spanSecs();

    while( t < 1.0 )
        t *= 10.0;

    setVGridLines( int(t) );
}


void MGraphX::calcYpxPerGrf()
{
    if( G && fixedNGrf > 0 )
        ypxPerGrf = G->height() / fixedNGrf;
}


void MGraphX::setYSelByUsrChan( int usrChan )
{
    ySel = -1;

    if( usrChan < 0 )
        return;

    for( int i = 0, n = Y.size(); i < n; ++i ) {

        if( Y[i]->usrChan == usrChan ) {
            ySel = i;
            return;
        }
    }
}


// Return y-center of ySel graph,
// or zero.
//
int MGraphX::getSelY0()
{
    if( ySel < 0 || !G )
        return 0;

    return ypxPerGrf*(ySel + 0.5F);
}


void MGraphX::setXSelRange( float begin_x, float end_x )
{
    if( begin_x <= end_x ) {
        xSelBegin   = begin_x;
        xSelEnd     = end_x;
    }
    else {
        xSelBegin   = end_x;
        xSelEnd     = begin_x;
    }
}


void MGraphX::setXSelEnabled( bool onoff )
{
    isXsel = onoff;
}


bool MGraphX::isXSelVisible() const
{
    return  isXsel
            && xSelEnd   >= 0.0F
            && xSelBegin <= 1.0F;
}


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
void MGraphX::getXSelVerts( float v[] ) const
{
    v[0] = v[2] = xSelBegin;
    v[4] = v[6] = xSelEnd;
    v[1] = v[7] =  1.0F;
    v[3] = v[5] = -1.0F;
}


void MGraphX::applyGLBkgndClr() const
{
    const QColor    &C = bkgnd_Color;

    glClearColor( C.redF(), C.greenF(), C.blueF(), C.alphaF() );
    glClear( GL_COLOR_BUFFER_BIT );
}


void MGraphX::applyGLGridClr() const
{
    const QColor    &C = grid_Color;

    glColor4f( C.redF(), C.greenF(), C.blueF(), C.alphaF() );
}


void MGraphX::applyGLLabelClr() const
{
    const QColor    &C = label_Color;

    glColor4f( C.redF(), C.greenF(), C.blueF(), C.alphaF() );
}


void MGraphX::applyGLTraceClr( int iy ) const
{
    const QColor    &C = yColor[Y[iy]->iclr];

    glColor4f( C.redF(), C.greenF(), C.blueF(), C.alphaF() );
}

/* ---------------------------------------------------------------- */
/* MGraph --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SharedData
{
public:
    QGLFormat   fmt;
public:
    SharedData()
    {
        // -----------
        // These settings seem appropriate but make no apparent difference
        //
        //    fmt.setDepth( false );
        //    fmt.setDepthBufferSize( 0 );
        //    fmt.setStencilBufferSize( 0 );
        //    fmt.setOverlay( false );
        // -----------

        // -----------
        // Crucial settings
        //
            fmt.setSwapInterval( 0 );   // disable syncing
        // -----------
    }
};

static SharedData   shr;


QMap<QString,MGraph::shrRef>  MGraph::usr2Ref;


MGraph::MGraph( const QString &usr, QWidget *parent, MGraphX *X )
#ifdef OPENGL54
    :   QOpenGLWidget(parent)
#elif 0
    :   QGLWidget(shr.fmt, parent, getShr( usr )), usr(usr),
#else
    :   QGLWidget(shr.fmt, parent), usr(usr),
#endif
        X(X), ownsX(false), inited(false)
{
#ifdef OPENGL54
    Q_UNUSED( usr )
#endif

    if( X ) {
        ownsX = true;
        attach( X );
    }

    setFont( QFont( "Lucida Sans Typewriter", -1 ) );
    setCursor( Qt::CrossCursor );
}


MGraph::~MGraph()
{
    if( X && ownsX )
        delete X;

    QMap<QString,shrRef>::iterator  it;

    if( !usr.isEmpty() && (it = usr2Ref.find( usr )) != usr2Ref.end() ) {

        if( --it.value().nG <= 0 )
            usr2Ref.remove( usr );
    }
}


void MGraph::attach( MGraphX *newX )
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


void MGraph::detach()
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
void MGraph::initializeGL()
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

    inited = true;
}


// Note: makeCurrent() called automatically.
//
void MGraph::resizeGL( int w, int h )
{
    if( !inited )
        return;

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, w, h );
    gluOrtho2D( 0.0, 1.0, -1.0, 1.0 );

    if( X )
        X->calcYpxPerGrf();
}


// Note: makeCurrent() called automatically.
//
void MGraph::paintGL()
{
    if( !X || !isVisible() )
        return;

// -----
// Setup
// -----

#ifdef OPENGL54
//    glClear(...);
#endif

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

// ----
// Grid
// ----

    X->applyGLBkgndClr();
    drawGrid();

// ------
// Labels
// ------

    drawLabels();

// ------
// Points
// ------

    X->dataMtx.lock();

    int span = X->verts.size();

    if( span > 0 && X->Y.size() ) {

        glPushMatrix();
        glScalef( 1.0F/span, 1.0F, 1.0F );
        drawPointsMain();
        glPopMatrix();
    }

    X->dataMtx.unlock();

// ----------
// Selections
// ----------

    drawYSel();
    drawXSel();
    need_update = false;

// -------
// Restore
// -------
}


// Window coords:
// [L,R] = [0,width()],
// [T,B] = [0,height()].
//
void MGraph::mouseMoveEvent( QMouseEvent *evt )
{
    int ny;

    if( X && (ny = X->Y.size()) ) {

        double  x   = evt->x(),
                y   = evt->y() + X->clipTop;
        int     iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;

        y -= iy * X->ypxPerGrf;

        emit( cursorOverWindowCoords( x, y, iy ) );

        win2LogicalCoords( x, y /= X->ypxPerGrf, iy );
        emit( cursorOver( x, y, iy ) );
    }
}


void MGraph::mousePressEvent( QMouseEvent *evt )
{
    int ny;

    if( X && (ny = X->Y.size()) ) {

        double  x   = evt->x(),
                y   = evt->y() + X->clipTop;
        int     iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;

        y -= iy * X->ypxPerGrf;

        if( evt->buttons() & Qt::LeftButton ) {

            emit( lbutClickedWindowCoords( x, y, iy ) );

            win2LogicalCoords( x, y /= X->ypxPerGrf, iy );
            emit( lbutClicked( x, y, iy ) );
        }
        else if( evt->buttons() & Qt::RightButton ) {

            emit( rbutClickedWindowCoords( x, y, iy ) );

            win2LogicalCoords( x, y /= X->ypxPerGrf, iy );
            emit( rbutClicked( x, y, iy ) );
        }
    }
}


void MGraph::mouseReleaseEvent( QMouseEvent *evt )
{
    if( !(evt->buttons() & Qt::LeftButton) )
        emit( lbutReleased() );

    if( !(evt->buttons() & Qt::RightButton) )
        emit( rbutReleased() );
}


void MGraph::mouseDoubleClickEvent( QMouseEvent *evt )
{
    int ny;

    if( X && (ny = X->Y.size()) && (evt->buttons() & Qt::LeftButton) ) {

        double  x   = evt->x(),
                y   = evt->y() + X->clipTop;
        int     iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;

        y -= iy * X->ypxPerGrf;

        win2LogicalCoords( x, y /= X->ypxPerGrf, iy );
        emit( lbutDoubleClicked( x, y, iy ) );
    }
}


const MGraph *MGraph::getShr( const QString &usr )
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


// Input graph coords:
// [L,R] = [0,g-width()],
// [T,B] = [0,1] (caller divides y by ypxPerGrf).
//
// Output:
// [L,R] = [min_x,max_x]
// [B,T] = [-1,1]/yscale
//
void MGraph::win2LogicalCoords( double &x, double &y, int iy )
{
    x = X->min_x + x * X->spanSecs() / width();

// Remap [B,T] from [1,0] to [-1,1] as follows:
// - Mul by -2: [-2,0]
// - Add 1:     [-1,1]

    y = (1.0 - 2.0 * y) / X->Y[iy]->yscl;
}


// Draw each graph's y = 0.
//
void MGraph::drawBaselines()
{
    GLfloat h[] = {0.0F, 0.0F, 1.0F, 0.0F};

    int     clipHgt = height();
    float   yscl    = 2.0F / clipHgt;

    for( int iy = 0, ny = X->Y.size(); iy < ny; ++iy ) {

        if( X->Y[iy]->isDigType )
            continue;

        float   y0_px = (iy+0.5F)*X->ypxPerGrf;

        if( y0_px < X->clipTop || y0_px > X->clipTop + clipHgt )
            continue;

        float   y0 = 1.0F - yscl*(y0_px - X->clipTop);

        h[1] = h[3] = y0;
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }
}


void MGraph::drawGrid()
{
// ----
// Save
// ----

    GLfloat savedWidth;
    GLfloat savedClr[4];
    GLint   savedPat = 0, savedRepeat = 0;
    bool    wasEnabled = glIsEnabled( GL_LINE_STIPPLE );

    glGetFloatv( GL_LINE_WIDTH, &savedWidth );
    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    glGetIntegerv( GL_LINE_STIPPLE_PATTERN, &savedPat );
    glGetIntegerv( GL_LINE_STIPPLE_REPEAT, &savedRepeat );

// -----
// Setup
// -----

    glLineWidth( 1.0F );
    X->applyGLGridClr();

    glLineStipple( 1, X->gridStipplePat );

    if( !wasEnabled )
        glEnable( GL_LINE_STIPPLE );

// ---------
// Verticals
// ---------

    glVertexPointer( 2, GL_FLOAT, 0, &X->gridVs[0] );
    glDrawArrays( GL_LINES, 0, 2 * X->nVGridLines );

// -----------
// Horizontals
// -----------

    glLineStipple( 1, 0xffff );
    drawBaselines();

// -------
// Restore
// -------

    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
    glLineStipple( savedRepeat, savedPat );

    if( !wasEnabled )
        glDisable( GL_LINE_STIPPLE );
}


void MGraph::drawLabels()
{
// ----
// Save
// ----

    GLfloat savedClr[4];
    GLint   savedPat = 0, savedRepeat = 0;
    bool    wasEnabled = glIsEnabled( GL_LINE_STIPPLE );

    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    glGetIntegerv( GL_LINE_STIPPLE_PATTERN, &savedPat );
    glGetIntegerv( GL_LINE_STIPPLE_REPEAT, &savedRepeat );

// -----
// Setup
// -----

    X->applyGLLabelClr();

    if( !wasEnabled )
        glEnable( GL_LINE_STIPPLE );

// ------
// Labels
// ------

    QFontMetrics    FM      = fontMetrics();
    int             clipHgt = height(),
                    right   = width() - 4,
                    fontMid = FM.boundingRect( 'A' ).height() / 2
                                - X->clipTop;

    for( int iy = 0, ny = X->Y.size(); iy < ny; ++iy ) {

        bool    isL = !X->Y[iy]->lhsLabel.isEmpty(),
                isR = !X->Y[iy]->rhsLabel.isEmpty();

        if( !(isL || isR) )
            continue;

        float   y_base = fontMid + (iy+0.5F)*X->ypxPerGrf;

        if( y_base < 0 || y_base > clipHgt )
            continue;

        if( isL ) {

            renderText(
                4,
                y_base,
                X->Y[iy]->lhsLabel );
        }

        if( isR ) {

            renderText(
                right - FM.width( X->Y[iy]->rhsLabel ),
                y_base,
                X->Y[iy]->rhsLabel );
        }
    }

// -------
// Restore
// -------

    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
    glLineStipple( savedRepeat, savedPat );

    if( !wasEnabled )
        glDisable( GL_LINE_STIPPLE );
}


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
void MGraph::drawYSel()
{
    if( X->ySel < 0 || !X->Y.size() )
        return;

    float   top_px  = X->ySel * X->ypxPerGrf,
            bot_px  = top_px  + X->ypxPerGrf;
    int     clipHgt = height();

    if( bot_px < X->clipTop || top_px > X->clipTop + clipHgt )
        return;

// ----
// Save
// ----

    GLfloat savedWidth;
    GLfloat savedClr[4];
    int     saved_polygonmode[2];

    glGetFloatv( GL_LINE_WIDTH, &savedWidth );
    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    glGetIntegerv( GL_POLYGON_MODE, saved_polygonmode );

// -----
// Setup
// -----

    glLineWidth( 4.0F );
    glColor3f( 0.0F, 0.0F, 0.0F );
    glPolygonMode( GL_FRONT, GL_LINE );

// ----
// Draw
// ----

    float   mrg     = 2.0F / width(),
            yscl    = 2.0F / clipHgt,
            top     = 1.0F - yscl*(top_px - X->clipTop),
            bot     = 1.0F - yscl*(bot_px - X->clipTop);
    float   vertices[8] = {
        mrg, top,
        mrg, bot,
        1.0F - mrg, bot,
        1.0F - mrg, top };

    glVertexPointer( 2, GL_FLOAT, 0, vertices );
    glDrawArrays( GL_QUADS, 0, 4 );

// -------
// Restore
// -------

    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
    glPolygonMode( GL_FRONT, saved_polygonmode[0] );
}


// GL_QUADS interprets array CCW fashion:
// A----D
// |    |
// B----C
//
void MGraph::drawXSel()
{
    if( !X->isXSelVisible() )
        return;

// ----
// Save
// ----

    int saved_polygonmode[2];

    glGetIntegerv( GL_POLYGON_MODE, saved_polygonmode );

// -----
// Setup
// -----

// invert
    glBlendFunc( GL_ONE_MINUS_DST_COLOR, GL_ZERO );
    glColor4f( 0.5F, 0.5F, 0.5F, 0.5F );
    glPolygonMode( GL_FRONT, GL_FILL ); // filled polygon

// ----
// Draw
// ----

    float   vertices[8];

    X->getXSelVerts( vertices );
    glVertexPointer( 2, GL_FLOAT, 0, vertices );
    glDrawArrays( GL_QUADS, 0, 4 );

// -------
// Restore
// -------

    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glPolygonMode( GL_FRONT, saved_polygonmode[0] );
}


// Refer to scaling for analog case.
// Here we'll work again in yval units [-1,1],
// hence, adopt the same scale = ypxPerGrf/clipHgt.
//
void MGraph::draw1Digital( int iy )
{
    glEnableClientState( GL_COLOR_ARRAY );

    QVector<Vec2f>  &V      = X->verts;
    QVector<quint8> &C      = X->vdigclr;
    MGraphY         *Y      = X->Y[iy];
    const float     *y;
    int             clipHgt = height();

    float   lo_px   = (iy+1)*X->ypxPerGrf,
            yscl    = 2.0F / clipHgt,
            lo      = 1.0F - yscl*(lo_px - X->clipTop),
            mrg     = 0.04F * 2.0F, // top&bot, ea as frac of [-1,1] range
            scl     = (float)X->ypxPerGrf / clipHgt,
            off     = scl * mrg,
            ht      = scl * (2.0F - 2*mrg) / 16;
    uint    len     = Y->yval.all( (float* &)y );

// Compose a WHITE color group and a GREEN group.
// WHITE-dark1, WHITE-bright1, GREEN-dark2, GREEN-bright2.
// Each set of 4 lines will use the WHITE or GREEN group.

    quint8  clrs[(3*2)*2] = {
                90,90,90,   // 80,80,80
                250,250,250,
                100,100,20, // 70,100,20
                120,255,0};

    for( int line = 0; line < 16; ++line ) {

        float   y0      = lo + off + line * ht;
        quint8  *cgrp   = &clrs[6*((line / 4) & 1)];    // which group

        for( uint i = 0; i < len; ++i ) {

            quint8  *cdst   = &C[3*i];
            int     b       = (quint16(y[i]) >> line) & 1;
//            int     b       = (((quint32*)y)[i] >> (line+16)) & 1; // test
            quint8  *csrc   = cgrp + 3*b;

            V[i].y  = y0 + 0.80F * ht * b;
            cdst[0] = csrc[0];
            cdst[1] = csrc[1];
            cdst[2] = csrc[2];
        }

        glColorPointer( 3, GL_UNSIGNED_BYTE, 0, &C[0] );
        glVertexPointer( 2, GL_FLOAT, 0, &V[0] );
        glDrawArrays( GL_LINE_STRIP, 0, len );
    }

    glDisableClientState( GL_COLOR_ARRAY );
}


// The whole viewport logical span is [-1,1] and clipHgt pixel span.
// yvals are in range [-1,1].
// To scale yvals into pixels, mult by ypxPerGrf/2.
// To scale that into the viewport, mult by 2/clipHgt.
// Hence, scale = ypxPerGrf/clipHgt.
//
void MGraph::draw1Analog( int iy )
{
    QVector<Vec2f>  &V      = X->verts;
    MGraphY         *Y      = X->Y[iy];
    const float     *y;
    int             clipHgt = height();

    float   y0_px   = (iy+0.5F)*X->ypxPerGrf,
            yscl    = 2.0F / clipHgt,
            y0      = 1.0F - yscl*(y0_px - X->clipTop),
            scl     = Y->yscl * X->ypxPerGrf / clipHgt;
    uint    len     = Y->yval.all( (float* &)y );

    X->applyGLTraceClr( iy );

    for( uint i = 0; i < len; ++i )
        V[i].y = y0 + scl*y[i];

    glVertexPointer( 2, GL_FLOAT, 0, &V[0] );
    glDrawArrays( GL_LINE_STRIP, 0, len );
}


void MGraph::drawPointsMain()
{
// ----
// Save
// ----

    GLfloat     savedWidth;
    GLfloat     savedClr[4];

    glGetFloatv( GL_LINE_WIDTH, &savedWidth );
    glGetFloatv( GL_CURRENT_COLOR, savedClr );
    //glGetFloatv( GL_POINT_SIZE, &savedWidth );
    //glPointSize( 1.0F );

// -----
// Setup
// -----

    glLineWidth( 1.0F );

// ----
// Loop
// ----

    int ny      = X->Y.size(),
        clipHgt = height();

    for( int iy = 0; iy < ny; ++iy ) {

        float   top_px  = iy * X->ypxPerGrf,
                bot_px  = top_px + X->ypxPerGrf;

        if( bot_px < X->clipTop - X->ypxPerGrf
            || top_px > X->clipTop + clipHgt + X->ypxPerGrf ) {

            continue;
        }

        if( X->Y[iy]->isDigType )
            draw1Digital( iy );
        else
            draw1Analog( iy );
    }

// ------
// Cursor
// ------

    if( X->drawCursor
        && ny
        && X->spanSecs() >= 0.329
        && X->Y[0]->yval.size() ) {

        QColor  c( 0x80, 0x80, 0x80 );
        glColor4f( c.redF(), c.greenF(), c.blueF(), c.alphaF() );

        GLfloat    x   = X->Y[0]->yval.cursor();
        GLfloat    h[] = {x, -1.0F, x, 1.0F};
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }

// -------
// Restore
// -------

    //glPointSize( savedWidth );
    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
}

/* ---------------------------------------------------------------- */
/* MGScroll ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

MGScroll::MGScroll( const QString &usr, QWidget *parent )
    : QAbstractScrollArea(parent)
{
    theX    = new MGraphX;
    theM    = new MGraph( usr, this, theX );

    theM->setImmedUpdate( true );
    theM->setMouseTracking( true );

    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setViewport( theM );
}


void MGScroll::scrollTo( int y )
{
    theX->clipTop = y;
    verticalScrollBar()->setSliderPosition( y );
}


void MGScroll::adjustLayout()
{
    int ny = theX->Y.size(),
        vh = viewport()->height(),
        gh;

    if( ny > 1 )
        gh = ny * theX->ypxPerGrf;
    else
        theX->ypxPerGrf = gh = vh;

    verticalScrollBar()->setPageStep( vh );
    verticalScrollBar()->setRange( 0, gh - vh );
    updateGeometry();
}


void MGScroll::scrollToSelected()
{
    if( theX->Y.size() <= 1 )
        return;

    if( theX->ySel < 0 )
        return;

    int sc_min  = verticalScrollBar()->minimum(),
        sc_max  = verticalScrollBar()->maximum(),
        pos     = theX->getSelY0() - theM->height() / 2;

    if( pos < sc_min )
        pos = sc_min;
    else if( pos > sc_max )
        pos = sc_max;

    scrollTo( pos );
}


void MGScroll::resizeEvent( QResizeEvent *e )
{
    Q_UNUSED( e )

    adjustLayout();

    theM->makeCurrent();
    theM->resizeGL( theM->width(), theM->height() );

#ifdef OPENGL54
    theM->update();
#endif
}


void MGScroll::scrollContentsBy( int dx, int dy )
{
    Q_UNUSED( dx )
    Q_UNUSED( dy )

    theX->clipTop = verticalScrollBar()->sliderPosition();
    theM->update();
}


bool MGScroll::viewportEvent( QEvent *e )
{
    QEvent::Type    type = e->type();

    if( type == QEvent::Resize )
        return QAbstractScrollArea::viewportEvent( e );
    else
        return theM->event( e );
}


