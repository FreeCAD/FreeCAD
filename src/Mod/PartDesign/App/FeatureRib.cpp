// SPDX-License-Identifier: LGPL-2.1-or-later
#include "FeatureRib.h"
#include "App/PropertyContainer.h"
#include "App/PropertyUnits.h"
#include "Base/Vector3D.h"
#include "Mod/Part/App/TopoShape.h"
#include <BRepClass3d_BndBoxTree.hxx>
#include <Mod/Part/App/TopoShapeOpCode.h>
#include <Mod/Part/App/Part2DObject.h>
#include <gp_Dir.hxx>
#include <gp_Pln.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <GeomAbs_CurveType.hxx>
#include <BRepPrimAPI_MakeBox.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepTools_History.hxx>
#include <Precision.hxx>

#include <BRepTools_WireExplorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepBndLib.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BSplineCurve.hxx>
#include <GeomConvert.hxx>
#include <GeomLib.hxx>
#include <GeomAPI_ProjectPointOnCurve.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Vertex.hxx>
#include <TopoDS_Wire.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Edge.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Standard_Failure.hxx>
#include <gp.hxx>
#include <gp_Vec.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>

#include <algorithm>
#include <cmath>
#include <iostream>
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
}

Rib::Rib()
{
    defineAdditive();
    // Register Rib-specific input properties here as they are implemented.
    // Profile and recompute tracking are already provided by ProfileBased.
    //

    ADD_PROPERTY_TYPE(
        ExtendType,
        (0L),
        "Rib",
        App::Prop_None,
        "Continuity used to extend the rib profile"
    );
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

    ADD_PROPERTY_TYPE(
        ExtentType,
        (0L),
        "Rib",
        App::Prop_None,
        "Extend the rib to the body."
    );
    ExtentType.setEnums(extentTypes);
    ExtentType.setValue("Shape");


    ADD_PROPERTY_TYPE(Distance, (1.0), "Rib", App::Prop_None, "Distance to sweep towards when distance is specified.");
    ADD_PROPERTY_TYPE(Direction, (Base::Vector3d(0, 0, -1)), "Rib", App::Prop_None,
                      "Sweep direction in Rib-local coordinates, for both Shape and Distance extents.");
    ADD_PROPERTY_TYPE(DraftAngle, (0.0), "Rib", App::Prop_None, "Draft angle for the rib.");
    ADD_PROPERTY_TYPE(UseCustomPullDirection, (false), "Rib", App::Prop_None,
                      "Use a custom in-plane draft axis instead of the direction opposite the sweep.");
    ADD_PROPERTY_TYPE(PullDirection, (Base::Vector3d(0, 0, 1)), "Rib", App::Prop_None,
                      "Custom draft axis and neutral-plane normal in Rib-local coordinates.");
    ADD_PROPERTY_TYPE(FilletRadius, (0.0), "Rib", App::Prop_None, "Fillet radius for the rib.");
}

short Rib::mustExecute() const
{
    if (ExtendType.isTouched() || Thickness.isTouched() || DraftAngle.isTouched()
        || Direction.isTouched() || ExtentType.isTouched() || Distance.isTouched()
        || UseCustomPullDirection.isTouched() || PullDirection.isTouched()
        || FilletRadius.isTouched()) {
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




gp_Vec Rib::calculateRibDirection(const gp_Vec& profileCenter, const gp_Vec& bodyCenter) const
{
    gp_Vec direction = bodyCenter - profileCenter;
    if (direction.Magnitude() < Precision::Confusion()) {
        throw std::runtime_error("Rib direction cannot be calculated");
    }
    return direction;
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
    Handle(Geom_BoundedCurve)& curve,
    const Handle(Geom_TrimmedCurve)& original,
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
        tangent.Reverse(); // At the curve start, outward is opposite increasing parameters.
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
    const Handle(Geom_BoundedCurve)& curve,
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
            std::string("Rib profile ") + endName
            + " extension does not intersect the body within reach"
        );
    }
}

TopoDS_Edge extendRibTerminalEdge(
    const TopoDS_Edge& edge,
    const RibProfileEnds& ends,
    const TopoDS_Shape& body,
    double reach,
    int continuity,
    const Handle(BRepTools_History)& history
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
    const bool extendFirst = firstVertex.IsSame(ends.startVertex) || firstVertex.IsSame(ends.endVertex);
    const bool extendLast = lastVertex.IsSame(ends.startVertex) || lastVertex.IsSame(ends.endVertex);
    const bool isLine = BRepAdaptor_Curve(edge).GetType() == GeomAbs_Line;

    // A single-edge profile has both free endpoints on the same curve copy.
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

    // Reuse unextended vertices so neighboring edges remain topologically joined.
    BRepBuilderAPI_MakeEdge maker(
        curve, newFirst, newLast, curve->FirstParameter(), curve->LastParameter()
    );
    if (!maker.IsDone()) {
        throw std::runtime_error("Failed to build extended rib edge");
    }
    TopoDS_Edge replacement = maker.Edge();
    replacement.Orientation(edge.Orientation());
    history->AddModified(edge, replacement);
    return replacement;
}
} // namespace

Part::TopoShape Rib::extendRibProfile(
    const Part::TopoShape& body,
    const Part::TopoShape& profile,
    double reach,
    long continuity
) const
{
    if (continuity == 0) {
        return profile; // Off: preserve the shape and its FreeCAD element map.
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
    const int curveContinuity = static_cast<int>(continuity); // Validated as 1 or 2 above.

    // 2. Extend the terminal edges and check each new portion against the body.
    // GeomLib has no topology history, so our helper records replacements for FreeCAD.
    Handle(BRepTools_History) history = new BRepTools_History;
    const TopoDS_Edge extendedStart = extendRibTerminalEdge(
        ends.startEdge, ends, body.getShape(), reach, curveContinuity, history
    );
    TopoDS_Edge extendedEnd = extendedStart;
    if (!ends.startEdge.IsSame(ends.endEdge)) {
        extendedEnd = extendRibTerminalEdge(
            ends.endEdge, ends, body.getShape(), reach, curveContinuity, history
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
        extendedWire, Part::MapperHistory(history), {profile},
        continuity == 1 ? "RibExtendC1" : "RibExtendC2"
    );
    return result;
}





Part::TopoShape Rib::makeRibSurface(const Part::TopoShape& profile, const gp_Vec& extentDirection) const
{
    if (!std::isfinite(extentDirection.Magnitude()) || extentDirection.Magnitude() <= Precision::Confusion()) {
        throw std::runtime_error("Rib sweep requires an direction vector magnitude > 0 and less than inf");
    }

    // create the sweep surface, keep history
    auto swept = profile.makeElementPrism(extentDirection, "RibProfileSweep");
    if (!swept.hasSubShape(TopAbs_FACE)) {
        throw std::runtime_error("Rib profile sweep produced no faces");
    }

    // avoid unwanted edge creation; clean the sweep before creating the cutting tool
    // b-splines need special handling - compare to GeomAbs_BSplineCurve
    bool containsBSplines = false;
    for (TopExp_Explorer explorer(profile.getShape(), TopAbs_EDGE); explorer.More(); explorer.Next()) {
        const BRepAdaptor_Curve curve(TopoDS::Edge(explorer.Current()));
        // test for bsplines
        if (curve.GetType() == GeomAbs_BSplineCurve) {
            containsBSplines = true;
            break;
        }
    }
    // clean
    // unify edges, and faces and concatenate bspines if there are present
    ShapeUpgrade_UnifySameDomain cleaner(swept.getShape(), true, true, containsBSplines);
    cleaner.SetSafeInputMode(true);
    cleaner.Build();

    Part::TopoShape unified(0, swept.Hasher);
    unified.makeShapeWithElementMap(
        cleaner.Shape(), Part::MapperHistory(cleaner.History()), {swept},
        "RibSurfaceUnify"
    );

    // freecad has a different refine method... but unifysamedomain worked during dev..
    // swept = unified;
    // return swept.makeElementRefine("RibSurfaceRefine");
    return unified;
}




Part::TopoShape Rib::makeRibTool(
    const Part::TopoShape& surface,
    const double& thickness,
    const long& thicknessPlacementType
) const
{

    const auto sketchProfileNormal = getProfileNormal();


    if (!std::isfinite(thicknessTravel.Magnitude())
        || thicknessTravel.Magnitude() <= Precision::Confusion()
        || !std::isfinite(offset.Magnitude())) {
        throw std::runtime_error("Rib thickness travel and offset must be finite, with nonzero thickness");
    }

    // we should have a single face/surface after the sweep and unifysamedomain refinement
    // since all co-planar edges should be removed...
    const auto faces = surface.getSubTopoShapes(TopAbs_FACE);
    if (faces.size() != 1) {
        throw std::runtime_error("Rib must have one face to form the rib tool");
    }





    auto tool = faces.front().makeElementPrism(thicknessTravel, "RibThickness");
    if (!tool.hasSubShape(TopAbs_SOLID)) {
        throw std::runtime_error("Rib thickness extrusion produced no solid");
    }

    // we need to offset the tool to get a centered (or other-sided) placement of the tool
    gp_Trsf placement;
    placement.SetTranslation(offset);
    return tool.makeElementTransform(placement, "RibToolPlacement");
}





Part::TopoShape Rib::cutRibTool(const Part::TopoShape& tool, const Part::TopoShape& base) const
{
    // cut the tool with the body to get a (compound) rib result
    Part::TopoShape cut(0, tool.Hasher);
    cut.makeElementBoolean(Part::OpCodes::Cut, {tool, base}, "RibBodyCut");
    return cut;
}


bool Rib::profileTouchesSolid(const TopoDS_Shape& solid, const TopoDS_Shape& reference)
{
    BRepExtrema_DistShapeShape distance(solid, reference);
    return distance.IsDone() && distance.Value() <= Precision::Confusion();
}


Part::TopoShape Rib::selectRibMaterial(
    const Part::TopoShape& cutResult,
    const Part::TopoShape& originalProfile,
    const gp_Vec& travel,
    bool requireBodyTermination
) const
{
    // These shapes are used only for distance queries, so the far reference needs
    // no history. TopoShape::moved() leaves the original profile unchanged.
    gp_Trsf translation;
    translation.SetTranslation(travel);
    const auto farProfile = originalProfile.moved(TopLoc_Location(translation));

    // we need to find the part of the rib cutting tool that we want to keep
    // e.g., the part in contact with the original profile we used for the sweek
    std::vector<Part::TopoShape> kept;
    for (const auto& solid : cutResult.getSubTopoShapes(TopAbs_SOLID)) {
        const bool touchesOriginal = profileTouchesSolid(solid.getShape(), originalProfile.getShape());

        // if no contact with orignal, discard and test the next one
        if (!touchesOriginal) {
            continue;
        }

        // shape termination passes through the body, but distance does not (necessarly)
        if (requireBodyTermination && profileTouchesSolid(solid.getShape(), farProfile.getShape())) {
            throw std::runtime_error("Rib did not terminate at the body");
        }

        kept.push_back(solid);
    }


    if (kept.empty()) {
        throw std::runtime_error("No rib material connected to the original profile remains after the cut");
    }
    // normally we would have only one piece...
    if (kept.size() == 1) {
        return kept.front();
    }
    // ...unless profile passes through the body multiple times
    // so we allow for that
    Part::TopoShape retained(0, cutResult.Hasher);
    retained.makeElementCompound(kept, "RibRetained");
    return retained;
}






Part::TopoShape Rib::fuseRibWithBase(
    const Part::TopoShape& base,
    const Part::TopoShape& retained,
    bool refine
) const
{
    // we fuse the part of the rib that was incontact with the profile to the body
    Part::TopoShape result(0, base.Hasher);
    result.makeElementBoolean(Part::OpCodes::Fuse, {base, retained}, "RibFinalFuse");
    if (refine) {
        // allow freecad refine to be applied
        result = result.makeElementRefine("RibFinalRefine");
    }
    // should be solid... but we can allow?
    // if (result.getSubTopoShapes(TopAbs_SOLID).size() != 1
    //     || !BRepCheck_Analyzer(result.getShape()).IsValid()) {
    //     throw std::runtime_error("Final rib body is not one valid solid");
    // }
    return result;
}



Part::TopoShape Rib::applyDraft(
    const Part::TopoShape& tool,
    const std::vector<Part::TopoShape>& faces,
    const gp_Dir& pullDirection,
    double draftAngle,
    const gp_Pln& neutralPlane
) const
{
    if (!std::isfinite(draftAngle)) {
        throw std::runtime_error("Rib draft angle must be finite");
    }
    if (std::abs(draftAngle) <= Precision::Angular()) {
        return tool;
    }

    Part::TopoShape draftedTool(0, tool.Hasher);
    draftedTool.makeElementDraft(
        tool,
        faces,
        pullDirection,
        draftAngle, // angle in radians
        neutralPlane, // draft plane (unchanged section)
        false, // retry by skipping failed faces
        "RibDraft"
    );

    if (draftedTool.isNull() || !draftedTool.hasSubShape(TopAbs_SOLID)
        || !BRepCheck_Analyzer(draftedTool.getShape()).IsValid()) {
        throw std::runtime_error("Rib draft did not produce a valid solid tool");
    }

    return draftedTool;
}




Base::Vector3d Rib::calculateSweepDirection(const Part::TopoShape& profileShape, const Part::TopoShape& baseShape) const
{
    Base::Vector3d center;
    Base::Vector3d bodyCenter;

    if (!profileShape.getCenterOfGravity(center)) {
        throw std::runtime_error("Cannot calculate the rib profile center");
    }

    if (!baseShape.getCenterOfGravity(bodyCenter)) {
        throw std::runtime_error("Cannot calculate the body center");
    }

    Base::Vector3d sweepDirection =  bodyCenter - center;
    return sweepDirection.Normalized();
}

// Convert finite, three-dimensional bounds into a solid in the same coordinate frame.
Part::TopoShape Rib::bboxToShape(const Bnd_Box& bbox)
{
    if (bbox.IsVoid() || bbox.IsOpen()) {
        throw std::runtime_error("Cannot make a solid from an empty or unbounded bounding box");
    }

    // OCCT includes the bounding box's gap (padding) in these limits.
    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    for (double value : {xmin, ymin, zmin, xmax, ymax, zmax}) {
        if (!std::isfinite(value)) {
            throw std::runtime_error("Bounding box coordinates must be finite");
        }
    }
    const double dx = xmax - xmin;
    const double dy = ymax - ymin;
    const double dz = zmax - zmin;
    for (double length : {dx, dy, dz}) {
        if (!std::isfinite(length) || length <= Precision::Confusion()) {
            throw std::runtime_error("Bounding box must have three nonzero dimensions to form a solid");
        }
    }

    BRepPrimAPI_MakeBox maker(gp_Pnt(xmin, ymin, zmin), dx, dy, dz);
    maker.Build();
    if (!maker.IsDone()) {
        throw std::runtime_error("Failed to build the bounding box solid");
    }
    // This is new helper geometry; there is no source element history to transfer.
    return Part::TopoShape(maker.Shape());
}




Part::TopoShape Rib::makeDraftedRibTool(
    const Part::TopoShape& tool,
    const Part::TopoShape& retained,
    const Part::TopoShape& body,
    const gp_Pln& profilePlane,
    const gp_Vec& travel,
    double draftAngle,
    bool holdRoot
) const
{
    if (!std::isfinite(draftAngle)) {
        throw std::runtime_error("Rib draft angle must be finite");
    }
    if (std::abs(draftAngle) <= Precision::Angular()) {
        return tool;
    }

    // Z runs from root to free end, Y through the thickness, X across the profile.
    // A custom pull rotates the in-plane frame; it must still lie in that plane.
    gp_Vec pullVector = -travel;
    if (UseCustomPullDirection.getValue()) {
        const auto value = PullDirection.getValue();
        pullVector = gp_Vec(value.x, value.y, value.z);
    }
    if (!std::isfinite(pullVector.Magnitude())
        || pullVector.Magnitude() <= Precision::Confusion()) {
        throw std::runtime_error("Rib draft pull direction must be nonzero and finite");
    }
    const gp_Dir pull(pullVector);
    const gp_Dir normal = profilePlane.Axis().Direction();
    if (std::abs(pull.Dot(normal)) > Precision::Angular()) {
        throw std::runtime_error("Rib draft pull direction must lie in the profile plane");
    }
    const gp_Ax3 frame(profilePlane.Location(), pull, normal.Crossed(pull));
    gp_Trsf toFrame;
    toFrame.SetTransformation(frame);
    const gp_Trsf fromFrame = toFrame.Inverted();
    const double tolerance = 10 * Precision::Confusion();
    const auto ribBounds = retained.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
    const double height = ribBounds.MaxZ - ribBounds.MinZ;
    if (!std::isfinite(height) || height <= Precision::Confusion()) {
        throw std::runtime_error("Retained rib has no draft height");
    }

    // Keep these planes on the actual rib, not on the padded construction box.
    const double centerX = (ribBounds.MinX + ribBounds.MaxX) / 2;
    const double centerY = (ribBounds.MinY + ribBounds.MaxY) / 2;
    const gp_Pln rootPlane(
        gp_Pnt(centerX, centerY, ribBounds.MinZ).Transformed(fromFrame), pull
    );
    const gp_Pln topPlane(
        gp_Pnt(centerX, centerY, ribBounds.MaxZ).Transformed(fromFrame), pull
    );

    // Only the body's section in the profile plane sets the lateral reach.
    // Off-plane bosses must not make this compact tool arbitrarily long.
    BRepAlgoAPI_Section section(body.getShape(), profilePlane, false);
    section.SetNonDestructive(true);
    section.Build();
    if (!section.IsDone() || section.HasErrors()) {
        throw std::runtime_error("Cannot section the body for the rib draft bounds");
    }
    double xmin = ribBounds.MinX;
    double xmax = ribBounds.MaxX;
    const Part::TopoShape bodySection(section.Shape());
    if (bodySection.hasSubShape(TopAbs_EDGE)) {
        const auto sectionBounds = bodySection.moved(TopLoc_Location(toFrame)).getBoundBoxOptimal();
        xmin = std::min(xmin, sectionBounds.MinX);
        xmax = std::max(xmax, sectionBounds.MaxX);
    }

    // Compare attachment faces, not just minimum distance: a single touching
    // point cannot establish that the entire root is coplanar with the box end.
    Part::TopoShape ribFaces(0, retained.Hasher), bodyFaces(0, body.Hasher), contact;
    ribFaces.makeElementCompound(retained.getSubTopoShapes(TopAbs_FACE));
    bodyFaces.makeElementCompound(body.getSubTopoShapes(TopAbs_FACE));
    contact.makeElementBoolean(Part::OpCodes::Common, {ribFaces, bodyFaces}, "RibRootContact");
    bool planarRoot = contact.hasSubShape(TopAbs_FACE);
    for (const auto& face : contact.getSubTopoShapes(TopAbs_FACE)) {
        gp_Pln plane;
        if (!face.findPlane(plane)
            || !plane.Axis().Direction().IsParallel(pull, Precision::Angular())
            || rootPlane.Distance(plane.Location()) > tolerance) {
            planarRoot = false;
            break;
        }
    }

    const double rootPadding = planarRoot ? tolerance : std::max(tolerance, 0.2 * height);
    Bnd_Box bounds;
    bounds.Update(
        xmin - tolerance, ribBounds.MinY - tolerance, ribBounds.MinZ - rootPadding,
        xmax + tolerance, ribBounds.MaxY + tolerance, ribBounds.MaxZ + tolerance
    );
    const auto box = bboxToShape(bounds).makeElementTransform(fromFrame, "RibDraftBox");
    Part::TopoShape compact(0, tool.Hasher);
    compact.makeElementBoolean(Part::OpCodes::Common, {tool, box}, "RibCompactTool");
    if (!compact.hasSubShape(TopAbs_SOLID) || !compact.isValid()) {
        throw std::runtime_error("Rib draft bounds produced no valid solid tool");
    }

    // Isolate shared geometry before DraftAngle edits it. Draft only the planar
    // thickness sides, not the caps introduced by the bounding box intersection.
    compact = compact.makeElementCopy("RibDraftInput");
    std::vector<Part::TopoShape> sides;
    for (const auto& face : compact.getSubTopoShapes(TopAbs_FACE)) {
        gp_Pln plane;
        if (face.findPlane(plane)
            && plane.Axis().Direction().IsParallel(normal, Precision::Angular())
            && profilePlane.Distance(plane.Location()) > Precision::Confusion()) {
            sides.push_back(face);
        }
    }
    if (sides.empty()) {
        throw std::runtime_error("No offset rib sidewalls available for draft");
    }

    return applyDraft(compact, sides, pull, draftAngle, holdRoot ? rootPlane : topPlane);
}



App::DocumentObjectExecReturn* Rib::execute()
{
    // get the rib profile wire and base shape
    Part::TopoShape profileShape;
    Part::TopoShape baseShape;
    Part::TopoShape toolProfile;
    try {
        if (!std::isfinite(Thickness.getValue()) || Thickness.getValue() <= 0.0) {
            return new App::DocumentObjectExecReturn("Rib thickness must be positive and finite");
        }
        profileShape = getRibProfileWire();
        baseShape = getBaseTopoShape();

        // move everything into the preceding feature's local frame
        positionByPrevious();
        const auto inverseLocation = getLocation().Inverted();
        baseShape.move(inverseLocation);
        profileShape.move(inverseLocation);


        // keep both wrappers for element mapping
        const TopoDS_Shape& profile = profileShape.getShape();
        const TopoDS_Shape& body = baseShape.getShape();


        // Extent chooses where to stop, not the direction. The vector shown in
        // the task panel must also control Shape mode, regardless of body mass.
        const bool upToBody = ExtentType.isValue("Shape");
        const Base::Vector3d sweepDirection = Direction.getValue();


        // Note: there is an occt method for making ribs (BRepFeat_MakeLinearForm) but, it does
        // have limitations, and afaik we cant specify an intermediate distance to sweep
        // and we still need to take care of extending edges and making sure we extend them far
        // enough that we don't have missing segments (or small triangles/infill) on curved surfaces
        // Alternatives:
        // 1. Use section in combination with our extended profile to get a max wire region for our
        // rib surface at that planar location
        //   - would need to get that for each planar extent, including draft etc. to make sure we
        //     extend far enough
        // 2. directly construct surfaces and form a solid using these profiles/sections
        // 3. use oversized rib tool (long extensions) - offset/sweep -> cut and select -> draft -> fillet



        // get combined bounding box from the profle and body
        // and multiply by a factor so that we sweep beyond them
        const double reachFactor = 2.0;
        Bnd_Box bounds;
        BRepBndLib::Add(profile, bounds);
        BRepBndLib::Add(body, bounds);
        // how far we extend the profile ends
        const double reach = reachFactor * std::sqrt(bounds.SquareExtent());



        // get the extend type / continuity from the property and extend the profile ends
        toolProfile = extendRibProfile(baseShape, profileShape, reach, ExtendType.getValue());


        //
        gp_Vec travel(sweepDirection.x, sweepDirection.y, sweepDirection.z);
        if (!std::isfinite(travel.Magnitude()) || travel.Magnitude() <= gp::Resolution()) {
            throw std::runtime_error("Rib sweep direction must be nonzero and finite");
        }
        travel.Normalize();



        // FreeCAD gives the sketch normal in the source frame; rotate it into the
        // same Rib-local frame as the two shapes. gp_Vec ignores translation.
        const bool sketchProfile = Profile.getValue()->isDerivedFrom<Part::Part2DObject>();
        gp_Vec sketchNormal;
        if (sketchProfile) {
            const auto normal = getProfileNormal();
            sketchNormal = gp_Vec(normal.x, normal.y, normal.z);
            sketchNormal.Transform(inverseLocation.Transformation());
            sketchNormal.Normalize();




            const double normalComponent = travel.Dot(sketchNormal);
            if (std::abs(normalComponent) > Precision::Angular()) {
                throw std::runtime_error("Rib direction must lie in the sketch plane");
            }
            // Remove floating-point residue only; never silently redirect the
            // selected vector towards the body's center or another plane.
            travel -= sketchNormal * normalComponent;
            if (travel.Magnitude() <= Precision::Angular()) {
                throw std::runtime_error("Cannot determine an in-plane rib sweep direction");
            }
            travel.Normalize();
        }

        const double sweepLength = upToBody ? reach : Distance.getValue();
        if (!std::isfinite(sweepLength) || sweepLength <= Precision::Confusion()) {
            throw std::runtime_error("Rib sweep distance must be positive and finite");
        }
        travel *= sweepLength;

        // Sweep first so a selected-edge profile (which may be just one straight
        // edge) gets its thickness normal from the actual surface it generates.
        const auto surface = makeRibSurface(toolProfile, travel);


        // extrude the rib surface to a thickness; offset to recenter or place on either side.
        const auto tool = makeRibTool(
            surface,
            Thickness.getValue(),
            PlacementType.getValue()
        );


        // First find the undrafted material; it bounds the subsequent draft tool.
        const auto cut = cutRibTool(tool, baseShape);



        // keep the pieces of the cut that are connected to the profile and discard the rest
        const auto retained = selectRibMaterial(cut, profileShape, travel, upToBody);



        // TODO: draft the tool before fusing

        // draft the tool if necessary
        Part::TopoShape draftRetained = retained;
        // freecad stores angles in degrees; OCCT expects radians
        const double draftAngle = DraftAngle.getValue() * std::acos(-1.0) / 180.0;
        if (!std::isfinite(draftAngle)) {
            throw std::runtime_error("Rib draft angle must be finite");
        }

        // use the bounding box of the retained material to trim the original un-cut tool
        // then draft the trimmed tool (otherwise we can only acheive small draft angles w/o self intersections)
        if (std::abs(draftAngle) > Precision::Angular()) {
            const auto draftedTool = makeDraftedRibTool(
                tool, retained, baseShape, surfacePlane, travel, draftAngle
            );
            // cut and select as before but with the now drafted tool
            const auto draftCut = cutRibTool(draftedTool, baseShape);
            draftRetained = selectRibMaterial(draftCut, profileShape, travel, upToBody);
        }



        // we need to get the common faces before fusing to optionally fillet them
        // maybe use these for draft neutral plane/setting root etc.


        // Keep the junction face boundaries until after the fillet has selected its edges.
        const auto fused = fuseRibWithBase(baseShape, draftRetained, false);


        auto result = fused;
        // auto result = filletIntersectingEdges(fused, baseShape);
        if (Refine.getValue()) {
            result = result.makeElementRefine("RibFinalRefine");
        }

        // Junction fillets can add material outside the original rib tool. Include
        // it in AddSubShape so downstream patterns receive the complete addition.
        // Part::TopoShape addition = draftRetained;
        // if (FilletRadius.getValue() > 0.0) {
        //     addition.makeElementBoolean(Part::OpCodes::Cut, {result, baseShape}, "RibFilletedAddition");
        // }
        // AddSubShape.setValue(addition);
        Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (const Standard_Failure& error) {
        return new App::DocumentObjectExecReturn(error.GetMessageString());
    }
    catch (const std::exception& error) {
        return new App::DocumentObjectExecReturn(error.what());
    }

}

}  // namespace PartDesign
