// SPDX-License-Identifier: LGPL-2.1-or-later

#include "ThinProfile.h"

#include <cmath>
#include <BRepAlgoAPI_Check.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepOffsetAPI_MakeOffset.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRepLib.hxx>
#include <BRepLib_CheckCurveOnSurface.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Precision.hxx>
#include <ShapeFix_Face.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <TopTools_IndexedMapOfShape.hxx>

#include <Base/Exception.h>
#include <Base/BoundBox.h>

namespace PartDesign
{
namespace
{
using Part::TopoShape;

TopoShape offset(
    const TopoShape& wire,
    const gp_Pln& plane,
    double distance,
    Part::JoinType join,
    long tag,
    const char* op
)
{
    if (std::abs(distance) <= Precision::Confusion()) {
        return wire;
    }

    // Supplying the support plane is essential: a single straight edge does
    // not define one. Do not infer it from world axes or extrusion direction.
    BRepBuilderAPI_MakeFace support(plane, TopoDS::Wire(wire.getShape()), false);
    BRepOffsetAPI_MakeOffset builder(
        support.Face(),
        static_cast<GeomAbs_JoinType>(join),
        !wire.isClosed()
    );
    builder.Perform(distance);
    if (!builder.IsDone() || builder.Shape().IsNull()) {
        throw Base::CADKernelError("Cannot offset thin profile with the requested thickness and join");
    }

    return TopoShape(tag, wire.Hasher).makeElementShape(builder, {wire}, op);
}

TopoShape strip(
    TopoShape wire,
    const gp_Pln& plane,
    double sideA,
    double sideB,
    Part::JoinType join,
    long tag,
    const TopTools_IndexedMapOfShape& roundVertices
)
{
    bool closed = wire.isClosed();
    if (closed) {
        BRepBuilderAPI_MakeFace face(plane, TopoDS::Wire(wire.getShape()), true);
        ShapeFix_Face fix(face.Face());
        fix.FixOrientation();
        TopoShape oriented(tag, wire.Hasher, BRepTools::OuterWire(fix.Face()));
        oriented.mapSubElement(wire);
        wire = oriented;
    }

    // OCCT's positive offset is right of an open wire, outside a positively
    // oriented closed wire. Keep the public side convention independent of it.
    TopoShape a = offset(wire, plane, closed ? sideA : -sideA, join, tag, "ThinSideA");
    TopoShape b = offset(wire, plane, closed ? -sideB : sideB, join, tag, "ThinSideB");

    std::vector<TopoShape> boundaries {a, b};
    if (!closed) {
        if (a.countSubShapes(TopAbs_WIRE) != 1 || b.countSubShapes(TopAbs_WIRE) != 1) {
            throw Base::CADKernelError("Thin profile offset split at a corner or collapsed");
        }
        TopoDS_Vertex a0, a1, b0, b1;
        TopExp::Vertices(TopoDS::Wire(a.getSubShape(TopAbs_WIRE, 1)), a0, a1);
        TopExp::Vertices(TopoDS::Wire(b.getSubShape(TopAbs_WIRE, 1)), b0, b1);
        if (a0.IsNull() || a1.IsNull() || b0.IsNull() || b1.IsNull()) {
            throw Base::CADKernelError("Thin profile offset lost a free endpoint");
        }
        double direct = BRep_Tool::Pnt(a0).SquareDistance(BRep_Tool::Pnt(b0))
            + BRep_Tool::Pnt(a1).SquareDistance(BRep_Tool::Pnt(b1));
        double crossed = BRep_Tool::Pnt(a0).SquareDistance(BRep_Tool::Pnt(b1))
            + BRep_Tool::Pnt(a1).SquareDistance(BRep_Tool::Pnt(b0));
        if (crossed < direct) {
            std::swap(b0, b1);
        }
        for (const auto& pair : {std::make_pair(a0, b0), std::make_pair(a1, b1)}) {
            BRepBuilderAPI_MakeEdge cap(pair.first, pair.second);
            if (!roundVertices.IsEmpty()) {
                const gp_Pnt start = BRep_Tool::Pnt(pair.first);
                const gp_Pnt end = BRep_Tool::Pnt(pair.second);
                const gp_Pnt center((start.XYZ() + end.XYZ()) / 2);
                TopoDS_Vertex v0, v1;
                TopExp::Vertices(TopoDS::Wire(wire.getShape()), v0, v1);
                auto endpoint = center.SquareDistance(BRep_Tool::Pnt(v0))
                        < center.SquareDistance(BRep_Tool::Pnt(v1))
                    ? v0
                    : v1;
                if (roundVertices.Contains(endpoint)) {
                    gp_Vec outward;
                    for (BRepTools_WireExplorer it(TopoDS::Wire(wire.getShape())); it.More();
                         it.Next()) {
                        TopoDS_Vertex first, last;
                        TopExp::Vertices(it.Current(), first, last);
                        BRepAdaptor_Curve curve(it.Current());
                        gp_Pnt point;
                        if (endpoint.IsSame(first)) {
                            curve.D1(curve.FirstParameter(), point, outward);
                            outward.Reverse();
                            break;
                        }
                        if (endpoint.IsSame(last)) {
                            curve.D1(curve.LastParameter(), point, outward);
                            break;
                        }
                    }
                    outward.Normalize();
                    const gp_Pnt middle = center.Translated(outward * (sideA + sideB) / 2);
                    cap = BRepBuilderAPI_MakeEdge(GC_MakeArcOfCircle(start, middle, end).Value());
                }
            }
            boundaries.push_back(TopoShape(tag, wire.Hasher).makeElementShape(cap, {a, b}, "ThinCap"));
        }
        TopoShape boundary(tag, wire.Hasher);
        boundary.makeElementWires(boundaries, "ThinBoundary");
        if (boundary.countSubShapes(TopAbs_WIRE) != 1 || !boundary.isClosed()) {
            throw Base::CADKernelError("Cannot close thin profile endpoints");
        }
        boundaries = {boundary};
    }

    return TopoShape(tag, wire.Hasher).makeElementFace(boundaries, "ThinRegion", nullptr, &plane);
}
}  // namespace

ThinProfileGraph makeThinProfileGraph(const Part::TopoShape& profile, long tag)
{
    auto edges = profile.getSubTopoShapes(TopAbs_EDGE);
    if (edges.empty()) {
        throw Base::ValueError("Thin mode requires profile edges");
    }

    for (auto& edge : edges) {
        edge.setShape(edge.getShape().Oriented(TopAbs_FORWARD), false);
    }

    // Split geometric crossings and share coincident vertices using the kernel's
    // arrangement/history machinery. Wire assembly alone cannot detect an X.
    if (edges.size() > 1) {
        std::vector<std::vector<TopoShape>> modified;
        TopoShape split(tag, profile.Hasher);
        split.makeElementGeneralFuse(edges, modified, 0.0, "ThinCrossings");
        edges = split.getSubTopoShapes(TopAbs_EDGE);
    }

    TopTools_IndexedMapOfShape vertices;
    std::vector<std::pair<int, int>> ends;
    for (const auto& edge : edges) {
        TopoDS_Vertex a, b;
        TopExp::Vertices(TopoDS::Edge(edge.getShape()), a, b);
        ends.emplace_back(vertices.Add(a) - 1, vertices.Add(b) - 1);
    }

    std::vector<std::vector<size_t>> incidence(vertices.Extent());
    for (size_t i = 0; i < ends.size(); ++i) {
        incidence[ends[i].first].push_back(i);
        incidence[ends[i].second].push_back(i);
    }

    std::vector<bool> used(edges.size(), false);
    std::vector<TopoShape> chains;
    auto walk = [&](size_t seed, int vertex) {
        std::vector<TopoShape> chain;
        auto current = seed;
        while (!used[current]) {
            used[current] = true;
            chain.push_back(edges[current]);
            vertex = ends[current].first == vertex ? ends[current].second : ends[current].first;
            if (incidence[vertex].size() != 2) {
                break;
            }
            current = incidence[vertex][0] == current ? incidence[vertex][1] : incidence[vertex][0];
        }
        chains.push_back(TopoShape(tag, profile.Hasher).makeElementWires(chain, "ThinChain"));
    };

    for (size_t v = 0; v < incidence.size(); ++v) {
        if (incidence[v].size() != 2) {
            for (auto e : incidence[v]) {
                if (!used[e]) {
                    walk(e, v);
                }
            }
        }
    }
    for (size_t e = 0; e < edges.size(); ++e) {
        if (!used[e]) {
            walk(e, ends[e].first);
        }
    }
    ThinProfileGraph result;
    for (size_t v = 0; v < incidence.size(); ++v) {
        if (incidence[v].size() == 1) {
            result.freeVertices.Add(vertices(v + 1));
        }
    }

    for (auto wire : chains) {
        // Anchor a chain to source topology, not selection order or world axes.
        for (const auto& edge : edges) {
            bool found = false;
            for (BRepTools_WireExplorer it(TopoDS::Wire(wire.getShape())); it.More(); it.Next()) {
                if (it.Current().IsSame(edge.getShape())) {
                    if (it.Current().Orientation() != TopAbs_FORWARD) {
                        wire.setShape(wire.getShape().Reversed(), false);
                    }
                    found = true;
                    break;
                }
            }
            if (found) {
                break;
            }
        }
        result.chains.push_back(wire);
    }

    return result;
}

static Part::TopoShape makeThinProfileLocal(
    const Part::TopoShape& profile,
    const gp_Pln& plane,
    double sideA,
    double sideB,
    Part::JoinType join,
    long tag,
    bool roundEnds
)
{
    if (!std::isfinite(sideA) || !std::isfinite(sideB) || sideA < 0 || sideB < 0
        || sideA + sideB <= 2 * Precision::Confusion()) {
        throw Base::ValueError(
            "Thin profile thickness must be positive; side distances cannot be negative"
        );
    }

    const auto graph = makeThinProfileGraph(profile, tag);
    std::vector<TopoShape> regions;
    const TopTools_IndexedMapOfShape noRoundEnds;
    for (const auto& wire : graph.chains) {
        regions.push_back(
            strip(wire, plane, sideA, sideB, join, tag, roundEnds ? graph.freeVertices : noRoundEnds)
        );
    }

    TopoShape result(tag, profile.Hasher);
    if (regions.size() == 1) {
        result = regions.front();
    }
    else {
        result.makeElementFuse(regions, "ThinJunctions");
    }

    // Offset approximations can underestimate an edge's p-curve deviation.
    // SameParameter/UpdateInnerTolerances use sparse samples and can miss its
    // maximum. Use OCCT's extrema check, retaining topology/history and imposing
    // a budget based on input accuracy and wall width, not an arbitrary repair.
    double toleranceBudget = std::max(10 * Precision::Confusion(), (sideA + sideB) * 1e-6);
    for (const auto& edge : profile.getSubTopoShapes(TopAbs_EDGE)) {
        toleranceBudget
            = std::max(toleranceBudget, 10 * BRep_Tool::Tolerance(TopoDS::Edge(edge.getShape())));
    }

    BRep_Builder topology;
    for (const auto& faceShape : result.getSubTopoShapes(TopAbs_FACE)) {
        const auto face = TopoDS::Face(faceShape.getShape());
        for (const auto& edgeShape : faceShape.getSubTopoShapes(TopAbs_EDGE)) {
            const auto edge = TopoDS::Edge(edgeShape.getShape());
            if (BRep_Tool::Degenerated(edge)) {
                continue;
            }
            BRepLib_CheckCurveOnSurface check(edge, face);
            check.Perform();
            if (!check.IsDone()) {
                throw Base::CADKernelError("Cannot verify thin-profile curve accuracy");
            }
            if (check.MaxDistance() > BRep_Tool::Tolerance(edge)) {
                const double tolerance = check.MaxDistance() + Precision::Confusion();
                if (tolerance > toleranceBudget) {
                    throw Base::CADKernelError(
                        "Thin profile exceeds the input modeling tolerance budget"
                    );
                }
                topology.UpdateEdge(edge, tolerance);
            }
        }
    }
    BRepLib::UpdateTolerances(result.getShape());

    if (result.isNull() || !result.hasSubShape(TopAbs_FACE) || !result.isValid()
        || !BRepAlgoAPI_Check(result.getShape(), false, true).IsValid()) {
        throw Base::CADKernelError("Thin profile is invalid or self-intersecting");
    }

    return result;
}

Part::TopoShape makeThinProfile(
    const Part::TopoShape& profile,
    const gp_Pln& plane,
    double sideA,
    double sideB,
    Part::JoinType join,
    long tag,
    bool roundEnds
)
{
    // Work in the support frame. Besides reducing coordinate magnitudes, this
    // keeps offset p-curves and the face maker's plane in exactly the same frame.
    gp_Trsf toPlane;
    toPlane.SetTransformation(plane.Position());
    auto local = TopoShape(tag, profile.Hasher)
                     .makeElementTransform(profile, toPlane, "ThinSupportFrame", Part::CopyType::copy);
    BRepTools::RemoveUnusedPCurves(local.getShape());

    // Native MakeOffset ignores an edge location for a single-edge wire (see
    // BRepOffsetAPI_MakeOffsetFix). Bake locations on copies so the supplied
    // support plane remains authoritative, including for a lone straight line.
    std::vector<TopoShape> baked;
    for (auto edge : local.getSubTopoShapes(TopAbs_EDGE)) {
        auto location = edge.getShape().Location();
        edge.setShape(edge.getShape().Located(TopLoc_Location()), false);
        baked.push_back(TopoShape(tag, profile.Hasher)
                            .makeElementTransform(
                                edge,
                                location.Transformation(),
                                "ThinBakeLocation",
                                Part::CopyType::copy
                            ));
    }
    local.makeElementCompound(
        baked,
        "ThinLocatedEdges",
        TopoShape::SingleShapeCompoundCreationPolicy::returnShape
    );
    const auto bounds = local.getBoundBoxOptimal();
    if (std::abs(bounds.MinZ) > 10 * Precision::Confusion()
        || std::abs(bounds.MaxZ) > 10 * Precision::Confusion()) {
        throw Base::ValueError("Thin Pad/Pocket requires coplanar profile edges; use Spatial Web for nonplanar geometry");
    }
    auto result = makeThinProfileLocal(local, gp_Pln(gp::XOY()), sideA, sideB, join, tag, roundEnds);
    result.move(toPlane.Inverted());
    return result;
}
}  // namespace PartDesign
