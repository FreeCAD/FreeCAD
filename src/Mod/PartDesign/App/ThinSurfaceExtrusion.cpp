// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ThinSurfaceExtrusion.h"

#include <algorithm>
#include <cmath>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopExp.hxx>
#include <BRepOffset_MakeOffset.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <BRepTools.hxx>
#include <BRep_Tool.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <Precision.hxx>
#include <ShapeFix_Face.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopoDS.hxx>
#include <gp_Pln.hxx>
#include <GC_MakeArcOfCircle.hxx>

#include <Base/Exception.h>
#include <Mod/Part/App/ProgressIndicator.h>

namespace PartDesign
{
namespace
{
// BRepOffset_MakeOffset is not a BRepBuilderAPI_MakeShape. Adapt its native
// history, rather than dropping names or reconstructing geometric matches.
struct OffsetMapper: Part::TopoShape::Mapper
{
    explicit OffsetMapper(BRepOffset_MakeOffset& builder)
        : builder(builder)
    {}
    const std::vector<TopoDS_Shape>& generated(const TopoDS_Shape& shape) const override
    {
        const auto& list = builder.Generated(shape);
        _res.assign(list.begin(), list.end());
        return _res;
    }
    const std::vector<TopoDS_Shape>& modified(const TopoDS_Shape& shape) const override
    {
        const auto& list = builder.Modified(shape);
        _res.assign(list.begin(), list.end());
        return _res;
    }
    BRepOffset_MakeOffset& builder;
};

void validateWall(const Part::TopoShape& shape, bool checkSelfIntersection = true)
{
    if (shape.isNull() || shape.countSubShapes(TopAbs_SOLID) != 1 || !shape.isValid()
        || (checkSelfIntersection && !BRepAlgoAPI_Check(shape.getShape(), false, true).IsValid())) {
        throw Base::CADKernelError(
            "Pipe thickness produced an invalid or self-intersecting wall; "
            "reduce thickness or change the join type"
        );
    }
}


Part::TopoShape thicken(
    const Part::TopoShape& reference,
    double distance,
    Part::JoinType join,
    bool intersection,
    long tag,
    const char* op
)
{
    // OCCT may change tolerances and p-curves: never share mutable offset input
    // between sides, recomputes, or the user's original profile.
    const auto shell = reference.makeElementCopy("PipeOffsetInput");

    BRepOffset_MakeOffset builder;
    builder.Initialize(
        shell.getShape(),
        distance,
        Precision::Confusion(),
        BRepOffset_Skin,
        intersection,
        false,
        static_cast<GeomAbs_JoinType>(join),
        true
    );
    builder.MakeOffsetShape(std::make_unique<Part::ProgressIndicator>()->Start());
    if (!builder.IsDone()) {
        throw Base::CADKernelError("Cannot thicken the extruded profile with these parameters");
    }

    Part::TopoShape wall(tag, shell.Hasher);
    wall.makeShapeWithElementMap(builder.Shape(), OffsetMapper(builder), {shell}, op);

    wall.fixSolidOrientation();
    validateWall(wall);

    ShapeAnalysis_ShapeTolerance tolerance;
    const double inputTolerance
        = std::max(Precision::Confusion(), tolerance.Tolerance(reference.getShape(), 1));
    if (tolerance.Tolerance(wall.getShape(), 1)
        > std::max(10 * inputTolerance, std::abs(distance) * 1e-6)) {
        throw Base::CADKernelError("Spatial wall closure exceeds the input modeling tolerance budget");
    }

    return wall;
}

Part::TopoShape segmentedWall(
    const Part::TopoShape& wire,
    const gp_Dir& direction,
    double length,
    double distance,
    Part::JoinType join,
    bool intersection,
    long tag
)
{
    const gp_Vec axis(direction);
    std::vector<Part::TopoShape> edges, pieces;

    for (BRepTools_WireExplorer it(TopoDS::Wire(wire.getShape())); it.More(); it.Next()) {
        Part::TopoShape edge(tag, wire.Hasher, it.Current());
        edge.mapSubElement(wire);
        edges.push_back(edge);
        const bool reverse = edge.getShape().Orientation() == TopAbs_REVERSED;
        edge.setShape(edge.getShape().Oriented(TopAbs_FORWARD), false);
        auto sheet = edge.makeElementPrism(axis * length, "PipeSegmentSurface");
        pieces.push_back(
            thicken(sheet, reverse ? -distance : distance, join, intersection, tag, "PipeSegmentWall")
        );
    }
    auto tangentAt = [](const Part::TopoShape& edge, bool end) {
        BRepAdaptor_Curve curve(TopoDS::Edge(edge.getShape()));
        const bool reverse = edge.getShape().Orientation() == TopAbs_REVERSED;
        gp_Pnt point;
        gp_Vec tangent;
        curve.D1(end != reverse ? curve.LastParameter() : curve.FirstParameter(), point, tangent);
        if (reverse) {
            tangent.Reverse();
        }
        return std::make_pair(point, tangent);
    };
    const size_t corners = wire.isClosed() ? edges.size() : edges.size() - 1;
    for (size_t i = 0; i < corners; ++i) {
        const auto& before = edges[i];
        const auto& after = edges[(i + 1) % edges.size()];
        auto [point, incoming] = tangentAt(before, true);
        auto [next, outgoing] = tangentAt(after, false);
        incoming -= axis * incoming.Dot(axis);
        outgoing -= axis * outgoing.Dot(axis);
        incoming.Normalize();
        outgoing.Normalize();
        const double turn = axis.Dot(incoming.Crossed(outgoing));
        if (std::abs(turn) <= Precision::Angular() || distance * turn >= 0) {
            continue;  // Tangency or a concave join: the segment union does the trimming.
        }
        const auto n1 = axis.Crossed(incoming);
        const auto n2 = axis.Crossed(outgoing);
        const auto a = point.Translated(n1 * distance);
        const auto b = point.Translated(n2 * distance);
        BRepBuilderAPI_MakeWire boundary;
        boundary.Add(BRepBuilderAPI_MakeEdge(point, a).Edge());
        if (join == Part::JoinType::arc) {
            const auto middle = point.Translated((n1 + n2).Normalized() * distance);
            boundary.Add(BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(a, middle, b).Value()).Edge());
        }
        else {
            const double denominator = 1 + n1.Dot(n2);
            if (denominator <= Precision::Angular()) {
                throw Base::ValueError("Sharp corner has an unbounded miter");
            }
            const auto miter = point.Translated((n1 + n2) * (distance / denominator));
            boundary.Add(BRepBuilderAPI_MakeEdge(a, miter).Edge());
            boundary.Add(BRepBuilderAPI_MakeEdge(miter, b).Edge());
        }
        boundary.Add(BRepBuilderAPI_MakeEdge(b, point).Edge());
        BRepBuilderAPI_MakeFace face(boundary.Wire());
        auto region = Part::TopoShape(tag, wire.Hasher)
                          .makeElementShape(face, {before, after}, "PipeCornerRegion");
        pieces.push_back(region.makeElementPrism(axis * length, "PipeCorner"));
    }

    auto wall = Part::TopoShape(tag, wire.Hasher).makeElementFuse(pieces, "PipeSegmentUnion");

    wall.fixSolidOrientation();
    validateWall(wall);

    return wall;
}
}  // namespace

}  // namespace PartDesign
