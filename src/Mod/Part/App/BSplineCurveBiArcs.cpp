// SPDX-License-Identifier: BSD-3-Clause

// This file is released under the BSD license
//
// Copyright (c) 2009, Daniel Heeks
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice, this
//      list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright notice, this
//      list of conditions and the following disclaimer in the documentation and/or
//      other materials provided with the distribution.
//    * Neither the name of Daniel Heeks nor the names of its contributors may be used
//      to endorse or promote products derived from this software without specific prior
//      written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <numbers>
#include <vector>
#include <cstdio>   // DEBUG-TEMP
#include <cstdlib>  // DEBUG-TEMP
#include <string>   // DEBUG-TEMP
#include <deque>

#include <ElCLib.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <GeomConvert.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Line.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <gp_XY.hxx>
#include <gp_Ax2.hxx>
#include <gp_Circ.hxx>
#include <gp_Lin.hxx>
#include <gp_Pln.hxx>


#include "BSplineCurveBiArcs.h"
#include "Geometry.h"
#include "Tools.h"


using Part::BSplineCurveBiArcs;
using Part::GeomBSplineCurve;
using Part::Geometry;

// Algorithm taken from HeeksCAD
namespace Part
{

bool tangentialArc(const gp_Pnt& p0, const gp_Vec& v0, const gp_Pnt& p1, gp_Pnt& c, gp_Dir& axis)
{
    if (p0.Distance(p1) > Precision::Intersection() && v0.Magnitude() > Precision::Intersection()) {
        gp_Vec v1(p0, p1);
        gp_Pnt halfway(p0.XYZ() + v1.XYZ() * 0.5);
        gp_Pln pln1(halfway, v1);
        gp_Pln pln2(p0, v0);
        gp_Lin plane_line;
        if (intersect(pln1, pln2, plane_line)) {
            gp_Lin l1(halfway, v1);
            gp_Pnt p2;
            closestPointsOnLines(plane_line, l1, c, p2);
            axis = -(plane_line.Direction());
            return true;
        }
    }

    return false;
}

class TangentialArc
{
public:
    gp_Pnt m_p0;  // start point
    gp_Vec m_v0;  // start direction
    gp_Pnt m_p1;  // end point
    gp_Pnt m_c;   // centre point
    gp_Dir m_a;   // axis
    bool m_is_a_line;
    TangentialArc(const gp_Pnt& p0, const gp_Vec& v0, const gp_Pnt& p1)
        : m_p0(p0)
        , m_v0(v0)
        , m_p1(p1)
    {
        // calculate a tangential arc that goes through p0 and p1, with a direction of v0 at p0
        m_is_a_line = !Part::tangentialArc(m_p0, m_v0, m_p1, m_c, m_a);
    }
    bool isRadiusEqual(const gp_Pnt& p, double tolerance) const
    {
        if (m_is_a_line) {
            return true;
        }

        double point_radius = gp_Vec(m_c.XYZ() - p.XYZ()).Magnitude();
        double diff = fabs(point_radius - radius());
        return diff <= tolerance;
    }
    double radius() const
    {
        double r0 = gp_Vec(m_p0.XYZ() - m_c.XYZ()).Magnitude();
        double r1 = gp_Vec(m_p1.XYZ() - m_c.XYZ()).Magnitude();
        double r = (r0 + r1) / 2;
        return r;
    }
    Geometry* makeArc() const
    {
        if (m_is_a_line) {
            GeomLineSegment* line = new GeomLineSegment();
            line->setPoints(
                Base::convertTo<Base::Vector3d>(m_p0),
                Base::convertTo<Base::Vector3d>(m_p1)
            );
            return line;
        }

        gp_Circ c(gp_Ax2(m_c, m_a), radius());
        GC_MakeArcOfCircle arc(c, m_p0, m_p1, true);
        GeomArcOfCircle* new_object = new GeomArcOfCircle();
        new_object->setHandle(arc.Value());
        return new_object;
    }
};

}  // namespace Part

namespace
{

// DEBUG-TEMP: counters for BIARCS_SAMPLE_LOG
long debugSamples = 0;     // DEBUG-TEMP
long debugCandidates = 0;  // DEBUG-TEMP

// Distance from p to an arc or line segment produced by toBiArcs (a trimmed circle or line)
double distanceToSegment(const gp_Pnt& p, const Geometry* segment)
{
    Handle(Geom_TrimmedCurve) trimmed = Handle(Geom_TrimmedCurve)::DownCast(segment->handle());
    if (trimmed.IsNull()) {
        return std::numeric_limits<double>::infinity();
    }
    const double first = trimmed->FirstParameter();
    const double last = trimmed->LastParameter();

    if (Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(trimmed->BasisCurve())) {
        // The nearest point on the full circle is at p's angle; use it if that angle is on the arc
        const gp_Circ circ = circle->Circ();
        const double angle
            = ElCLib::InPeriod(ElCLib::Parameter(circ, p), first, first + 2 * std::numbers::pi);
        if (angle <= last) {
            return p.Distance(ElCLib::Value(angle, circ));
        }
        return std::min(p.Distance(trimmed->StartPoint()), p.Distance(trimmed->EndPoint()));
    }
    if (Handle(Geom_Line) line = Handle(Geom_Line)::DownCast(trimmed->BasisCurve())) {
        const gp_Lin lin = line->Lin();
        const double param = std::clamp(ElCLib::Parameter(lin, p), first, last);
        return p.Distance(ElCLib::Value(param, lin));
    }
    return std::numeric_limits<double>::infinity();
}

// Distance from p to the line segment [a, b]
double distanceToChord(const gp_Pnt& p, const gp_Pnt& a, const gp_Pnt& b)
{
    const gp_XYZ ab = b.XYZ() - a.XYZ();
    const double length2 = ab.SquareModulus();
    const double s = length2 > 0 ? std::clamp((p.XYZ() - a.XYZ()).Dot(ab) / length2, 0.0, 1.0) : 0.0;
    return p.Distance(gp_Pnt(a.XYZ() + ab * s));
}

// Largest distance from any point of the chord [a, b] to `segment` (an arc or line from toBiArcs)
double maxDistanceFromChord(const gp_Pnt& a, const gp_Pnt& b, const Geometry* segment)
{
    Handle(Geom_TrimmedCurve) trimmed = Handle(Geom_TrimmedCurve)::DownCast(segment->handle());
    Handle(Geom_Circle) circle;
    if (!trimmed.IsNull()) {
        circle = Handle(Geom_Circle)::DownCast(trimmed->BasisCurve());
    }
    if (circle.IsNull()) {
        // Distance to a line segment only grows away from its closest point, so along [a, b] it
        // peaks at an end
        return std::max(distanceToSegment(a, segment), distanceToSegment(b, segment));
    }

    // Work in the arc's plane: the chord's coordinates there, and its height above the plane
    const gp_Circ circ = circle->Circ();
    const gp_XYZ c = circ.Location().XYZ();
    const gp_XYZ xAxis = circ.XAxis().Direction().XYZ();
    const gp_XYZ yAxis = circ.YAxis().Direction().XYZ();
    const gp_XYZ normal = circ.Axis().Direction().XYZ();
    const gp_XY a2((a.XYZ() - c).Dot(xAxis), (a.XYZ() - c).Dot(yAxis));
    const gp_XY b2((b.XYZ() - c).Dot(xAxis), (b.XYZ() - c).Dot(yAxis));
    const gp_XY d = b2 - a2;
    const double height
        = std::max(std::fabs((a.XYZ() - c).Dot(normal)), std::fabs((b.XYZ() - c).Dot(normal)));

    // Along the chord, the in-plane distance to the arc can only peak at: the chord's ends; where
    // the chord crosses the lines from the centre through the arc's end points or through the
    // middle of the arc's gap (where the nearest point of the arc switches between the circle and
    // an end point, or between the two end points); or the chord point closest to the centre.
    std::vector<double> params {0.0, 1.0};
    const double first = trimmed->FirstParameter();
    const double last = trimmed->LastParameter();
    for (double angle : {first, last, (first + last) / 2 + std::numbers::pi}) {
        const gp_XY direction(std::cos(angle), std::sin(angle));
        const double denominator = direction ^ d;
        if (std::fabs(denominator) > Precision::Intersection()) {
            params.push_back(-(direction ^ a2) / denominator);
        }
    }
    if (d.SquareModulus() > 0) {
        params.push_back(-(a2 * d) / d.SquareModulus());
    }

    double worst = 0;
    for (double s : params) {
        if (s >= 0 && s <= 1) {
            const gp_XY q = a2 + d * s;
            worst = std::max(
                worst,
                distanceToSegment(gp_Pnt(c + xAxis * q.X() + yAxis * q.Y()), segment)
            );
        }
    }
    // The real distance is at most the in-plane distance plus the height above the plane
    return worst + height;
}

// For a biarc candidate {first, second}: check the chord [a, b] by dividing it between the two
// segments at the boundary between them, and checking each part against its own segment only.
//
// The two segments meet at the joint J with a common direction there. The boundary is the plane
// (in 2D: the line) through J perpendicular to that direction; for arcs it passes through both
// centres. It is where the first segment's range ends and the second one's begins, so each side's
// own segment is the natural one to compare that side's part of the chord with.
bool chordFitsAcrossJoint(
    const gp_Pnt& a,
    const gp_Pnt& b,
    std::initializer_list<const Geometry*> candidate,
    double fitTolerance
)
{
    const Geometry* first = *candidate.begin();
    const Geometry* second = *std::next(candidate.begin());
    Handle(Geom_TrimmedCurve) firstCurve = Handle(Geom_TrimmedCurve)::DownCast(first->handle());
    if (firstCurve.IsNull()) {
        return false;
    }

    // The joint is the first segment's end that is nearer the second segment; the first
    // segment's other end tells us which side of the boundary is the first segment's side
    const gp_Pnt start = firstCurve->StartPoint();
    const gp_Pnt end = firstCurve->EndPoint();
    const bool jointAtEnd = distanceToSegment(end, second) <= distanceToSegment(start, second);
    const gp_Pnt joint = jointAtEnd ? end : start;
    const gp_Pnt farEnd = jointAtEnd ? start : end;

    // Direction of the first segment at the joint
    gp_Pnt unused;
    gp_Vec direction;
    firstCurve->D1(
        jointAtEnd ? firstCurve->LastParameter() : firstCurve->FirstParameter(),
        unused,
        direction
    );

    // side(p): signed distance of p from the boundary (times |direction|), positive on the
    // first segment's side
    const double firstSide = gp_Vec(joint, farEnd).Dot(direction) >= 0 ? 1.0 : -1.0;
    auto side = [&](const gp_Pnt& p) {
        return firstSide * gp_Vec(joint, p).Dot(direction);
    };
    auto fits = [&](const gp_Pnt& p, const gp_Pnt& q, const Geometry* segment) {
        return maxDistanceFromChord(p, q, segment) <= fitTolerance;
    };

    const double sideA = side(a);
    const double sideB = side(b);
    if (sideA >= 0 && sideB >= 0) {
        return fits(a, b, first);  // whole chord on the first segment's side
    }
    if (sideA <= 0 && sideB <= 0) {
        return fits(a, b, second);  // whole chord on the second segment's side
    }
    // The chord crosses the boundary: split it there
    const gp_Pnt crossing(a.XYZ() + (b.XYZ() - a.XYZ()) * (sideA / (sideA - sideB)));
    const Geometry* segmentA = sideA > 0 ? first : second;
    const Geometry* segmentB = sideB > 0 ? first : second;
    return fits(a, crossing, segmentA) && fits(crossing, b, segmentB);
}

// Check that the curve on [t_start, t_end] is within tolerance of the candidate (the one or two
// arcs/lines that createArcs proposes for that span).
//
// Idea: replace the curve by straight lines ("chords") that we know it stays close to, then check
// each chord exactly against the candidate. If the curve is within chordTolerance of a chord, and
// the chord is within fitTolerance of the candidate, the curve there is within
// chordTolerance + fitTolerance = tolerance of the candidate.
bool isWithinTolerance(
    const Handle(Geom_BSplineCurve) & curve,
    double t_start,
    double t_end,
    std::initializer_list<const Geometry*> candidate,
    double tolerance
)
{
    // --- How the tolerance is shared out ---
    // chordTolerance: how far the curve may be from a chord.
    // fitTolerance: how far a chord may be from the candidate.
    // Share of the tolerance for curve-to-chord deviation. Smaller values give the fit more room
    // (sometimes fewer output pieces) but need more chords; 0.1 balances the two.
    constexpr double chordFraction = 0.1;
    const double chordTolerance = chordFraction * tolerance;
    const double fitTolerance = tolerance - chordTolerance;
    // Safety limit on how many times a piece of curve may be halved
    constexpr int maxDepth = 25;

    ++debugCandidates;  // DEBUG-TEMP

    // --- Helpers ---
    // Whether point p is within tolerance of the candidate (of any of its segments)
    auto nearCandidate = [&](const gp_Pnt& p) {
        return std::ranges::any_of(candidate, [&](const Geometry* segment) {
            return distanceToSegment(p, segment) <= tolerance;
        });
    };

    // --- Work queue of curve pieces ---
    // A piece is a stretch of the curve, stored as its own small B-spline (so it has its own
    // poles), plus how many times it has been halved to get it. We start with the whole span.
    // Pieces are taken from the front and halves are added at the back, so all big pieces are
    // checked before any small ones; a bad candidate usually fails on a big piece, early.
    struct Piece
    {
        Handle(Geom_BSplineCurve) curve;
        int depth;
    };
    Handle(Geom_BSplineCurve) whole = Handle(Geom_BSplineCurve)::DownCast(curve->Copy());
    whole->Segment(t_start, t_end);
    std::deque<Piece> pieces {{whole, 0}};

    while (!pieces.empty()) {
        const Piece piece = pieces.front();
        pieces.pop_front();
        ++debugSamples;  // DEBUG-TEMP

        // --- Quick rejection ---
        // a and b are the piece's end points. They are actual points on the curve (a B-spline
        // starts at its first pole and ends at its last). If either is more than tolerance from
        // the candidate, the candidate definitely does not fit: reject it.
        const TColgp_Array1OfPnt& poles = piece.curve->Poles();
        const gp_Pnt a = poles(poles.Lower());
        const gp_Pnt b = poles(poles.Upper());
        if (!nearCandidate(a) || !nearCandidate(b)) {
            return false;
        }

        // --- How far can this piece of curve be from its chord a-b? ---
        // The piece always lies inside the region spanned by its poles (their convex hull). So if
        // every pole is within some distance of the chord, the whole piece is too. `deviation` is
        // the largest pole-to-chord distance: a guaranteed bound on curve-to-chord distance.
        double deviation = 0;
        for (int i = poles.Lower(); i <= poles.Upper(); ++i) {
            deviation = std::max(deviation, distanceToChord(poles(i), a, b));
        }

        // --- If the piece is close to its chord, check the chord against the candidate ---
        if (deviation <= chordTolerance) {
            // The chord passes if all of it is within fitTolerance of one candidate segment. Then
            // this piece of curve is within tolerance of the candidate: done with this piece.
            if (std::ranges::any_of(candidate, [&](const Geometry* segment) {
                    return maxDistanceFromChord(a, b, segment) <= fitTolerance;
                })) {
                continue;
            }
            // The chord failed against every single segment. If the candidate is a biarc, the
            // chord may run across the joint where the two arcs meet, with each arc covering only
            // part of it. So divide the chord between the arcs and check each part against its
            // own arc (see chordFitsAcrossJoint). Any division is valid for this purpose: if every
            // part is within fitTolerance of some segment, the chord is within fitTolerance of
            // the candidate.
            if (candidate.size() == 2 && chordFitsAcrossJoint(a, b, candidate, fitTolerance)) {
                continue;
            }
            // Otherwise treat the candidate as not fitting. This is not certain (the chord is
            // within chordTolerance of the curve, not on it), but rejecting is always safe:
            // createArcs then splits the span and tries again.
            return false;
        }

        // --- Otherwise halve the piece and queue both halves ---
        // Reached when the piece is not yet close enough to its chord (deviation too big).
        // A half's poles hug its chord about 4x more closely. Each half is checked from the top of
        // the loop like any other piece.
        if (piece.depth >= maxDepth) {
            // Halved too many times without passing: give up and reject the candidate
            return false;
        }
        const double first = piece.curve->FirstParameter();
        const double last = piece.curve->LastParameter();
        const double mid = (first + last) / 2;
        Handle(Geom_BSplineCurve) left = Handle(Geom_BSplineCurve)::DownCast(piece.curve->Copy());
        Handle(Geom_BSplineCurve) right = Handle(Geom_BSplineCurve)::DownCast(piece.curve->Copy());
        left->Segment(first, mid);
        right->Segment(mid, last);
        pieces.push_back({left, piece.depth + 1});
        pieces.push_back({right, piece.depth + 1});
    }

    // Every piece passed: the whole span is within tolerance of the candidate
    return true;
}

}  // namespace

void BSplineCurveBiArcs::createArcs(
    double tolerance,
    std::list<Geometry*>& new_spans,
    const gp_Pnt& p_start,
    const gp_Vec& v_start,
    double t_start,
    double t_end,
    gp_Pnt& p_end,
    gp_Vec& v_end
) const
{
    this->myCurve->D1(t_end, p_end, v_end);

    const Handle(Geom_BSplineCurve) bspline = Handle(Geom_BSplineCurve)::DownCast(this->myCurve);

    auto makeLine = [&]() {
        Part::GeomLineSegment* line = new Part::GeomLineSegment();
        line->setPoints(
            Base::convertTo<Base::Vector3d>(p_start),
            Base::convertTo<Base::Vector3d>(p_end)
        );
        return line;
    };

    gp_Pnt p1, p2, p3;

    Type can_do_spline_whole
        = calculateBiArcPoints(t_start, p_start, v_start, t_end, p_end, v_end, p1, p2, p3);

    Geometry* arc_object1 = nullptr;
    Geometry* arc_object2 = nullptr;
    Geometry* line_object = nullptr;

    if (can_do_spline_whole == Type::SingleArc) {
        Part::TangentialArc arc1(p_start, v_start, p2);
        Part::TangentialArc arc2(p2, gp_Vec(p3.XYZ() - p2.XYZ()), p_end);

        arc_object1 = arc1.makeArc();
        arc_object2 = arc2.makeArc();
        if (!isWithinTolerance(bspline, t_start, t_end, {arc_object1, arc_object2}, tolerance)) {
            delete arc_object1;
            delete arc_object2;
            can_do_spline_whole = Type::SplitCurve;
        }
    }
    else if (can_do_spline_whole == Type::SingleLine) {
        line_object = makeLine();
        if (!isWithinTolerance(bspline, t_start, t_end, {line_object}, tolerance)) {
            delete line_object;
            can_do_spline_whole = Type::SplitCurve;
        }
    }

    if (can_do_spline_whole == Type::SingleArc) {
        new_spans.push_back(arc_object1);
        new_spans.push_back(arc_object2);
    }
    else if (can_do_spline_whole == Type::SplitCurve) {
        double t_middle = t_start + ((t_end - t_start) * 0.5);
        gp_Pnt p_middle;
        gp_Vec v_middle;
        createArcs(tolerance, new_spans, p_start, v_start, t_start, t_middle, p_middle, v_middle);  // recursive
        gp_Pnt new_p_end;
        gp_Vec new_v_end;
        createArcs(tolerance, new_spans, p_middle, v_middle, t_middle, t_end, new_p_end, new_v_end);
    }
    else {
        // calculate_biarc_points failed, use the (validated) line
        new_spans.push_back(line_object);
    }
}

BSplineCurveBiArcs::Type BSplineCurveBiArcs::calculateBiArcPoints(
    double t_start,
    const gp_Pnt& p0,
    gp_Vec v_start,
    double t_end,
    const gp_Pnt& p4,
    gp_Vec v_end,
    gp_Pnt& p1,
    gp_Pnt& p2,
    gp_Pnt& p3
) const
{
    if (v_start.Magnitude() < Precision::Intersection()) {
        v_start = gp_Vec(p0, p1);
    }
    if (v_end.Magnitude() < Precision::Intersection()) {
        v_end = gp_Vec(p3, p4);
    }

    v_start.Normalize();
    v_end.Normalize();

    gp_Vec v = p0.XYZ() - p4.XYZ();

    double a = 2 * (v_start * v_end - 1);
    double c = v * v;
    double b = (v * 2) * (v_start + v_end);
    if (fabs(a) < Precision::Intersection()) {
        // Check the tangent of a value between t_start and t_end
        double t_mid = 0.9 * t_start + 0.1 * t_end;
        gp_Pnt p_mid;
        gp_Vec v_mid;
        this->myCurve->D1(t_mid, p_mid, v_mid);
        v_mid.Normalize();
        double a = 2 * (v_start * v_mid - 1);
        if (fabs(a) >= Precision::Intersection()) {
            return Type::SplitCurve;
        }
        return Type::SingleLine;
    }


    double d = b * b - 4 * a * c;
    if (d < 0.0) {
        return Type::SingleLine;
    }

    double sd = sqrt(d);
    double e1 = (-b - sd) / (2.0 * a);
    double e2 = (-b + sd) / (2.0 * a);
    if (e1 > 0 && e2 > 0) {
        return Type::SingleLine;
    }

    double e = e1;
    if (e2 > e) {
        e = e2;
    }
    if (e < 0) {
        return Type::SingleLine;
    }

    p1 = p0.XYZ() + v_start.XYZ() * e;
    p3 = p4.XYZ() - v_end.XYZ() * e;
    p2 = p1.XYZ() * 0.5 + p3.XYZ() * 0.5;


    return Type::SingleArc;
}

BSplineCurveBiArcs::BSplineCurveBiArcs(const Handle(Geom_Curve) & c)
{
    // Work on an exact, non-periodic B-spline version of the curve (conics become rational
    // B-splines), so isWithinTolerance can bound the curve's speed from its poles
    Handle(Geom_BSplineCurve) bspline = GeomConvert::CurveToBSplineCurve(c);
    if (bspline->IsPeriodic()) {
        bspline = Handle(Geom_BSplineCurve)::DownCast(bspline->Copy());
        bspline->SetNotPeriodic();
    }
    myCurve = bspline;
}

std::list<Geometry*> BSplineCurveBiArcs::toBiArcs(double tolerance) const
{
    debugSamples = 0;     // DEBUG-TEMP
    debugCandidates = 0;  // DEBUG-TEMP

    gp_Pnt p_start;
    gp_Vec v_start;
    gp_Pnt p_end;
    gp_Vec v_end;
    this->myCurve->D0(this->myCurve->FirstParameter(), p_start);
    this->myCurve->D0(this->myCurve->LastParameter(), p_end);

    std::list<Geometry*> list;

    // the spline is closed
    if (p_start.Distance(p_end) < Precision::Intersection()) {
        double midParameter = (this->myCurve->FirstParameter() + this->myCurve->LastParameter())
            / 2.0;
        this->myCurve->D1(this->myCurve->FirstParameter(), p_start, v_start);
        createArcs(
            tolerance,
            list,
            p_start,
            v_start,
            this->myCurve->FirstParameter(),
            midParameter,
            p_end,
            v_end
        );
        this->myCurve->D1(midParameter, p_start, v_start);
        createArcs(
            tolerance,
            list,
            p_start,
            v_start,
            midParameter,
            this->myCurve->LastParameter(),
            p_end,
            v_end
        );
    }
    else {
        this->myCurve->D1(this->myCurve->FirstParameter(), p_start, v_start);
        createArcs(
            tolerance,
            list,
            p_start,
            v_start,
            this->myCurve->FirstParameter(),
            this->myCurve->LastParameter(),
            p_end,
            v_end
        );
    }

    if (const char* path = std::getenv("BIARCS_SAMPLE_LOG")) {            // DEBUG-TEMP
        if (FILE* f = std::fopen(path, "a")) {                            // DEBUG-TEMP
            std::fprintf(f, "%ld %ld\n", debugSamples, debugCandidates);  // DEBUG-TEMP
            std::fclose(f);                                               // DEBUG-TEMP
        }  // DEBUG-TEMP
    }  // DEBUG-TEMP
    return list;
}
