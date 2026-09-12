// SPDX-License-Identifier: LGPL-2.1-or-later
#include "FeatureRib.h"

#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepClass3d_SolidClassifier.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <gp_Lin.hxx>
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
    // The document properties are the contract shared by execute(), expressions,
    // and the task panel. All geometric calculations below use internal mm/radians.

    ADD_PROPERTY_TYPE(ExtendType, (0L), "Rib", App::Prop_None, "Continuity used to extend the rib profile");
    ExtendType.setEnums(extendTypes);
    ExtendType.setValue("C1");
    ADD_PROPERTY_TYPE(Thickness, (2.0), "Rib", App::Prop_None, "Thickness of the rib");

    ADD_PROPERTY_TYPE(
        PlacementType,
        (0L),
        "Rib",
        App::Prop_None,
        "Apply thickness starting from the centerline or side."
    );
    PlacementType.setEnums(placementTypes);
    PlacementType.setValue("Centered");

    ADD_PROPERTY_TYPE(ExtentType, (0L), "Rib", App::Prop_None, "Extend the rib to the body.");
    ExtentType.setEnums(extentTypes);
    ExtentType.setValue("Shape");

    ADD_PROPERTY_TYPE(
        Distance,
        (1.0),
        "Rib",
        App::Prop_None,
        "Distance to sweep towards when distance is specified."
    );
    ADD_PROPERTY_TYPE(
        Direction,
        (Base::Vector3d(0, 0, -1)),
        "Rib",
        App::Prop_None,
        "Sweep direction in Rib-local coordinates, for both Shape and Distance extents."
    );
    ADD_PROPERTY_TYPE(DraftAngle, (0.0), "Rib", App::Prop_None, "Draft angle for the rib.");
    ADD_PROPERTY_TYPE(
        UseCustomPullDirection,
        (false),
        "Rib",
        App::Prop_None,
        "Use a custom in-plane draft axis instead of the direction opposite the sweep."
    );
    ADD_PROPERTY_TYPE(
        PullDirection,
        (Base::Vector3d(0, 0, 1)),
        "Rib",
        App::Prop_None,
        "Custom draft axis and neutral-plane normal in Rib-local coordinates."
    );
    ADD_PROPERTY_TYPE(
        DraftReference,
        (0L),
        "Rib",
        App::Prop_None,
        "Plane at which the entered thickness is preserved."
    );
    DraftReference.setEnums(draftReferences);
    DraftReference.setValue("Free end");
    ADD_PROPERTY_TYPE(FilletRadius, (0.0), "Rib", App::Prop_None, "Fillet radius for the rib.");
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

// These file-local helpers work on OCCT geometry, not FreeCAD document objects.
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

    // A free endpoint belongs to one edge. Interior vertices must join two edges.
    // Count occurrences: a closed edge uses its seam vertex twice.
    TopTools_IndexedDataMapOfShapeListOfShape vertexEdges;
    TopExp::MapShapesAndAncestors(wire, TopAbs_VERTEX, TopAbs_EDGE, vertexEdges);
    int endpoints = 0;
    for (int i = 1; i <= vertexEdges.Extent(); ++i) {
        const int degree = vertexEdges.FindFromIndex(i).Extent();
        if (degree == 1) {
            ++endpoints;
        }
        else if (degree != 2) {
            throw std::runtime_error("Rib profile must not branch");
        }
    }
    if (endpoints != 2) {
        throw std::runtime_error("Rib profile must have exactly two free endpoints");
    }

    // Two free endpoints alone do not rule out a disconnected closed loop.
    // The connected walk must visit every edge exactly once.
    TopTools_IndexedMapOfShape allEdges;
    TopExp::MapShapes(wire, TopAbs_EDGE, allEdges);
    TopTools_MapOfShape visited;
    for (BRepTools_WireExplorer walk(wire); walk.More(); walk.Next()) {
        if (!visited.Add(walk.Current())) {
            throw std::runtime_error("Rib profile must be a single traversable chain");
        }
    }
    if (visited.Extent() != allEdges.Extent()) {
        throw std::runtime_error("Rib profile must be a single traversable chain");
    }

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
    // Sample the original, because a previous extension can reparameterize the copy.
    const double parameter = after ? original->LastParameter() : original->FirstParameter();
    gp_Pnt endpoint;
    gp_Vec tangent;
    original->D1(parameter, endpoint, tangent);
    if (!std::isfinite(tangent.Magnitude()) || tangent.Magnitude() <= gp::Resolution()) {
        throw std::runtime_error("Rib profile endpoint has no regular tangent");
    }
    tangent.Normalize();
    if (!after) {
        tangent.Reverse();  // At the curve start, outward is opposite increasing parameters.
    }

    if (isLine) {
        // A line already satisfies C1/C2. Extend its bounds to keep it analytic
        // and its swept sides draftable, rather than converting it to a spline.
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
    // Locate the old endpoint on the final curve; its parameter may have changed.
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

    // Test only this new portion, including the old endpoint. Contact elsewhere
    // on the original profile must not hide an extension that misses the body.
    // Use the actual C1/C2 curve, not its tangent ray; they can take different paths.
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
    const gp_Dir& normal,
    double width,
    double offset,
    const Handle(BRepTools_History) & history
)
{
    // Restrict the underlying geometry to the portion used by this edge.
    double first = 0.0;
    double last = 0.0;
    Handle(Geom_Curve) geometry = BRep_Tool::Curve(edge, first, last);
    if (geometry.IsNull()) {
        throw std::runtime_error("Rib terminal edge has no 3D curve");
    }
    Handle(Geom_TrimmedCurve) original = new Geom_TrimmedCurve(geometry, first, last);
    Handle(Geom_BoundedCurve) curve = Handle(Geom_BoundedCurve)::DownCast(original->Copy());

    // Parameter order can oppose wire traversal. Match vertex identity instead
    // of assuming that the wire's start is the curve's first parameter.
    TopoDS_Vertex firstVertex;
    TopoDS_Vertex lastVertex;
    TopExp::Vertices(edge, firstVertex, lastVertex, false);
    const bool extendFirst = firstVertex.IsSame(ends.startVertex)
        || firstVertex.IsSame(ends.endVertex);
    const bool extendLast = lastVertex.IsSame(ends.startVertex) || lastVertex.IsSame(ends.endVertex);
    const bool isLine = BRepAdaptor_Curve(edge).GetType() == GeomAbs_Line;

    // Require coverage of the WHOLE transverse edge, not just its midpoint or
    // its two vertices. A solid cut also accepts ON-boundary contact, including
    // face junctions and periodic seams, without depending on face numbering.
    const auto covered = [&](const gp_Pnt& point) {
        const auto cross = BRepBuilderAPI_MakeEdge(
                               point.Translated(gp_Vec(normal) * (offset - width / 2)),
                               point.Translated(gp_Vec(normal) * (offset + width / 2))
        )
                               .Edge();
        BRepAlgoAPI_Cut outside(cross, body);
        if (!outside.IsDone() || outside.HasErrors()) {
            throw std::runtime_error("Cannot classify the rib endpoint width against the body");
        }
        GProp_GProps properties;
        BRepGProp::LinearProperties(outside.Shape(), properties);
        return properties.Mass() <= Precision::Confusion();
    };

    // Stop inside the support, not arbitrarily beyond the whole body. If the
    // original end already has full-width contact, preserve it: extending past
    // a sharp top edge could leave the body instead of improving the junction.
    const auto extendEnd = [&](bool after) {
        gp_Pnt point;
        gp_Vec tangent;
        original->D1(after ? last : first, point, tangent);
        if (covered(point)) {
            return false;
        }
        if (tangent.Magnitude() <= gp::Resolution()) {
            throw std::runtime_error("Rib endpoint has no regular tangent");
        }
        tangent.Normalize();
        if (!after) {
            tangent.Reverse();
        }
        IntCurvesFace_ShapeIntersector ray;
        ray.Load(body, Precision::Confusion());
        ray.PerformNearest(gp_Lin(point, gp_Dir(tangent)), 0, reach);
        double hit = ray.IsDone() && ray.NbPnt() ? std::max(0., ray.WParameter(1)) : 0.;
        // A ray beginning ON a face may report the exit on the far side as
        // its first hit. Start locally when the endpoint already touches a solid.
        for (TopExp_Explorer it(body, TopAbs_SOLID); it.More(); it.Next()) {
            BRepClass3d_SolidClassifier classifier(it.Current(), point, Precision::Confusion());
            if (classifier.State() == TopAbs_IN || classifier.State() == TopAbs_ON) {
                hit = 0;
                break;
            }
        }
        double step = std::max(width * .01, 100 * Precision::Confusion());
        for (int attempt = 0; attempt < 24 && hit + step <= reach; ++attempt, step *= 1.5) {
            auto candidate = Handle(Geom_BoundedCurve)::DownCast(curve->Copy());
            extendRibCurveEnd(candidate, original, hit + step, continuity, after, isLine);
            const gp_Pnt endpoint = after ? candidate->EndPoint() : candidate->StartPoint();
            if (covered(endpoint)) {
                curve = candidate;
                return true;
            }
        }
        throw std::runtime_error(
            "Cannot extend the full rib width into the body; check direction, thickness, or draft"
        );
    };
    const bool changedFirst = extendFirst && extendEnd(false);
    const bool changedLast = extendLast && extendEnd(true);
    if (!changedFirst && !changedLast) {
        return edge;
    }

    // Check the final geometry before replacing vertices. The accepted endpoint
    // lies beyond first contact, with enough overlap for the requested width.
    TopoDS_Vertex newFirst = firstVertex;
    TopoDS_Vertex newLast = lastVertex;
    if (changedFirst) {
        const char* endName = firstVertex.IsSame(ends.startVertex) ? "start" : "end";
        requireRibExtensionContact(body, curve, original->StartPoint(), false, endName);
        newFirst = BRepBuilderAPI_MakeVertex(curve->StartPoint()).Vertex();
        history->AddModified(firstVertex, newFirst);
    }
    if (changedLast) {
        const char* endName = lastVertex.IsSame(ends.startVertex) ? "start" : "end";
        requireRibExtensionContact(body, curve, original->EndPoint(), true, endName);
        newLast = BRepBuilderAPI_MakeVertex(curve->EndPoint()).Vertex();
        history->AddModified(lastVertex, newLast);
    }

    // Reuse unextended vertices so neighboring edges remain topologically joined.
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
    long continuity,
    const gp_Dir& normal,
    double width
) const
{
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

    // 1. Validate one open chain and find the edge attached to each free endpoint.
    const TopoDS_Wire wire = TopoDS::Wire(profile.getShape());
    const RibProfileEnds ends = findRibProfileEnds(wire);
    const int curveContinuity = static_cast<int>(continuity);  // Validated as 1 or 2 above.

    // 2. Extend the terminal edges and check each new portion against the body.
    // GeomLib has no topology history, so our helper records replacements for FreeCAD.
    Handle(BRepTools_History) history = new BRepTools_History;
    const double offset = Thickness.getValue()
        * (PlacementType.isValue("Side A") ? .5 : (PlacementType.isValue("Side B") ? -.5 : 0));
    const TopoDS_Edge extendedStart = extendRibTerminalEdge(
        ends.startEdge,
        ends,
        body.getShape(),
        reach,
        curveContinuity,
        normal,
        width,
        offset,
        history
    );
    TopoDS_Edge extendedEnd = extendedStart;
    if (!ends.startEdge.IsSame(ends.endEdge)) {
        extendedEnd = extendRibTerminalEdge(
            ends.endEdge,
            ends,
            body.getShape(),
            reach,
            curveContinuity,
            normal,
            width,
            offset,
            history
        );
    }

    // 3. Rebuild in connected order. Interior edges are reused unchanged.
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

    // 4. FreeCAD bridge: carry the profile's element names through the OCCT history.
    Part::TopoShape result(0, profile.Hasher);
    result.makeShapeWithElementMap(
        extendedWire,
        Part::MapperHistory(history),
        {profile},
        continuity == 1 ? "RibExtendC1" : "RibExtendC2"
    );
    return result;
}


gp_Pln Rib::getRibProfilePlane() const
{
    // This public query also anchors the task-panel draggers. Return the world
    // plane; execute() transforms it together with the profile and body below.
    const auto profile = getRibProfileWire();
    const auto points = findRibProfileEnds(TopoDS::Wire(profile.getShape()));
    const gp_Pnt origin = BRep_Tool::Pnt(points.startVertex);
    if (Profile.getValue()->isDerivedFrom<Part::Part2DObject>()) {
        const auto normal = getProfileNormal();
        return gp_Pln(origin, gp_Dir(normal.x, normal.y, normal.z));
    }

    gp_Pln plane;
    if (profile.findPlane(plane)) {
        return plane;
    }
    // A straight edge does not determine a unique plane. The edge plus the
    // selected fill vector does; a parallel vector cannot generate a rib.
    const auto direction = Direction.getValue();
    gp_Vec travel(direction.x, direction.y, direction.z);
    travel.Transform(getLocation().Transformation());
    BRepAdaptor_Curve curve(points.startEdge);
    gp_Pnt point;
    gp_Vec tangent;
    curve.D1(curve.FirstParameter(), point, tangent);
    const gp_Vec normal = tangent.Crossed(travel);
    if (normal.Magnitude() <= Precision::Confusion()) {
        throw std::runtime_error("Rib direction must not be parallel to its profile");
    }
    return gp_Pln(origin, gp_Dir(normal));
}

gp_Vec Rib::getRibTravel(const gp_Pln& plane, double reach) const
{
    const auto value = Direction.getValue();
    gp_Vec travel(value.x, value.y, value.z);
    if (!std::isfinite(travel.Magnitude()) || travel.Magnitude() <= gp::Resolution()) {
        throw std::runtime_error("Rib direction must be nonzero and finite");
    }
    travel.Normalize();
    const gp_Vec normal(plane.Axis().Direction());
    const double component = travel.Dot(normal);
    if (std::abs(component) > Precision::Angular()) {
        throw std::runtime_error("Rib direction must lie in the sketch plane");
    }
    travel -= normal * component;  // Remove round-off, not an intentional normal component.
    travel.Normalize();
    const double distance = ExtentType.isValue("Shape") ? reach : Distance.getValue();
    if (!std::isfinite(distance) || distance <= Precision::Confusion()) {
        throw std::runtime_error("Rib distance must be positive and finite");
    }
    return travel * (Reversed.getValue() ? -distance : distance);
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
    const gp_Pnt first = BRep_Tool::Pnt(ends.startVertex);
    const gp_Pnt last = BRep_Tool::Pnt(ends.endVertex);

    // Add the complete connected set at once. Reversing a multi-edge wire's
    // orientation does not reverse its storage order; adding its edges one by
    // one can silently leave a partially connected outline in MakeWire.
    TopTools_ListOfShape edges;
    for (const auto& wire : {profile.getShape(), far.getShape().Reversed()}) {
        for (TopExp_Explorer it(wire, TopAbs_EDGE); it.More(); it.Next()) {
            edges.Append(it.Current());
        }
    }
    edges.Append(BRepBuilderAPI_MakeEdge(last, last.Translated(travel)).Edge());
    edges.Append(BRepBuilderAPI_MakeEdge(first.Translated(travel), first).Edge());
    BRepBuilderAPI_MakeWire outline;
    outline.Add(edges);
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
    // Placement changes only the neutral-section offset, never the meaning of
    // Thickness: the entered value is always the complete width.
    const double offset = placement == 0 ? 0.0 : (placement == 1 ? -thickness : -thickness / 2);
    auto tool = surface.makeElementPrism(gp_Vec(normal) * thickness, "RibThickness");
    gp_Trsf move;
    move.SetTranslation(gp_Vec(normal) * offset);
    tool = tool.makeElementTransform(move, "RibThicknessPlacement");
    requireSolid(tool, "Rib thickness did not produce a valid solid");
    return tool;
}

Part::TopoShape Rib::makeProfileReference(
    const Part::TopoShape& profile,
    const gp_Dir& normal,
    double width
) const
{
    // A narrow centreline is not a reliable seed after one-sided inward draft:
    // it can lie outside the material. The roof ribbon spans the tool width;
    // retained solids must share roof AREA, rather than just a touching vertex.
    gp_Trsf shift;
    shift.SetTranslation(gp_Vec(normal) * (-width / 2));
    return profile.makeElementPrism(gp_Vec(normal) * width, "RibRoofReference")
        .makeElementTransform(shift, "RibRoofReferencePlacement");
}

Part::TopoShape Rib::cutRibTool(const Part::TopoShape& tool, const Part::TopoShape& base) const
{
    Part::TopoShape cut(0, tool.Hasher);
    cut.makeElementBoolean(Part::OpCodes::Cut, {tool, base}, "RibBodyCut");
    return cut;
}

Part::TopoShape Rib::selectRibMaterial(
    const Part::TopoShape& cutResult,
    const Part::TopoShape& roof,
    const gp_Vec& travel,
    bool requireBodyTermination
) const
{
    gp_Trsf translation;
    translation.SetTranslation(travel);
    const auto farRoof = roof.moved(TopLoc_Location(translation));
    std::vector<Part::TopoShape> kept;
    for (const auto& solid : cutResult.getSubTopoShapes(TopAbs_SOLID)) {
        Part::TopoShape contact(0, roof.Hasher);
        contact.makeElementBoolean(Part::OpCodes::Common, {solid, roof});
        if (area(contact.getShape()) <= Precision::SquareConfusion()) {
            continue;  // This is a remote piece on the other side of the body.
        }
        if (requireBodyTermination) {
            contact.makeElementBoolean(Part::OpCodes::Common, {solid, farRoof});
            if (area(contact.getShape()) > Precision::SquareConfusion()) {
                throw std::runtime_error("Rib did not terminate at the body across its full width");
            }
        }
        kept.push_back(solid);
    }
    if (kept.empty()) {
        throw std::runtime_error("No rib material remains connected to the profile");
    }
    Part::TopoShape result(0, cutResult.Hasher);
    result.makeElementCompound(kept, "RibRetained");
    return result;
}

gp_Trsf Rib::getDraftFrame(const gp_Pln& plane, const gp_Vec& travel) const
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
    return toFrame;
}

double Rib::getExtensionWidth(const Part::TopoShape& retained, const gp_Pln& plane, const gp_Vec& travel) const
{
    // The two sides grow symmetrically about the neutral section's midpoint,
    // including Side A/B placement. Use the actual PULL height, not a world-space
    // bounding-box diagonal, to avoid demanding unnecessary endpoint extension.
    const auto bounds
        = retained.moved(TopLoc_Location(getDraftFrame(plane, travel))).getBoundBoxOptimal();
    const double slope = std::tan(DraftAngle.getValue() * std::acos(-1.0) / 180);
    const double growth = (bounds.MaxZ - bounds.MinZ)
        * (DraftReference.isValue("Root") ? -slope : slope);
    const double width = Thickness.getValue();
    return width + 2 * std::max(0.0, growth);
}

Part::TopoShape Rib::makeDraftedRibTool(
    const Part::TopoShape& surface,
    const Part::TopoShape& retained,
    const Part::TopoShape& body,
    const gp_Pln& plane,
    const gp_Vec& travel
) const
{
    const gp_Dir normal = plane.Axis().Direction();
    const auto toFrame = getDraftFrame(plane, travel);
    const gp_Trsf fromFrame = toFrame.Inverted();
    const auto bounds = retained.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    const auto bodyBounds = body.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    const double angle = DraftAngle.getValue() * std::acos(-1.0) / 180;
    const double slope = std::tan(angle);
    const double thickness = Thickness.getValue();
    const double neutral = DraftReference.isValue("Root") ? bounds.MinZ : bounds.MaxZ;
    const double height = bounds.MaxZ - bounds.MinZ;
    const double tolerance = 10 * Precision::Confusion();

    // The preliminary cut tells us the REQUIRED draft height. Extra tool below
    // a curved root is useful for a later trim, but must not create an artificial
    // face-crossing failure beneath a perfectly valid root.
    double bottom = bounds.MinZ - std::max(tolerance, height * .2);
    // Re-extending a curved profile for draft can move its contact slightly
    // higher on a boss. Keep the ORIGINAL neutral plane, but let the box top
    // enclose the rebuilt profile. Using the first cut's MaxZ here creates tiny
    // horizontal cap faces at the new contact and can make junction fillets fail.
    const auto surfaceBounds = surface.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    double top = std::max(bounds.MaxZ, surfaceBounds.MaxZ) + tolerance;
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

void Rib::checkDraftTermination(
    const Part::TopoShape& tool,
    const Part::TopoShape& retained,
    const gp_Pln& plane,
    const gp_Vec& travel
) const
{
    if (!ExtentType.isValue("Shape")) {
        return;
    }
    // The compact draft box has a temporary bottom, unlike the original long
    // sweep. Outward draft can bypass a narrow support and reach this bottom.
    // Reject that artificial cap instead of publishing an apparently valid rib
    // which extends through/past the body. Contact along an edge alone is fine.
    const auto frame = getDraftFrame(plane, travel);
    const auto framed = tool.moved(TopLoc_Location(frame));
    const double bottom = framed.getBoundBoxOptimal().MinZ;
    for (const auto& face : tool.getSubTopoShapes(TopAbs_FACE)) {
        gp_Pln facePlane;
        const auto localFace = face.moved(TopLoc_Location(frame));
        if (!localFace.findPlane(facePlane)
            || !facePlane.Axis().Direction().IsParallel(gp::DZ(), Precision::Angular())
            || std::abs(facePlane.Location().Z() - bottom) > 10 * Precision::Confusion()) {
            continue;
        }
        Part::TopoShape contact(0, retained.Hasher);
        contact.makeElementBoolean(Part::OpCodes::Common, {retained, face});
        if (area(contact.getShape()) > Precision::SquareConfusion()) {
            throw std::runtime_error("Drafted rib does not terminate across its full width on the body; reduce thickness or draft");
        }
    }
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
    if (FilletRadius.getValue() == 0) {
        return fused;
    }
    // Boolean intersections classify the ACTUAL fused faces, including pieces
    // split from cylindrical/spline support faces. Surface type or edge numbers
    // cannot reliably identify a body/rib junction.
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
    try {
        auto result = fused.makeElementFillet(
            junctions,
            FilletRadius.getValue(),
            FilletRadius.getValue(),
            "RibJunctionFillet"
        );
        requireSolid(result, "Requested rib junction fillet could not be constructed");
        return result;
    }
    catch (const Standard_Failure&) {
        throw std::runtime_error("Cannot fillet every rib-to-body junction at this radius; reduce the radius or revise the profile");
    }
}

void Rib::publishRib(const Part::TopoShape& result, const Part::TopoShape& body)
{
    // Store both contracts: Shape is the finished body, AddSubShape is only
    // the added feature (including the fillet) for preview and feature patterns.
    Part::TopoShape addition(0, result.Hasher);
    addition.makeElementBoolean(Part::OpCodes::Cut, {result, body}, "RibAddition");
    requireSolid(addition, "Rib produced no added material");
    AddSubShape.setValue(addition);
    Shape.setValue(result);
}

App::DocumentObjectExecReturn* Rib::execute()
{
    try {
        // 1. Read inputs and bring body, profile and plane into the Rib-local
        // frame. Never mix sketch coordinates with world-space OCCT operands.
        positionByPrevious();
        auto profile = getRibProfileWire();
        auto body = getBaseTopoShape();
        auto plane = getRibProfilePlane();
        const auto inverse = getLocation().Inverted();
        profile.move(inverse);
        body.move(inverse);
        plane.Transform(inverse.Transformation());
        requireSolid(body, "Rib requires an existing solid body");
        if (!std::isfinite(Thickness.getValue()) || Thickness.getValue() <= Precision::Confusion()
            || !std::isfinite(DraftAngle.getValue()) || std::abs(DraftAngle.getValue()) >= 89
            || !std::isfinite(FilletRadius.getValue()) || FilletRadius.getValue() < 0) {
            throw std::runtime_error(
                "Rib requires positive thickness, a draft between -89 and 89 degrees, and "
                "nonnegative fillet radius"
            );
        }

        // 2. Extend the terminal curves and form the undrafted tool. A finite
        // reach derived from BOTH inputs keeps all temporary boundaries remote.
        Bnd_Box bounds;
        BRepBndLib::Add(profile.getShape(), bounds);
        BRepBndLib::Add(body.getShape(), bounds);
        const double reach = 2 * std::sqrt(bounds.SquareExtent());
        const auto travel = getRibTravel(plane, reach);
        const double width = Thickness.getValue();
        const auto extended = extendRibProfile(
            body,
            profile,
            reach,
            ExtendType.getValue(),
            plane.Axis().Direction(),
            width
        );
        auto surface = makeRibSurface(extended, travel, plane);
        const auto tool = makeRibTool(
            surface,
            plane.Axis().Direction(),
            Thickness.getValue(),
            PlacementType.getValue()
        );

        // 3. The first body cut measures the useful height BEFORE drafting.
        // Use a roof ribbon rather than a centreline to seed retained material.
        const double roofWidth = 4
            * (Thickness.getValue()
               + reach * std::abs(std::tan(DraftAngle.getValue() * std::acos(-1.0) / 180)));
        const auto roof = makeProfileReference(profile, plane.Axis().Direction(), roofWidth);
        auto retained
            = selectRibMaterial(cutRibTool(tool, body), roof, travel, ExtentType.isValue("Shape"));

        // 4. Draft a simple envelope, clip it with the profile, then repeat the
        // body cut/selection. This order keeps the drafted walls in body contact.
        if (std::abs(DraftAngle.getValue()) > Precision::Angular()) {
            // The first cut gives the actual draft height. Recalculate extension
            // coverage for the larger width BEFORE constructing the drafted tool.
            // This deliberately repeats extension/surface construction, not the
            // entire execution pipeline, and prevents exposed drafted end caps.
            const double coverage = getExtensionWidth(retained, plane, travel);
            const auto draftProfile = extendRibProfile(
                body,
                profile,
                reach,
                ExtendType.getValue(),
                plane.Axis().Direction(),
                coverage
            );
            surface = makeRibSurface(draftProfile, travel, plane);
            const auto drafted = makeDraftedRibTool(surface, retained, body, plane, travel);
            retained = selectRibMaterial(
                cutRibTool(drafted, body),
                roof,
                travel,
                ExtentType.isValue("Shape")
            );
            checkDraftTermination(drafted, retained, plane, travel);
        }

        // 5. Finish only after selection: fuse, optionally fillet the junction,
        // refine redundant faces, and publish the body plus preview addition.
        auto result = fuseRibWithBase(body, retained);
        if (Refine.getValue()) {
            // Remove boolean splits BEFORE finding junctions. A redundant short
            // edge should not become a separate fillet contour or corner.
            result = result.makeElementRefine("RibFuseRefine");
        }
        result = filletIntersectingEdges(result, body);
        if (Refine.getValue()) {
            result = result.makeElementRefine("RibFinalRefine");
        }
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
