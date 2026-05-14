// SPDX-License-Identifier: BSD-3-Clause
// Curve.h

/*==============================
Copyright (c) 2011-2015 Dan Heeks

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
==============================*/


#pragma once

#include <vector>
#include <list>
#include <cmath>
#include "Point.h"
#include "Box2D.h"

namespace heeks
{

class Line
{
public:
    Point p0;
    Point v;

    // constructors
    Line(const Point& P0, const Point& V);

    double Dist(const Point& p) const;
};

class CVertex
{
public:
    int m_type;  // 0 - line ( or start point ), 1 - anti-clockwise arc, -1 - clockwise arc
    Point m_p;   // end point
    Point m_c;   // centre point in absolute coordinates

    CVertex()
        : m_type(0)
        , m_p(Point(0, 0))
        , m_c(Point(0, 0))
    {}
    CVertex(int type, const Point& p, const Point& c);
    CVertex(const Point& p);
};

class Span
{
    Point NearestPointNotOnSpan(const Point& p) const;
    double Parameter(const Point& p) const;
    Point NearestPointToSpan(const Span& p, double& d) const;

    static const Point null_point;
    static const CVertex null_vertex;

public:
    bool m_start_span;
    Point m_p;
    CVertex m_v;
    Span();
    Span(const Point& p, const CVertex& v, bool start_span = false)
        : m_start_span(start_span)
        , m_p(p)
        , m_v(v)
    {}
    Point NearestPoint(const Point& p) const;
    Point NearestPoint(const Span& p, double* d = NULL) const;
    void GetBox(CBox2D& box);
    double IncludedAngle() const;
    double GetArea() const;
    bool On(const Point& p, double* t = NULL) const;
    Point MidPerim(double d) const;
    Point MidParam(double param) const;
    double Length() const;
    Point GetVector(double fraction) const;
};

class CCurve
{
    // a closed curve, please make sure you add an end point, the same as the start point

public:
    std::list<CVertex> m_vertices;
    void append(const CVertex& vertex);

    void Discretize();
    Point NearestPoint(const Point& p) const;
    Point NearestPoint(const CCurve& p, double* d = NULL) const;
    Point NearestPoint(const Span& p, double* d = NULL) const;
    void GetBox(CBox2D& box);
    void Reverse();
    double GetArea() const;
    bool IsClockwise() const
    {
        return GetArea() > 0;
    }
    bool IsClosed() const;
    void ChangeStart(const Point& p);
    void ChangeEnd(const Point& p);
    void Break(const Point& p);
    void ExtractSeparateCurves(
        const std::list<Point>& ordered_points,
        std::list<CCurve>& separate_curves
    ) const;
    double Perim() const;
    Point PerimToPoint(double perim) const;
    double PointToPerim(const Point& p) const;
    void GetSpans(std::list<Span>& spans) const;
    void RemoveTinySpans();
    void operator+=(const CCurve& p);
};

}  // namespace heeks
