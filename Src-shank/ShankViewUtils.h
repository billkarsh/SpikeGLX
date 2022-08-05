#ifndef SHANKVIEWUTILS_H
#define SHANKVIEWUTILS_H

#ifdef OPENGL54
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#else
#include <QGLWidget>
#include <QGLFunctions>
#endif

#undef max  // inherited from WinDef.h via QGLWidget

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


