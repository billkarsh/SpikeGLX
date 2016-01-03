#ifndef VEC2_H
#define VEC2_H

/* ---------------------------------------------------------------- */
/* Types ---------------------------------------------------------- */
/* ---------------------------------------------------------------- */

struct Vec2
{
    double  x, y;

    Vec2() {}
    Vec2( double x, double y ) : x(x), y(y) {}
};

struct Vec2f
{
    float   x, y;

    Vec2f() {}
    Vec2f( float x, float y ) : x(x), y(y) {}
};

#endif  // VEC2_H


