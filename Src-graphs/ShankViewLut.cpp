
#include "ShankViewLut.h"

#ifdef Q_WS_MACX
#include <gl.h>
#include <agl.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif


/* ---------------------------------------------------------------- */
/* MACROS --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define BCKCLR  0.0f
#define MRGPX   0

/* ---------------------------------------------------------------- */
/* ShankViewLut --------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankViewLut::ShankViewLut( QWidget *parent )
#ifdef OPENGL54
    :   QOpenGLWidget(parent),
#else
    :
#endif
        inited(false)
{
    QGLFormat   fmt;
    fmt.setSwapInterval( 0 );

    QGLWidget( fmt, parent );

    setAutoFillBackground( false );
    setUpdatesEnabled( true );

    loadLut( lut );
}


// Note: makeCurrent() called automatically.
//
void ShankViewLut::initializeGL()
{
#ifdef OPENGL54
    initializeOpenGLFunctions();
#else
    initializeGLFunctions();
#endif

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    glEnableClientState( GL_VERTEX_ARRAY );

    inited = true;
}


// Note: makeCurrent() called automatically.
//
void ShankViewLut::resizeGL( int w, int h )
{
    if( !inited )
        return;

// ------------
// Coord system
// ------------

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( MRGPX, MRGPX, w - 2*MRGPX, h - 2*MRGPX );
    gluOrtho2D( 0.0, 1.0, 0.0, 256.0 );
}


// Note: makeCurrent() called automatically.
//
void ShankViewLut::paintGL()
{
// -----
// Setup
// -----

#ifdef OPENGL54
//    glClear(...);
#endif

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

// -----
// Paint
// -----

    glClearColor( BCKCLR, BCKCLR, BCKCLR, 1 );
    glClear( GL_COLOR_BUFFER_BIT );

    drawLines();

// -------
// Restore
// -------
}


void ShankViewLut::drawLines()
{
    const SColor    *c = &lut[0];
    GLfloat         h[] = {0.0F, 0.0F, 1.0F, 0.0F}; // x1,y1,x2,y2

    for( int i = 0; i < 256; ++i, ++c ) {

        glColor3ub( c->r, c->g, c->b );

        h[1] = h[3] = i;
        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }
}


