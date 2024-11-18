/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
// a class to handle changes to dimension reference geometry

// detects changes in reference geometry (2d & 3d) that would invalidate a dimension
// identifies replacement view/model geometry

#include "PreCompiled.h"
#ifndef _PreComp_
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#endif

#include <BRepAdaptor_Curve.hxx>
#include <BRepLProp_CLProps.hxx>
#include <BRep_Tool.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <TopExp.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Vertex.hxx>

#include <Base/Console.h>
#include <Base/Tools.h>

#include <Mod/Part/App/TopoShape.h>
#include <Mod/TechDraw/App/ShapeUtils.h>

#include "GeometryMatcher.h"
#include "DrawUtil.h"
#include "Preferences.h"

using namespace TechDraw;
using DU = DrawUtil;
using SU = ShapeUtils;

// a set of routines for comparing geometry for equality.

bool GeometryMatcher::compareGeometry(const Part::TopoShape &shape1, const Part::TopoShape &shape2)
{
    if (!Preferences::useExactMatchOnDims()) {
        return false;
    }
    if (shape1.isNull() || shape2.isNull()) {
        return false;
    }
    const TopoDS_Shape& geom1 = shape1.getShape();
    const TopoDS_Shape& geom2 = shape2.getShape();
    if (geom1.IsNull() || geom2.IsNull()) {
        return false;
    }

    if (geom1.ShapeType() == TopAbs_VERTEX) {
        return comparePoints(geom1, geom2);
    }
    if (geom1.ShapeType() == TopAbs_EDGE) {
        return compareEdges(geom1, geom2);
    }
    if (geom1.ShapeType() == TopAbs_FACE) {
        return compareFaces(geom1, geom2);
    }
    return false;
}


bool GeometryMatcher::comparePoints(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    if (shape1.ShapeType() != TopAbs_VERTEX || shape2.ShapeType() != TopAbs_VERTEX) {
        // can not compare these shapes
        return false;
    }
    auto vert1 = TopoDS::Vertex(shape1);
    Base::Vector3d point1 = DU::toVector3d(BRep_Tool::Pnt(vert1));
    auto vert2 = TopoDS::Vertex(shape2);
    Base::Vector3d point2 = DU::toVector3d(BRep_Tool::Pnt(vert2));
    return point1.IsEqual(point2, EWTOLERANCE);
}

bool GeometryMatcher::compareFaces(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    if (shape1.ShapeType() != TopAbs_FACE || shape2.ShapeType() != TopAbs_FACE) {
        // can not compare these shapes
        return false;
    }
    TopoDS_Face face1 = TopoDS::Face(shape1);
    TopoDS_Face face2 = TopoDS::Face(shape2);

    //Note: face1.IsSame(face2) and face1.IsEqual(face2) do not work.

    GProp_GProps props1, props2;
    BRepGProp::SurfaceProperties(face1, props1);
    BRepGProp::SurfaceProperties(face2, props2);

    // Check if areas are approximately equal
    return fabs(props1.Mass() - props2.Mass()) < 1e-5;
}

bool GeometryMatcher::compareEdges(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2)
{
    if (shape1.ShapeType() != TopAbs_EDGE || shape2.ShapeType() != TopAbs_EDGE) {
       return false;
    }
    TopoDS_Edge edge1 = TopoDS::Edge(shape1);
    TopoDS_Edge edge2 = TopoDS::Edge(shape2);
    if (edge1.IsNull() || edge2.IsNull()) {
        return false;
    }

    BRepAdaptor_Curve adapt1(edge1);
    BRepAdaptor_Curve adapt2(edge2);

    if (adapt1.GetType() == GeomAbs_Line && adapt2.GetType() == GeomAbs_Line) {
        return compareLines(edge1, edge2);
    }

    if (adapt1.GetType() == GeomAbs_Circle && adapt2.GetType() == GeomAbs_Circle) {
        if (adapt1.IsClosed() && adapt2.IsClosed()) {
            return compareCircles(edge1, edge2);
        }
        return compareCircleArcs(edge1, edge2);
    }

    if (adapt1.GetType() == GeomAbs_Ellipse && adapt2.GetType() == GeomAbs_Ellipse) {
        if (adapt1.IsClosed() && adapt2.IsClosed()) {
            return compareEllipses(edge1, edge2);
        }
        return compareEllipseArcs(edge1, edge2);
    }

    if (adapt1.GetType() == GeomAbs_BSplineCurve && adapt2.GetType() == GeomAbs_BSplineCurve) {
        return compareBSplines(edge1, edge2);
    }

    // if we reach this point, we have dissimilar geometry types
    return compareDifferent(edge1, edge2);
}

bool GeometryMatcher::compareLines(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    // how does the edge that was NOT null in compareEdges become null here?
    // should not happen, but does!
    if (edge1.IsNull() || edge2.IsNull()) {
        //        Base::Console().Message("GM::compareLine - an input edge is null\n");
        return false;
    }
    return compareEndPoints(edge1, edge2);
}

bool GeometryMatcher::compareCircles(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    // how does the edge that was NOT null in compareEdges become null here?
    if (edge1.IsNull() || edge2.IsNull()) {
        return false;
    }

    BRepAdaptor_Curve adapt1(edge1);
    BRepAdaptor_Curve adapt2(edge2);
    gp_Circ circle1 = adapt1.Circle();
    gp_Circ circle2 = adapt2.Circle();
    double radius1 = circle1.Radius();
    double radius2 = circle2.Radius();
    auto center1 = DU::toVector3d(circle1.Location());
    auto center2 = DU::toVector3d(circle2.Location());
    return DU::fpCompare(radius1, radius2, EWTOLERANCE) && center1.IsEqual(center2, EWTOLERANCE);
}

bool GeometryMatcher::compareEllipses(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    // how does the edge that was NOT null in compareEdges become null here?
    if (edge1.IsNull() || edge2.IsNull()) {
        return false;
    }

    BRepAdaptor_Curve adapt1(edge1);
    BRepAdaptor_Curve adapt2(edge2);
    gp_Elips ellipse1 = adapt1.Ellipse();
    gp_Elips ellipse2 = adapt2.Ellipse();
    double major1 = ellipse1.MajorRadius();
    double minor1 = ellipse1.MinorRadius();
    double major2 = ellipse2.MajorRadius();
    double minor2 = ellipse2.MinorRadius();
    auto center1 = DU::toVector3d(ellipse1.Location());
    auto center2 = DU::toVector3d(ellipse2.Location());
    return  (DU::fpCompare(major1, major2, EWTOLERANCE) &&
             DU::fpCompare(minor1, minor2, EWTOLERANCE) &&
             center1.IsEqual(center2, EWTOLERANCE));
}

// for our purposes, only lines or circles masquerading as bsplines are of interest
bool GeometryMatcher::compareBSplines(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    // how does the edge that was NOT null in compareEdges become null here?
    if (edge1.IsNull() || edge2.IsNull()) {
        return false;
    }

    if (GeometryUtils::isLine(edge1) && GeometryUtils::isLine(edge2)) {
        return compareEndPoints(edge1, edge2);
    }

    if (!(GeometryUtils::isCircle(edge1) && GeometryUtils::isCircle(edge2))) {
        return false;
    }

    // deal with bsplines as circles
    bool isArc1(false);
    bool isArc2(false);
    TopoDS_Edge circleEdge1;
    TopoDS_Edge circleEdge2;
    try {
        circleEdge1 = GeometryUtils::asCircle(edge1, isArc1);
        circleEdge2 = GeometryUtils::asCircle(edge2, isArc2);
    }
    catch (Base::RuntimeError&) {
        Base::Console().Error("GeometryMatcher failed to make circles from splines\n");
        return false;
    }
    if (!isArc1 && !isArc2) {
        // full circles
        return compareCircles(circleEdge1, circleEdge2);
    }
    if (isArc1 && isArc2) {
        // circular arcs
        return compareCircleArcs(circleEdge1, circleEdge2);
    }

    return false;
}

// this is a weak comparison.  we should also check center & radius?
bool GeometryMatcher::compareCircleArcs(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    return compareCircles(edge1, edge2) && compareEndPoints(edge1, edge2);
}

bool GeometryMatcher::compareEllipseArcs(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    return compareEndPoints(edge1, edge2);
}

// this is where we would try to match a bspline against a line or a circle.
// not sure how successful this would be.  For now, we just say it doesn't match
bool GeometryMatcher::compareDifferent(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    BRepAdaptor_Curve adapt1(edge1);
    BRepAdaptor_Curve adapt2(edge2);
    return false;
}

bool GeometryMatcher::compareEndPoints(const TopoDS_Edge& edge1, const TopoDS_Edge& edge2)
{
    // how does the edge that was NOT null in compareEdges become null here?
    if (edge1.IsNull() || edge2.IsNull()) {
        return false;
    }

    auto ends1 = SU::getEdgeEnds(edge1);
    auto ends2 = SU::getEdgeEnds(edge2);
    return (ends1.first.IsEqual(ends2.first, EWTOLERANCE) &&
            ends1.second.IsEqual(ends2.second, EWTOLERANCE));
}

