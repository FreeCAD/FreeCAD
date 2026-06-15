// SPDX-License-Identifier: LGPL-2.1-or-later

/*************************************************************************** \
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            * \
 *                                                                         * \
 *   This file is part of the FreeCAD CAx development system.              * \
 *                                                                         * \
 *   This library is free software; you can redistribute it and/or         * \
 *   modify it under the terms of the GNU Library General Public           * \
 *   License as published by the Free Software Foundation; either          * \
 *   version 2 of the License, or (at your option) any later version.      * \
 *                                                                         * \
 *   This library  is distributed in the hope that it will be useful,      * \
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * \
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * \
 *   GNU Library General Public License for more details.                  * \
 *                                                                         * \
 *   You should have received a copy of the GNU Library General Public     * \
 *   License along with this library; see the file COPYING.LIB. If not,    * \
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         * \
 *   Suite 330, Boston, MA  02111-1307, USA                                * \
 *                                                                         * \
 ***************************************************************************/

#include "Canonizer.h"

namespace Part
{

Handle(Geom_Curve) Canonizer::getSimplestCurve(const TopoDS_Edge& shape, float tol)
{
    if (shape.IsNull()) {
        return {};
    }

    ShapeAnalysis_CanonicalRecognition canonRecon(shape);

    gp_Lin line;
    if (canonRecon.IsLine(tol, line)) {
        return new Geom_Line(line);
    }
    gp_Circ circle;
    if (canonRecon.IsCircle(tol, circle)) {
        return new Geom_Circle(circle);
    }
    gp_Elips ellipse;
    if (canonRecon.IsEllipse(tol, ellipse)) {
        return new Geom_Ellipse(ellipse);
    }
    BRepAdaptor_Curve adapt(shape);
    return adapt.Curve().Curve();
}

Handle(Geom_Surface) Canonizer::getSimplestSurface(const TopoDS_Face& shape, float tol)
{
    if (shape.IsNull()) {
        return {};
    }

    ShapeAnalysis_CanonicalRecognition canonRecon(shape);

    gp_Pln plane;
    if (canonRecon.IsPlane(tol, plane)) {
        return new Geom_Plane(plane);
    }
    gp_Cylinder cylinder;
    if (canonRecon.IsCylinder(tol, cylinder)) {
        return new Geom_CylindricalSurface(cylinder);
    }
    gp_Cone cone;
    if (canonRecon.IsCone(tol, cone)) {
        return new Geom_ConicalSurface(cone);
    }
    gp_Sphere sphere;
    if (canonRecon.IsSphere(tol, sphere)) {
        return new Geom_SphericalSurface(sphere);
    }
    BRepAdaptor_Surface adapt(shape);
    return adapt.Surface().Surface();
}

/*
 * Return an edge based on a canonical curve if possible
 * else the initial edge
 * Canonical curves are Circle, Ellipse and Line
 */
TopoDS_Edge Canonizer::canonize(const TopoDS_Edge& edge, float tol)
{
    if (edge.IsNull()) {
        return edge;
    }

    BRepAdaptor_Curve initialCurve(edge);
    auto simplestCurve = getSimplestCurve(edge, tol);
    if (!simplestCurve || simplestCurve->IsInstance("Geom_BSplineCurve")) {
        return edge;
    }

    auto P1 = initialCurve.Value(initialCurve.FirstParameter());
    auto P2 = initialCurve.Value(initialCurve.LastParameter());

    // recreate an edge
    TopoDS_Edge result;
    BRepBuilderAPI_MakeEdge mkBuilder(simplestCurve, P1, P2);
    mkBuilder.Build();
    if (mkBuilder.IsDone()) {
        result = TopoDS::Edge(mkBuilder.Shape());
        result.Orientation(edge.Orientation());
        return result;
    }
    return edge;
}

/*
 * Base the edges of the wire on canonical curves if possible
 */
TopoDS_Wire Canonizer::canonize(const TopoDS_Wire& wire, float tol)
{
    if (wire.IsNull()) {
        return wire;
    }

    TopoDS_Wire result(wire);

    BRepBuilderAPI_MakeWire wireBuilder;

    for (BRepTools_WireExplorer exp(wire); exp.More(); exp.Next()) {
        auto edge = TopoDS::Edge(exp.Current());
        auto ne = canonize(edge, tol);
        wireBuilder.Add(ne);
    }

    wireBuilder.Build();

    if (wireBuilder.IsDone()) {
        result = wireBuilder.Wire();
    }
    result.Orientation(wire.Orientation());
    return result;
}

/*
 * Return a face based on a canonical surface if possible
 * else on the initial face surface
 * All the return face edges are also based on canonical curves if possible
 * Canonical surfaces are Cone, Cylinder, Plane and Sphere
 */
TopoDS_Face Canonizer::canonize(const TopoDS_Face& face, float tol)
{
    if (face.IsNull()) {
        return face;
    }

    auto simplestSurf = getSimplestSurface(face, tol);

    auto outerWire = BRepTools::OuterWire(face);
    auto simplOW = canonize(outerWire, tol);

    // recreate a face
    BRepBuilderAPI_MakeFace faceMaker(simplestSurf, simplOW, /*Inside*/ true);

    // compute holes
    for (TopExp_Explorer exp(face, TopAbs_WIRE); exp.More(); exp.Next()) {
        auto wire = TopoDS::Wire(exp.Current());

        if (wire.IsNotEqual(outerWire)) {
            auto simplW = canonize(wire, tol);
            faceMaker.Add(simplW);
        }
    }

    return faceMaker.Face();
}

}  // namespace Part
