// SPDX-License-Identifier: BSD-3-Clause

// ***************************************************************************************************************************************
//                    Point, CLine & Circle classes part of geometry.lib
//                    g.j.hawkesford August 2006 for Camtek Gmbh
//
// This program is released under the BSD license. See the file COPYING for details.
//
// ***************************************************************************************************************************************

#include "geometry.h"

namespace geoff_geometry
{
double TOLERANCE = 1.0e-06;
double TOLERANCE_SQ = TOLERANCE * TOLERANCE;
double TIGHT_TOLERANCE = 1.0e-09;
double UNIT_VECTOR_TOLERANCE = 1.0e-10;

// ***************************************************************************************************************************************
// point classes
// ***************************************************************************************************************************************
Point::Point(const Vector2d& v)
{
    x = v.getx();
    y = v.gety();
    ok = true;
}

Point Point::operator+(const Vector2d& v) const
{
    return Point(x + v.getx(), y + v.gety());
}

bool Point::operator==(const Point& p) const
{
    // p1 == p2 (uses TOLERANCE)
    if (FNE(this->x, p.x, TOLERANCE) || FNE(this->y, p.y, TOLERANCE)) {
        return false;
    }
    return true;
}


double Point::Dist(const Point& p) const
{  // distance between 2 points
    return Vector2d(*this, p).magnitude();
}

// ***************************************************************************************************************************************
// circle methods
// ***************************************************************************************************************************************

Circle::Circle(const Point& p, double rad)
{
    // Circle
    pc = p;
    radius = rad;
}

int Intof(const Circle& c0, const Circle& c1, Point& pLeft, Point& pRight)
{
    // inters of 2 circles
    // returns the number of intersctions
    Vector2d v(c0.pc, c1.pc);
    double d = v.normalise();
    if (d < TOLERANCE) {  // co-incident circles
        return 0;
    }

    double sum = fabs(c0.radius) + fabs(c1.radius);
    double diff = fabs(fabs(c0.radius) - fabs(c1.radius));
    if (d > sum + TOLERANCE || d < diff - TOLERANCE) {
        return 0;
    }

    // dist from centre of this circle to mid intersection
    double d0 = 0.5 * (d + (c0.radius + c1.radius) * (c0.radius - c1.radius) / d);
    if (d0 - c0.radius > TOLERANCE) {  // circles don't intersect
        return 0;
    }

    double h = (c0.radius - d0) * (c0.radius + d0);  // half distance between intersects squared
    if (h < 0) {
        d0 = c0.radius;  // tangent
    }
    pLeft = v * d0 + c0.pc;  // mid-point of intersects
    if (h < TOLERANCE_SQ) {  // tangent
        return 1;
    }
    h = sqrt(h);

    v = ~v;  // calculate 2 intersects
    pRight = v * h + pLeft;
    v = -v;
    pLeft = v * h + pLeft;
    return 2;
}

double IncludedAngle(const Vector2d& v0, const Vector2d& v1, int dir)
{
    // returns the absolute included angle between 2 vectors in the direction of dir ( 1=acw  -1=cw)
    double inc_ang = v0 * v1;
    if (inc_ang > 1. - UNIT_VECTOR_TOLERANCE) {
        return 0;
    }
    if (inc_ang < -1. + UNIT_VECTOR_TOLERANCE) {
        inc_ang = PI;
    }
    else {  // dot product,   v1 . v2  =  cos ang
        if (inc_ang > 1.0) {
            inc_ang = 1.0;
        }
        inc_ang = acos(inc_ang);  // 0 to pi radians

        if (dir * (v0 ^ v1) < 0) {
            inc_ang = 2 * PI - inc_ang;  // cp
        }
    }
    return dir * inc_ang;
}

int quadratic(double a, double b, double c, double& x0, double& x1)
{
    // solves quadratic equation ax² + bx + c = 0
    // returns number of real roots
    double epsilon =  1.0e-06;
    double epsilonsq = epsilon * epsilon;
    if (fabs(a) < epsilon) {
        if (fabs(b) < epsilon) {  // invalid
            return 0;
        }
        x0 = -c / b;
        return 1;
    }
    b /= a;
    c /= a;
    double s = b * b - 4 * c;
    if (s < -epsilon) {  // imaginary roots
        return 0;
    }
    x0 = -0.5 * b;
    if (s > epsilonsq) {
        s = 0.5 * sqrt(s);
        x1 = x0 - s;
        x0 += s;
        return 2;
    }
    return 1;
}

}  // namespace geoff_geometry
