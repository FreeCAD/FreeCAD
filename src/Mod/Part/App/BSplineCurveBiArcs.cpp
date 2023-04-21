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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <GC_MakeArcOfCircle.hxx>
# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Pln.hxx>
#endif

#include "BSplineCurveBiArcs.h"
#include "Geometry.h"
#include "Tools.h"


using Part::BSplineCurveBiArcs;
using Part::GeomBSplineCurve;
using Part::Geometry;

// Algorithm taken from HeeksCAD
namespace Part {

bool tangentialArc(const gp_Pnt& p0, const gp_Vec& v0, const gp_Pnt& p1, gp_Pnt& c, gp_Dir& axis)
{
    if (p0.Distance(p1) > Precision::Intersection() &&
        v0.Magnitude() > Precision::Intersection()){
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
    gp_Pnt m_p0; // start point
    gp_Vec m_v0; // start direction
    gp_Pnt m_p1; // end point
    gp_Pnt m_c; // centre point
    gp_Dir m_a; // axis
    bool m_is_a_line;
    TangentialArc(const gp_Pnt& p0, const gp_Vec& v0, const gp_Pnt& p1)
        : m_p0(p0), m_v0(v0), m_p1(p1)
    {
        // calculate a tangential arc that goes through p0 and p1, with a direction of v0 at p0
        m_is_a_line = !Part::tangentialArc(m_p0, m_v0, m_p1, m_c, m_a);
    }
    bool isRadiusEqual(const gp_Pnt &p, double tolerance) const
    {
        if (m_is_a_line)
            return true;

        double point_radius = gp_Vec(m_c.XYZ() - p.XYZ()).Magnitude();
        double diff =  fabs(point_radius - radius());
        return diff <= tolerance;
    }
    double radius() const
    {
        double r0 = gp_Vec(m_p0.XYZ() - m_c.XYZ()).Magnitude();
        double r1 = gp_Vec(m_p1.XYZ() - m_c.XYZ()).Magnitude();
        double r = (r0 + r1)/2;
        return r;
    }
    Geometry* makeArc() const
    {
        if (m_is_a_line) {
            GeomLineSegment* line = new GeomLineSegment();
            line->setPoints(Base::convertTo<Base::Vector3d>(m_p0),Base::convertTo<Base::Vector3d>(m_p1));
            return line;
        }

        gp_Circ c(gp_Ax2(m_c, m_a), radius());
        GC_MakeArcOfCircle arc(c, m_p0, m_p1, true);
        GeomArcOfCircle* new_object = new GeomArcOfCircle();
        new_object->setHandle(arc.Value());
        return new_object;
    }
};

}

void BSplineCurveBiArcs::createArcs(double tolerance, std::list<Geometry*>& new_spans,
                                    const gp_Pnt& p_start, const gp_Vec& v_start,
                                    double t_start, double t_end, gp_Pnt& p_end, gp_Vec& v_end) const
{
    this->myCurve->D1(t_end, p_end, v_end);

    gp_Pnt p1, p2, p3;

    Type can_do_spline_whole = calculateBiArcPoints(t_start, p_start, v_start, t_end, p_end, v_end, p1, p2, p3);

    Geometry* arc_object1 = nullptr;
    Geometry* arc_object2 = nullptr;

    if (can_do_spline_whole == Type::SingleArc) {
        Part::TangentialArc arc1(p_start, v_start, p2);
        Part::TangentialArc arc2(p2, gp_Vec(p3.XYZ() - p2.XYZ()), p_end);

        gp_Pnt p_middle1, p_middle2;
        this->myCurve->D0(t_start + ((t_end - t_start) * 0.25), p_middle1);
        this->myCurve->D0(t_start + ((t_end - t_start) * 0.75), p_middle2);

        if (!arc1.isRadiusEqual(p_middle1, tolerance) ||
            !arc2.isRadiusEqual(p_middle2, tolerance)) {
            can_do_spline_whole = Type::SplitCurve;
        }
        else {
            arc_object1 = arc1.makeArc();
            arc_object2 = arc2.makeArc();
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
        createArcs(tolerance, new_spans, p_start, v_start, t_start, t_middle, p_middle, v_middle);// recursive
        gp_Pnt new_p_end;
        gp_Vec new_v_end;
        createArcs(tolerance, new_spans, p_middle, v_middle, t_middle, t_end, new_p_end, new_v_end);
    }
    else {
        // calculate_biarc_points failed, just add a line
        Part::GeomLineSegment* line = new Part::GeomLineSegment();
        line->setPoints(Base::convertTo<Base::Vector3d>(p_start),Base::convertTo<Base::Vector3d>(p_end));
        new_spans.push_back(line);
    }
}

BSplineCurveBiArcs::Type
BSplineCurveBiArcs::calculateBiArcPoints(double t_start, const gp_Pnt& p0, gp_Vec v_start,
                                         double t_end, const gp_Pnt& p4, gp_Vec v_end,
                                         gp_Pnt& p1, gp_Pnt& p2, gp_Pnt& p3) const
{
    if (v_start.Magnitude() < Precision::Intersection())
        v_start = gp_Vec(p0, p1);
    if (v_end.Magnitude() < Precision::Intersection())
        v_end = gp_Vec(p3, p4);

    v_start.Normalize();
    v_end.Normalize();

    gp_Vec v = p0.XYZ() - p4.XYZ();

    double a = 2*(v_start*v_end-1);
    double c = v*v;
    double b = (v*2)*(v_start+v_end);
    if (fabs(a) < Precision::Intersection()) {
        // Check the tangent of a value between t_start and t_end
        double t_mid = 0.9 * t_start + 0.1 * t_end;
        if (fabs(t_mid) > 0.1) {
            gp_Pnt p_mid;
            gp_Vec v_mid;
            this->myCurve->D1(t_mid, p_mid, v_mid);
            v_mid.Normalize();
            double a = 2*(v_start*v_mid-1);
            if (fabs(a) >= Precision::Intersection()) {
                return Type::SplitCurve;
            }
        }
        return Type::SingleLine;
    }


    double d = b*b-4*a*c;
    if (d < 0.0)
        return Type::SingleLine;

    double sd = sqrt(d);
    double e1 = (-b - sd) / (2.0 * a);
    double e2 = (-b + sd) / (2.0 * a);
    if (e1 > 0 && e2 > 0)
        return Type::SingleLine;

    double e = e1;
    if (e2 > e)
        e = e2;
    if (e < 0)
        return Type::SingleLine;

    p1 = p0.XYZ() + v_start.XYZ() * e;
    p3 = p4.XYZ() - v_end.XYZ() * e;
    p2 = p1.XYZ() * 0.5 + p3.XYZ() * 0.5;


    return Type::SingleArc;
}

BSplineCurveBiArcs::BSplineCurveBiArcs(const Handle(Geom_Curve)& c)
    : myCurve(c)
{

}

std::list<Geometry*> BSplineCurveBiArcs::toBiArcs(double tolerance) const
{
    gp_Pnt p_start;
    gp_Vec v_start;
    gp_Pnt p_end;
    gp_Vec v_end;
    this->myCurve->D0(this->myCurve->FirstParameter(), p_start);
    this->myCurve->D0(this->myCurve->LastParameter(), p_end);

    std::list<Geometry*> list;

    // the spline is closed
    if (p_start.Distance(p_end) < Precision::Intersection()) {
        this->myCurve->D1(this->myCurve->FirstParameter(), p_start, v_start);
        createArcs(tolerance, list, p_start, v_start, this->myCurve->FirstParameter(),
                   this->myCurve->LastParameter()/2, p_end, v_end);
        this->myCurve->D1(this->myCurve->LastParameter()/2, p_start, v_start);
        createArcs(tolerance, list, p_start, v_start, this->myCurve->LastParameter()/2,
                   this->myCurve->LastParameter(), p_end, v_end);
    }
    else {
        this->myCurve->D1(this->myCurve->FirstParameter(), p_start, v_start);
        createArcs(tolerance, list, p_start, v_start, this->myCurve->FirstParameter(),
                   this->myCurve->LastParameter(), p_end, v_end);
    }

    return list;
}
