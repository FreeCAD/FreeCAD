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
#include <array>
#include <optional>

#include <App/DocumentObject.h>
#include <App/DocumentObserver.h>

#include "ShapeFinder.h"

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
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>


using namespace Measure;

namespace
{

// Mutable and sentinel-terminated because setEnums() takes const char**.
std::array<const char*, 7> SnapModeLabels
    = {"Auto", "None", "Vertex", "Center", "Midpoint", "Axis", nullptr};

constexpr std::array<const char*, 6> SnaplessMeasureTypes
    = {"LENGTH", "AREA", "DIAMETER", "RADIUS", "CENTEROFMASS", "POSITION"};

// A degenerate or curveless edge (sphere pole, cone apex) carries no 3D curve;
// constructing an adaptor on it raises, so callers must reject it first.
bool edgeHasCurve(const TopoDS_Edge& edge)
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
std::optional<gp_Pnt> centerOf(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
    if (adapt.GetType() != GeomAbs_Circle) {
        return {};
    }
    return adapt.Circle().Location();
}

std::optional<gp_Ax1> circleAxisOf(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
    if (adapt.GetType() != GeomAbs_Circle) {
        return {};
    }
    return adapt.Circle().Axis();
}

std::optional<gp_Ax1> lineAxisOf(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
    if (adapt.GetType() != GeomAbs_Line) {
        return {};
    }
    const gp_Lin line = adapt.Line();
    return gp_Ax1(line.Location(), line.Direction());
}

// Arc-length middle of an edge (differs from the parameter midpoint on a
// non-uniform curve).
std::optional<gp_Pnt> midpointOf(const TopoDS_Shape& shape)
{
    if (shape.IsNull() || shape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
    const double length = GCPnts_AbscissaPoint::Length(adapt);
    GCPnts_AbscissaPoint mid(adapt, length / 2.0, adapt.FirstParameter());
    if (!mid.IsDone()) {
        return {};
    }
    return adapt.Value(mid.Parameter());
}

// The axis a shape stands in for: a face's surface axis, else a circular or straight
// edge's supporting line.
std::optional<gp_Ax1> axisOfShape(const TopoDS_Shape& shape)
{
    if (shape.ShapeType() == TopAbs_FACE) {
        return MeasureSnap::axisOfFace(TopoDS::Face(shape));
    }
    if (const auto circle = circleAxisOf(shape)) {
        return circle;
    }
    return lineAxisOf(shape);
}

std::optional<gp_Pnt> boundsCentre(const Bnd_Box& box)
{
    if (box.IsVoid()) {
        return {};
    }
    const gp_Pnt lo = box.CornerMin();
    const gp_Pnt hi = box.CornerMax();
    return gp_Pnt((lo.X() + hi.X()) / 2.0, (lo.Y() + hi.Y()) / 2.0, (lo.Z() + hi.Z()) / 2.0);
}

// No cursor picks the first edge endpoint (deterministic at recompute); a cursor
// picks the nearer one.
std::optional<gp_Pnt> vertexOf(const TopoDS_Shape& shape, const Base::Vector3d* cursor)
{
    if (shape.IsNull()) {
        return {};
    }
    if (shape.ShapeType() == TopAbs_VERTEX) {
        return BRep_Tool::Pnt(TopoDS::Vertex(shape));
    }
    if (shape.ShapeType() != TopAbs_EDGE) {
        return {};
    }
    TopoDS_Vertex v1;
    TopoDS_Vertex v2;
    TopExp::Vertices(TopoDS::Edge(shape), v1, v2);
    if (v1.IsNull()) {
        return {};
    }
    const gp_Pnt p1 = BRep_Tool::Pnt(v1);
    if (!cursor || v2.IsNull()) {
        return p1;
    }
    const gp_Pnt p2 = BRep_Tool::Pnt(v2);
    const gp_Pnt cur(cursor->x, cursor->y, cursor->z);
    return (cur.SquareDistance(p1) <= cur.SquareDistance(p2)) ? p1 : p2;
}

// Preview point and direction for an axis snap. The point is the cursor, else the
// face bbox centre, projected onto the axis so it sits on the visible geometry.
std::optional<MeasureSnap::SnapPoint> axisPointOf(const TopoDS_Shape& shape, const Base::Vector3d* cursor)
{
    if (shape.IsNull()) {
        return {};
    }
    const auto axis = axisOfShape(shape);
    if (!axis) {
        return {};
    }
    gp_Pnt target;
    if (cursor) {
        target = gp_Pnt(cursor->x, cursor->y, cursor->z);
    }
    else if (shape.ShapeType() == TopAbs_FACE) {
        Bnd_Box box;
        BRepBndLib::Add(shape, box);
        target = boundsCentre(box).value_or(axis->Location());
    }
    else if (circleAxisOf(shape)) {
        target = axis->Location();  // circle centre lies on its axis
    }
    else {
        target = midpointOf(shape).value_or(axis->Location());
    }
    return MeasureSnap::SnapPoint {MeasureSnap::projectOntoAxis(*axis, target), axis->Direction()};
}

}  // namespace

const char** MeasureSnap::snapModeEnums()
{
    return SnapModeLabels.data();
}

const char* MeasureSnap::snapModeLabel(MeasureSnapMode mode)
{
    const auto index = static_cast<std::size_t>(mode);
    // The last slot is the setEnums() sentinel.
    return index < SnapModeLabels.size() - 1 ? SnapModeLabels[index] : SnapModeLabels.front();
}

bool MeasureSnap::typeUsesSnapping(const std::string& measureTypeIdentifier)
{
    // Unknown types (Python-registered, third-party) keep the preview.
    return std::ranges::none_of(SnaplessMeasureTypes, [&measureTypeIdentifier](const char* id) {
        return measureTypeIdentifier == id;
    });
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
        App::DocumentObject* root = subject.getObject();
        if (!root) {
            return {};
        }
        // Same resolver the measurement engine uses, so the preview lands where
        // the snap will; it follows link/assembly placement and scale chains.
        return ShapeFinder::getLocatedShape(*root, subject.getSubName());
    }
    catch (...) {
        return {};
    }
}

std::vector<gp_Pnt> MeasureSnap::previewPoints(const TopoDS_Shape& shape, MeasureSnapMode type)
{
    std::vector<gp_Pnt> points;
    // resolveShape hands back a null shape when it cannot resolve, and a curveless
    // edge raises in the adaptors below.
    if (shape.IsNull()) {
        return points;
    }
    if (shape.ShapeType() == TopAbs_EDGE && !edgeHasCurve(TopoDS::Edge(shape))) {
        return points;
    }
    switch (type) {
        case MeasureSnapMode::Center:
            if (const auto centre = centerOf(shape)) {
                points.push_back(*centre);
            }
            break;
        case MeasureSnapMode::Midpoint:
            if (const auto mid = midpointOf(shape)) {
                points.push_back(*mid);
            }
            break;
        case MeasureSnapMode::Vertex:
            if (shape.ShapeType() == TopAbs_VERTEX) {
                points.push_back(BRep_Tool::Pnt(TopoDS::Vertex(shape)));
            }
            else if (shape.ShapeType() == TopAbs_EDGE && !BRep_Tool::IsClosed(TopoDS::Edge(shape))) {
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
            const auto axis = axisOfShape(shape);
            if (!axis) {
                break;
            }
            Bnd_Box box;
            BRepBndLib::Add(shape, box);
            if (const auto segment = axisPreviewSegment(*axis, box)) {
                points.push_back(segment->first);
                points.push_back(segment->second);
            }
            break;
        }
        default:
            break;
    }
    return points;
}

std::optional<std::pair<gp_Pnt, gp_Pnt>> MeasureSnap::axisPreviewSegment(
    const gp_Ax1& axis,
    const Bnd_Box& bounds
)
{
    const auto centre = boundsCentre(bounds);
    if (!centre) {
        return {};
    }
    const gp_Pnt lo = bounds.CornerMin();
    const gp_Pnt hi = bounds.CornerMax();
    const gp_Pnt onAxis = projectOntoAxis(axis, *centre);
    // Overshoot the shape so the line reads as a reference, not the edge itself.
    constexpr double extentFactor = 0.6;
    constexpr double minHalfLength = 1.0;
    const double half = std::max(extentFactor * lo.Distance(hi), minHalfLength);
    const gp_Vec step = gp_Vec(axis.Direction()) * half;
    return std::pair {onAxis.Translated(-step), onAxis.Translated(step)};
}

std::optional<MeasureSnap::SnapPoint> MeasureSnap::computeSnapPoint(
    const TopoDS_Shape& shape,
    MeasureSnapMode mode,
    const Base::Vector3d* cursor
)
{
    if (shape.IsNull()) {
        return {};
    }

    if (shape.ShapeType() == TopAbs_EDGE && !edgeHasCurve(TopoDS::Edge(shape))) {
        return {};
    }

    auto pointOnly = [](const std::optional<gp_Pnt>& point) -> std::optional<SnapPoint> {
        if (!point) {
            return {};
        }
        return SnapPoint {*point, {}};
    };

    switch (mode) {
        case MeasureSnapMode::Center:
            return pointOnly(centerOf(shape));
        case MeasureSnapMode::Midpoint:
            return pointOnly(midpointOf(shape));
        case MeasureSnapMode::Vertex:
            return pointOnly(vertexOf(shape, cursor));
        case MeasureSnapMode::Axis:
            return axisPointOf(shape, cursor);
        default:
            return {};
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
        // A closed edge's two vertices are the same seam point, so it has no
        // endpoint to snap to.
        int flags = static_cast<int>(MeasureSnapFlag::FlagMidpoint);
        if (!BRep_Tool::IsClosed(edge)) {
            flags |= static_cast<int>(MeasureSnapFlag::FlagVertex);
        }
        BRepAdaptor_Curve adapt(edge);
        if (adapt.GetType() == GeomAbs_Circle) {
            flags |= static_cast<int>(MeasureSnapFlag::FlagCenter);
            flags |= static_cast<int>(MeasureSnapFlag::FlagAxis);
        }
        else if (adapt.GetType() == GeomAbs_Line) {
            flags |= static_cast<int>(MeasureSnapFlag::FlagAxis);
        }
        return flags;
    }

    // Wires offer no flags; see centerOf.
    if (shape.ShapeType() == TopAbs_WIRE) {
        return 0;
    }

    if (shape.ShapeType() == TopAbs_FACE) {
        if (axisOfFace(TopoDS::Face(shape))) {
            return static_cast<int>(MeasureSnapFlag::FlagAxis);
        }
        return 0;
    }

    return 0;
}

std::optional<gp_Ax1> MeasureSnap::axisOfFace(const TopoDS_Face& face)
{
    if (face.IsNull()) {
        return {};
    }
    BRepAdaptor_Surface surf(face);
    switch (surf.GetType()) {
        case GeomAbs_Cylinder:
            return surf.Cylinder().Axis();
        case GeomAbs_Cone:
            return surf.Cone().Axis();
        case GeomAbs_SurfaceOfRevolution:
            return surf.AxeOfRevolution();
        default:
            return {};
    }
}

gp_Pnt MeasureSnap::projectOntoAxis(const gp_Ax1& axis, const gp_Pnt& p)
{
    const gp_Lin line(axis);
    return ElCLib::Value(ElCLib::Parameter(line, p), line);
}

std::optional<std::pair<gp_Pnt, gp_Pnt>> MeasureSnap::closestPointsOnAxes(
    const gp_Ax1& a,
    const gp_Ax1& b
)
{
    const gp_Lin lineA(a);
    const gp_Lin lineB(b);
    Extrema_ExtElC ext(lineA, lineB, Precision::Angular());
    if (!ext.IsDone()) {
        return {};
    }
    if (ext.IsParallel()) {
        const gp_Pnt onA = a.Location();
        return std::pair {onA, projectOntoAxis(b, onA)};
    }
    if (ext.NbExt() < 1) {
        return {};
    }
    Extrema_POnCurv pOnA;
    Extrema_POnCurv pOnB;
    ext.Points(1, pOnA, pOnB);
    return std::pair {pOnA.Value(), pOnB.Value()};
}

TopoDS_Edge MeasureSnap::boundedAxisEdge(const gp_Ax1& axis, const Bnd_Box& pairBounds)
{
    const auto centre = boundsCentre(pairBounds);
    if (!centre) {
        return TopoDS_Edge();
    }
    const gp_Pnt lo = pairBounds.CornerMin();
    const gp_Pnt hi = pairBounds.CornerMax();
    const gp_Pnt onAxis = projectOntoAxis(axis, *centre);
    // A collapsed box would ask for a zero-length edge, which MakeEdge refuses.
    constexpr double minHalfSpan = 1.0;
    const gp_Vec halfSpan = gp_Vec(axis.Direction()) * std::max(lo.Distance(hi), minHalfSpan);
    BRepBuilderAPI_MakeEdge mkEdge(onAxis.Translated(-halfSpan), onAxis.Translated(halfSpan));
    if (!mkEdge.IsDone()) {
        return TopoDS_Edge();
    }
    return mkEdge.Edge();
}
