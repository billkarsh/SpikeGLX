#ifndef SHANKVIEWUTILS_H
#define SHANKVIEWUTILS_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>

/* ---------------------------------------------------------------- */
/* SColor --------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct SColor {
    quint8  r, g, b;
    SColor() : r(0), g(0), b(0) {}
    SColor( quint8 c ) : r(c), g(c), b(c) {}
    SColor( quint8 r, quint8 g, quint8 b ) : r(r), g(g), b(b) {}
};

/* ---------------------------------------------------------------- */
/* Utils ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

void loadLut( std::vector<SColor> &lut );

#endif  // SHANKVIEWUTILS_H


