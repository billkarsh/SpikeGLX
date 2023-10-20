
#include "SOGraph.h"
#include "Util.h"

#include <QPainter>

#include <math.h>

#ifdef Q_WS_MACX
#include <gl.h>
#include <agl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


SOGraph::SOGraph( QWidget *parent )
    :   QOpenGLWidget(parent), yave(0), yscl(500), label(0)
{
    for( int it = 0; it < NPNT; ++it )
        V[it].x = it;

    clear();

    setAutoFillBackground( false );
    setUpdatesEnabled( true );
    setFont( QFont( "Lucida Sans Typewriter", -1 ) );
}


void SOGraph::clear()
{
    QMutexLocker    ml( &dataMtx );

    memset( wring, 0, sizeof(wring) );
    memset( mean, 0, sizeof(mean) );
    whead = 0;
    wn    = 0;
}


// sampIdx <  0: retire entries >= RETIRESECS sec old.
// sampIdx >= 0: add given wave.
//
void SOGraph::addWave( const float *src, qint64 sampIdx )
{
#define RETIRESECS  30

    QMutexLocker ml( &dataMtx );

    if( sampIdx < 0 ) {

        // pop oldest?

        if( wn > 0 && (-sampIdx - wring[whead].tstamp)/sRate >= RETIRESECS ) {

            whead = (whead + 1) % NWAV;
            --wn;
        }
    }
    else {

        // Add at cursor

        int cursor = (whead + wn) % NWAV,
            newlen = qMin( wn + 1, NWAV );

        wring[cursor].tstamp = sampIdx;
        memcpy( wring[cursor].y, src, NPNT*sizeof(float) );

        whead = (whead + wn + 1 - newlen) % NWAV;
        wn    = newlen;
    }

// Average

    memset( mean, 0, sizeof(mean) );

    for( int iw = 0; iw < wn; ++iw ) {
        const float *pw = wring[(whead + iw) % NWAV].y;
        for( int it = 0; it < NPNT; ++it )
            mean[it] += pw[it];
    }

    float   mmin,
            mmax = mmin = mean[0] / wn;

    for( int it = 0; it < NPNT; ++it ) {
        mean[it] /= wn;
        if( mean[it] < mmin ) mmin = mean[it];
        if( mean[it] > mmax ) mmax = mean[it];
    }

    // adaptive baseline...only update if changes 40%

    mmax = 0.5 * (mmax + mmin);

    if( fabs( (mmax - yave)/yave ) > 0.40 )
        yave = mmax;

// Update

    QMetaObject::invokeMethod( this, "update", Qt::QueuedConnection );
}


// Note: makeCurrent() called automatically.
//
void SOGraph::initializeGL()
{
    initializeOpenGLFunctions();

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnableClientState( GL_VERTEX_ARRAY );
}


// Note: makeCurrent() called automatically.
//
void SOGraph::resizeGL( int w, int h )
{
    if( !isValid() )
        return;

// ------------
// Coord system
// ------------

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( 0, 0, w, h );
}


// Note: makeCurrent() called automatically.
//
void SOGraph::paintGL()
{
// -----
// Setup
// -----

    int w = width(), h = height();

    if( !isVisible() || !w || !h )
        return;

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

// -----
// Paint
// -----

    glClearColor( 0, 0, 0, 1 );
    glClear( GL_COLOR_BUFFER_BIT );

    QMutexLocker    ml( &dataMtx );

    glPushMatrix();
    gluOrtho2D( 0, NPNT - 1, yave - yscl/2, yave + yscl/2 );
    drawPoints();
    glPopMatrix();

    gluOrtho2D( 0, w, h, 0 );
    drawRateBar();
    drawRateBox();
    drawLabels();

// -------
// Restore
// -------
}


double SOGraph::rate()
{
    double  rate = 0;

    if( wn >= 2 ) {

        for( int iw = 1; iw < wn; ++iw ) {
            rate += wring[(whead + iw)%NWAV].tstamp -
                    wring[(whead + iw-1)%NWAV].tstamp;
        }

        rate = (wn - 1) * sRate / rate;
    }

    return rate;
}


void SOGraph::drawPoints()
{
    glColor3ub( 0x60, 0x60, 0x20 );
    glLineWidth( 1.0f );

    for( int iw = 0; iw < wn; ++iw ) {
        const float *pw = wring[(whead + iw) % NWAV].y;
        for( int it = 0; it < NPNT; ++it )
            V[it].y = pw[it];
        glVertexPointer( 2, GL_FLOAT, 0, V );
        glDrawArrays( GL_LINE_STRIP, 0, NPNT );
    }

    glColor3ub( 0, 0xFF, 0 );

    for( int it = 0; it < NPNT; ++it )
        V[it].y = mean[it];
    glVertexPointer( 2, GL_FLOAT, 0, V );
    glDrawArrays( GL_LINE_STRIP, 0, NPNT );
}


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
// Scale bottom edge = 0.01, log10 = -2.
// Scale top    edge = 100,  log10 = 2.
// Scale span in log10 = 4.
//
void SOGraph::drawRateBar()
{
    double  R = rate();

    if( R <= 0.01 )
        return;

    float   vert[8];
    QRect   r   = rateRect().adjusted( 1, 1, -1, -1 );
    int     top = r.bottom()
                - r.height() * qBound( 0.0, (log10(R) + 2)/5, 1.0 );

    vert[0] = vert[2] = r.left();
    vert[6] = vert[4] = r.right();
    vert[1] = vert[7] = top;
    vert[3] = vert[5] = r.bottom();

    glColor3ub( 0, 0, 0xFF );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, vert );
    glDrawArrays( GL_QUADS, 0, 4 );
}


// GL_QUADS interprets array CCW fashion:
// A----D  (0,1)----(6,7)
// |    |    |        |
// B----C  (2,3)----(4,5)
//
void SOGraph::drawRateBox()
{
    float   vert[8];
    QRect   r = rateRect();

    vert[0] = vert[2] = r.left();
    vert[6] = vert[4] = r.right();
    vert[1] = vert[7] = r.top();
    vert[3] = vert[5] = r.bottom();

    glColor3ub( 0x80, 0x80, 0x80 );
    glLineWidth( 1.0f );
    glPolygonMode( GL_FRONT, GL_LINE );
    glVertexPointer( 2, GL_FLOAT, 0, vert );
    glDrawArrays( GL_QUADS, 0, 4 );
}


void SOGraph::drawLabels()
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

    if( !wasEnabled )
        glEnable( GL_LINE_STIPPLE );

    glPolygonMode( GL_FRONT, GL_FILL );

// -----
// Label
// -----

    glColor3ub( 0, 0x50, 0xFF );

    QFont   font = QFont();

    font.setPointSize( 20 );
    font.setWeight( QFont::Bold );

    QFontMetrics    FMl( font );
    int             ftHt = FMl.boundingRect( 'A' ).height();

    renderTextWin( 4, 4 + ftHt, QString("%1").arg( label ), font );

// -----
// Scale
// -----

    glColor3ub( 0xA0, 0xA0, 0xA0 );

    font.setPointSize( 8 );
    font.setWeight( QFont::Bold );

    QFontMetrics    FMs( font );
    ftHt = FMs.boundingRect( 'A' ).height() / 2;

    QRect   r  = rateRect();
    int     cl = (r.left() + r.right()) / 2,
            ht = r.height(),
            tx;

    tx = FMs.boundingRect( ".0.1" ).width();
    renderTextWin( cl - tx/2, r.bottom() - ht/5 + ftHt, "0.1", font );

    tx = FMs.boundingRect( "1" ).width();
    renderTextWin( cl - tx/2, r.bottom() - 2*ht/5 + ftHt, "1", font );

    tx = FMs.boundingRect( "10" ).width();
    renderTextWin( cl - tx/2, r.bottom() - 3*ht/5 + ftHt, "10", font );

    tx = FMs.boundingRect( "100" ).width();
    renderTextWin( cl - tx/2, r.bottom() - 4*ht/5 + ftHt, "100", font );

// -------
// Restore
// -------

    glColor4f( savedClr[0], savedClr[1], savedClr[2], savedClr[3] );
    glLineStipple( savedRepeat, savedPat );

    if( !wasEnabled )
        glDisable( GL_LINE_STIPPLE );
}


QRect SOGraph::rateRect()
{
    int h = 120,
        w = 32,
        m = 4,
        H = height(),
        W = width();

    return QRect( W - w - m, H - h - m, w, h );
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
    const QFont     &font,
    const QColor    &bkgnd_color )
{
    GLfloat color[4];
    glGetFloatv( GL_CURRENT_COLOR, &color[0] );

    QColor  col;
    col.setRgbF( color[0], color[1], color[2], color[3] );

    QPen    old_pen     = p.pen();
    QFont   old_font    = p.font();

    p.setBackground( bkgnd_color );
    p.setBackgroundMode( Qt::OpaqueMode );
    p.setPen( col );
    p.setFont( font );
    p.setRenderHints( QPainter::Antialiasing | QPainter::TextAntialiasing );
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


void SOGraph::clipToView( int *view )
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

void SOGraph::renderTextWin(
    int             x,
    int             y,
    const QString   &str,
    const QFont     &font )
{
    if( str.isEmpty() )
        return;

    qt_save_gl_state( false );

    clipToView( 0 );

    QPainter    p( this );

    qt_gl_draw_text( p, x, y, str, font, QColor(0, 0, 0, 1) );

    p.end();

    qt_restore_gl_state( false );
}


