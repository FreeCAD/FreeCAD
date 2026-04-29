// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include <BRepAdaptor_Surface.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <Base/Exception.h>

#include "FeatureFlex.h"

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Builder.hxx>
#include <BRepBuilderAPI_FindPlane.hxx>
#include <BRepBuilderAPI_GTransform.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_NurbsConvert.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepBuilderAPI_Copy.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRepLib.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GeomAPI_PointsToBSpline.hxx>
#include <GeomAPI_PointsToBSplineSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_BSplineSurface.hxx>
#include <GeomConvert.hxx>
#include <Geom_Circle.hxx>
#include <Geom_ConicalSurface.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Ellipse.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Geom_SphericalSurface.hxx>
#include <ShapeAnalysis_CanonicalRecognition.hxx>
#include <ShapeFix_Solid.hxx>
#include <ShapeFix_Wire.hxx>
#include <ShapeUpgrade_ShellSewing.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Builder.hxx>
#include <TopoDS_Shell.hxx>
#include <gp_Pnt.hxx>
#include <gp_GTrsf.hxx>
#include <gp_Trsf.hxx>
#include <Precision.hxx>

using namespace Part;

PROPERTY_SOURCE(Part::Flex, Part::Feature)

Flex::Flex()
{
    ADD_PROPERTY_TYPE(Base, (nullptr), "Flex", App::Prop_None, "Shape to deform");
    ADD_PROPERTY_TYPE(Pitch, (1.0), "Flex", App::Prop_None, "Pitch for the twist deformation");
    ADD_PROPERTY_TYPE(Samples, (10), "Flex", App::Prop_None, "Samples count for geometry approximation");
}

short Flex::mustExecute() const
{
    if (Base.isTouched() || Pitch.isTouched() || Samples.isTouched()) {
        return 1;
    }
    return 0;
}

Flex::FlexParameters Flex::computeFinalParameters()
{
    Flex::FlexParameters result;
    result.pitch = Pitch.getValue();
    result.samples = Samples.getValue();

    return result;
}

TopoShape Flex::FlexShape(const TopoShape& source, const Flex::FlexParameters& params)
{
    TopoShape result;

    return twist(source, params);
}

namespace
{

Handle(Geom_Curve) getSimplestCurve(TopoDS_Edge shape, float tol)
{
    ShapeAnalysis_CanonicalRecognition canonRecon(shape);

    gp_Lin line;
    if (canonRecon.IsLine(tol, line)) {
        return new Geom_Line(line);
    }
    gp_Circ circle;
    if (canonRecon.IsCircle(tol, circle)) {
        return new Geom_Circle(circle);
    }
    gp_Elips ellipse;
    if (canonRecon.IsEllipse(tol, ellipse)) {
        return new Geom_Ellipse(ellipse);
    }
    BRepAdaptor_Curve adapt(shape);
    return adapt.Curve().Curve();
}

Handle(Geom_Surface) getSimplestSurface(TopoDS_Face shape, float tol)
{
    ShapeAnalysis_CanonicalRecognition canonRecon(shape);

    gp_Pln plane;
    if (canonRecon.IsPlane(tol, plane)) {
        return new Geom_Plane(plane);
    }
    gp_Cylinder cylinder;
    if (canonRecon.IsCylinder(tol, cylinder)) {
        return new Geom_CylindricalSurface(cylinder);
    }
    gp_Cone cone;
    if (canonRecon.IsCone(tol, cone)) {
        return new Geom_ConicalSurface(cone);
    }
    gp_Sphere sphere;
    if (canonRecon.IsSphere(tol, sphere)) {
        return new Geom_SphericalSurface(sphere);
    }
    BRepAdaptor_Surface adapt(shape);
    return adapt.Surface().Surface();
}

/*
 * Return an edge based on a canonical curve if possible
 * else the initial edge
 * Canonical curves are Circle, Ellipse and Line
 */
TopoDS_Edge canonize(TopoDS_Edge edge, float tol)
{

    BRepAdaptor_Curve initialCurve(edge);
    auto simplestCurve = getSimplestCurve(edge, tol);
    if (!simplestCurve || simplestCurve->IsInstance("Geom_BSplineCurve")) {
        return edge;
    }

    auto P1 = initialCurve.Value(initialCurve.FirstParameter());
    auto P2 = initialCurve.Value(initialCurve.LastParameter());

    // recreate an edge
    TopoDS_Edge result;
    BRepBuilderAPI_MakeEdge mkBuilder(simplestCurve, P1, P2);
    mkBuilder.Build();
    if (mkBuilder.IsDone()) {
        result = TopoDS::Edge(mkBuilder.Shape());
        result.Orientation(edge.Orientation());
    }
    return result;
}

/*
 * Base the edges of the wire on canonical curves if possible
 */
TopoDS_Wire canonize(TopoDS_Wire wire, float tol)
{
    TopoDS_Wire result(wire);

    BRepBuilderAPI_MakeWire wireBuilder;

    for (BRepTools_WireExplorer exp(wire); exp.More(); exp.Next()) {
        auto edge = TopoDS::Edge(exp.Current());
        auto ne = canonize(edge, tol);
        wireBuilder.Add(ne);
    }

    wireBuilder.Build();

    if (wireBuilder.IsDone()) {
        result = wireBuilder.Wire();
    }
    result.Orientation(wire.Orientation());
    return result;
}

/*
 * Return a face based on a canonical surface if possible
 * else on the initial face surface
 * All the return face edges are also based on canonical curves if possible
 * Canonical surfaces are Cone, Cylinder, Plane and Sphere
 */
TopoDS_Face canonize(const TopoDS_Face& face, float tol)
{
    auto simplestSurf = getSimplestSurface(face, tol);

    auto outerWire = BRepTools::OuterWire(face);
    auto simplOW = canonize(outerWire, tol);

    // recreate a face
    BRepBuilderAPI_MakeFace faceMaker(simplestSurf, simplOW, /*Inside*/ true);

    // compute holes
    for (TopExp_Explorer exp(face, TopAbs_WIRE); exp.More(); exp.Next()) {
        auto wire = TopoDS::Wire(exp.Current());

        if (wire.IsNotEqual(outerWire)) {
            auto simplW = canonize(wire, tol);
            faceMaker.Add(simplW);
        }
    }

    return faceMaker.Face();
}

gp_Pnt twistAlongX(gp_Pnt from, float pitch)
{
    auto alpha = 4.0 * atan(1.0) / pitch;
    auto x = from.X();
    auto y = from.Y() * cos(alpha * from.X()) - from.Z() * sin(alpha * from.X());
    auto z = from.Y() * sin(alpha * from.X()) + from.Z() * cos(alpha * from.X());

    return gp_Pnt(x, y, z);
}

gp_Pnt bendXAlongCurve(gp_Pnt from, BRepAdaptor_Curve curve, float factor)
{
    auto u = curve.FirstParameter()
        + from.X() * factor * (curve.LastParameter() - curve.FirstParameter());
    gp_Pnt ptCurve;
    gp_Vec axCurve;
    curve.D1(u, ptCurve, axCurve);
    axCurve.Normalize();
    auto x = ptCurve.X() - axCurve.Y() * from.Y() - axCurve.Z() * from.Z();
    auto y = from.Y() + ptCurve.Y();
    auto z = from.Z() + ptCurve.Z();

    return gp_Pnt(x, y, z);
}

TopoDS_Edge deform(TopoDS_Edge edge, std::function<gp_Pnt(gp_Pnt)> deformFunction, int samples)
{
    // discretize
    BRepAdaptor_Curve adapt(edge);
    double first = adapt.FirstParameter();
    double last = adapt.LastParameter();
    GCPnts_UniformAbscissa discretizer;
    discretizer.Initialize(adapt, samples, first, last);
    int nbPoints = discretizer.NbPoints();
    TColgp_Array1OfPnt points(1, nbPoints);
    TColgp_Array1OfPnt deformPoints(1, nbPoints);

    // deform
    for (int i = 1; i <= nbPoints; i++) {
        auto p = adapt.Value(discretizer.Parameter(i));
        points(i) = p;
        deformPoints(i) = deformFunction(p);
    }

    // approximate the points
    GeomAPI_PointsToBSpline fit(
        deformPoints,
        Approx_ParametrizationType::Approx_ChordLength,
        3,
        samples / 2,
        GeomAbs_Shape::GeomAbs_C2,
        1e-6
    );
    auto resultCurve = fit.Curve();

    // recreate an edge
    BRepBuilderAPI_MakeEdge mkBuilder(
        resultCurve,
        resultCurve->FirstParameter(),
        resultCurve->LastParameter()
    );
    auto ne = TopoDS::Edge(mkBuilder.Shape());
    ne.Orientation(edge.Orientation());
    return ne;
}

TopoDS_Wire deform(TopoDS_Wire wire, std::function<gp_Pnt(gp_Pnt)> deformFunction, int samples)
{
    std::vector<TopoDS_Edge> newEdges;

    // for each edge
    for (BRepTools_WireExplorer exp(wire); exp.More(); exp.Next()) {
        auto edge = TopoDS::Edge(exp.Current());
        auto ne = deform(edge, deformFunction, samples);
        newEdges.push_back(ne);
    }

    BRepBuilderAPI_MakeWire wireBuilder;
    for (auto& ne : newEdges) {
        wireBuilder.Add(ne);
    }
    wireBuilder.Build();

    TopoDS_Wire newWire;
    if (wireBuilder.IsDone()) {
        newWire = wireBuilder.Wire();
    }
    newWire.Orientation(wire.Orientation());
    return newWire;
}

TopoDS_Face deform(TopoDS_Face face, std::function<gp_Pnt(gp_Pnt)> deformFunction, int samples)
{
    TopoDS_Face resultFace;

    // deform the outerwire
    auto outerwire = BRepTools::OuterWire(face);
    auto newOuter = deform(outerwire, deformFunction, samples);
    if (newOuter.IsNull()) {
        return resultFace;
    }

    // get the surface
    BRepBuilderAPI_NurbsConvert nurbs_convert;
    nurbs_convert.Perform(face);
    TopoDS_Shape face_shape = nurbs_convert.Shape();
    auto surf = BRep_Tool::Surface(TopoDS::Face(face_shape));
    auto bSurf = GeomConvert::SurfaceToBSplineSurface(surf);

    // deform the surface
    double u1, u2, v1, v2;
    bSurf->Bounds(u1, u2, v1, v2);
    if (!bSurf->IsUPeriodic()) {
        auto du = u2 - u1;
        u1 -= du;
        u2 += du;
    }
    if (!bSurf->IsVPeriodic()) {
        auto dv = v2 - v1;
        v1 -= dv;
        v2 += dv;
    }
    TColgp_Array2OfPnt points(1, samples, 1, samples);
    for (auto i = 1; i <= samples; i++) {
        auto u = u1 + (u2 - u1) / samples * i;
        for (auto j = 1; j <= samples; j++) {
            auto v = v1 + (v2 - v1) / samples * j;
            auto pt = bSurf->Value(u, v);
            points(i, j) = deformFunction(pt);
        }
    }
    GeomAPI_PointsToBSplineSurface fiter(points, 3, samples / 2, GeomAbs_C2, 1e-6);
    auto newSurf = fiter.Surface();

    if (bSurf->IsUPeriodic()) {
        newSurf->SetUPeriodic();
    }
    if (bSurf->IsVPeriodic()) {
        newSurf->SetVPeriodic();
    }

    // create a face with a possible broken wire
    auto tmpFace = BRepBuilderAPI_MakeFace(newSurf, newOuter, /*Inside*/ true).Face();

    // replace the BSplineSurface with a plane if possible
    BRepBuilderAPI_FindPlane findPlane(tmpFace, 1e-6);
    if (findPlane.Found()) {
        auto plane = findPlane.Plane();
        tmpFace = BRepBuilderAPI_MakeFace(plane->Pln(), newOuter, /*Inside*/ true).Face();
    }

    // fix the wire on the face
    ShapeFix_Wire fixer(newOuter, tmpFace, 1e-6);
    fixer.FixEdgeCurves();
    fixer.FixConnected();
    fixer.FixClosed();
    newOuter = fixer.Wire();

    // recreate the face with the fixed wire
    resultFace = BRepBuilderAPI_MakeFace(newSurf, newOuter, /*Inside*/ true).Face();

    auto check = BRepCheck_Analyzer(resultFace, false, true);
    if (!check.IsValid()) {
        auto res = BRepBuilderAPI_MakeFace(newOuter).Face();
        if (!res.IsNull()) {
            check.Init(res, false);
            if (check.IsValid()) {
                resultFace = res;
            }
        }
    }

    // compute holes
    BRepBuilderAPI_MakeFace faceMaker;
    faceMaker.Init(resultFace);
    for (TopExp_Explorer exp(face, TopAbs_WIRE); exp.More(); exp.Next()) {
        auto wire = TopoDS::Wire(exp.Current());

        if (wire.IsNotEqual(outerwire)) {
            auto w = deform(wire, deformFunction, samples);
            // fix the wire on the face
            ShapeFix_Wire fixer(w, resultFace, 1e-6);
            fixer.FixEdgeCurves();
            fixer.FixConnected();
            fixer.FixClosed();
            w = fixer.Wire();
            faceMaker.Add(w);
        }
    }

    return faceMaker.Face();
}

TopoDS_Shell deform(TopoDS_Shell shell, std::function<gp_Pnt(gp_Pnt)> deformFunction, int samples)
{
    TopoDS_Builder topoBuilder;

    TopoDS_Shell resultShell;
    topoBuilder.MakeShell(resultShell);

    // for each face
    for (TopExp_Explorer expF(shell, TopAbs_FACE); expF.More(); expF.Next()) {
        auto face = TopoDS::Face(expF.Current());
        auto resultFace = deform(face, deformFunction, samples);

        topoBuilder.Add(resultShell, canonize(resultFace, 1e-6));
    }

    ShapeUpgrade_ShellSewing sewShell;
    auto sewed = sewShell.ApplySewing(resultShell);
    if (!sewed.IsNull()) {
        resultShell = TopoDS::Shell(sewed);
        return resultShell;
    }
    return resultShell;
}

}  // namespace


TopoShape Flex::twist(const TopoShape& source, const Flex::FlexParameters& params)
{
    double pitch = params.pitch;
    TopoDS_Shape shape = source.getShape();
    auto func = [pitch](gp_Pnt pt) {
        return twistAlongX(pt, pitch);
    };
    TopoShape transTopo;
    try {
        TopExp_Explorer expS(shape, TopAbs_SHELL);
        auto resultShell = deform(TopoDS::Shell(expS.Current()), func, params.samples);
        BRepBuilderAPI_MakeSolid solidifer(resultShell);

        auto resultSolid = solidifer.Solid();
        ShapeFix_Solid fixer(resultSolid);
        fixer.SolidFromShell(resultShell);
        fixer.Perform();
        resultSolid = TopoDS::Solid(fixer.Solid());
        transTopo.setShape(resultSolid);
    }
    catch (...) {
        Base::Console().warning("FeatureFlex failed on twist\n");
        return transTopo;
    }
    return transTopo;
}

App::DocumentObjectExecReturn* Flex::execute()
{
    //    Base::Console().message("FS::execute()\n");
    App::DocumentObject* link = Base.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }

    try {
        Flex::FlexParameters params = computeFinalParameters();
        TopoShape result = FlexShape(
            Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform),
            params
        );
        this->Shape.setValue(result);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
