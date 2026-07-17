#ifndef SHANKVIEW_H
#define SHANKVIEW_H

#include "ShankMap.h"
#include "ShankViewUtils.h"
#include "IMROTbl.h"

#include <QMutex>
#include <QAbstractScrollArea>

/* ---------------------------------------------------------------- */
/* SVAnaRgn ------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SVAnaRgn {
    int     row0, rowN;
    quint8  shk, r, g, b;
    SVAnaRgn( int row0, int rowN, int shk, int r, int g, int b )
    : row0(row0), rowN(rowN), shk(shk), r(r), g(g), b(b)    {}
};

/* ---------------------------------------------------------------- */
/* ShankView ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

class ShankView : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

    friend class ShankScroll;

// Indexing
// --------
// Rects vR and colors vC store entry (s,c,r) at:
//  [s*(nc*nr) + c*(nr) + r] * sizeof(entry).

private:
    std::vector<SColor>         lut;
    const ShankMap              *smap;
    QMap<ShankMapDesc,uint>     ISM;
    std::vector<SVAnaRgn>       vA;
    std::vector<float>          vG,     // virtual grid
                                vR,     // active sites
                                vBlue,  // blue emitter centers
                                vRed;   // red emitter centers
    std::vector<SColor>         vGclr,  // grid cell visibility
                                vRclr;  // active colors
    std::vector<int>            col2vis_ev,
                                col2vis_od;
    std::vector<qint16>         vis_evn,
                                vis_odd;
    std::vector<IMRO_Site>      vX,     // excludes
                                vI;     // includes
    std::vector<IMRO_ROI>       vROI,
                                vW;     // where
    mutable QMutex              dataMtx;
    float                       shkWid,
                                hlfWid,
                                pmrg,
                                colWid;
    int                         rowPix,
                                _ncolhwr,
                                bnkRws, // nonzero => imec
                                slidePos,
                                vBot,
                                vTop,
                                sel,
                                iBlue,
                                iRed;
    uint8_t                     sr_mask;

public:
    ShankView( QWidget *parent = 0 );

    void setRowPix( int y )     {QMutexLocker ml( &dataMtx ); rowPix = y;}
    void setSlider( int y )     {QMutexLocker ml( &dataMtx ); slidePos = y;}

    int idealWidth();
    int deltaWidth();

    void setShankMap( const ShankMap *map );
    const ShankMap *getSmap()   {return smap;}
    void setSel( int ic, bool update = true );
    int getSel()                {return sel;}

    void setImro( const IMROTbl *R, uint8_t sr_mask );
    void setROI( tconstImroROIs vR, tconstImroSites vX )
        {QMutexLocker ml( &dataMtx ); vROI = vR; this->vX = vX;}
    void setWhere( tconstImroROIs vW, tconstImroSites vI )
        {QMutexLocker ml( &dataMtx ); this->vW = vW; this->vI = vI;}

    void colorPads( const double *val, double rngMax );
    void setAnatomy( const std::vector<SVAnaRgn> &vA )
        {QMutexLocker ml( &dataMtx ); this->vA = vA;}
    void setEmitters( int blue, int red )
        {QMutexLocker ml( &dataMtx ); iBlue = blue, iRed = red;}

signals:
    void cursorOver( int ic, bool shift );
    void lbutClicked( int ic, bool shift );
    void lbutReleased();
    void gridHover( int s, int r, bool quiet = false );
    void gridClicked( int s, int c, int r, bool shift, bool ctrl );

public slots:
    void updateNow()    {update();}

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

    void mouseMoveEvent( QMouseEvent *evt );
    void mousePressEvent( QMouseEvent *evt );
    void mouseReleaseEvent( QMouseEvent *evt );

private:
    float viewportPix();
    float spanPix();
    void setClipping();
    void resizePads();
    void drawAnatomy();
    void drawTips();
    void drawTipXs();
    void drawShanks();
    void drawTops();
    void drawGrid();
    void drawPads();
    void drawSel();
    void drawBanks();
    void drawOpto();
    void drawExcludes();
    void drawIncludes();
    void drawROIs();
    void drawWheres();
    void drawTri( float l, float t, float w, float h, SColor c );
    void drawRect( float l, float t, float w, float h, SColor c );
    bool evt2Pad( int &s, int &c, int &r, const QMouseEvent *evt );
    int getSelY();
    int getRowYDelta( int row );
};

/* ---------------------------------------------------------------- */
/* ShankScroll ---------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankScroll : public QAbstractScrollArea
{
    Q_OBJECT

public:
    ShankView   *theV;

public:
    ShankScroll( QWidget *parent = 0 );

    void setRowPix( int rPix );
    void scrollTo( int y );
    void adjustLayout();

public slots:
    void scrollToSelected();
    void scrollToRow( int row );

protected:
    virtual void resizeEvent( QResizeEvent *e );
    virtual void scrollContentsBy( int dx, int dy );
    virtual bool viewportEvent( QEvent *e );
};

#endif  // SHANKVIEW_H


