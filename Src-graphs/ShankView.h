#ifndef SHANKVIEW_H
#define SHANKVIEW_H

#include "ShankMap.h"
#include "ShankViewUtils.h"
#include "IMROTbl.h"

#include <QMutex>
#include <QAbstractScrollArea>

/* ---------------------------------------------------------------- */
/* ShankView ------------------------------------------------------ */
/* ---------------------------------------------------------------- */

#ifdef OPENGL54
class ShankView : public QOpenGLWidget, protected QOpenGLFunctions
#else
class ShankView : public QGLWidget, protected QGLFunctions
#endif
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
    std::vector<float>          vR;
    std::vector<SColor>         vC;
    std::vector<IMRO_Site>      vStrike;
    std::vector<IMRO_ROI>       vROI;
    mutable QMutex              dataMtx;
    float                       shkWid,
                                hlfWid,
                                pmrg,
                                colWid;
    int                         rowPix,
                                bnkRws,
                                slidePos,
                                vBot,
                                vTop,
                                sel;

public:
    ShankView( QWidget *parent = 0 );

    void setRowPix( int y )     {QMutexLocker ml( &dataMtx ); rowPix = y;}
    void setSlider( int y )     {QMutexLocker ml( &dataMtx ); slidePos = y;}

    void setShankMap( const ShankMap *map );
    const ShankMap *getSmap()   {return smap;}
    void setSel( int ic );
    int getSel()                {return sel;}

    void setBnkRws( int B )
        {QMutexLocker ml( &dataMtx ); bnkRws = B;}
    void setStrike( const std::vector<IMRO_Site> &S )
        {QMutexLocker ml( &dataMtx ); vStrike = S;}
    void setROI( const std::vector<IMRO_ROI> &R )
        {QMutexLocker ml( &dataMtx ); vROI = R;}

    void colorPads( const std::vector<double> &val, double rngMax );

signals:
    void cursorOver( int ic, bool shift );
    void lbutClicked( int ic, bool shift );

public slots:
#ifdef OPENGL54
    void updateNow()    {update();}
#else
    void updateNow()    {updateGL();}
#endif

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

    void mouseMoveEvent( QMouseEvent *evt );
    void mousePressEvent( QMouseEvent *evt );

private:
    float viewportPix();
    float spanPix();
    void setClipping();
    void resizePads();
    void drawTips();
    void drawShks();
    void drawTops();
    void drawPads();
    void drawSel();
    void drawBanks();
    void drawStruck();
    void drawROIs();
    void drawTri( float l, float t, float w, float h, SColor c );
    void drawRect( float l, float t, float w, float h, SColor c );
    bool evt2Pad( int &s, int &c, int &r, const QMouseEvent *evt );
    int getSelY();
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

protected:
    virtual void resizeEvent( QResizeEvent *e );
    virtual void scrollContentsBy( int dx, int dy );
    virtual bool viewportEvent( QEvent *e );
};

#endif  // SHANKVIEW_H


