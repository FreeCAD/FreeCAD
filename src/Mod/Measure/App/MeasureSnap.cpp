// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Krrish777 <777krrish[at]gmail.com>                 *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "MeasureSnap.h"

#include <algorithm>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeature.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/TopoShape.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <ElCLib.hxx>
#include <Extrema_ExtElC.hxx>
#include <Extrema_POnCurv.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <gp_Circ.hxx>
#include <gp_Cone.hxx>
#include <gp_Cylinder.hxx>
#include <gp_Lin.hxx>
#include <gp_Vec.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>


using namespace Measure;

// Labels in MeasureSnapMode order; nullptr-terminated for setEnums().
static const char* SnapModeEnums[] = {"Auto", "None", "Vertex", "Center", "Midpoint", "Axis", nullptr};

// A degenerate or curveless edge (sphere pole, cone apex) carries no 3D curve;
// constructing an adaptor on it raises, so callers must reject it first.
static bool edgeHasCurve(const TopoDS_Edge& edge)
{
    if (BRep_Tool::Degenerated(edge)) {
        return false;
    }
    double first = 0.0;
    double last = 0.0;
    return !BRep_Tool::Curve(edge, first, last).IsNull();
}

// Circle center of a circular edge; a single circular-edge wire is not
// recognized as a circle by BRepAdaptor_CompCurve, so wires are not handled.
static bool centerOf(const TopoDS_Shape& shape, gp_Pnt& out)
{
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_EDGE) {
        BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
        if (adapt.GetType() == GeomAbs_Circle) {
            out = adapt.Circle().Location();
            return true;
        }
        return false;
    }
    return false;
}

// Arc-length middle of an edge (differs from the parameter midpoint on a
// non-uniform curve).
static bool midpointOf(const TopoDS_Shape& shape, gp_Pnt& out)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return false;
    }
    BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
    const double length = GCPnts_AbscissaPoint::Length(adapt);
    GCPnts_AbscissaPoint mid(adapt, length / 2.0, adapt.FirstParameter());
    if (!mid.IsDone()) {
        return false;
    }
    out = adapt.Value(mid.Parameter());
    return true;
}

// No cursor picks the first edge endpoint (deterministic at recompute); a cursor
// picks the nearer one.
static bool vertexOf(const TopoDS_Shape& shape, const Base::Vector3d* cursor, gp_Pnt& out)
{
    if (shape.IsNull()) {
        return false;
    }
    if (shape.ShapeType() == TopAbs_VERTEX) {
        out = BRep_Tool::Pnt(TopoDS::Vertex(shape));
        return true;
    }
    if (shape.ShapeType() == TopAbs_EDGE) {
        TopoDS_Vertex v1;
        TopoDS_Vertex v2;
        TopExp::Vertices(TopoDS::Edge(shape), v1, v2);
        if (v1.IsNull()) {
            return false;
        }
        const gp_Pnt p1 = BRep_Tool::Pnt(v1);
        if (!cursor || v2.IsNull()) {
            out = p1;
            return true;
        }
        const gp_Pnt p2 = BRep_Tool::Pnt(v2);
        const gp_Pnt cur(cursor->x, cursor->y, cursor->z);
        out = (cur.SquareDistance(p1) <= cur.SquareDistance(p2)) ? p1 : p2;
        return true;
    }
    return false;
}

// Preview point and direction for an axis snap. The point is the cursor, else the
// face bbox centre, projected onto the axis so it sits on the visible geometry.
static bool axisPointOf(const TopoDS_Shape& shape, const Base::Vector3d* cursor, gp_Pnt& out, gp_Dir* outDir)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_FACE) {
        return false;
    }
    gp_Ax1 axis;
    if (!MeasureSnap::axisOfFace(TopoDS::Face(shape), axis)) {
        return false;
    }
    if (outDir) {
        *outDir = axis.Direction();
    }
    gp_Pnt target;
    if (cursor) {
        target = gp_Pnt(cursor->x, cursor->y, cursor->z);
    }
    else {
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        const gp_Pnt lo = box.CornerMin();
        const gp_Pnt hi = box.CornerMax();
        target = gp_Pnt((lo.X() + hi.X()) / 2.0, (lo.Y() + hi.Y()) / 2.0, (lo.Z() + hi.Z()) / 2.0);
    }
    out = MeasureSnap::projectOntoAxis(axis, target);
    return true;
}

const char** MeasureSnap::snapModeEnums()
{
    return SnapModeEnums;
}

MeasureSnapMode MeasureSnap::snapModeFromIndex(long index)
{
    if (index < 0 || index > static_cast<long>(MeasureSnapMode::Axis)) {
        return MeasureSnapMode::Auto;
    }
    return static_cast<MeasureSnapMode>(index);
}

MeasureSnapMode MeasureSnap::pickPreviewType(int availableFlags, MeasureSnapMode activeMode)
{
    auto has = [availableFlags](MeasureSnapFlag flag) {
        return (availableFlags & static_cast<int>(flag)) != 0;
    };

    switch (activeMode) {
        case MeasureSnapMode::Vertex:
            return has(MeasureSnapFlag::FlagVertex) ? MeasureSnapMode::Vertex : MeasureSnapMode::None;
        case MeasureSnapMode::Center:
            return has(MeasureSnapFlag::FlagCenter) ? MeasureSnapMode::Center : MeasureSnapMode::None;
        case MeasureSnapMode::Midpoint:
            return has(MeasureSnapFlag::FlagMidpoint) ? MeasureSnapMode::Midpoint
                                                      : MeasureSnapMode::None;
        case MeasureSnapMode::Axis:
            return has(MeasureSnapFlag::FlagAxis) ? MeasureSnapMode::Axis : MeasureSnapMode::None;
        case MeasureSnapMode::Auto:
            if (has(MeasureSnapFlag::FlagCenter)) {
                return MeasureSnapMode::Center;
            }
            if (has(MeasureSnapFlag::FlagMidpoint)) {
                return MeasureSnapMode::Midpoint;
            }
            if (has(MeasureSnapFlag::FlagVertex)) {
                return MeasureSnapMode::Vertex;
            }
            if (has(MeasureSnapFlag::FlagAxis)) {
                return MeasureSnapMode::Axis;
            }
            return MeasureSnapMode::None;
        case MeasureSnapMode::None:
            return MeasureSnapMode::None;
    }
    return MeasureSnapMode::None;
}

TopoDS_Shape MeasureSnap::resolveShape(const App::SubObjectT& subject)
{
    try {
        const auto chain = subject.getSubObjectList();
        if (chain.empty()) {
            return {};
        }
        App::DocumentObject* obj = chain.back();
        if (!obj || !obj->getNameInDocument()) {
            return {};
        }
        if (obj->isDerivedFrom<Part::Feature>()) {
            Part::TopoShape ts = static_cast<const Part::Feature*>(obj)->Shape.getShape();
            ts.setPlacement(
                App::GeoFeature::getGlobalPlacement(obj, subject.getObject(), subject.getSubName())
            );
            ts = ts.getSubTopoShape(subject.getElementName(), true);
            if (!ts.isNull()) {
                return ts.getShape();
            }
        }
        Part::TopoShape ts = Part::Feature::getTopoShape(
            obj,
            Part::ShapeOption::NeedSubElement | Part::ShapeOption::ResolveLink
                | Part::ShapeOption::Transform,
            subject.getElementName()
        );
        if (ts.isNull()) {
            return {};
        }
        ts.setPlacement(
            App::GeoFeature::getGlobalPlacement(obj, subject.getObject(), subject.getSubName())
        );
        return ts.getShape();
    }
    catch (...) {
        return {};
    }
}

std::vector<gp_Pnt> MeasureSnap::previewPoints(const TopoDS_Shape& shape, MeasureSnapMode type)
{
    std::vector<gp_Pnt> points;
    gp_Pnt p;
    switch (type) {
        case MeasureSnapMode::Center:
            if (centerOf(shape, p)) {
                points.push_back(p);
            }
            break;
        case MeasureSnapMode::Midpoint:
            if (midpointOf(shape, p)) {
                points.push_back(p);
            }
            break;
        case MeasureSnapMode::Vertex:
            if (shape.ShapeType() == TopAbs_VERTEX) {
                points.push_back(BRep_Tool::Pnt(TopoDS::Vertex(shape)));
            }
            else if (shape.ShapeType() == TopAbs_EDGE) {
                TopoDS_Vertex v1;
                TopoDS_Vertex v2;
                TopExp::Vertices(TopoDS::Edge(shape), v1, v2);
                if (!v1.IsNull()) {
                    points.push_back(BRep_Tool::Pnt(v1));
                }
                if (!v2.IsNull()) {
                    points.push_back(BRep_Tool::Pnt(v2));
                }
            }
            break;
        case MeasureSnapMode::Axis: {
            if (shape.ShapeType() != TopAbs_FACE) {
                break;
            }
            gp_Ax1 axis;
            if (!axisOfFace(TopoDS::Face(shape), axis)) {
                break;
            }
            Bnd_Box box;
            BRepBndLib::Add(shape, box);
            gp_Pnt a;
            gp_Pnt b;
            if (axisPreviewSegment(axis, box, a, b)) {
                points.push_back(a);
                points.push_back(b);
            }
            break;
        }
        default:
            break;
    }
    return points;
}

bool MeasureSnap::axisPreviewSegment(const gp_Ax1& axis, const Bnd_Box& bounds, gp_Pnt& a, gp_Pnt& b)
{
    if (bounds.IsVoid()) {
        return false;
    }
    const gp_Pnt lo = bounds.CornerMin();
    const gp_Pnt hi = bounds.CornerMax();
    const gp_Pnt centre((lo.X() + hi.X()) / 2.0, (lo.Y() + hi.Y()) / 2.0, (lo.Z() + hi.Z()) / 2.0);
    const gp_Pnt onAxis = projectOntoAxis(axis, centre);
    // Overshoot the shape so the line reads as a reference, not the edge itself.
    constexpr double extentFactor = 0.6;
    constexpr double minHalfLength = 1.0;
    const double half = std::max(extentFactor * lo.Distance(hi), minHalfLength);
    const gp_Vec step = gp_Vec(axis.Direction()) * half;
    a = onAxis.Translated(-step);
    b = onAxis.Translated(step);
    return true;
}

bool MeasureSnap::computeSnapPoint(
    const TopoDS_Shape& shape,
    MeasureSnapMode mode,
    const Base::Vector3d* cursor,
    gp_Pnt& out,
    gp_Dir* outAxisDir
)
{
    if (shape.IsNull()) {
        return false;
    }

    if (shape.ShapeType() == TopAbs_EDGE && !edgeHasCurve(TopoDS::Edge(shape))) {
        return false;
    }

    switch (mode) {
        case MeasureSnapMode::Center:
            return centerOf(shape, out);
        case MeasureSnapMode::Midpoint:
            return midpointOf(shape, out);
        case MeasureSnapMode::Vertex:
            return vertexOf(shape, cursor, out);
        case MeasureSnapMode::Axis:
            return axisPointOf(shape, cursor, out, outAxisDir);
        default:
            return false;
    }
}

int MeasureSnap::getAvailableSnapTypes(const TopoDS_Shape& shape)
{
    if (shape.IsNull()) {
        return 0;
    }

    if (shape.ShapeType() == TopAbs_VERTEX) {
        return static_cast<int>(MeasureSnapFlag::FlagVertex);
    }

    if (shape.ShapeType() == TopAbs_EDGE) {
        const TopoDS_Edge& edge = TopoDS::Edge(shape);
        if (!edgeHasCurve(edge)) {
            return 0;
        }
        int flags = static_cast<int>(MeasureSnapFlag::FlagVertex)
            | static_cast<int>(MeasureSnapFlag::FlagMidpoint);
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            flags |= static_cast<int>(MeasureSnapFlag::FlagCenter);
        }
        return flags;
    }

    // Wires offer no flags; see centerOf.
    if (shape.ShapeType() == TopAbs_WIRE) {
        return 0;
    }

    if (shape.ShapeType() == TopAbs_FACE) {
        gp_Ax1 axis;
        if (axisOfFace(TopoDS::Face(shape), axis)) {
            return static_cast<int>(MeasureSnapFlag::FlagAxis);
        }
        return 0;
    }

    return 0;
}

bool MeasureSnap::axisOfFace(const TopoDS_Face& face, gp_Ax1& out)
{
    if (face.IsNull()) {
        return false;
    }
    BRepAdaptor_Surface surf(face);
    switch (surf.GetType()) {
        case GeomAbs_Cylinder:
            out = surf.Cylinder().Axis();
            return true;
        case GeomAbs_Cone:
            out = surf.Cone().Axis();
            return true;
        case GeomAbs_SurfaceOfRevolution:
            out = surf.AxeOfRevolution();
            return true;
        default:
            return false;
    }
}

gp_Pnt MeasureSnap::projectOntoAxis(const gp_Ax1& axis, const gp_Pnt& p)
{
    const gp_Lin line(axis);
    return ElCLib::Value(ElCLib::Parameter(line, p), line);
}

bool MeasureSnap::closestPointsOnAxes(const gp_Ax1& a, const gp_Ax1& b, gp_Pnt& onA, gp_Pnt& onB)
{
    const gp_Lin lineA(a);
    const gp_Lin lineB(b);
    // Placement round-trips leave nominally parallel axes a hair off; without this
    // slack the skew solve puts the feet ~separation/angle away from the shapes.
    constexpr double parallelTol = 1e-6;
    Extrema_ExtElC ext(lineA, lineB, parallelTol);
    if (!ext.IsDone()) {
        return false;
    }
    if (ext.IsParallel()) {
        onA = a.Location();
        onB = projectOntoAxis(b, onA);
        return true;
    }
    if (ext.NbExt() < 1) {
        return false;
    }
    Extrema_POnCurv pOnA;
    Extrema_POnCurv pOnB;
    ext.Points(1, pOnA, pOnB);
    onA = pOnA.Value();
    onB = pOnB.Value();
    return true;
}

TopoDS_Edge MeasureSnap::boundedAxisEdge(const gp_Ax1& axis, const Bnd_Box& pairBounds)
{
    if (pairBounds.IsVoid()) {
        return TopoDS_Edge();
    }
    const gp_Pnt lo = pairBounds.CornerMin();
    const gp_Pnt hi = pairBounds.CornerMax();
    const gp_Pnt centre((lo.X() + hi.X()) / 2.0, (lo.Y() + hi.Y()) / 2.0, (lo.Z() + hi.Z()) / 2.0);
    const gp_Pnt onAxis = projectOntoAxis(axis, centre);
    const gp_Vec halfSpan = gp_Vec(axis.Direction()) * lo.Distance(hi);
    return BRepBuilderAPI_MakeEdge(onAxis.Translated(-halfSpan), onAxis.Translated(halfSpan)).Edge();
}
