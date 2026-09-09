// SPDX-License-Identifier: LGPL-2.1-or-later
#include "ThinExtrusionGeometry.h"
#include "ThinProfile.h"

#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Splitter.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepGProp.hxx>
#include <GProp_GProps.hxx>
#include <Precision.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <TopoDS_Iterator.hxx>
#include <optional>
#include <BRepLProp_CLProps.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <GC_MakeArcOfCircle.hxx>
#include <BRep_Tool.hxx>
#include <GeomAdaptor_Curve.hxx>
#include <Geom_BoundedCurve.hxx>
#include <Geom_BezierCurve.hxx>
#include <GeomConvert.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomLib.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <Base/BoundBox.h>

#include <Base/Exception.h>

namespace PartDesign
{
ThinEndpointJet thinEndpointJet(const TopoDS_Edge& edge, bool after, bool requireC2)
{
    double first, last;
    const auto curve = BRep_Tool::Curve(edge, first, last);
    if (curve.IsNull()) {
        throw Base::ValueError("Endpoint extension requires a 3D curve");
    }

    const double parameter = after ? last : first;
    ThinEndpointJet jet;
    curve->D1(parameter, jet.point, jet.velocity);
    jet.tangent = jet.velocity;
    if (jet.tangent.Magnitude() <= gp::Resolution()) {
        BRepAdaptor_Curve adaptor(edge);
        BRepLProp_CLProps properties(adaptor, parameter, 2, Precision::Confusion());
        if (!properties.IsTangentDefined()) {
            throw Base::ValueError(
                "Endpoint has no defined tangent; use Extend Off or repair the profile"
            );
        }
        if (requireC2) {
            throw Base::ValueError("C2 extension requires a regular spline endpoint; use C1 or repair coincident end poles");
        }
        gp_Dir limitingTangent;
        properties.Tangent(limitingTangent);
        jet.tangent = gp_Vec(limitingTangent);
    }

    if (requireC2) {
        curve->D2(parameter, jet.point, jet.velocity, jet.acceleration);
    }

    if (!after) {
        jet.tangent.Reverse();
    }
    jet.tangent.Normalize();
    return jet;
}

Handle(Geom_Curve) makeThinC2Transition(const ThinEndpointJet& jet, double reach)
{
    double transition = reach;
    if (jet.acceleration.Magnitude() > gp::Resolution()) {
        transition
            = std::min(transition, jet.velocity.SquareMagnitude() / jet.acceleration.Magnitude());
    }
    if (transition <= Precision::Confusion()) {
        throw Base::ValueError(
            "C2 endpoint transition is below modeling tolerance; use C1 or repair the profile"
        );
    }

    const double h = transition / jet.velocity.Magnitude();
    const gp_Vec step = jet.tangent * transition;
    TColgp_Array1OfPnt poles(1, 6);
    poles(1) = jet.point;
    poles(2) = jet.point.Translated(step / 5);
    poles(3) = jet.point.Translated(step * .4 + jet.acceleration * (h * h / 20));
    poles(4) = jet.point.Translated(step * .6);
    poles(5) = jet.point.Translated(step * .8);
    poles(6) = jet.point.Translated(step);
    return GeomConvert::CurveToBSplineCurve(new Geom_BezierCurve(poles));
}

gp_Dir thinDirectionTowardBody(
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    const gp_Dir& normal,
    const Part::TopoShape* sweepProfile
)
{
    GProp_GProps properties;
    BRepGProp::LinearProperties(profile.getShape(), properties);
    if (body.isNull() || properties.Mass() <= Precision::Confusion()) {
        throw Base::ValueError("Automatic rib direction requires a profile and a previous solid");
    }

    const gp_Pnt center = properties.CentreOfMass();

    // Intersect first, rather than projecting a nearest 3D point: the latter
    // need not lie on the body after projection into the profile plane.
    BRepAlgoAPI_Section section(body.getShape(), gp_Pln(center, normal));
    if (!section.IsDone() || section.Shape().IsNull()) {
        throw Base::ValueError("The body does not cross the profile plane; set a reference direction");
    }

    BRepExtrema_DistShapeShape distance(BRepBuilderAPI_MakeVertex(center).Vertex(), section.Shape());
    if (!distance.IsDone() || distance.NbSolution() == 0
        || distance.Value() <= Precision::Confusion()) {
        throw Base::ValueError(
            "Cannot infer rib direction at the body boundary; set a reference direction"
        );
    }

    gp_Vec direction;
    std::vector<gp_Pnt> contacts;
    for (int i = 1; i <= distance.NbSolution(); ++i) {
        const auto point = distance.PointOnShape2(i);
        if (std::none_of(contacts.begin(), contacts.end(), [&point](const gp_Pnt& other) {
                return other.Distance(point) <= Precision::Confusion();
            })) {
            contacts.push_back(point);
            direction += gp_Vec(center, point);
        }
    }

    direction -= gp_Vec(normal) * direction.Dot(gp_Vec(normal));
    if (direction.Magnitude() <= Precision::Confusion()) {
        throw Base::ValueError(
            "The body surrounds the profile symmetrically; set a reference direction"
        );
    }

    auto usable = [&](const gp_Vec& candidate) {
        if (candidate.Magnitude() <= Precision::Confusion()) {
            return false;
        }
        // A whole edge parallel to growth sweeps zero area. Test its exact
        // projected bounds, including geometrically straight spline edges.
        gp_Trsf frame;
        frame.SetTransformation(gp_Ax3(center, normal, gp_Dir(candidate.Crossed(gp_Vec(normal)))));
        const auto& construction = sweepProfile ? *sweepProfile : profile;
        for (const auto& edge : construction.getSubTopoShapes(TopAbs_EDGE)) {
            const auto measured = Part::TopoShape().makeElementTransform(edge, frame);
            const auto bounds = measured.getBoundBoxOptimal();
            if (bounds.MaxX - bounds.MinX <= Precision::Confusion()) {
                return false;
            }
        }
        return true;
    };

    if (usable(direction)) {
        return gp_Dir(direction);
    }

    // The nearest body patch is not necessarily a usable fill target. Search
    // other section edges in distance order, never a hard-coded world axis.
    double best = std::numeric_limits<double>::max();
    gp_Vec alternative;
    std::vector<gp_Vec> alternatives;
    for (const auto& edge : Part::TopoShape(section.Shape()).getSubTopoShapes(TopAbs_EDGE)) {
        BRepExtrema_DistShapeShape candidateDistance(
            BRepBuilderAPI_MakeVertex(center).Vertex(),
            edge.getShape()
        );
        for (int i = 1; candidateDistance.IsDone() && i <= candidateDistance.NbSolution(); ++i) {
            gp_Vec candidate(center, candidateDistance.PointOnShape2(i));
            candidate -= gp_Vec(normal) * candidate.Dot(gp_Vec(normal));
            const double length = candidate.Magnitude();
            if (length <= best + Precision::Confusion() && usable(candidate)) {
                if (length < best - Precision::Confusion()) {
                    best = length;
                    alternatives.clear();
                }
                if (std::none_of(alternatives.begin(), alternatives.end(), [&candidate](const gp_Vec& other) {
                        return (candidate - other).Magnitude() <= Precision::Confusion();
                    })) {
                    alternatives.push_back(candidate);
                }
            }
        }
    }

    if (best == std::numeric_limits<double>::max()) {
        throw Base::ValueError(
            "No automatic direction can extrude every profile edge; set a reference direction"
        );
    }

    for (const auto& candidate : alternatives) {
        alternative += candidate;
    }
    if (!usable(alternative)) {
        throw Base::ValueError("Automatic rib direction is ambiguous; set a reference direction");
    }

    return gp_Dir(alternative);
}

gp_Dir thinDirectionTowardReference(const Part::TopoShape& profile, const Part::TopoShape& reference)
{
    if (reference.isNull()) {
        throw Base::ValueError("Toward Reference requires non-empty reference geometry");
    }
    GProp_GProps properties;
    BRepGProp::LinearProperties(profile.getShape(), properties);
    if (properties.Mass() <= Precision::Confusion()) {
        throw Base::ValueError("Toward reference requires a non-empty edge profile");
    }

    const gp_Pnt center = properties.CentreOfMass();
    BRepExtrema_DistShapeShape distance(
        BRepBuilderAPI_MakeVertex(center).Vertex(),
        reference.getShape()
    );
    if (!distance.IsDone() || distance.NbSolution() == 0
        || distance.Value() <= Precision::Confusion()) {
        throw Base::ValueError("Reference does not define a direction from the profile center");
    }

    const gp_Pnt target = distance.PointOnShape2(1);
    for (int i = 2; i <= distance.NbSolution(); ++i) {
        if (target.Distance(distance.PointOnShape2(i)) > Precision::Confusion()) {
            throw Base::ValueError(
                "Reference has multiple nearest points; select a more specific reference"
            );
        }
    }

    return gp_Dir(gp_Vec(center, target));
}

gp_Dir thinReferenceAxis(const Part::TopoShape& reference)
{
    if (reference.isNull()) {
        throw Base::ValueError("Select non-empty geometry for the reference direction");
    }

    if (reference.shapeType() == TopAbs_EDGE) {
        BRepAdaptor_Curve curve(TopoDS::Edge(reference.getShape()));
        switch (curve.GetType()) {
            case GeomAbs_Line:
                return curve.Line().Direction();
            case GeomAbs_Circle:
                return curve.Circle().Axis().Direction();
            case GeomAbs_Ellipse:
                return curve.Ellipse().Axis().Direction();
            case GeomAbs_Hyperbola:
                return curve.Hyperbola().Axis().Direction();
            case GeomAbs_Parabola:
                return curve.Parabola().Axis().Direction();
            default:
                break;
        }
    }
    else if (reference.shapeType() == TopAbs_FACE) {
        const auto face = TopoDS::Face(reference.getShape());
        BRepAdaptor_Surface surface(face);
        switch (surface.GetType()) {
            case GeomAbs_Plane: {
                auto normal = surface.Plane().Axis().Direction();
                if (face.Orientation() == TopAbs_REVERSED) {
                    normal.Reverse();
                }
                return normal;
            }
            case GeomAbs_Cylinder:
                return surface.Cylinder().Axis().Direction();
            case GeomAbs_Cone:
                return surface.Cone().Axis().Direction();
            case GeomAbs_Torus:
                return surface.Torus().Axis().Direction();
            case GeomAbs_SurfaceOfRevolution:
                return surface.AxeOfRevolution().Direction();
            default:
                break;
        }
    }
    else if (reference.shapeType() != TopAbs_VERTEX) {
        std::optional<gp_Dir> direction;
        for (TopoDS_Iterator child(reference.getShape()); child.More(); child.Next()) {
            const auto axis = thinReferenceAxis(Part::TopoShape(child.Value()));
            if (direction && !direction->IsParallel(axis, Precision::Angular())) {
                throw Base::ValueError(
                    "Reference has multiple axis directions; select an edge or face"
                );
            }
            if (!direction) {
                direction = axis;
            }
        }
        if (direction) {
            return *direction;
        }
    }

    throw Base::ValueError("Reference has no unique axis or normal; use Toward Reference for points or arbitrary geometry");
}

namespace
{
bool touches(const Part::TopoShape& a, const Part::TopoShape& b)
{
    BRepExtrema_DistShapeShape distance(a.getShape(), b.getShape());
    if (!distance.IsDone()) {
        throw Base::CADKernelError("Cannot classify rib/body contact");
    }

    return distance.Value() <= Precision::Confusion();
}

bool containsEdge(const Part::TopoShape& shape, const Part::TopoShape& edge)
{
    BRepAlgoAPI_Common common(shape.getShape(), edge.getShape());
    if (!common.IsDone()) {
        throw Base::CADKernelError("Cannot classify rib fillet edges");
    }

    GProp_GProps original, overlap;
    BRepGProp::LinearProperties(edge.getShape(), original);
    BRepGProp::LinearProperties(common.Shape(), overlap);
    return original.Mass() > Precision::Confusion()
        && std::abs(original.Mass() - overlap.Mass()) <= Precision::Confusion();
}
}  // namespace

Part::TopoShape extendThinProfile(
    const Part::TopoShape& profile,
    const Part::TopoShape& body,
    bool naturalCurve,
    long tag,
    bool throughBody
)
{
    if (body.isNull() || !body.hasSubShape(TopAbs_SOLID)) {
        throw Base::ValueError("Endpoint extension requires a target body");
    }

    auto edges = profile.getSubTopoShapes(TopAbs_EDGE);
    auto split = profile;
    if (edges.size() > 1) {
        std::vector<std::vector<Part::TopoShape>> history;
        split.makeElementGeneralFuse(edges, history, 0.0, "RibExtensionGraph");
    }

    TopTools_IndexedDataMapOfShapeListOfShape graph;
    TopExp::MapShapesAndAncestors(split.getShape(), TopAbs_VERTEX, TopAbs_EDGE, graph);

    std::vector<Part::TopoShape> result {profile};
    auto bounds = body.getBoundBox();
    bounds.Add(profile.getBoundBox());
    const double reach = 2 * bounds.CalcDiagonalLength();

    for (int v = 1; v <= graph.Extent(); ++v) {
        if (graph.FindFromIndex(v).Extent() != 1) {
            continue;
        }

        const auto vertex = TopoDS::Vertex(graph.FindKey(v));
        const auto edge = TopoDS::Edge(graph.FindFromIndex(v).First());
        TopoDS_Vertex firstVertex, lastVertex;
        TopExp::Vertices(edge, firstVertex, lastVertex);
        if (firstVertex.IsSame(lastVertex)) {
            continue;
        }

        Part::TopoShape endpoint(tag, profile.Hasher, vertex);
        if (!throughBody && touches(endpoint, body)) {
            continue;
        }

        const bool after = vertex.IsSame(lastVertex);
        double first, last;
        auto curve = BRep_Tool::Curve(edge, first, last);
        const double parameter = after ? last : first;
        const auto jet = thinEndpointJet(edge, after, naturalCurve);
        gp_Pnt point = jet.point;
        const gp_Vec tangent = jet.tangent;

        IntCurvesFace_ShapeIntersector intersector;
        intersector.Load(body.getShape(), Precision::Confusion());
        BRepBuilderAPI_MakeEdge builder;

        if (!naturalCurve) {
            intersector.PerformNearest(gp_Lin(point, gp_Dir(tangent)), Precision::Confusion(), reach);
            if (!intersector.IsDone() || intersector.NbPnt() == 0) {
                throw Base::ValueError("A dangling endpoint's tangent does not reach the target body");
            }

            builder = BRepBuilderAPI_MakeEdge(
                point,
                throughBody ? point.Translated(tangent * reach) : intersector.Pnt(1)
            );
        }
        else {
            auto basis = Handle(Geom_TrimmedCurve)::DownCast(curve);
            if (!basis.IsNull()) {
                curve = basis->BasisCurve();
            }

            double limit;
            if (curve->IsPeriodic()) {
                const double available = throughBody
                    ? curve->Period() - (last - first) - Precision::PConfusion()
                    : curve->Period();
                limit = parameter + (after ? 1 : -1) * available;
            }
            else {
                auto bounded = Handle(Geom_BoundedCurve)::DownCast(curve);
                if (!bounded.IsNull()) {
                    // A long ExtendCurveToPoint can curl back or move the join
                    // parameter. Instead construct an independent quintic Hermite
                    // continuation, retaining the original profile verbatim.
                    // Its initial derivatives match D1*h and D2*h*h; its far
                    // end is C2 with a straight tangent ray. Limit the transition
                    // scale by the endpoint jet, not by the particular model.

                    const auto continuation = makeThinC2Transition(jet, reach);
                    const gp_Pnt tail = continuation->Value(1);
                    Handle(GeomAdaptor_Curve) adaptor = new GeomAdaptor_Curve(continuation);
                    intersector.Perform(adaptor, 0, 1);

                    double end = 2;
                    for (int i = 1; intersector.IsDone() && i <= intersector.NbPnt(); ++i) {
                        if (intersector.WParameter(i) > Precision::PConfusion()) {
                            end = std::min(end, intersector.WParameter(i));
                        }
                    }

                    if (throughBody) {
                        BRepBuilderAPI_MakeEdge transitionEdge(continuation);
                        result.push_back(
                            Part::TopoShape(tag, profile.Hasher)
                                .makeElementShape(transitionEdge, {split, body}, "RibC2Transition")
                        );
                        builder = BRepBuilderAPI_MakeEdge(tail, tail.Translated(tangent * reach));
                    }
                    else if (end <= 1) {
                        builder = BRepBuilderAPI_MakeEdge(continuation, 0, end);
                    }
                    else {
                        intersector.PerformNearest(gp_Lin(tail, gp_Dir(tangent)), 0, reach);
                        if (!intersector.IsDone() || intersector.NbPnt() == 0) {
                            throw Base::ValueError(
                                "A dangling endpoint's C2 extension does not reach the target body"
                            );
                        }
                        BRepBuilderAPI_MakeEdge transitionEdge(continuation);
                        result.push_back(
                            Part::TopoShape(tag, profile.Hasher)
                                .makeElementShape(transitionEdge, {split, body}, "RibC2Transition")
                        );
                        builder = BRepBuilderAPI_MakeEdge(tail, intersector.Pnt(1));
                    }

                    result.push_back(
                        Part::TopoShape(tag, profile.Hasher)
                            .makeElementShape(builder, {split, body}, "RibEndpointExtension")
                    );
                    continue;
                }
                else {
                    gp_Vec derivative;
                    curve->D1(parameter, point, derivative);
                    limit = parameter + (after ? 1 : -1) * reach / derivative.Magnitude();
                }
            }

            Handle(GeomAdaptor_Curve) adaptor = new GeomAdaptor_Curve(curve);
            const double lower = std::min(parameter, limit);
            const double upper = std::max(parameter, limit);

            // OCCT reports periodic intersections in the curve's native period,
            // even when a continuation crosses the parameter seam. Map them
            // into this endpoint's continuation interval before sorting hits.
            intersector.Perform(
                adaptor,
                curve->IsPeriodic() ? curve->FirstParameter() : lower,
                curve->IsPeriodic() ? curve->FirstParameter() + curve->Period() : upper
            );

            std::vector<double> hits;
            double delta = std::numeric_limits<double>::max();
            for (int i = 1; intersector.IsDone() && i <= intersector.NbPnt(); ++i) {
                double hit = intersector.WParameter(i);
                if (curve->IsPeriodic()) {
                    hit = lower + std::fmod(hit - lower, curve->Period());
                    if (hit < lower) {
                        hit += curve->Period();
                    }
                }
                if (hit < lower - Precision::PConfusion() || hit > upper + Precision::PConfusion()) {
                    continue;
                }
                const double distance = std::abs(hit - parameter);
                if (distance > Precision::PConfusion()) {
                    hits.push_back(distance);
                    delta = std::min(delta, distance);
                }
            }

            if (hits.empty()) {
                throw Base::ValueError(
                    "A dangling endpoint's natural curve does not reach the target body"
                );
            }

            double end = throughBody ? limit : parameter + (after ? delta : -delta);
            if (throughBody && curve->IsPeriodic()) {
                // Do not wrap both ends around the unused period: they could
                // overlap each other. Continue into the first body barrier,
                // stopping within that first intersection interval. Continuing
                // to the exit can wrap behind the body and fold the swept face.
                double next = std::abs(limit - parameter);
                for (const double distance : hits) {
                    if (distance > delta + Precision::PConfusion() && distance < next) {
                        next = distance;
                    }
                }
                const double extent = (delta + next) / 2;
                end = parameter + (after ? extent : -extent);
            }
            builder
                = BRepBuilderAPI_MakeEdge(curve, std::min(parameter, end), std::max(parameter, end));
        }

        result.push_back(
            Part::TopoShape(tag, profile.Hasher)
                .makeElementShape(builder, {split, body}, "RibEndpointExtension")
        );
    }

    return Part::TopoShape(tag, profile.Hasher)
        .makeElementCompound(
            result,
            "RibExtendedProfile",
            Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape
        );
}

void checkThinExtensionBoundary(
    const Part::TopoShape& wall,
    const Part::TopoShape& extendedProfile,
    const gp_Dir& normal,
    const gp_Dir& growth
)
{
    const auto graph = makeThinProfileGraph(extendedProfile, wall.Tag);
    auto bounds = wall.getBoundBox();
    bounds.Add(extendedProfile.getBoundBox());
    const double reach = 4 * bounds.CalcDiagonalLength();

    const gp_Dir guardNormal(gp_Vec(normal).Crossed(gp_Vec(growth)));
    for (int i = 1; i <= graph.freeVertices.Extent(); ++i) {
        const auto point = BRep_Tool::Pnt(TopoDS::Vertex(graph.freeVertices(i)));
        const auto guard
            = BRepBuilderAPI_MakeFace(gp_Pln(point, guardNormal), -reach, reach, -reach, reach).Face();
        if (touches(wall, Part::TopoShape(guard))) {
            throw Base::ValueError(
                "A rib endpoint is not bounded by the body across the full thickness"
            );
        }
    }
}

Part::TopoShape roundThinExtrusionEnds(
    const Part::TopoShape& tool,
    const Part::TopoShape& profile,
    const gp_Dir& direction,
    double length,
    double sideA,
    double sideB,
    bool reversed,
    long tag
)
{
    const auto chains = makeThinProfileGraph(profile, tag);
    const auto directed = Part::TopoShape(tag, profile.Hasher)
                              .makeElementCompound(
                                  chains.chains,
                                  "RibCapGraph",
                                  Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape
                              );

    TopTools_IndexedDataMapOfShapeListOfShape graph;
    TopExp::MapShapesAndAncestors(directed.getShape(), TopAbs_VERTEX, TopAbs_EDGE, graph);
    std::vector<Part::TopoShape> pieces {tool};
    for (int i = 1; i <= graph.Extent(); ++i) {
        if (graph.FindFromIndex(i).Extent() != 1) {
            continue;
        }
        const auto vertex = TopoDS::Vertex(graph.FindKey(i));
        const auto directedEdge = graph.FindFromIndex(i).First();
        const auto edge = TopoDS::Edge(directedEdge.Oriented(TopAbs_FORWARD));
        TopoDS_Vertex first, last;
        TopExp::Vertices(edge, first, last);
        if (first.IsSame(last)) {
            continue;
        }
        const bool atEnd = vertex.IsSame(last);
        BRepAdaptor_Curve curve(edge);
        gp_Pnt point;
        gp_Vec tangent;
        curve.D1(atEnd ? curve.LastParameter() : curve.FirstParameter(), point, tangent);
        const gp_Vec axis(direction);
        tangent -= axis * tangent.Dot(axis);
        tangent.Normalize();
        const gp_Vec normal = axis.Crossed(tangent)
            * (directedEdge.Orientation() == TopAbs_REVERSED ? -1 : 1);
        const double radius = (sideA + sideB) / 2;
        const auto center = point.Translated(normal * ((sideA - sideB) / 2));
        const auto a = point.Translated(normal * sideA);
        const auto b = point.Translated(-normal * sideB);
        const auto middle = center.Translated(tangent * (atEnd ? radius : -radius));
        BRepBuilderAPI_MakeEdge arc(GC_MakeArcOfCircle(a, middle, b).Value());
        BRepBuilderAPI_MakeEdge diameter(b, a);
        BRepBuilderAPI_MakeWire boundary(arc.Edge(), diameter.Edge());
        BRepBuilderAPI_MakeFace face(boundary.Wire());
        auto cap
            = Part::TopoShape(tag, profile.Hasher).makeElementShape(face, {profile}, "RibRoundEnd");
        cap = cap.makeElementPrism(axis * (reversed ? -length : length), "RibRoundEndPrism");
        pieces.push_back(cap);
    }
    return pieces.size() == 1
        ? tool
        : Part::TopoShape(tag, profile.Hasher).makeElementFuse(pieces, "RibCappedWall");
}

Part::TopoShape trimThinExtrusionToBoundary(
    const Part::TopoShape& tool,
    const Part::TopoShape& body,
    const Part::TopoShape& source,
    const gp_Vec& travel,
    long tag,
    bool requireTermination
)
{
    if (body.isNull() || !body.hasSubShape(TopAbs_FACE)) {
        throw Base::ValueError("Rib/Web requires a target body or face for boundary termination");
    }

    Part::TopoShape remainder;
    if (body.hasSubShape(TopAbs_SOLID)) {
        remainder = tool.makeElementCut(body, "RibBoundary");
    }
    else {
        BRepAlgoAPI_Splitter splitter;
        TopTools_ListOfShape arguments, targets;
        arguments.Append(tool.getShape());
        targets.Append(body.getShape());
        splitter.SetArguments(arguments);
        splitter.SetTools(targets);
        splitter.Build();
        remainder = Part::TopoShape(tag, tool.Hasher)
                        .makeElementShape(splitter, {tool, body}, "RibTargetSplit");
    }

    auto farSource = source;
    gp_Trsf translation;
    translation.SetTranslation(travel);
    farSource.move(translation);

    std::vector<Part::TopoShape> selected;
    for (auto solid : remainder.getSubTopoShapes(TopAbs_SOLID)) {
        if (!touches(solid, source)) {
            continue;
        }
        if (requireTermination && touches(solid, farSource)) {
            throw Base::ValueError("Rib/Web footprint misses the next body boundary");
        }
        selected.push_back(solid);
    }

    if (selected.empty()) {
        throw Base::ValueError("No rib material exists between the profile and target body");
    }

    return Part::TopoShape(tag, tool.Hasher)
        .makeElementCompound(
            selected,
            "RibTrim",
            Part::TopoShape::SingleShapeCompoundCreationPolicy::returnShape
        );
}

Part::TopoShape finishThinExtrusion(
    const Part::TopoShape& result,
    const Part::TopoShape& body,
    const Part::TopoShape& tool,
    double rootRadius,
    double exposedRadius,
    long tag
)
{
    if (rootRadius < 0 || exposedRadius < 0) {
        throw Base::ValueError("Rib fillet radii cannot be negative");
    }

    Part::TopoShape finished = result;

    // Classify against the body's boundary, not its volume: for a Pocket,
    // interior tool edges belong to the removed volume but are not junctions.
    Part::TopoShape boundary;
    if (!body.isNull()) {
        boundary = Part::TopoShape(tag, body.Hasher)
                       .makeElementCompound(body.getSubTopoShapes(TopAbs_FACE), "ThinBodyBoundary");
    }

    for (bool root : {true, false}) {
        const double radius = root ? rootRadius : exposedRadius;
        if (radius <= Precision::Confusion()) {
            continue;
        }

        std::vector<Part::TopoShape> edges;
        for (const auto& edge : finished.getSubTopoShapes(TopAbs_EDGE)) {
            if (!containsEdge(tool, edge)) {
                continue;
            }

            bool onBody = !boundary.isNull() && containsEdge(boundary, edge);
            bool oldEdge = false;
            if (onBody) {
                for (const auto& old : body.getSubTopoShapes(TopAbs_EDGE)) {
                    if (containsEdge(old, edge)) {
                        oldEdge = true;
                        break;
                    }
                }
            }

            if ((root && onBody && !oldEdge) || (!root && !onBody)) {
                edges.push_back(edge);
            }
        }

        if (edges.empty()) {
            throw Base::ValueError(
                root ? "No root edges available for the requested fillet"
                     : "No exposed edges available for the requested fillet"
            );
        }

        finished = Part::TopoShape(tag, result.Hasher)
                       .makeElementFillet(
                           finished,
                           edges,
                           radius,
                           radius,
                           root ? "RibRootFillet" : "RibEdgeFillet"
                       );
        if (!finished.isValid()) {
            throw Base::CADKernelError("The requested rib fillet produces invalid geometry");
        }
    }

    return finished;
}
}  // namespace PartDesign
