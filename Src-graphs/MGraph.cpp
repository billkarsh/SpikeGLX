
#include "MGraph.h"
#include "Util.h"
#include "MainApp.h"

#include <QDesktopWidget>
#include <QPoint>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QVBoxLayout>

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
    iclr        = 0;
    drawBinMax  = false;
    isDigType   = false;
}


void MGraphY::erase()
{
    yval.erase();
    yval.zeroFill();

    yval2.erase();
    yval2.zeroFill();
}


void MGraphY::resize( int n )
{
    yval.resizeAndErase( n );
    yval.zeroFill();

    yval2.resizeAndErase( n );
    yval2.zeroFill();
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
    grid_Color      = QColor( 0x53, 0x85, 0x96 );
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
    verts2x.resize( 2 * n );
    vdigclr.resize( 3 * n );

    for( int i = 0; i < n; ++i )
        verts[i].x = i;

    for( int i = 0; i < n; ++i ) {
        verts2x[2*i].x      = i;
        verts2x[2*i+1].x    = i;
    }
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

    spanMtx.lock();
    max_x = min_x + t;
    spanMtx.unlock();

// -------------------
// Init points buffers
// -------------------

    double  spanSmp = t * srate;
    QRect   rect    = QApplication::desktop()->screenGeometry();
    int     maxDim  = 2 * qMax( rect.width(), rect.height() );

    dsmpMtx.lock();
    dwnSmp = int(spanSmp/maxDim);
    if( dwnSmp < 1 )
        dwnSmp = 1;
    dsmpMtx.unlock();

    uint    newSize = (uint)ceil( spanSmp/dwnSmp );

    foreach( MGraphY *y, Y )
        y->resize( newSize );

    initVerts( newSize );
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

    ypxPerGrf = qMax( ypxPerGrf, 1 );
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


void MGraphX::evQReset()
{
    for( int clr = 0; clr < 4; ++clr )
        evQ[clr].clear();
}


void MGraphX::evQExtendLast( double end, double minSecs, int clr )
{
    if( evQ[clr].size() ) {

        EvtSpan &E = evQ[clr].back();

        E.end = qMax( end, E.start + minSecs );
    }
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

#ifndef OPENGL54

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

#endif


QMap<QString,MGraph::shrRef>  MGraph::usr2Ref;


MGraph::MGraph( const QString &usr, QWidget *parent, MGraphX *X )
#ifdef OPENGL54
    :   QOpenGLWidget(parent),
#elif 0
    :   QGLWidget(shr.fmt, parent, getShr( usr )), usr(usr),
#else
    :   QGLWidget(shr.fmt, parent), usr(usr),
#endif
        X(X), ownsX(false)
{
#ifdef OPENGL54
    Q_UNUSED( usr )
#endif

    if( X ) {
        ownsX = true;
        attach( X );
    }

    setAutoFillBackground( false );
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
}


// Note: makeCurrent() called automatically.
//
void MGraph::resizeGL( int w, int h )
{
    if( !isValid() )
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
    if( !X || !isVisible() || !width() || !height() )
        return;

// -----
// Setup
// -----

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    X->dataMtx.lock();

// ----
// Draw
// ----

    X->applyGLBkgndClr();

    drawXSel();
    drawEvents();
    drawGrid();

    int span = X->verts.size();

    if( span > 1 && X->Y.size() ) {

        glPushMatrix();
        glScalef( 1.0F/(span - 1), 1.0F, 1.0F );
        drawPointsMain();
        glPopMatrix();
    }

    drawLabels();
    drawYSel();

    need_update = false;

// -------
// Restore
// -------

    X->dataMtx.unlock();
}


void MGraph::leaveEvent( QEvent *evt )
{
    Q_UNUSED( evt )

    emit( cursorOutside() );
}


// Window coords:
// [L,R] = [0,width()],
// [T,B] = [0,ypxPerGrf].
//
void MGraph::mouseMoveEvent( QMouseEvent *evt )
{
    int ny;

    if( X && (ny = X->Y.size()) ) {

        double  x   = evt->x(),
                y   = evt->y() + X->clipTop;
        int     iy0 = X->clipTop / X->ypxPerGrf,
                iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;
        else if( iy < iy0 )
            iy = iy0;

        y -= iy * X->ypxPerGrf;

        emit( cursorOverWindowCoords( x, y, iy ) );

        win2LogicalCoords( x, y, iy );
        emit( cursorOver( x, y, iy ) );
    }
}


void MGraph::mousePressEvent( QMouseEvent *evt )
{
    int ny;

    if( X && (ny = X->Y.size()) ) {

        double  x   = evt->x(),
                y   = evt->y() + X->clipTop;
        int     iy0 = X->clipTop / X->ypxPerGrf,
                iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;
        else if( iy < iy0 )
            iy = iy0;

        y -= iy * X->ypxPerGrf;

        if( evt->buttons() & Qt::LeftButton ) {

            emit( lbutClickedWindowCoords( x, y, iy ) );

            win2LogicalCoords( x, y, iy );
            emit( lbutClicked( x, y, iy ) );
        }
        else if( evt->buttons() & Qt::RightButton ) {

            emit( rbutClickedWindowCoords( x, y, iy ) );

            win2LogicalCoords( x, y, iy );
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
        int     iy0 = X->clipTop / X->ypxPerGrf,
                iy  = y / X->ypxPerGrf;

        if( iy >= ny )
            iy = ny - 1;
        else if( iy < iy0 )
            iy = iy0;

        y -= iy * X->ypxPerGrf;

        win2LogicalCoords( x, y, iy );
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
// [L,R] = [0,width()],
// [T,B] = [0,ypxPerGrf].
//
// Output:
// [L,R] = [min_x,max_x]
// [B,T] = [-1,1]/yscale
//
void MGraph::win2LogicalCoords( double &x, double &y, int iy )
{
    X->spanMtx.lock();
    x = X->min_x + x * X->spanSecs() / width();
    X->spanMtx.unlock();

// Remap [B,T] from [ypxPerGrf,0] to [-1,1] as follows:
// - Div by ypxPerGrf:  [1,0]
// - Mul by -2:         [-2,0]
// - Add 1:             [-1,1]

    y = (1.0 - 2.0 * y / X->ypxPerGrf) / X->Y[iy]->yscl;
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

    QFont   font = QFont();

    font.setPointSize(X->ypxPerGrf-3 > 12 ? 12 : qMax(X->ypxPerGrf-3, 6));
    font.setWeight(font.pointSize() >= 10 ? QFont::DemiBold : QFont::Bold);

    QFontMetrics    FM( font );
    int             clipHgt = height(),
                    right   = width() - 4,
                    ftHt    = FM.boundingRect( 'A' ).height(),
                    fontMid = ftHt / 2 - X->clipTop;
    float           offset  = (X->ypxPerGrf > 2.25 * ftHt ? 0.25F : 0.5F);

    for( int iy = 0, ny = X->Y.size(); iy < ny; ++iy ) {

        bool    isL = !X->Y[iy]->lhsLabel.isEmpty(),
                isR = !X->Y[iy]->rhsLabel.isEmpty();

        if( !(isL || isR) )
            continue;

        float   y_base = fontMid + (iy + offset)*X->ypxPerGrf;

        if( y_base < 0 || y_base > clipHgt )
            continue;

        if( isL ) {

            renderTextWin(
                4,
                y_base,
                X->Y[iy]->lhsLabel, font );
        }

        if( isR ) {

            renderTextWin(
                right - X->Y[iy]->rhsLabel.size() * FM.width( 'S' ),
                y_base,
                X->Y[iy]->rhsLabel, font );
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


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
void MGraph::drawEvents()
{
    int nEv[4]  = {0,0,0,0},
        sum     = 0;

    QMutexLocker    ml( &X->spanMtx );

    for( int clr = 0; clr < 4; ++clr )
        sum += (nEv[clr] = X->evQ[clr].size());

    if( !sum )
        return;

// ----
// Save
// ----

    int saved_polygonmode[2];

    glGetIntegerv( GL_POLYGON_MODE, saved_polygonmode );

// ---------------------------
// Loop over colors and events
// ---------------------------

    double  span = X->spanSecs();
    float   vert[8];

    for( int clr = 0; clr < 4; ++clr ) {

        std::deque<EvtSpan> &Q = X->evQ[clr];

        // Pop expired records
        // (Not too aggressively: allow right edges time to grow)

        while( nEv[clr] ) {

            EvtSpan &E = Q.front();

            if( E.end <= E.start || E.end <= X->min_x - 5.0 ) {

                Q.pop_front();
                --nEv[clr];
            }
            else
                break;
        }

        if( !nEv[clr] )
            continue;

        // Set the color

        switch( clr ) {
            case 0:  glColor4f( 0.0F, 1.0F, 0.0F, 0.2F ); break;    // green
            case 1:  glColor4f( 1.0F, 0.0F, 1.0F, 0.2F ); break;    // magenta
            case 2:  glColor4f( 0.0F, 1.0F, 1.0F, 0.2F ); break;    // cyan
            default: glColor4f( 1.0F, 0.3F, 0.0F, 0.2F ); break;    // orange
        }

        // Iterate and draw

        std::deque<EvtSpan>::const_iterator it = Q.begin(), end = Q.end();

        for( ; it != end; ++it ) {

            if( it->end <= X->min_x )
                continue;

            vert[0] = vert[2] = (it->start - X->min_x)/span;
            vert[4] = vert[6] = (it->end   - X->min_x)/span;
            vert[1] = vert[7] =  1.0F;
            vert[3] = vert[5] = -1.0F;

            glVertexPointer( 2, GL_FLOAT, 0, vert );
            glDrawArrays( GL_QUADS, 0, 4 );
        }
    }

// -------
// Restore
// -------

    glPolygonMode( GL_FRONT, saved_polygonmode[0] );
}


// Refer to scaling for analog case.
// Here we'll work again in yval units [-1,1],
// hence, adopt the same scale = ypxPerGrf/clipHgt.
//
void MGraph::draw1Digital( int iy )
{
    glEnableClientState( GL_COLOR_ARRAY );

    std::vector<Vec2f>  &V      = X->verts;
    std::vector<quint8> &C      = X->vdigclr;
    MGraphY             *Y      = X->Y[iy];
    const float         *y;
    int                 clipHgt = height();

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
void MGraph::draw1BinMax( int iy )
{
    std::vector<Vec2f>  &V      = X->verts2x;
    MGraphY             *Y      = X->Y[iy];
    const float         *y,
                        *y2;
    int                 clipHgt = height();

    float   y0_px   = (iy+0.5F)*X->ypxPerGrf,
            yscl    = 2.0F / clipHgt,
            y0      = 1.0F - yscl*(y0_px - X->clipTop),
            scl     = Y->yscl * X->ypxPerGrf / clipHgt;
    uint    len     = Y->yval.all( (float* &)y );

    Y->yval2.all( (float* &)y2 );

    X->applyGLTraceClr( iy );

    for( uint i = 0; i < len; ++i ) {
        V[2*i].y    = y0 + scl*y[i];
        V[2*i+1].y  = y0 + scl*y2[i];
    }

    glVertexPointer( 2, GL_FLOAT, 0, &V[0] );
    glDrawArrays( GL_LINE_STRIP, 0, 2*len );
}


// The whole viewport logical span is [-1,1] and clipHgt pixel span.
// yvals are in range [-1,1].
// To scale yvals into pixels, mult by ypxPerGrf/2.
// To scale that into the viewport, mult by 2/clipHgt.
// Hence, scale = ypxPerGrf/clipHgt.
//
void MGraph::draw1Analog( int iy )
{
    std::vector<Vec2f>  &V      = X->verts;
    MGraphY             *Y      = X->Y[iy];
    const float         *y;
    int                 clipHgt = height();

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
        else if( X->Y[iy]->drawBinMax )
            draw1BinMax( iy );
        else
            draw1Analog( iy );
    }

// ------
// Cursor
// ------

    X->spanMtx.lock();

    if( X->drawCursor
        && ny
        && X->spanSecs() >= 0.329
        && X->Y[0]->yval.size() ) {

        QColor  c( 0xa0, 0xa0, 0xa0 );
        glColor4f( c.redF(), c.greenF(), c.blueF(), c.alphaF() );

        GLfloat x   = X->Y[0]->yval.cursor();
        GLfloat h[] = {x, -1.0F, x, 1.0F};
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }

    X->spanMtx.unlock();

// -------
// Restore
// -------

    //glPointSize( savedWidth );
    glLineWidth( savedWidth );
    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
}


static void qt_save_gl_state( bool bAll )
{
    glPushClientAttrib( GL_CLIENT_ALL_ATTRIB_BITS );
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    if( bAll ) {

        glMatrixMode( GL_TEXTURE );
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode( GL_PROJECTION );
        glPushMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();

        glShadeModel( GL_FLAT );
        glDisable( GL_CULL_FACE );
        glDisable( GL_LIGHTING );
        glDisable( GL_STENCIL_TEST );
        glDisable( GL_DEPTH_TEST );
        glEnable( GL_BLEND );
        glBlendFunc( GL_ONE, GL_ONE_MINUS_SRC_ALPHA );
    }
}


static void qt_restore_gl_state( bool bAll )
{
    if( bAll ) {

        glMatrixMode( GL_TEXTURE );
        glPopMatrix();
        glMatrixMode( GL_PROJECTION );
        glPopMatrix();
        glMatrixMode( GL_MODELVIEW );
        glPopMatrix();
    }

    glPopAttrib();
    glPopClientAttrib();
}


static void qt_gl_draw_text(
    QPainter        &p,
    int             x,
    int             y,
    const QString   &str,
    const QFont     &font )
{
    GLfloat color[4];
    glGetFloatv( GL_CURRENT_COLOR, &color[0] );

    QColor  col;
    col.setRgbF( color[0], color[1], color[2], color[3] );

    QPen    old_pen     = p.pen();
    QFont   old_font    = p.font();

    p.setPen( col );
    p.setFont( font );
    p.setRenderHints(
        QPainter::Antialiasing | QPainter::TextAntialiasing );
    p.drawText( x, y, str );

    p.setPen( old_pen );
    p.setFont( old_font );
}


static inline void transformPoint(
    GLdouble        out[4],
    const GLdouble  m[16],
    const GLdouble  in[4] )
{
#define M(row,col)   m[row+4*col]
    out[0] =
        M(0,0)*in[0] + M(0,1)*in[1] + M(0,2)*in[2] + M(0,3)*in[3];
    out[1] =
        M(1,0)*in[0] + M(1,1)*in[1] + M(1,2)*in[2] + M(1,3)*in[3];
    out[2] =
        M(2,0)*in[0] + M(2,1)*in[1] + M(2,2)*in[2] + M(2,3)*in[3];
    out[3] =
        M(3,0)*in[0] + M(3,1)*in[1] + M(3,2)*in[2] + M(3,3)*in[3];
#undef M
}


static inline bool project(
    GLdouble        *winx,
    GLdouble        *winy,
    GLdouble        *winz,
    const GLint     viewport[4],
    const GLdouble  proj[16],
    const GLdouble  model[16],
    GLdouble        objx,
    GLdouble        objy,
    GLdouble        objz )
{
    GLdouble    out[4], in[4];

    in[0] = objx;
    in[1] = objy;
    in[2] = objz;
    in[3] = 1.0;
    transformPoint( out, model, in );
    transformPoint( in, proj, out );

    if( !in[3] ) {

        *winx = 0;
        *winy = 0;
        *winz = 0;
        return false;
    }

    in[0] /= in[3];
    in[1] /= in[3];
    in[2] /= in[3];

    *winx = viewport[0] + (1 + in[0]) * viewport[2] / 2;
    *winy = viewport[1] + (1 + in[1]) * viewport[3] / 2;
    *winz = (1 + in[2]) / 2;

    return true;
}


bool MGraph::isAutoBufSwap()
{
#ifdef OPENGL54
    return true;
#else
    return autoBufferSwap();
#endif
}


void MGraph::setAutoBufSwap( bool on )
{
#ifdef OPENGL54
    Q_UNUSED( on )
#else
    setAutoBufferSwap( on );
#endif
}


void MGraph::clipToView( int *view )
{
    GLint   _view[4];
    bool    use_scissor_testing = glIsEnabled( GL_SCISSOR_TEST );

    if( !use_scissor_testing ) {

        // If the user hasn't set a scissor box, we set one that
        // covers the current viewport.

        if( !view ) {
            glGetIntegerv( GL_VIEWPORT, &_view[0] );
            view = &_view[0];
        }

        QRect   viewport( view[0], view[1], view[2], view[3] );

        if( viewport != rect() ) {
            glScissor( view[0], view[1], view[2], view[3] );
            glEnable( GL_SCISSOR_TEST );
        }
    }
    else
        glEnable( GL_SCISSOR_TEST );
}


/*!
   Renders the string \a str into the GL context of this widget.

   \a x and \a y are specified in window coordinates, with the origin
   in the upper left-hand corner of the window. If \a font is not
   specified, the currently set application font will be used to
   render the string. To change the color of the rendered text you can
   use the glColor() call (or the qglColor() convenience function),
   just before the renderText() call.

   The \a listBase parameter is obsolete and will be removed in a
   future version of Qt.

   \note This function clears the stencil buffer.

   \note This function is not supported on OpenGL/ES systems.

   \note This function temporarily disables depth-testing when the
   text is drawn.

   \note This function can only be used inside a
   QPainter::beginNativePainting()/QPainter::endNativePainting() block
   if the default OpenGL paint engine is QPaintEngine::OpenGL. To make
   QPaintEngine::OpenGL the default GL engine, call
   QGL::setPreferredPaintEngine(QPaintEngine::OpenGL) before the
   QApplication constructor.

   \l{Overpainting Example}{Overpaint} with QPainter::drawText() instead.
*/

void MGraph::renderTextWin(
    int             x,
    int             y,
    const QString   &str,
    const QFont     &font )
{
    if( str.isEmpty() )
        return;

    qt_save_gl_state( false );

    clipToView( 0 );

    bool    auto_swap = isAutoBufSwap();
    setAutoBufSwap( false );

    QPainter    p( this );

    qt_gl_draw_text( p, x, y, str, font );

    p.end();
    setAutoBufSwap( auto_swap );

    qt_restore_gl_state( false );
}


/*! \overload

    \a x, \a y and \a z are specified in scene or object coordinates
    relative to the currently set projection and model matrices. This
    can be useful if you want to annotate models with text labels and
    have the labels move with the model as it is rotated etc.

    \note This function is not supported on OpenGL/ES systems.

    \note If depth testing is enabled before this function is called,
    then the drawn text will be depth-tested against the models that
    have already been drawn in the scene.  Use \c{glDisable(GL_DEPTH_TEST)}
    before calling this function to annotate the models without
    depth-testing the text.

    \l{Overpainting Example}{Overpaint} with QPainter::drawText() instead.
*/

void MGraph::renderTextMdl(
    double          x,
    double          y,
    double          z,
    const QString   &str,
    const QFont     &font )
{
    if( str.isEmpty() )
        return;

    GLdouble    model[4][4], proj[4][4];
    GLint       view[4];
    int         W = width(), H = height();
    bool        use_depth_testing = glIsEnabled( GL_DEPTH_TEST );

    glGetDoublev( GL_MODELVIEW_MATRIX, &model[0][0] );
    glGetDoublev( GL_PROJECTION_MATRIX, &proj[0][0] );
    glGetIntegerv( GL_VIEWPORT, &view[0] );

    project( &x, &y, &z, &view[0], &proj[0][0], &model[0][0], x, y, z );
    y = H - y;   // y is inverted

    qt_save_gl_state( true );

    clipToView( view );

    if( use_depth_testing )
        glEnable( GL_DEPTH_TEST );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, W, H );
    glOrtho( 0, W, H, 0, 0, 1 );
    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glTranslated( 0, 0, -z );

    glAlphaFunc( GL_GREATER, 0.0 );
    glEnable( GL_ALPHA_TEST );

    bool    auto_swap = isAutoBufSwap();
    setAutoBufSwap( false );

    QPainter    p( this );

    qt_gl_draw_text( p, qRound( x ), qRound( y ), str, font );

    p.end();
    setAutoBufSwap( auto_swap );

    qt_restore_gl_state( true );
}

/* ---------------------------------------------------------------- */
/* MGScroll ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

MGScroll::MGScroll( const QString &usr, QWidget *parent )
    :   QAbstractScrollArea(parent)
{
    theX    = new MGraphX;
    theM    = new MGraph( usr, this, theX );

// In former SpikeGLX versions we used setViewport( theM ) which
// worked fine for Qt5.3. However, theM fails to get all required
// messages in Qt5.4 and later (MGraph descends from QOpenGLWidget).
// The strategy of embedding theM into the default viewport works
// in all Qt versions tested (through 5.6).

    QVBoxLayout *VL = new QVBoxLayout( viewport() );
    VL->setSpacing( 0 );
    VL->setMargin( 0 );
    VL->addWidget( theM );

    theM->setImmedUpdate( true );
    theM->setMouseTracking( true );

    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
}


void MGScroll::scrollTo( int y )
{
    int sc_min  = verticalScrollBar()->minimum(),
        sc_max  = verticalScrollBar()->maximum();

    if( y < sc_min )
        y = sc_min;
    else if( y > sc_max )
        y = sc_max;

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

    theX->ypxPerGrf = qMax( theX->ypxPerGrf, 1 );

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

    scrollTo( theX->getSelY0() - theM->height() / 2 );
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
        adjustLayout();

    return theM->event( e );
}


