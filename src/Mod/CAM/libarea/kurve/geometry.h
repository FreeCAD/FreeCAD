// SPDX-License-Identifier: BSD-3-Clause

/////////////////////////////////////////////////////////////////////////////////////////

//                    geometry.lib header

//                    g.j.hawkesford August 2003
//						modified with 2d & 3d vector methods 2006
//
// This program is released under the BSD license. See the file COPYING for details.
//
/////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cmath>
#include <vector>
#include <list>

using namespace std;

namespace geoff_geometry
{

class Vector2d;
class Point;
class Circle;
class Span;

extern double TOLERANCE;     // CAD Geometry resolution (inexact, eg. from import)
extern double TOLERANCE_SQ;  // tolerance squared for faster coding.
extern double TIGHT_TOLERANCE;
extern double UNIT_VECTOR_TOLERANCE;

inline bool FEQ(double a, double b, double tolerance = TOLERANCE)
{
    return fabs(a - b) <= tolerance;
}
inline bool FNE(double a, double b, double tolerance = TOLERANCE)
{
    return fabs(a - b) > tolerance;
}

#define PI 3.1415926535897932384626433832795e0

#define LINEAR 0  // linear
#define ACW 1     // anti-clockwise
#define CW -1     // clockwise

// 2d Point class
class Point
{
public:
    bool ok;   // true if this point is defined correctly
    double x;  // x value
    double y;  // y value

    // constructors etc...
    inline Point()
    {
        x = 0;
        y = 0;
        ok = false;
    }
    inline Point(double xord, double yord, bool okay = true)
    {
        x = xord;
        y = yord;
        ok = okay;
    }

    Point(const Vector2d& v);

    // operators
    bool operator==(const Point& p) const;
    bool operator!=(const Point& p) const
    {
        return !(*this == p);
    }
    inline Point operator+(const Point& p) const
    {
        return Point(x + p.x, y + p.y);
    }  // p0 = p1 + p2;
    Point operator+(const Vector2d& v) const;  // p1 = p0 + v0;

    // methods
    double Dist(const Point& p) const;                    // distance between 2 points
};

#define INVALID_POINT Point(9.9999999e50, 0, false)

// 2d vector class
class Vector2d
{
private:
    double dx, dy;

public:
    // constructors
    inline Vector2d()
    {
        dx = 0;
        dy = 0;
    }
    // inline	Vector2d(const Vector2d &v) { dx = v.dx; dy = v.dy;}
    inline Vector2d(double x, double y)
    {
        dx = x, dy = y;
    }
    inline Vector2d(const Point& p0, const Point& p1)
    {
        dx = p1.x - p0.x;
        dy = p1.y - p0.y;
    }

    inline Point operator+(const Point& p) const
    {
        return Point(this->dx + p.x, this->dy + p.y);
    }  // p1 = v0 + p0;

    inline const Vector2d operator-(void) const
    {
        return Vector2d(-dx, -dy);
    }  // v1 = -v0;  (unary minus)

    inline double operator*(const Vector2d& v) const
    {
        return (dx * v.dx + dy * v.dy);
    }  // dot product	m0.m1.cos a = v0 * v1
    inline Vector2d operator*(double c) const
    {
        return Vector2d(dx * c, dy * c);
    }  // scalar product

    inline double operator^(const Vector2d& v) const
    {
        return (dx * v.dy - dy * v.dx);
    }  // cross product m0.m1.sin a = v0 ^ v1
    inline Vector2d operator~(void) const
    {
        return Vector2d(-dy, dx);
    }  // perp to left

    // methods
    inline double getx() const
    {
        return dx;
    }
    inline double gety() const
    {
        return dy;
    }
    double normalise()
    {
        double m = magnitude();
        if (m < TIGHT_TOLERANCE) {
            dx = dy = 0;
            return 0;
        }
        dx /= m;
        dy /= m;
        return m;
    }  // normalise & returns magnitude
    inline double magnitudesqd(void) const
    {
        return (dx * dx + dy * dy);
    }  // magnitude squared
    inline double magnitude(void) const
    {
        return (sqrt(magnitudesqd()));
    }  // magnitude
};


class Circle
{
public:
    Point pc;
    double radius;

    Circle(const Point& p, double r);
};

// 2d box class
class Box
{
public:
    Point min;
    Point max;
    bool ok;

    Box()
    {
        min.x = min.y = 1.0e61;
        max.x = max.y = -1.0e61;
        ok = false;
    };
    Box(Point& pmin, Point& pmax)
    {
        min = pmin;
        max = pmax;
        ok = true;
    };

    bool outside(const Box& b) const;  // returns true if box is outside box
};

inline void MinMax(const Point& p, Point& pmin, Point& pmax)
{
    if (p.x > pmax.x) {
        pmax.x = p.x;
    }
    if (p.y > pmax.y) {
        pmax.y = p.y;
    }
    if (p.x < pmin.x) {
        pmin.x = p.x;
    }
    if (p.y < pmin.y) {
        pmin.y = p.y;
    }
}

// 2D line arc span
class Span
{
public:
    Point p0;  // start
    Point p1;  // end
    Point pc;  // centre
    int dir;   // arc direction (CW or ACW or 0 for straight)

    Vector2d vs;                // direction at start or for straight

    double length;  // span length
    double radius;  // arc radius
    double angle;   // included arc angle  ( now arc is parameterised start -> start + angle

    Box box;  // span box

    // methods
    void SetProperties();          // set span properties
    void minmax(Box& box, bool start = true);           // minmax of span
    void minmax(Point& pmin, Point& pmax, bool start = true);  // minmax of span
    int Intof(const Span& sp, Point& pInt1, Point& pInt2, double t[4]) const;
    bool OnSpan(const Point& p) const;   //  tests if p is on sp *** FAST TEST p MUST LIE on
                                         //  unbounded span
    bool OnSpan(
        const Point& p,
        double* t
    ) const;  //  tests if p is on sp *** FAST TEST p MUST LIE on unbounded span

    // constructor
    Span()
    {
        dir = 0;
        length = 0;
        radius = 0;
        angle = 0;
    }
    Span(int spandir, const Point& pn, const Point& pf, const Point& c)
    {
        dir = spandir;
        p0 = pn, p1 = pf, pc = c;
        SetProperties();
    };
};

double IncludedAngle(const Vector2d& v0, const Vector2d& v1, int dir = 1);  // angle between 2 vectors

// point definitions
int Intof(const Circle& c0, const Circle& c1, Point& pLeft, Point& pRight);  //    ditto

// misc
int quadratic(double a, double b, double c, double& x0, double& x1);  // solve quadratic

// finite Span routines
int Intof(const Span& sp0, const Span& sp1, Point& p0, Point& p1, double t[4]);
int LineLineIntof(const Span& L0, const Span& L1, Point& p, double t[2]);
int LineArcIntof(const Span& line, const Span& arc, Point& p0, Point& p1, double t[4]);
int ArcArcIntof(const Span& arc0, const Span& arc1, Point& pLeft, Point& pRight);

}  // End namespace geoff_geometry
