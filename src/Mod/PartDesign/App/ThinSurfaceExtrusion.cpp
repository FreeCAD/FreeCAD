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

Part::TopoShape makeThinSurfaceExtrusion(
    const std::vector<Part::TopoShape>& edges,
    const gp_Dir& direction,
    double length,
    double thickness,
    int side,
    bool reversed,
    Part::JoinType join,
    bool intersection,
    long tag
)
{
    const double tolerance = Precision::Confusion();
    if (!std::isfinite(length) || length <= tolerance || !std::isfinite(thickness)
        || thickness <= 2 * tolerance || side < 0 || side > 2) {
        throw Base::ValueError(
            "Pipe length and thickness must be positive and exceed modeling tolerance"
        );
    }

    if (edges.empty()) {
        throw Base::ValueError("Select one connected, nonbranching edge chain or closed loop");
    }

    Part::TopoShape wire(tag, edges.front().Hasher);
    auto orientedEdges = edges;
    for (auto& edge : orientedEdges) {
        edge.setShape(edge.getShape().Oriented(TopAbs_FORWARD), false);
    }

    wire.makeElementWires(orientedEdges, "PipeProfile", tolerance);
    if (wire.countSubShapes(TopAbs_WIRE) != 1 || wire.countSubShapes(TopAbs_EDGE) != edges.size()) {
        throw Base::ValueError(
            "Pipe requires one connected, nonbranching chain without duplicate edges"
        );
    }

    wire = wire.getSubTopoShape(TopAbs_WIRE, 1);
    if (!wire.isValid() || !BRepAlgoAPI_Check(wire.getShape(), false, true).IsValid()) {
        throw Base::ValueError("Pipe profile is invalid or self-intersecting");
    }

    // Anchor open profiles to the underlying curve of the first source edge,
    // not its incidental topological orientation in a face/wire. Callers order
    // edges by source topology, not by selection order or global coordinates.

    for (BRepTools_WireExplorer it(TopoDS::Wire(wire.getShape())); it.More(); it.Next()) {
        if (it.Current().IsSame(edges.front().getShape())) {
            if (it.Current().Orientation() != TopAbs_FORWARD) {
                Part::TopoShape oriented(tag, wire.Hasher, wire.getShape().Reversed());
                oriented.mapSubElement(wire);
                wire = oriented;
            }
            break;
        }
    }

    double sign = side == 1 ? -1.0 : 1.0;
    gp_Pln plane;
    const bool planarProfile = wire.findPlane(plane);
    const bool normalExtrusion = planarProfile
        && plane.Axis().Direction().IsParallel(direction, Precision::Angular());
    if (wire.isClosed() && planarProfile) {
        // Establish inside/outside independently of the input wire orientation.
        // The face orientation fixer changes orientations, not the profile curves.
        if (plane.Axis().Direction().Dot(direction) < 0) {
            plane.UReverse();
        }
        BRepBuilderAPI_MakeFace face(plane, TopoDS::Wire(wire.getShape()), true);
        ShapeFix_Face fixer(face.Face());
        fixer.FixOrientation();
        Part::TopoShape oriented(tag, wire.Hasher, BRepTools::OuterWire(fixer.Face()));
        oriented.mapSubElement(wire);
        wire = oriented;
        sign = -sign;
    }

    // Build in the unreversed direction. Reversal is a translation of the
    // complete wall, so it never reverses the surface normal or thickness side.
    const gp_Vec travel = gp_Vec(direction) * length;
    auto shell = wire.makeElementPrism(travel, "PipeExtrude");
    if (shell.countSubShapes(TopAbs_FACE) != edges.size() || !shell.isValid()) {
        throw Base::ValueError("Profile cannot be extruded along this direction (degenerate surface)");
    }

    gp_Pln support;
    if (shell.findPlane(support)) {
        // A planar side-profile sweep is a material region, not a spatial
        // offset problem. Merge its smooth boundary with OCCT history, then
        // extrude the entire width once (including centered placement).
        // OCCT concatenates spline boundaries, but not mixed line/spline
        // chains. Exact NURBS conversion also includes straight extensions.
        Part::TopoShape region;
        try {
            auto converted = shell;
            if (edges.size() > 1 && std::any_of(edges.begin(), edges.end(), [](const auto& edge) {
                    const auto type = BRepAdaptor_Curve(TopoDS::Edge(edge.getShape())).GetType();
                    return type == GeomAbs_BSplineCurve || type == GeomAbs_BezierCurve;
                })) {
                BRepBuilderAPI_NurbsConvert convert(wire.getShape(), true);
                converted = wire.makeElementShape(convert, "RibRegionCurves")
                                .makeElementPrism(travel, "RibRegionSweep");
            }

            ShapeUpgrade_UnifySameDomain unify(converted.getShape(), true, true, true);
            unify.Build();
            region = Part::TopoShape(tag, wire.Hasher)
                         .makeShapeWithElementMap(
                             unify.Shape(),
                             Part::MapperHistory(unify.History()),
                             {converted},
                             "RibRegion"
                         );
        }
        catch (const Standard_Failure&) {
            // Concatenation can reject a singular parameterization even when
            // the limiting tangent is valid. Keep its original boundary;
            // seam removal must not invalidate an otherwise usable profile.
            region = shell.makeElementRefine("RibRegion", Part::RefineFail::shapeUntouched);
        }

        if (region.countSubShapes(TopAbs_FACE) == 1) {
            auto face = region.getSubTopoShape(TopAbs_FACE, 1);
            if (!face.isValid() || !BRepAlgoAPI_Check(face.getShape(), false, true).IsValid()) {
                throw Base::CADKernelError("Rib profile sweep overlaps itself");
            }

            const auto sourceFace = TopoDS::Face(shell.getSubShape(TopAbs_FACE, 1));
            const BRepAdaptor_Surface surface(sourceFace);
            gp_Pnt point;
            gp_Vec du, dv;
            surface.D1(
                (surface.FirstUParameter() + surface.LastUParameter()) / 2,
                (surface.FirstVParameter() + surface.LastVParameter()) / 2,
                point,
                du,
                dv
            );

            gp_Vec width = du.Crossed(dv).Normalized()
                * (sourceFace.Orientation() == TopAbs_REVERSED ? -sign : sign) * thickness;

            auto wall = face.makeElementPrism(width, "RibWidth");

            gp_Trsf placement;
            placement.SetTranslation(
                (side == 2 ? -width / 2 : gp_Vec()) + (reversed ? -travel : gp_Vec())
            );
            wall.move(placement);
            wall.fixSolidOrientation();

            // A normal prism of a valid, non-self-intersecting planar region
            // cannot self-intersect. Check the solid topology without repeating
            // the expensive face/face interference analysis in three dimensions.
            validateWall(wall, false);
            return wall;
        }
    }

    auto buildSide = [&](double distance, const char* op) {
        try {
            return thicken(shell, distance, join, intersection, tag, op);
        }
        catch (const Base::CADKernelError&) {
            if (normalExtrusion) {
                throw;
            }
        }
        catch (const Standard_Failure&) {
            if (normalExtrusion) {
                throw;
            }
        }
        return segmentedWall(wire, direction, length, distance, join, intersection, tag);
    };

    auto wall = buildSide(sign * thickness / (side == 2 ? 2.0 : 1.0), "PipeSideA");
    if (side == 2) {
        auto other = buildSide(-sign * thickness / 2.0, "PipeSideB");
        wall = Part::TopoShape(tag, wire.Hasher).makeElementFuse({wall, other}, "PipeBoth");
        validateWall(wall);
    }

    if (reversed) {
        gp_Trsf translation;
        translation.SetTranslation(-travel);
        wall.move(translation);
    }

    return wall;
}
}  // namespace PartDesign
