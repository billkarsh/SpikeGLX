
#include "ShankView.h"

#include <QMouseEvent>
#include <QScrollBar>

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

#define PTR( a )    (const void*)(a)

#define BCKCLR  0.9f
#define SHKCLR  0.55f
#define GRDCLR  0.35f

// Shank spanPix() = TIPPX + nr*rowPix + TOPPX.
// x-coords are in range [-1,1].
// y-coords are in range [0,spanPix()].

#define MRGPX   8
#define ROIPX   16
#define PADMRG  2
#define VLFT    -1.0f
#define VRGT    1.0f
#define WIDMAX  1.5f
#define SHKSEP  0.5f
#define COLSEP  0.5f
#define ROWSEP  0.5f
#define TIPPX   (4*rowPix)
#define TOPPX   rowPix

/* ---------------------------------------------------------------- */
/* ShankView ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

ShankView::ShankView( QWidget *parent )
#ifdef OPENGL54
    :   QOpenGLWidget(parent),
#else
    :
#endif
        smap(0), rowPix(8), bnkRws(0), slidePos(0), sel(0)
{
#ifndef OPENGL54
    QGLFormat   fmt;
    fmt.setSwapInterval( 0 );
    QGLWidget( fmt, parent );
#endif

    setAutoFillBackground( false );
    setUpdatesEnabled( true );

    loadLut( lut );
}


int ShankView::idealWidth()
{
    QMutexLocker    ml( &dataMtx );

    int     ns  = smap->ns,
            nc  = smap->nc;
    float   pad = rowPix/(1.0f+ROWSEP),
            shk = 2*PADMRG + nc*pad + (nc-1)*pad*COLSEP;
    return 2*MRGPX + 2*ROIPX + ns*shk + (ns-1)*shk*SHKSEP;
}


int ShankView::deltaWidth()
{
    return idealWidth() - width();
}


void ShankView::setShankMap( const ShankMap *map )
{
    dataMtx.lock();
        smap = map;
        map->inverseMap( ISM );
    dataMtx.unlock();

    resizePads();

    setMouseTracking( true );
    setCursor( Qt::CrossCursor );

    updateNow();
}


void ShankView::setSel( int ic, bool update )
{
    dataMtx.lock();
        sel = ic;
    dataMtx.unlock();

    if( update )
        updateNow();
}


void ShankView::setImro( const IMROTbl *R )
{
    QMutexLocker    ml( &dataMtx );

    col2vis_ev = R->col2vis_ev;
    col2vis_od = R->col2vis_od;

    int nC = R->nCol_vis();
    vis_evn.assign( nC, 0 );
    vis_odd.assign( nC, 0 );

    nC = R->col2vis_ev.size();
    for( int ic = 0; ic < nC; ++ic ) {
        vis_evn[col2vis_ev[ic]] = 1;
        vis_odd[col2vis_od[ic]] = 1;
    }

    _ncolhwr    = R->nCol_hwr();
    bnkRws      = R->nAP() / _ncolhwr;
}


// Compare each val[i] to range [0..rngMax] and assign
// appropriate lut color to the vRclr[{i}] for that pad.
//
// Assumed: val.size() = smap->e.size().
//
void ShankView::colorPads( const double *val, double rngMax )
{
    QMutexLocker    ml( &dataMtx );

    if( !smap )
        return;

    for( int i = 0, ne = smap->e.size(); i < ne; ++i ) {

        if( !smap->e[i].u )
            continue;

        int ilut = 0;

        if( val[i] > 0 ) {

            if( val[i] >= rngMax )
                ilut = 255;
            else
                ilut = 255 * val[i]/rngMax;
        }

        SColor  *C = &vRclr[4*i];

        C[0] = lut[ilut];
        C[1] = lut[ilut];
        C[2] = lut[ilut];
        C[3] = lut[ilut];
    }
}


void ShankView::setAnatomyPP()
{
    QMutexLocker    ml( &dataMtx );
}


// Note: makeCurrent() called automatically.
//
void ShankView::initializeGL()
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
}


// Note: makeCurrent() called automatically.
//
void ShankView::resizeGL( int w, int h )
{
    if( !isValid() )
        return;

// ------------
// Coord system
// ------------

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    glViewport( MRGPX, MRGPX, w - 2*MRGPX, h - 2*MRGPX );

    resizePads();
}


// Note: makeCurrent() called automatically.
//
void ShankView::paintGL()
{
// -----
// Setup
// -----

    QMutexLocker    ml( &dataMtx );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

#ifdef OPENGL54
    glViewport( MRGPX, MRGPX, width() - 2*MRGPX, height() - 2*MRGPX );
#endif

    setClipping();
    gluOrtho2D( VLFT, VRGT, vBot, vTop );

// -----
// Paint
// -----

    glClearColor( BCKCLR, BCKCLR, BCKCLR, 1 );
    glClear( GL_COLOR_BUFFER_BIT );

    if( !smap || !smap->nr )
        return;

    drawTips();
    drawShanks();
    drawTops();
    drawGrid();
    drawPads();
    drawBanks();
    drawExcludes();
    drawROIs();
    drawSel();

// -------
// Restore
// -------
}


void ShankView::mouseMoveEvent( QMouseEvent *evt )
{
    int s, c, r;

    if( evt2Pad( s, c, r, evt ) ) {

        QMap<ShankMapDesc,uint>::iterator   it;

        it = ISM.find( ShankMapDesc( s, c, r, 1 ) );

        if( it != ISM.end() ) {

            emit( cursorOver( it.value(), evt->modifiers() & Qt::SHIFT ) );
            return;
        }

        emit( gridHover( s, c, r ) );
        return;
    }

    emit( cursorOver( -1, false ) );
    emit( gridHover( -1, -1, -1 ) );
}


void ShankView::mousePressEvent( QMouseEvent *evt )
{
    int s, c, r;

    if( evt2Pad( s, c, r, evt ) ) {

        QMap<ShankMapDesc,uint>::iterator   it;

        it = ISM.find( ShankMapDesc( s, c, r, 1 ) );

        if( it != ISM.end() ) {

            emit( lbutClicked(
                it.value(),
                (evt->modifiers() & Qt::SHIFT)
                || (evt->buttons() & Qt::RightButton) ) );
        }

        emit( gridClicked( s, c, r ) );
    }
}


float ShankView::viewportPix()
{
    return height() - 2*MRGPX;
}


float ShankView::spanPix()
{
    return TIPPX + rowPix*smap->nr + TOPPX;
}


void ShankView::setClipping()
{
    vTop = spanPix() - slidePos;
    vBot = vTop - viewportPix();
}


// In view width (V) we fit (s) shanks of width (w)
// and (s-1) spaces of width (f*w), so,
//
//  s*w + (s-1)*f*w = V,
//  w = V / (s + (s-1)*f).
//
// w no larger than WIDMAX.
//
// Same reasoning used for H pad sizing.
// V pad sizing set by GUI.
//
// A - D
// |   |
// B - C
//
void ShankView::resizePads()
{
    QMutexLocker    ml( &dataMtx );

    int w = width() - 2*MRGPX;

    if( !smap || w <= 0 ) {

        vG.clear();
        vR.clear();
        return;
    }

    int ns = smap->ns,
        nc = smap->nc,
        nr = smap->nr,
        ng = ns*nc*nr,
        ne = smap->e.size();

    shkWid = (VRGT-VLFT-2*ROIPX*(VRGT-VLFT)/w) / (ns + (ns-1)*SHKSEP);

    if( shkWid > WIDMAX )
        shkWid = WIDMAX;

    hlfWid = shkWid * (ns + (ns-1)*SHKSEP) / 2;

    if( int(vG.size()) != 8*ng )
        vG.resize( 8*ng );              // 2 float/vtx, 4 vtx/rect

    if( int(vGclr.size()) != 4*ng )
        vGclr.assign( 4*ng, SColor() ); // 1 color/vtx, 4 vtx/rect

    if( int(vR.size()) != 8*ne )
        vR.resize( 8*ne );              // 2 float/vtx, 4 vtx/rect

    if( int(vRclr.size()) != 4*ne )
        vRclr.assign( 4*ne, SColor() ); // 1 color/vtx, 4 vtx/rect

    pmrg    = PADMRG*(VRGT-VLFT)/w;
    colWid  = (shkWid - 2*pmrg)/(nc + (nc-1)*COLSEP);

    float           *V;
    SColor          *C;
    const qint16    *visevn = &vis_evn[0],
                    *visodd = &vis_odd[0];
    float           sStep   = shkWid*(1.0f+SHKSEP),
                    cStep   = colWid*(1.0f+COLSEP),
                    hPad    = rowPix/(1.0f+ROWSEP);

    if( vG.size() ) {

        V = &vG[0];
        C = &vGclr[0];

        float   L, R, B, T, c;

        for( int is = 0; is < ns; ++is ) {

            for( int ic = 0; ic < nc; ++ic ) {

                L = -hlfWid + is*sStep + pmrg + ic*cStep;
                R = L + colWid;

                for( int ir = 0; ir < nr; ++ir, V += 8, C += 4 ) {

                    B = TIPPX + ir*rowPix;
                    T = B + hPad;

                    V[0] = L;
                    V[1] = T;
                    V[2] = L;
                    V[3] = B;
                    V[4] = R;
                    V[5] = B;
                    V[6] = R;
                    V[7] = T;

                    if( ir & 1 )
                        c = (visodd[ic] ? GRDCLR : SHKCLR);
                    else
                        c = (visevn[ic] ? GRDCLR : SHKCLR);

                    memset( C, c*255, 4*sizeof(SColor) );
                }
            }
        }
    }

    V = &vR[0];

    int c = SHKCLR*255;

    for( int i = 0; i < ne; ++i, V += 8 ) {

        const ShankMapDesc  &E = smap->e[i];
        float               L, R, B, T;

        L = -hlfWid + E.s*sStep + pmrg + E.c*cStep;
        R = L + colWid;
        B = TIPPX + E.r*rowPix;
        T = B + hPad;

        V[0] = L;
        V[1] = T;
        V[2] = L;
        V[3] = B;
        V[4] = R;
        V[5] = B;
        V[6] = R;
        V[7] = T;

        if( !E.u )
            memset( &vRclr[4*i], c, 4*sizeof(SColor) );
    }
}


void ShankView::drawTips()
{
    int                 ns = smap->ns,
                        nv = 3 * ns,
                        nf = 2 * nv;
    float               lf = -hlfWid,
                        md = shkWid/2;
    std::vector<float>  vert( nf );

    for( int i = 0; i < ns; ++i, lf += shkWid*(1.0f+SHKSEP) ) {

        float   *V = &vert[6*i];

        V[0]    = lf;
        V[1]    = TIPPX;

        V[2]    = lf + md;
        V[3]    = 0.0f;

        V[4]    = lf + shkWid;
        V[5]    = TIPPX;
    }

    glColor3f( SHKCLR, SHKCLR, SHKCLR );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, &vert[0] );
    glDrawArrays( GL_TRIANGLES, 0, nv );
}


void ShankView::drawShanks()
{
    int                 ns = smap->ns,
                        nv = 4 * ns,
                        nf = 2 * nv;
    float               lf = -hlfWid,
                        tp = spanPix() - TOPPX;
    std::vector<float>  vert( nf );

    for( int i = 0; i < ns; ++i, lf += shkWid*(1.0f+SHKSEP) ) {

        float   *V = &vert[8*i];

        V[0]    = lf;
        V[1]    = tp;

        V[2]    = lf;
        V[3]    = TIPPX;

        V[4]    = lf + shkWid;
        V[5]    = TIPPX;

        V[6]    = lf + shkWid;
        V[7]    = tp;
    }

    glColor3f( SHKCLR, SHKCLR, SHKCLR );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, &vert[0] );
    glDrawArrays( GL_QUADS, 0, nv );
}


void ShankView::drawTops()
{
    drawRect( -hlfWid, spanPix(), 2*hlfWid, TOPPX, SColor( SHKCLR*255 ) );
}


void ShankView::drawGrid()
{
    if( !bnkRws || !vG.size() )
        return;

    glEnableClientState( GL_COLOR_ARRAY );

    glColorPointer( 3, GL_UNSIGNED_BYTE, 0, &vGclr[0] );
    glPolygonMode( GL_FRONT, GL_LINE );
    glVertexPointer( 2, GL_FLOAT, 0, &vG[0] );
    glDrawArrays( GL_QUADS, 0, vG.size()/2 );

    glDisableClientState( GL_COLOR_ARRAY );
}


void ShankView::drawPads()
{
    if( !vR.size() )
        return;

    glEnableClientState( GL_COLOR_ARRAY );

    glColorPointer( 3, GL_UNSIGNED_BYTE, 0, &vRclr[0] );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, &vR[0] );
    glDrawArrays( GL_QUADS, 0, vR.size()/2 );

    glDisableClientState( GL_COLOR_ARRAY );
}


// A - D
// |   |
// B - C
//
void ShankView::drawSel()
{
    if( sel < 0 || !vR.size() )
        return;

    float   *sv     = &vR[8*sel],
            xoff    = 4*(VRGT-VLFT)/(width()-2*MRGPX),
            yoff    = 4,
            vert[8];

    vert[0] = sv[0] - xoff;
    vert[1] = sv[1] + yoff;
    vert[2] = sv[2] - xoff;
    vert[3] = sv[3] - yoff;
    vert[4] = sv[4] + xoff;
    vert[5] = sv[5] - yoff;
    vert[6] = sv[6] + xoff;
    vert[7] = sv[7] + yoff;

    glLineWidth( 4.0f );

    glColor3f( 0, 0, 0 );
    glPolygonMode( GL_FRONT, GL_LINE );
    glVertexPointer( 2, GL_FLOAT, 0, vert );
    glDrawArrays( GL_QUADS, 0, 4 );

    glLineWidth( 1.0f );
}


void ShankView::drawBanks()
{
    if( !bnkRws )
        return;

    GLfloat h[] = {-1.0f, 0.0f, 1.0f, 0.0f};

    float   halfSep = 0.5f * ROWSEP/(1.0f + ROWSEP);
    int     nL      = smap->nr / bnkRws;

    if( nL * bnkRws >= smap->nr )
        --nL;

    glColor3f( GRDCLR, GRDCLR, GRDCLR );

    for( int iL = 1; iL <= nL; ++iL ) {

        h[1] = h[3] = TIPPX + (iL*bnkRws - halfSep) * rowPix;

        glVertexPointer( 2, GL_FLOAT, 0, h );
        glDrawArrays( GL_LINES, 0, 2 );
    }
}


// A - D
// |   |
// B - C
//
void ShankView::drawExcludes()
{
    if( !bnkRws )
        return;

    int nx = vX.size();

    if( !nx )
        return;

    GLfloat vert[8];
    float   xoff    = 2*(VRGT-VLFT)/(width()-2*MRGPX),
            yoff    = 2;
    int     nc      = smap->nc,
            nr      = smap->nr;

    glLineWidth( 2.0f );

    glColor3f( 0, 0, 0 );
    glPolygonMode( GL_FRONT, GL_LINE );

    for( int ix = 0; ix < nx; ++ix ) {

        IMRO_Site   &S = vX[ix];

        int     c  = (S.r & 1 ? col2vis_od[S.c] : col2vis_ev[S.c]);
        float   *V = &vG[8*(S.s*(nc*nr)+c*nr+S.r)];

        vert[0] = V[0] - xoff;
        vert[1] = V[1] + yoff;
        vert[2] = V[2] - xoff;
        vert[3] = V[3] - yoff;
        vert[4] = V[4] + xoff;
        vert[5] = V[5] - yoff;
        vert[6] = V[6] + xoff;
        vert[7] = V[7] + yoff;

        glVertexPointer( 2, GL_FLOAT, 0, vert );
        glDrawArrays( GL_QUADS, 0, 4 );
    }

    glLineWidth( 1.0f );
}


void ShankView::drawROIs()
{
    if( !bnkRws )
        return;

    int nb = vROI.size();

    if( !nb )
        return;

    glLineWidth( 4.0f );

    glColor3f( 0.0f, 0.75f, 0.0f );
    glPolygonMode( GL_FRONT, GL_LINE );

    float   vsep    = ROWSEP/(1.0f + ROWSEP),
            dv      = 0.33f * vsep * rowPix,
            dh      = 0.20f * colWid*COLSEP;
    int     nc      = smap->nc,
            nr      = smap->nr;

    for( int ib = 0; ib < nb; ++ib ) {

        IMRO_ROI    &B = vROI[ib];
        float       *V;
        float       vert[8];
        int         nrow    = B.rLim - B.r0,
                    c0      = qMax( 0, B.c0 ),
                    cL      = (B.cLim >= 1 ? B.cLim : _ncolhwr) - 1;

        if( nrow > 1 ) {
            c0 = qMin( col2vis_ev[c0], col2vis_od[c0] );
            cL = qMax( col2vis_ev[cL], col2vis_od[cL] );
        }
        else if( B.r0 & 1 ) {
            c0 = col2vis_od[c0];
            cL = col2vis_od[cL];
        }
        else {
            c0 = col2vis_ev[c0];
            cL = col2vis_ev[cL];
        }

        V  = &vG[8*(B.s*nc*nr+c0*nr)];
        vert[0] = vert[2] = V[0] - dh;

        V  = &vG[8*(B.s*nc*nr+cL*nr)];
        vert[4] = vert[6] = V[6] + dh;

        V = &vG[8*(B.rLim-1)];
        vert[1] = vert[7] = V[1] + dv;

        V = &vG[8*B.r0];
        vert[3] = vert[5] = V[3] - dv;

        glVertexPointer( 2, GL_FLOAT, 0, vert );
        glDrawArrays( GL_QUADS, 0, 4 );
    }

    glLineWidth( 1.0f );
}


void ShankView::drawTri( float l, float t, float w, float h, SColor c )
{
    float vert[6] = {
            l    , t,
            l+w/2, t+h,
            l+w  , t };

    glColor3ub( c.r, c.g, c.b );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, vert );
    glDrawArrays( GL_TRIANGLES, 0, 3 );
}


// A - D
// |   |
// B - C
//
void ShankView::drawRect( float l, float t, float w, float h, SColor c )
{
    float vert[8] = {
            l  , t,
            l  , t-h,
            l+w, t-h,
            l+w, t };

    glColor3ub( c.r, c.g, c.b );
    glPolygonMode( GL_FRONT, GL_FILL );
    glVertexPointer( 2, GL_FLOAT, 0, vert );
    glDrawArrays( GL_QUADS, 0, 4 );
}


// Coords from event record:
// [L,R] = [0,width()],
// [T,B] = [0,height()].
//
// Return true if cursor in possible pad.
//
// Note: Pad may not be implemented (used).
//
bool ShankView::evt2Pad( int &s, int &c, int &r, const QMouseEvent *evt )
{
    float   w = width() - 2*MRGPX;

    if( !smap || w <= 0 )
        return false;

// To local view x-coords

    float   x = (VRGT-VLFT)*(evt->x()-MRGPX)/w + VLFT + hlfWid;

    if( x <= 0 )
        return false;

// Which shank and col

    float   ds = shkWid*(1.0f+SHKSEP),
            dc = colWid*(1.0f+COLSEP);

    s  = x / ds;
    x -= s*ds;

    if( x > shkWid )
        return false;

    x -= pmrg;

    if( x <= 0 )
        return false;

    x += colWid*COLSEP/2;   // half-gap margin

    c = x / dc;

// To local view y-coords

    float   vmrg = (rowPix/(1.0f+ROWSEP)) * ROWSEP/2,   // half-gap margin
            y    = spanPix() - (evt->y() - MRGPX + slidePos) - TIPPX + vmrg;

    if( y <= 0 )
        return false;

// Which row

    r = y / rowPix;

    if( r >= int(smap->nr) )
        return false;

    return true;
}


// Tip + sel*pads + 1/2 pad.
//
int ShankView::getSelY()
{
    if( sel < 0 || !smap || !smap->e.size() || width() <= 0 )
        return 0;

    return TIPPX + rowPix*smap->e[sel].r + rowPix/(2*(1.0f+ROWSEP));
}

/* ---------------------------------------------------------------- */
/* ShankScroll ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

ShankScroll::ShankScroll( QWidget *parent )
    :   QAbstractScrollArea(parent)
{
    theV = new ShankView( this );

    setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setViewport( theV );
}


void ShankScroll::setRowPix( int rPix )
{
    theV->setRowPix( rPix );
    theV->resizePads();
    adjustLayout();
    scrollToSelected();
}


void ShankScroll::scrollTo( int y )
{
    theV->setSlider( y );
    verticalScrollBar()->setSliderPosition( y );
}


void ShankScroll::adjustLayout()
{
    int vh = theV->viewportPix();

    verticalScrollBar()->setPageStep( vh );
    verticalScrollBar()->setRange( 0, theV->spanPix() - vh );
    updateGeometry();
}


void ShankScroll::scrollToSelected()
{
    int sc_min  = verticalScrollBar()->minimum(),
        sc_max  = verticalScrollBar()->maximum(),
        pos     = theV->spanPix() - theV->getSelY() - theV->viewportPix()/2;

    if( pos < sc_min )
        pos = sc_min;
    else if( pos > sc_max )
        pos = sc_max;

    if( pos != theV->slidePos )
        scrollTo( pos );
    else
        theV->update();
}


void ShankScroll::resizeEvent( QResizeEvent *e )
{
    Q_UNUSED( e )

    adjustLayout();

    theV->makeCurrent();
    theV->resizeGL( theV->width(), theV->height() );

#ifdef OPENGL54
    theV->update();
#endif
}


void ShankScroll::scrollContentsBy( int dx, int dy )
{
    Q_UNUSED( dx )
    Q_UNUSED( dy )

    theV->setSlider( verticalScrollBar()->sliderPosition() );
    theV->update();
}


bool ShankScroll::viewportEvent( QEvent *e )
{
    QEvent::Type    type = e->type();

    if( type == QEvent::Resize )
        return QAbstractScrollArea::viewportEvent( e );
    else
        return theV->event( e );
}


