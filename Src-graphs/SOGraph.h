#ifndef SOGRAPH_H
#define SOGRAPH_H

#include "Vec2.h"

#include <QList>
#include <QMutex>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>

/* ---------------------------------------------------------------- */
/* SOGraph -------------------------------------------------------- */
/* ---------------------------------------------------------------- */

class SOGraph : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

#define NWAV    32
#define NPNT    64

private:
    struct Wave {
        qint64  tstamp;
        float   y[NPNT];
    };

    Vec2f   V[NPNT];        // drawing buffer
    Wave    wring[NWAV];    // ring of waves
    float   mean[NPNT],
            yave;           // mean middle value
    int     whead,
            wn;

public:
    double          sRate;
    mutable QMutex  dataMtx;
    int             yscl,
                    label;

public:
    SOGraph( QWidget *parent = 0 );
    void clear();
    void addWave( const float *src, qint64 sampIdx );

public slots:
    void updateNow()    {update();}

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

private:
    double rate();

    void drawPoints();
    void drawRateBar();
    void drawRateBox();
    void drawLabels();

    QRect rateRect();

    void clipToView( int *view );
    void renderTextWin(
        int             x,
        int             y,
        const QString   &str,
        const QFont     &font = QFont() );
};

#endif  // SOGRAPH_H


