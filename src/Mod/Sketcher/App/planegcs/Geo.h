/***************************************************************************
 *   Copyright (c) 2011 Konstantinos Poulios <logari81@gmail.com>          *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#ifndef PLANEGCS_GEO_H
#define PLANEGCS_GEO_H

#include <cmath>

#include "Util.h"


namespace GCS
{
class Point
{
public:
    Point()
    {
        x = nullptr;
        y = nullptr;
    }
    Point(double* px, double* py)
    {
        x = px;
        y = py;
    }
    double* x;
    double* y;
    int PushOwnParams(VEC_pD& pvec);
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt);
};

using VEC_P = std::vector<Point>;

/// Class DeriVector2 holds a vector value and its derivative on the
/// parameter that the derivatives are being calculated for now. x,y is the
/// actual vector (v). dx,dy is a derivative of the vector by a parameter
///(dv/dp). The easiest way to fill the vector in is by passing a point and
/// a derivative parameter pointer to its constructor. x,y are read from the
/// pointers in Point, and dx,dy are set to either 0 or 1 depending on what
/// pointers of Point match the supplied pointer. The derivatives can be set
/// manually as well. The class also provides a bunch of methods to do math
/// on it (and derivatives are calculated implicitly).
///
class DeriVector2
{
public:
    DeriVector2()
    {
        x = 0;
        y = 0;
        dx = 0;
        dy = 0;
    }
    DeriVector2(double x, double y)
    {
        this->x = x;
        this->y = y;
        this->dx = 0;
        this->dy = 0;
    }
    DeriVector2(double x, double y, double dx, double dy)
    {
        this->x = x;
        this->y = y;
        this->dx = dx;
        this->dy = dy;
    }
    DeriVector2(const Point& p, const double* derivparam);
    double x, dx;
    double y, dy;

    double length() const
    {
        return sqrt(x * x + y * y);
    }
    // returns length and writes length deriv into the dlength argument.
    double length(double& dlength) const;

    // unlike other vectors in FreeCAD, this normalization creates a new vector instead of
    // modifying existing one.
    DeriVector2 getNormalized() const;  // returns zero vector if the original is zero.
    // calculates scalar product of two vectors and returns the result. The derivative
    // of the result is written into argument dprd.
    double scalarProd(const DeriVector2& v2, double* dprd = nullptr) const;
    // calculates the norm of the cross product of the two vectors.
    // DeriVector2 are considered as 3d vectors with null z. The derivative
    // of the result is written into argument dprd.
    double crossProdNorm(const DeriVector2& v2, double& dprd) const;
    DeriVector2 sum(const DeriVector2& v2) const
    {  // adds two vectors and returns result
        return DeriVector2(x + v2.x, y + v2.y, dx + v2.dx, dy + v2.dy);
    }
    DeriVector2 subtr(const DeriVector2& v2) const
    {  // subtracts two vectors and returns result
        return DeriVector2(x - v2.x, y - v2.y, dx - v2.dx, dy - v2.dy);
    }
    DeriVector2 mult(double val) const
    {
        return DeriVector2(x * val, y * val, dx * val, dy * val);
    }  // multiplies the vector by a number. Derivatives are scaled.
    DeriVector2 multD(double val, double dval) const
    {  // multiply vector by a variable with a derivative.
        return DeriVector2(x * val, y * val, dx * val + x * dval, dy * val + y * dval);
    }
    // divide vector by a variable with a derivative
    DeriVector2 divD(double val, double dval) const;
    DeriVector2 rotate90ccw() const
    {
        return DeriVector2(-y, x, -dy, dx);
    }
    DeriVector2 rotate90cw() const
    {
        return DeriVector2(y, -x, dy, -dx);
    }
    DeriVector2 linCombi(double m1, const DeriVector2& v2, double m2) const
    {  // linear combination of two vectors
        return DeriVector2(x * m1 + v2.x * m2,
                           y * m1 + v2.y * m2,
                           dx * m1 + v2.dx * m2,
                           dy * m1 + v2.dy * m2);
    }
};

///////////////////////////////////////
// Geometries
///////////////////////////////////////

class Curve  // a base class for all curve-based objects (line, circle/arc, ellipse/arc)
{
public:
    virtual ~Curve()
    {}
    // returns normal vector. The vector should point to the left when one
    //  walks along the curve from start to end. Ellipses and circles are
    //  assumed to be walked counterclockwise, so the vector should point
    //  into the shape.
    // derivparam is a pointer to a curve parameter (or point coordinate) to
    //  compute the derivative for. The derivative is returned through dx,dy
    //  fields of DeriVector2.
    virtual DeriVector2 CalculateNormal(const Point& p,
                                        const double* derivparam = nullptr) const = 0;

    /**
     * @brief Value: returns point (vector) given the value of parameter
     * @param u: value of parameter
     * @param du: derivative of parameter by derivparam
     * @param derivparam: pointer to sketch parameter to calculate the derivative for
     * @return
     */
    virtual DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const;

    // adds curve's parameters to pvec (used by constraints)
    virtual int PushOwnParams(VEC_pD& pvec) = 0;
    // recunstruct curve's parameters reading them from pvec starting from index cnt.
    // cnt will be incremented by the same value as returned by PushOwnParams()
    virtual void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) = 0;
    // DeepSOIC: I haven't found a way to simply copy a curve object provided pointer to a curve
    // object.
    virtual Curve* Copy() = 0;
};

class Line: public Curve
{
public:
    Line()
    {}
    ~Line() override
    {}
    Point p1;
    Point p2;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Line* Copy() override;
};

class Circle: public Curve
{
public:
    Circle()
    {
        rad = nullptr;
    }
    ~Circle() override
    {}
    Point center;
    double* rad;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Circle* Copy() override;
};

class Arc: public Circle
{
public:
    Arc()
    {
        startAngle = nullptr;
        endAngle = nullptr;
        rad = nullptr;
    }
    ~Arc() override
    {}
    double* startAngle;
    double* endAngle;
    // double *rad; //inherited
    Point start;
    Point end;
    // Point center; //inherited
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Arc* Copy() override;
};

class MajorRadiusConic: public Curve
{
public:
    ~MajorRadiusConic() override
    {}
    virtual double getRadMaj(const DeriVector2& center,
                             const DeriVector2& f1,
                             double b,
                             double db,
                             double& ret_dRadMaj) const = 0;
    virtual double getRadMaj(double* derivparam, double& ret_dRadMaj) const = 0;
    virtual double getRadMaj() const = 0;
    // DeriVector2 CalculateNormal(Point &p, double* derivparam = 0) = 0;
};

class Ellipse: public MajorRadiusConic
{
public:
    Ellipse()
    {
        radmin = nullptr;
    }
    ~Ellipse() override
    {}
    Point center;
    Point focus1;
    double* radmin;
    double getRadMaj(const DeriVector2& center,
                     const DeriVector2& f1,
                     double b,
                     double db,
                     double& ret_dRadMaj) const override;
    double getRadMaj(double* derivparam, double& ret_dRadMaj) const override;
    double getRadMaj() const override;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Ellipse* Copy() override;
};

class ArcOfEllipse: public Ellipse
{
public:
    ArcOfEllipse()
    {
        startAngle = nullptr;
        endAngle = nullptr;
        radmin = nullptr;
    }
    ~ArcOfEllipse() override
    {}
    double* startAngle;
    double* endAngle;
    // double *radmin; //inherited
    Point start;
    Point end;
    // Point center;  //inherited
    // double *focus1.x; //inherited
    // double *focus1.y; //inherited
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    ArcOfEllipse* Copy() override;
};

class Hyperbola: public MajorRadiusConic
{
public:
    Hyperbola()
    {
        radmin = nullptr;
    }
    ~Hyperbola() override
    {}
    Point center;
    Point focus1;
    double* radmin;
    double getRadMaj(const DeriVector2& center,
                     const DeriVector2& f1,
                     double b,
                     double db,
                     double& ret_dRadMaj) const override;
    double getRadMaj(double* derivparam, double& ret_dRadMaj) const override;
    double getRadMaj() const override;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Hyperbola* Copy() override;
};

class ArcOfHyperbola: public Hyperbola
{
public:
    ArcOfHyperbola()
    {
        startAngle = nullptr;
        endAngle = nullptr;
        radmin = nullptr;
    }
    ~ArcOfHyperbola() override
    {}
    // parameters
    double* startAngle;
    double* endAngle;
    Point start;
    Point end;
    // interface helpers
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    ArcOfHyperbola* Copy() override;
};

class Parabola: public Curve
{
public:
    Parabola()
    {}
    ~Parabola() override
    {}
    Point vertex;
    Point focus1;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    Parabola* Copy() override;
};

class ArcOfParabola: public Parabola
{
public:
    ArcOfParabola()
    {
        startAngle = nullptr;
        endAngle = nullptr;
    }
    ~ArcOfParabola() override
    {}
    // parameters
    double* startAngle;
    double* endAngle;
    Point start;
    Point end;
    // interface helpers
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    ArcOfParabola* Copy() override;
};

class BSpline: public Curve
{
public:
    BSpline()
    {
        periodic = false;
        degree = 2;
    }
    ~BSpline() override
    {}
    // parameters
    VEC_P poles;  // TODO: use better data structures so poles.x and poles.y
    VEC_pD weights;
    VEC_pD knots;
    // dependent parameters (depends on previous parameters,
    // but an "arcrules" constraint alike would be required to gain the commodity of simple
    // coincident with endpoint constraints)
    Point start;
    Point end;
    // not solver parameters
    VEC_I mult;
    int degree;
    bool periodic;
    VEC_I knotpointGeoids;  // geoids of knotpoints as to index Geom array
    // knot vector with repetitions for multiplicity and "padding" for periodic spline
    // interface helpers
    VEC_D flattenedknots;
    DeriVector2 CalculateNormal(const Point& p, const double* derivparam = nullptr) const override;
    DeriVector2 Value(double u, double du, const double* derivparam = nullptr) const override;
    int PushOwnParams(VEC_pD& pvec) override;
    void ReconstructOnNewPvec(VEC_pD& pvec, int& cnt) override;
    BSpline* Copy() override;
    /// finds the value B_i(x) such that spline(x) = sum(poles[i] * B_i(x))
    /// x is the point at which combination is needed
    /// k is the range in `flattenedknots` that contains x
    /// i is index of control point
    /// p is the degree
    double getLinCombFactor(double x, size_t k, size_t i, unsigned int p);
    inline double getLinCombFactor(double x, size_t k, size_t i)
    {
        return getLinCombFactor(x, k, i, degree);
    }
    void setupFlattenedKnots();
    /// finds spline(x) for the given parameter and knot/pole vector
    /// x is the point at which combination is needed
    /// k is the range in `flattenedknots` that contains x
    /// p is the degree
    /// d is the vector of (relevant) poles (this will be changed)
    /// flatknots is the vector of knots
    static double splineValue(double x, size_t k, unsigned int p, VEC_D& d, const VEC_D& flatknots);
};

}  // namespace GCS

#endif  // PLANEGCS_GEO_H
