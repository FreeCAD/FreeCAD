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

}  // namespace PartDesign
