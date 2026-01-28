
#include "ShankViewLut.h"


/* ---------------------------------------------------------------- */
/* MACROS --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

#define BCKCLR  0.0f
#define MRGPX   0

/* ---------------------------------------------------------------- */
/* ShankViewLut --------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankViewLut::ShankViewLut( QWidget *parent )
    :   QOpenGLWidget(parent)
{
    setAutoFillBackground( false );
    setUpdatesEnabled( true );

    loadLut( lut );
}


// Note: makeCurrent() called automatically.
//
void ShankViewLut::initializeGL()
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
void ShankViewLut::resizeGL( int w, int h )
{
    // Only called by OpenGL
    //
    // if( !isValid() )
    //     return;

// ------------
// Coord system
// ------------

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( MRGPX, MRGPX, w - 2*MRGPX, h - 2*MRGPX );
    glOrtho( 0.0, 1.0, 0.0, 256.0, -1, 1 );
}


// Note: makeCurrent() called automatically.
//
void ShankViewLut::paintGL()
{
// -----
// Setup
// -----

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


