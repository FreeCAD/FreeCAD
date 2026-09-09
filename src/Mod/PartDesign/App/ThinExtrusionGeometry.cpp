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

}  // namespace PartDesign
