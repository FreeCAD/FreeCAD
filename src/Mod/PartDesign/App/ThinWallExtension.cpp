// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ThinWallExtension.h"
#include "ThinExtrusion.h"
#include "ThinExtrusionGeometry.h"

#include <algorithm>
#include <cmath>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <Base/BoundBox.h>
#include <Base/Exception.h>

namespace PartDesign
{
Part::TopoShape makeBodyBoundedThinWall(
    const Part::TopoShape& profile,
    const gp_Pln& plane,
    const Part::TopoShape& body,
    const gp_Vec& travel,
    double sideA,
    double sideB,
    Part::JoinType join,
    double draftAngle,
    bool holdTop,
    long tag,
    const std::vector<Part::TopoShape>& extensionEdges,
    bool roundEnds,
    const Part::TopoShape* terminationFace,
    bool curvatureContinuous,
    bool subtractive
)
{
    using Part::TopoShape;

    if (body.isNull() || !body.hasSubShape(TopAbs_SOLID)) {
        throw Base::ValueError("Full-height end extension requires an existing solid body");
    }
    if (travel.Magnitude() <= Precision::Confusion()) {
        throw Base::ValueError("Full-height end extension requires a positive extrusion length");
    }

    const auto graph = makeThinProfileGraph(profile, tag);
    if (graph.freeVertices.IsEmpty()) {
        throw Base::ValueError("Full-height end extension requires a profile with free endpoints");
    }

    // Graph construction may split/copy source edges. Match endpoints by position
    // after checking that each selected edge is actually contained in the profile.
    TopTools_IndexedMapOfShape selectedEnds;
    for (const auto& edge : extensionEdges) {
        GProp_GProps original;
        BRepGProp::LinearProperties(edge.getShape(), original);
        double overlapLength = 0;
        // The raw profile may contain crossing edges: such a compound is not a
        // valid Boolean argument. The graph has already split those crossings.
        for (const auto& chain : graph.chains) {
            for (const auto& sourceEdge : chain.getSubTopoShapes(TopAbs_EDGE)) {
                BRepAlgoAPI_Common common(edge.getShape(), sourceEdge.getShape());
                if (!common.IsDone()) {
                    throw Base::CADKernelError("Cannot validate an extension edge");
                }
                GProp_GProps overlap;
                BRepGProp::LinearProperties(common.Shape(), overlap);
                overlapLength += overlap.Mass();
            }
        }
        if (original.Mass() <= Precision::Confusion()
            || std::abs(original.Mass() - overlapLength) > Precision::Confusion()) {
            throw Base::ValueError("Extension edges must be contained in the thin profile");
        }
        bool found = false;
        for (const auto& vertex : edge.getSubTopoShapes(TopAbs_VERTEX)) {
            const auto v = TopoDS::Vertex(vertex.getShape());
            for (int i = 1; i <= graph.freeVertices.Extent(); ++i) {
                const auto end = TopoDS::Vertex(graph.freeVertices(i));
                const double tolerance = std::max(
                    Precision::Confusion(),
                    BRep_Tool::Tolerance(v) + BRep_Tool::Tolerance(end)
                );
                if (BRep_Tool::Pnt(v).Distance(BRep_Tool::Pnt(end)) <= tolerance) {
                    selectedEnds.Add(end);
                    found = true;
                }
            }
        }
        if (!found) {
            throw Base::ValueError("A selected extension edge has no free endpoints");
        }
    }

    auto bounds = body.getBoundBoxOptimal();
    bounds.Add(profile.getBoundBoxOptimal());
    const double reach = 2 * (bounds.CalcDiagonalLength() + travel.Magnitude() + sideA + sideB);

    std::vector<TopoShape> edges;
    std::vector<TopoShape> guards;

    for (const auto& chain : graph.chains) {
        for (const auto& edge : chain.getSubTopoShapes(TopAbs_EDGE)) {
            edges.push_back(edge);
            TopoDS_Vertex first, last;
            TopExp::Vertices(TopoDS::Edge(edge.getShape()), first, last);
            for (const auto& endpoint : {first, last}) {
                if (!graph.freeVertices.Contains(endpoint)) {
                    continue;
                }
                if (!extensionEdges.empty() && !selectedEnds.Contains(endpoint)) {
                    continue;
                }

                const bool atStart = endpoint.IsSame(first);
                const auto jet
                    = thinEndpointJet(TopoDS::Edge(edge.getShape()), !atStart, curvatureContinuous);
                gp_Pnt point = jet.point;
                const gp_Vec tangent = jet.tangent;

                // Straight endpoints are already C2; avoid artificial spline seams.
                if (curvatureContinuous && jet.acceleration.Magnitude() > gp::Resolution()) {
                    auto continuation = makeThinC2Transition(jet, reach);
                    point = continuation->Value(1);
                    if (atStart) {
                        continuation = continuation->Reversed();
                    }
                    BRepBuilderAPI_MakeEdge transition(continuation);
                    edges.push_back(TopoShape(tag, profile.Hasher)
                                        .makeElementShape(transition, {edge}, "ThinC2Transition"));
                }

                const gp_Pnt far = point.Translated(tangent * reach);
                // Preserve the source chain's orientation for asymmetric widths.
                BRepBuilderAPI_MakeEdge extension(atStart ? far : point, atStart ? point : far);
                edges.push_back(TopoShape(tag, profile.Hasher)
                                    .makeElementShape(extension, {edge}, "ThinEndExtension"));

                const gp_Vec across = gp_Vec(plane.Axis().Direction()).Crossed(tangent);
                const gp_Vec normal = across.Crossed(travel);
                if (normal.Magnitude() <= Precision::Confusion()) {
                    throw Base::ValueError("Extrusion direction cannot define an end boundary");
                }
                BRepBuilderAPI_MakeFace
                    guard(gp_Pln(far, gp_Dir(normal)), -4 * reach, 4 * reach, -4 * reach, 4 * reach);
                guards.emplace_back(tag, profile.Hasher, guard.Face());
            }
        }
    }

    auto extended = TopoShape(tag, profile.Hasher).makeElementCompound(edges, "ThinExtendedEdges");
    auto region = makeThinProfile(extended, plane, sideA, sideB, join, tag, roundEnds);
    auto tool = terminationFace
        ? makeThinExtrusionUntil(region, *terminationFace, gp_Dir(travel), true, tag, &profile)
        : region.makeElementPrism(travel, "ThinExtendedPrism");

    // Draft before clipping so the actual bowl surface remains the boundary
    // after sidewall thickness changes, including on curved attachments.
    tool = draftThinExtrusion(tool, extended, body, gp_Dir(travel), draftAngle, holdTop, tag, &profile);

    TopoShape remainder(tag, profile.Hasher);
    if (subtractive) {
        BRepAlgoAPI_Common common(tool.getShape(), body.getShape());
        if (!common.IsDone()) {
            throw Base::CADKernelError("Cannot intersect the extended cutting tool with the body");
        }
        remainder.makeElementShape(common, {tool, body}, "ThinWallBoundary");
    }
    else {
        remainder = tool.makeElementCut(body, "ThinWallBoundary");
    }

    auto seed = profile.makeElementPrism(travel, "ThinWallSeed");

    std::vector<TopoShape> selected;
    for (const auto& solid : remainder.getSubTopoShapes(TopAbs_SOLID)) {
        BRepAlgoAPI_Common common(solid.getShape(), seed.getShape());
        if (!common.IsDone()) {
            throw Base::CADKernelError("Cannot classify the body-bounded wall");
        }
        GProp_GProps contact;
        BRepGProp::SurfaceProperties(common.Shape(), contact);
        if (contact.Mass() <= Precision::SquareConfusion()) {
            continue;
        }

        for (const auto& guard : guards) {
            BRepExtrema_DistShapeShape distance(solid.getShape(), guard.getShape());
            if (!distance.IsDone()) {
                throw Base::CADKernelError("Cannot verify the extended wall boundary");
            }
            if (distance.Value() <= Precision::Confusion()) {
                throw Base::ValueError(
                    "An extended end is not bounded by the body over the full height; "
                    "reduce the length or choose an enclosing body"
                );
            }
        }

        selected.push_back(solid);
    }

    if (selected.empty()) {
        throw Base::ValueError("No wall material remains between the profile and body");
    }

    auto result = TopoShape(tag, profile.Hasher)
                      .makeElementCompound(
                          selected,
                          "ThinBodyBoundedWall",
                          TopoShape::SingleShapeCompoundCreationPolicy::returnShape
                      );
    if (!result.isValid()) {
        throw Base::CADKernelError("Full-height end extension produced invalid geometry");
    }

    return result;
}
}  // namespace PartDesign
