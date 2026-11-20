// SPDX-License-Identifier: LGPL-2.1-or-later
#include "FeatureRib.h"

#include <Base/Type.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <BRepAdaptor_Curve.hxx>

#include <BRepClass3d_SolidClassifier.hxx>

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakeBox.hxx>

#include <BRepTools_History.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <GeomLib.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopoDS.hxx>
#include <gp.hxx>

#include <algorithm>
#include <cmath>

#include <stdexcept>
#include <string>
#include <vector>

namespace PartDesign
{
PROPERTY_SOURCE(PartDesign::Rib, PartDesign::ProfileBased)

namespace
{
const char* extendTypes[] = {"Off", "C1", "C2", nullptr};
const char* placementTypes[] = {"Side A", "Side B", "Centered", nullptr};
const char* extentTypes[] = {"Shape", "Distance", nullptr};
const char* draftReferences[] = {"Free end", "Root", nullptr};

double area(const TopoDS_Shape& shape)
{
    GProp_GProps properties;
    BRepGProp::SurfaceProperties(shape, properties);
    return properties.Mass();
}

void requireSolid(const Part::TopoShape& shape, const char* message)
{
    if (shape.isNull() || !shape.hasSubShape(TopAbs_SOLID) || !shape.isValid()) {
        throw std::runtime_error(message);
    }
}
}  // namespace

Rib::Rib()
{
    defineAdditive();

    ADD_PROPERTY_TYPE(
        ExtendType,
        (0L),
        "Rib",
        App::Prop_None,
        "Continuity used to extend the rib profile's free edges"
    );
    ExtendType.setEnums(extendTypes);
    ExtendType.setValue("C1");

    ADD_PROPERTY_TYPE(Thickness, (1.0), "Rib", App::Prop_None, "Thickness of the rib");

    ADD_PROPERTY_TYPE(
        PlacementType,
        (0L),
        "Rib",
        App::Prop_None,
        "Apply thickness starting from the centerline or side"
    );
    PlacementType.setEnums(placementTypes);
    PlacementType.setValue("Centered");

    ADD_PROPERTY_TYPE(
        ExtentType,
        (0L),
        "Rib",
        App::Prop_None,
        "Extend the rib to the body or a specified distance"
    );
    ExtentType.setEnums(extentTypes);
    ExtentType.setValue("Shape");

    ADD_PROPERTY_TYPE(
        Distance,
        (1.0),
        "Rib",
        App::Prop_None,
        "Distance to sweep towards when distance is specified"
    );

    ADD_PROPERTY_TYPE(
        Direction,
        (Base::Vector3d(0, 0, -1)),
        "Rib",
        App::Prop_None,
        "Sweep direction in Rib-local coordinates, for both Shape and Distance extents"
    );

    ADD_PROPERTY_TYPE(DraftAngle, (0.0), "Rib", App::Prop_None, "Draft angle for the rib.");

    ADD_PROPERTY_TYPE(
        UseCustomPullDirection,
        (false),
        "Rib",
        App::Prop_None,
        "Use a custom in-plane draft axis instead of the direction opposite the sweep"
    );

    ADD_PROPERTY_TYPE(
        PullDirection,
        (Base::Vector3d(0, 0, 1)),
        "Rib",
        App::Prop_None,
        "Custom draft axis and neutral-plane normal in Rib-local coordinates"
    );

    ADD_PROPERTY_TYPE(
        DraftReference,
        (0L),
        "Rib",
        App::Prop_None,
        "Plane at which the entered thickness is preserved"
    );
    DraftReference.setEnums(draftReferences);
    DraftReference.setValue("Free end");

    ADD_PROPERTY_TYPE(
        FilletRadius,
        (0.0),  // default to no fillet
        "Rib",
        App::Prop_None,
        "Fillet radius for the rib-body intersection"
    );
}

short Rib::mustExecute() const
{
    if (ExtendType.isTouched() || Thickness.isTouched() || DraftAngle.isTouched()
        || Direction.isTouched() || ExtentType.isTouched() || Distance.isTouched()
        || UseCustomPullDirection.isTouched() || PullDirection.isTouched() || FilletRadius.isTouched()
        || PlacementType.isTouched() || DraftReference.isTouched() || Reversed.isTouched()) {
        return 1;
    }

    return ProfileBased::mustExecute();
}

Part::TopoShape Rib::getRibProfileWire() const
{
    if (!Profile.getValue()) {
        throw std::runtime_error("No profile selected");
    }
    if (!Profile.getValue()->isDerivedFrom(Base::Type::fromName("Sketcher::SketchObject"))) {
        throw std::runtime_error("Rib profile must be a sketch");
    }

    // one connected wire only for rib
    auto wires = getTopoShapeProfileWires();
    if (wires.size() != 1) {
        throw std::runtime_error("The rib profile must contain exactly one connected wire");
    }

    // check for connected wire with two free endpoints
    const TopoDS_Wire wire = TopoDS::Wire(wires.front().getShape());
    if (!BRepCheck_Analyzer(wire).IsValid()) {
        throw std::runtime_error("The rib profile is not a valid wire");
    }

    // get the first and last vertices of the wire
    TopoDS_Vertex firstVertex;
    TopoDS_Vertex lastVertex;
    TopExp::Vertices(wire, firstVertex, lastVertex);

    // need exactly two free end points, no circular/closed wire
    if (firstVertex.IsNull() || lastVertex.IsNull()) {
        throw std::runtime_error("The rib profile must have two free endpoints");
    }
    if (firstVertex.IsSame(lastVertex)) {
        throw std::runtime_error("The rib profile must be open, not closed");
    }

    // return a ref first item (wire)
    return wires.front();
}

// helpers
namespace
{
struct RibProfileEnds
{
    TopoDS_Vertex startVertex;
    TopoDS_Vertex endVertex;
    TopoDS_Edge startEdge;
    TopoDS_Edge endEdge;
};

RibProfileEnds findRibProfileEnds(const TopoDS_Wire& wire)
{
    RibProfileEnds ends;

    TopExp::Vertices(wire, ends.startVertex, ends.endVertex);
    if (ends.startVertex.IsNull() || ends.endVertex.IsNull()
        || ends.startVertex.IsSame(ends.endVertex) || !BRepCheck_Analyzer(wire).IsValid()) {
        throw std::runtime_error("Rib extension requires a valid open wire");
    }

    // a free endpoint belongs to one edge; interior vertices must join two edges
    // closed hsas same start/end
    TopTools_IndexedDataMapOfShapeListOfShape vertexEdges;
    TopExp::MapShapesAndAncestors(wire, TopAbs_VERTEX, TopAbs_EDGE, vertexEdges);
    // int endpoints = 0;
    // for (int i = 1; i <= vertexEdges.Extent(); ++i) {
    //     const int degree = vertexEdges.FindFromIndex(i).Extent();
    //     if (degree == 1) {
    //         ++endpoints;
    //     }
    //     else if (degree != 2) {
    //         throw std::runtime_error("Rib profile must not branch");
    //     }
    // }
    // if (endpoints != 2) {
    //     throw std::runtime_error("Rib profile must have exactly two free endpoints");
    // }

    // // Two free endpoints alone do not rule out a disconnected closed loop.
    // // The connected walk must visit every edge exactly once.
    // TopTools_IndexedMapOfShape allEdges;
    // TopExp::MapShapes(wire, TopAbs_EDGE, allEdges);
    // TopTools_MapOfShape visited;
    // for (BRepTools_WireExplorer walk(wire); walk.More(); walk.Next()) {
    //     if (!visited.Add(walk.Current())) {
    //         throw std::runtime_error("Rib profile must be a single traversable chain");
    //     }
    // }
    // if (visited.Extent() != allEdges.Extent()) {
    //     throw std::runtime_error("Rib profile must be a single traversable chain");
    // }

    // A vertex supplies the position; its attached edge supplies the curve.
    ends.startEdge = TopoDS::Edge(vertexEdges.FindFromKey(ends.startVertex).First());
    ends.endEdge = TopoDS::Edge(vertexEdges.FindFromKey(ends.endVertex).First());
    return ends;
}


void extendRibCurveEnd(
    Handle(Geom_BoundedCurve) & curve,
    const Handle(Geom_TrimmedCurve) & original,
    double reach,
    int continuity,
    bool after,
    bool isLine
)
{
    // test original b/c previous extension may alter copy
    const double parameter = after ? original->LastParameter() : original->FirstParameter();
    gp_Pnt endpoint;
    gp_Vec tangent;
    original->D1(parameter, endpoint, tangent);
    if (!std::isfinite(tangent.Magnitude()) || tangent.Magnitude() <= gp::Resolution()) {
        throw std::runtime_error("Rib profile endpoint has no regular tangent");
    }
    tangent.Normalize();
    if (!after) {
        tangent.Reverse();  // at the curve start, outward is opposite increasing parameters.
    }

    if (isLine) {
        // line is already c1,2
        const double first = curve->FirstParameter() - (after ? 0.0 : reach);
        const double last = curve->LastParameter() + (after ? reach : 0.0);
        curve = new Geom_TrimmedCurve(original->BasisCurve(), first, last);
    }
    else {
        const gp_Pnt target = endpoint.Translated(tangent * reach);
        GeomLib::ExtendCurveToPoint(curve, target, continuity, after);
    }
}


void requireRibExtensionContact(
    const TopoDS_Shape& body,
    const Handle(Geom_BoundedCurve) & curve,
    const gp_Pnt& originalEndpoint,
    bool after,
    const char* endName
)
{
    // must have an intersection with the body from both extensions (or we can't build a rib)
    // locate endpoints (in case of change)
    GeomAPI_ProjectPointOnCurve projection(originalEndpoint, curve);
    if (projection.NbPoints() == 0 || projection.LowerDistance() > Precision::Confusion()) {
        throw std::runtime_error("Cannot locate the rib extension's original endpoint");
    }
    const double join = projection.LowerDistanceParameter();
    const double first = after ? join : curve->FirstParameter();
    const double last = after ? curve->LastParameter() : join;
    BRepBuilderAPI_MakeEdge extension(curve, first, last);
    if (!extension.IsDone()) {
        throw std::runtime_error("Failed to build rib extension for the contact check");
    }

    // test the extension for contact with the body
    BRepExtrema_DistShapeShape contact(body, extension.Edge());
    if (!contact.IsDone()) {
        throw std::runtime_error("Failed to check rib extension contact with the body");
    }
    if (contact.Value() > Precision::Confusion()) {
        throw std::runtime_error(
            std::string("Rib profile ") + endName + " extension does not intersect the body within reach"
        );
    }
}


TopoDS_Edge extendRibTerminalEdge(
    const TopoDS_Edge& edge,
    const RibProfileEnds& ends,
    const TopoDS_Shape& body,
    double reach,
    int continuity,
    const Handle(BRepTools_History) & history
)
{
    // restrict the underlying geometry to the portion used by this edge.
    double first = 0.0;
    double last = 0.0;
    Handle(Geom_Curve) geometry = BRep_Tool::Curve(edge, first, last);
    if (geometry.IsNull()) {
        throw std::runtime_error("Rib terminal edge has no 3D curve");
    }
    Handle(Geom_TrimmedCurve) original = new Geom_TrimmedCurve(geometry, first, last);
    Handle(Geom_BoundedCurve) curve = Handle(Geom_BoundedCurve)::DownCast(original->Copy());

    // don't assume order
    TopoDS_Vertex firstVertex;
    TopoDS_Vertex lastVertex;
    TopExp::Vertices(edge, firstVertex, lastVertex, false);
    const bool extendFirst = firstVertex.IsSame(ends.startVertex)
        || firstVertex.IsSame(ends.endVertex);
    const bool extendLast = lastVertex.IsSame(ends.startVertex) || lastVertex.IsSame(ends.endVertex);
    const bool isLine = BRepAdaptor_Curve(edge).GetType() == GeomAbs_Line;

    // Use the full reach to form an oversized tool for the later body cut.
    // A single-edge profile extends both ends of this same curve copy.
    if (extendFirst) {
        extendRibCurveEnd(curve, original, reach, continuity, false, isLine);
    }
    if (extendLast) {
        extendRibCurveEnd(curve, original, reach, continuity, true, isLine);
    }

    // Check the final geometry before replacing vertices. Keep the full reach;
    // the later sweep needs overshoot, not an edge trimmed to first contact.
    TopoDS_Vertex newFirst = firstVertex;
    TopoDS_Vertex newLast = lastVertex;
    if (extendFirst) {
        const char* endName = firstVertex.IsSame(ends.startVertex) ? "start" : "end";
        requireRibExtensionContact(body, curve, original->StartPoint(), false, endName);
        newFirst = BRepBuilderAPI_MakeVertex(curve->StartPoint()).Vertex();
        history->AddModified(firstVertex, newFirst);
    }
    if (extendLast) {
        const char* endName = lastVertex.IsSame(ends.startVertex) ? "start" : "end";
        requireRibExtensionContact(body, curve, original->EndPoint(), true, endName);
        newLast = BRepBuilderAPI_MakeVertex(curve->EndPoint()).Vertex();
        history->AddModified(lastVertex, newLast);
    }

    // reuse unextended vertices so neighboring edges remain topologically joined.
    BRepBuilderAPI_MakeEdge
        maker(curve, newFirst, newLast, curve->FirstParameter(), curve->LastParameter());
    if (!maker.IsDone()) {
        throw std::runtime_error("Failed to build extended rib edge");
    }
    TopoDS_Edge replacement = maker.Edge();
    replacement.Orientation(edge.Orientation());
    history->AddModified(edge, replacement);
    return replacement;
}
}  // namespace


Part::TopoShape Rib::extendRibProfile(
    const Part::TopoShape& body,
    const Part::TopoShape& profile,
    double reach,
    long continuity
) const
{
    // extend each of the open profile edges with specified continuity
    if (continuity == 0) {
        return profile;  // Off: preserve the shape and its FreeCAD element map.
    }
    if (body.isNull()) {
        throw std::runtime_error("Rib extension requires a body");
    }
    if (!std::isfinite(reach) || reach <= Precision::Confusion()) {
        throw std::runtime_error("Rib extension reach must be positive and finite");
    }
    if (continuity != 1 && continuity != 2) {
        throw std::runtime_error("Rib extension continuity must be C1 or C2");
    }
    if (profile.isNull() || profile.getShape().ShapeType() != TopAbs_WIRE) {
        throw std::runtime_error("Rib extension requires a wire");
    }

    // check that we have one open chain and find the free edges
    const TopoDS_Wire wire = TopoDS::Wire(profile.getShape());
    const RibProfileEnds ends = findRibProfileEnds(wire);
    const int curveContinuity = static_cast<int>(continuity);

    // extend the terminal edges and check each new portion against the body
    Handle(BRepTools_History) history = new BRepTools_History;
    const TopoDS_Edge extendedStart
        = extendRibTerminalEdge(ends.startEdge, ends, body.getShape(), reach, curveContinuity, history);

    TopoDS_Edge extendedEnd = extendedStart;
    if (!ends.startEdge.IsSame(ends.endEdge)) {
        extendedEnd
            = extendRibTerminalEdge(ends.endEdge, ends, body.getShape(), reach, curveContinuity, history);
    }

    // rebuild in connected order - interior edges are reused unchanged
    BRep_Builder builder;
    TopoDS_Wire extendedWire;
    builder.MakeWire(extendedWire);
    for (BRepTools_WireExplorer walk(wire); walk.More(); walk.Next()) {
        const TopoDS_Edge& edge = walk.Current();
        if (edge.IsSame(ends.startEdge)) {
            builder.Add(extendedWire, extendedStart);
        }
        else if (edge.IsSame(ends.endEdge)) {
            builder.Add(extendedWire, extendedEnd);
        }
        else {
            builder.Add(extendedWire, edge);
        }
    }
    if (!BRepCheck_Analyzer(extendedWire).IsValid()) {
        throw std::runtime_error("Extended rib profile is not a valid wire");
    }

    // fc naming history
    Part::TopoShape result(0, profile.Hasher);
    result.makeShapeWithElementMap(
        extendedWire,
        Part::MapperHistory(history),
        {profile},
        "RibExtend"  // Continuity changes geometry, not the identity of this operation.
    );
    return result;
}


gp_Vec Rib::makeSweepVector(const gp_Pln& plane, double reach) const
{
    // Direction sets orientation only; its magnitude must not change the sweep length.
    const auto value = Direction.getValue();
    gp_Vec direction(value.x, value.y, value.z);
    const double magnitude = direction.Magnitude();
    if (!std::isfinite(magnitude) || magnitude <= gp::Resolution()) {
        throw std::runtime_error("Rib direction must be nonzero and finite");
    }
    direction.Normalize();
    if (std::abs(direction.Dot(gp_Vec(plane.Axis().Direction()))) > Precision::Angular()) {
        throw std::runtime_error("Rib direction must lie in the sketch plane");
    }

    const double length = ExtentType.isValue("Shape") ? reach : Distance.getValue();
    if (!std::isfinite(length) || length <= Precision::Confusion()) {
        throw std::runtime_error("Rib distance must be positive and finite");
    }
    return direction * (Reversed.getValue() ? -length : length);
}


Part::TopoShape Rib::makeRibSurface(
    const Part::TopoShape& profile,
    const gp_Vec& travel,
    const gp_Pln& plane
) const
{
    // Close the profile against its translated copy. Building a planar face
    // explicitly keeps spline roofs exact without introducing one extrusion
    // surface per curve segment. Distance mode retains the translated profile,
    // rather than replacing it with an arbitrary flat bottom.
    gp_Trsf translation;
    translation.SetTranslation(travel);
    Part::TopoShape far(0, profile.Hasher);
    far.makeElementTransform(profile, translation, "RibFarProfile");
    const auto ends = findRibProfileEnds(TopoDS::Wire(profile.getShape()));
    const auto farEnds = findRibProfileEnds(TopoDS::Wire(far.getShape()));
    // Reuse the wire vertices: coincident new vertices make MakeWire replace
    // edges while joining, which can break multi-edge wire connectivity/history.
    const auto endConnector = BRepBuilderAPI_MakeEdge(ends.endVertex, farEnds.endVertex).Edge();
    const auto startConnector = BRepBuilderAPI_MakeEdge(farEnds.startVertex, ends.startVertex).Edge();

    BRepBuilderAPI_MakeWire outline;
    // MakeWire::Add(wire) uses storage order, not connected traversal order.
    // In particular, reversing a multi-edge wire reverses its edge orientations
    // without reversing storage order. Walk it from the connected endpoint so
    // MakeWire cannot reject a disconnected first edge and silently omit a tail.
    const auto appendWire = [&outline](const TopoDS_Wire& wire) {
        for (BRepTools_WireExplorer walk(wire); walk.More(); walk.Next()) {
            outline.Add(walk.Current());
            if (!outline.IsDone()) {
                throw std::runtime_error("Cannot connect the rib profile boundary");
            }
        }
    };
    appendWire(TopoDS::Wire(profile.getShape()));
    outline.Add(endConnector);
    appendWire(TopoDS::Wire(far.getShape().Reversed()));
    outline.Add(startConnector);
    if (!outline.IsDone()) {
        throw std::runtime_error("Cannot close the rib profile in the fill direction");
    }
    BRepBuilderAPI_MakeFace face(plane, outline.Wire(), true);
    if (!face.IsDone() || !BRepCheck_Analyzer(face.Face()).IsValid()
        || area(face.Face()) <= Precision::SquareConfusion()) {
        throw std::runtime_error("Rib sweep overlaps itself or has no planar area");
    }

    // Keep the original edges in the face and record its generation for naming.
    Handle(BRepTools_History) history = new BRepTools_History;
    history->AddGenerated(ends.startVertex, startConnector);
    history->AddGenerated(ends.endVertex, endConnector);
    for (TopExp_Explorer it(profile.getShape(), TopAbs_EDGE); it.More(); it.Next()) {
        history->AddGenerated(it.Current(), face.Face());
    }
    Part::TopoShape surface(0, profile.Hasher);
    surface.makeShapeWithElementMap(
        face.Face(),
        Part::MapperHistory(history),
        {profile, far},
        "RibProfileSurface"
    );
    return surface;
}


Part::TopoShape Rib::makeRibTool(
    const Part::TopoShape& surface,
    const gp_Dir& normal,
    double thickness,
    long placement
) const
{
    // creat the rib tool
    auto tool = surface.makeElementPrism(gp_Vec(normal) * thickness, "RibThickness");

    // move to set the rib on either side or centered on the profile
    const double offset = placement == 0 ? 0.0 : (placement == 1 ? -thickness : -thickness / 2);

    gp_Trsf move;
    move.SetTranslation(gp_Vec(normal) * offset);
    tool = tool.makeElementTransform(move, "RibThicknessPlacement");

    requireSolid(tool, "Rib thickness did not produce a valid solid");
    return tool;
}


Part::TopoShape Rib::cutRibTool(const Part::TopoShape& tool, const Part::TopoShape& base) const
{
    Part::TopoShape cut(0, tool.Hasher);
    cut.makeElementBoolean(Part::OpCodes::Cut, {tool, base}, "RibBodyCut");
    return cut;
}


Part::TopoShape Rib::selectRibMaterial(
    const Part::TopoShape& cutResult,
    const Part::TopoShape& profile,
    const Part::TopoShape& farLimit
) const
{
    std::vector<Part::TopoShape> kept;
    for (const auto& solid : cutResult.getSubTopoShapes(TopAbs_SOLID)) {
        // test for contact between original profile and solid(s) after the rib tool cut
        BRepExtrema_DistShapeShape distance(profile.getShape(), solid.getShape());
        if (!distance.IsDone()) {
            throw std::runtime_error("Cannot measure rib-profile contact");
        }
        if (distance.Value() > Precision::Confusion()) {
            continue;
        }
        // if a solid also touches the far limit, reject it (for up to shape)
        if (!farLimit.isNull()) {
            BRepExtrema_DistShapeShape contact(solid.getShape(), farLimit.getShape());
            if (!contact.IsDone()) {
                throw std::runtime_error("Cannot check rib termination");
            }
            if (contact.Value() <= Precision::Confusion()) {
                throw std::runtime_error("Rib did not terminate at the body");
            }
        }
        kept.push_back(solid);
    }
    if (kept.empty()) {
        throw std::runtime_error("No rib material remains connected to the profile");
    }
    Part::TopoShape result(0, cutResult.Hasher);
    // Selection changes membership, not the identities of the retained elements.
    result.makeElementCompound(kept, "", Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape);
    return result;
}


Part::TopoShape Rib::makeDraftedRibTool(
    const Part::TopoShape& surface,
    const Part::TopoShape& retained,
    const Part::TopoShape& body,
    const gp_Pln& plane,
    const gp_Vec& travel
) const
{
    // Frame: Y is thickness, Z is pull (root towards free end), X completes the
    // profile plane. A custom pull may rotate within that plane, not out of it.
    gp_Vec pullVector = -travel;
    if (UseCustomPullDirection.getValue()) {
        const auto value = PullDirection.getValue();
        pullVector = gp_Vec(value.x, value.y, value.z);
    }
    if (!std::isfinite(pullVector.Magnitude()) || pullVector.Magnitude() <= gp::Resolution()) {
        throw std::runtime_error("Rib draft pull direction must be nonzero and finite");
    }
    const gp_Dir pull(pullVector), normal = plane.Axis().Direction();
    if (std::abs(pull.Dot(normal)) > Precision::Angular()) {
        throw std::runtime_error("Rib draft pull direction must lie in the profile plane");
    }
    gp_Trsf toFrame;
    toFrame.SetTransformation(gp_Ax3(plane.Location(), pull, normal.Crossed(pull)));
    const gp_Trsf fromFrame = toFrame.Inverted();
    const auto bounds = retained.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    const auto bodyBounds = body.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    const double angle = DraftAngle.getValue() * std::acos(-1.0) / 180;
    const double slope = std::tan(angle);
    const double thickness = Thickness.getValue();
    const double neutral = DraftReference.isValue("Root") ? bounds.MinZ : bounds.MaxZ;
    const double height = bounds.MaxZ - bounds.MinZ;

    // bbox already include a tolerance gap
    const double tolerance = Precision::Confusion();  // 10*Precision::Confusion();

    // The preliminary cut tells us the REQUIRED draft height. Extra tool below
    // a curved root is useful for a later trim, but must not create an artificial
    // face-crossing failure beneath a perfectly valid root.
    double bottom = bounds.MinZ;  // - std::max(tolerance, height*.2);
    double top = bounds.MaxZ;     // + tolerance;
    const auto widthAt = [&](double z) {
        return thickness + 2 * (neutral - z) * slope;
    };
    if (widthAt(bounds.MinZ) <= tolerance || widthAt(bounds.MaxZ) <= tolerance) {
        throw std::runtime_error("Draft closes the rib before its required extent; reduce the angle");
    }
    if (slope < 0) {
        bottom = std::max(bottom, bounds.MinZ - widthAt(bounds.MinZ) / (-4 * slope));
    }
    if (slope > 0) {
        top = std::min(top, bounds.MaxZ + widthAt(bounds.MaxZ) / (4 * slope));
    }

    const double y0 = PlacementType.isValue("Side A")
        ? 0.0
        : (PlacementType.isValue("Side B") ? -thickness : -thickness / 2);
    // A wider drafted rib meets a curved boss farther along X than the narrow
    // preliminary rib. Include the body's X range, not just the retained rib,
    // otherwise the envelope itself creates triangular cut-offs at the bosses.
    const double xmin = std::min(bounds.MinX, bodyBounds.MinX) - tolerance;
    const double xmax = std::max(bounds.MaxX, bodyBounds.MaxX) + tolerance;
    // End walls remain outside the retained profile. We select ONLY the two
    // broad faces, so unlike MakeDraft those end walls are never tapered.
    Part::TopoShape envelope(
        BRepPrimAPI_MakeBox(gp_Pnt(xmin, y0, bottom), xmax - xmin, thickness, top - bottom).Shape()
    );
    std::vector<Part::TopoShape> sides;
    for (const auto& face : envelope.getSubTopoShapes(TopAbs_FACE)) {
        gp_Pln facePlane;
        if (face.findPlane(facePlane)
            && facePlane.Axis().Direction().IsParallel(gp::DY(), Precision::Angular())) {
            sides.push_back(face);
        }
    }
    if (sides.size() != 2) {
        throw std::runtime_error("Cannot identify both rib envelope side faces");
    }
    // FreeCAD's existing wrapper calls BRepOffsetAPI_DraftAngle and carries its
    // history. retry=false is essential: a failed face must never be skipped.
    envelope = envelope.makeElementDraft(
        sides,
        gp::DZ(),
        angle,
        gp_Pln(gp_Pnt(0, 0, neutral), gp::DZ()),
        false,
        "RibDraftEnvelope"
    );
    requireSolid(envelope, "Draft could not create a valid envelope");
    envelope = envelope.makeElementTransform(fromFrame, "RibDraftEnvelopePlacement");

    // Re-form a sufficiently wide profile prism: intersecting with the original
    // thin tool would accidentally discard all outward-drafted material.
    const double wide = 2 * (thickness + std::max(widthAt(bottom), widthAt(top)));
    const auto profileTool = makeRibTool(surface, normal, wide, 2);
    Part::TopoShape drafted(0, surface.Hasher);
    drafted.makeElementBoolean(Part::OpCodes::Common, {profileTool, envelope}, "RibDraftProfileClip");
    requireSolid(drafted, "Draft clipping produced no valid rib tool");
    return drafted;
}

Part::TopoShape Rib::fuseRibWithBase(const Part::TopoShape& base, const Part::TopoShape& retained) const
{
    Part::TopoShape result(0, base.Hasher);
    result.makeElementBoolean(Part::OpCodes::Fuse, {base, retained}, "RibFinalFuse");
    requireSolid(result, "Rib fusion failed");
    if (result.getSubTopoShapes(TopAbs_SOLID).size() != 1) {
        throw std::runtime_error("The rib must join the body into one solid");
    }
    return result;
}


Part::TopoShape Rib::filletIntersectingEdges(const Part::TopoShape& fused, const Part::TopoShape& body) const
{
    // skip if fillet radius is zero
    if (FilletRadius.getValue() < Precision::Approximation()) {
        return fused;
    }

    // find the intersecting edges from common faces between the fused shape and the body
    TopTools_MapOfShape bodyFaces;
    for (const auto& face : fused.getSubTopoShapes(TopAbs_FACE)) {
        Part::TopoShape common(0, fused.Hasher);
        common.makeElementBoolean(Part::OpCodes::Common, {face, body});
        if (area(common.getShape()) > Precision::SquareConfusion()) {
            bodyFaces.Add(face.getShape());
        }
    }


    TopTools_IndexedDataMapOfShapeListOfShape edgeFaces;
    TopExp::MapShapesAndAncestors(fused.getShape(), TopAbs_EDGE, TopAbs_FACE, edgeFaces);

    std::vector<Part::TopoShape> junctions;
    for (int i = 1; i <= edgeFaces.Extent(); ++i) {
        const auto& faces = edgeFaces.FindFromIndex(i);
        if (faces.Extent() == 2
            && bodyFaces.Contains(faces.First()) != bodyFaces.Contains(faces.Last())) {
            junctions.emplace_back(edgeFaces.FindKey(i));
        }
    }

    if (junctions.empty()) {
        throw std::runtime_error("No wall-to-body junction edges are available for the fillet");
    }

    // fillet
    auto result = fused.makeElementFillet(
        junctions,
        FilletRadius.getValue(),
        FilletRadius.getValue(),
        "RibJunctionFillet"
    );

    requireSolid(result, "Requested rib junction fillet could not be constructed");

    return result;
}


void Rib::publishRib(const Part::TopoShape& result, const Part::TopoShape& body)
{
    // Shape is the finished body; AddSubShape is only the added feature
    Part::TopoShape addition(0, result.Hasher);
    addition.makeElementBoolean(Part::OpCodes::Cut, {result, body}, "RibAddition");
    requireSolid(addition, "Rib produced no added material");
    AddSubShape.setValue(addition);
    Shape.setValue(result);
}


App::DocumentObjectExecReturn* Rib::execute()
{
    try {

        // -------
        // Step 0: we need a solid body to build the rib against
        positionByPrevious();
        auto body = getBaseTopoShape();
        requireSolid(body, "Rib requires an existing solid body");

        // -------
        // Step 1: common coordinate system
        auto profile = getRibProfileWire();

        // get the sketch plane which is how we define the rib
        const auto origin = getVerifiedSketch()->Placement.getValue().getPosition();
        const auto normal = getProfileNormal();
        gp_Pln plane(gp_Pnt(origin.x, origin.y, origin.z), gp_Dir(normal.x, normal.y, normal.z));

        // coordinate systems
        const auto inverse = getLocation().Inverted();
        profile.move(inverse);
        body.move(inverse);
        plane.Transform(inverse.Transformation());

        // -------
        // step 2. extend the open ends of the profile
        // sweep and thickness to create the rib tool
        Bnd_Box bounds;
        BRepBndLib::Add(profile.getShape(), bounds);
        BRepBndLib::Add(body.getShape(), bounds);

        // reach is how far we will extend the ends of the profile and sweep
        // to create an intentionally oversized rib tool
        const double reach = 2 * std::sqrt(bounds.SquareExtent());

        // how far to sweep
        const auto travel = makeSweepVector(plane, reach);

        // extend the profile ends to create the rib tool
        const auto extended = extendRibProfile(body, profile, reach, ExtendType.getValue());

        // form the rib surface
        const auto surface = makeRibSurface(extended, travel, plane);

        // create the rib tool
        const auto tool = makeRibTool(
            surface,
            plane.Axis().Direction(),
            Thickness.getValue(),
            PlacementType.getValue()
        );

        // -------
        // step 3; the first body cut measures the useful rib region before drafting.
        Part::TopoShape farLimit;
        if (ExtentType.isValue("Shape")) {
            gp_Trsf translation;
            translation.SetTranslation(travel);
            farLimit = profile.moved(TopLoc_Location(translation));
        }
        auto retained = selectRibMaterial(cutRibTool(tool, body), profile, farLimit);

        // -------
        // step 4; draft the box envelope, clip it with the profile, then repeat
        // the body cut and selection. The neutral reference uses the useful rib region.
        if (std::abs(DraftAngle.getValue()) > Precision::Angular()) {
            const auto drafted = makeDraftedRibTool(surface, retained, body, plane, travel);
            retained = selectRibMaterial(cutRibTool(drafted, body), profile, farLimit);
        }

        // -------
        // 5. fillet if fillet radius is non-zero
        auto result = filletIntersectingEdges(fuseRibWithBase(body, retained), body);
        if (Refine.getValue()) {
            result = result.makeElementRefine("RibFinalRefine");
        }

        // done
        publishRib(result, body);
        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& error) {
        AddSubShape.setValue(Part::TopoShape());
        return new App::DocumentObjectExecReturn(error.GetMessageString());
    }
    catch (const std::exception& error) {
        AddSubShape.setValue(Part::TopoShape());
        return new App::DocumentObjectExecReturn(error.what());
    }
}
}  // namespace PartDesign
