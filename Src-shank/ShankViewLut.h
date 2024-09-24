#ifndef SHANKVIEWLUT_H
#define SHANKVIEWLUT_H

#include "ShankViewUtils.h"

/* ---------------------------------------------------------------- */
/* ShankViewLut --------------------------------------------------- */
/* ---------------------------------------------------------------- */

class ShankViewLut : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

private:
    std::vector<SColor> lut;

public:
    ShankViewLut( QWidget *parent = 0 );

protected:
    void initializeGL();
    void resizeGL( int w, int h );
    void paintGL();

private:
    void drawLines();
};

#endif  // SHANKVIEWLUT_H


