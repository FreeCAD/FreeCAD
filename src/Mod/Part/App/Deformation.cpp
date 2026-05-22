// SPDX-License-Identifier: LGPL-2.1-or-later

/*************************************************************************** \
 *   Copyright (c) 2026 F. Foinant-Willig <flachyjoe@gmail.com>            * \
 *                                                                         * \
 *   This file is part of the FreeCAD CAx development system.              * \
 *                                                                         * \
 *   This library is free software; you can redistribute it and/or         * \
 *   modify it under the terms of the GNU Library General Public           * \
 *   License as published by the Free Software Foundation; either          * \
 *   version 2 of the License, or (at your option) any later version.      * \
 *                                                                         * \
 *   This library  is distributed in the hope that it will be useful,      * \
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        * \
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         * \
 *   GNU Library General Public License for more details.                  * \
 *                                                                         * \
 *   You should have received a copy of the GNU Library General Public     * \
 *   License along with this library; see the file COPYING.LIB. If not,    * \
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         * \
 *   Suite 330, Boston, MA  02111-1307, USA                                * \
 *                                                                         * \
 ***************************************************************************/

#include "Deformation.h"
#include "Base/Exception.h"
#include "Canonizer.h"

#include "Base/Console.h"

using namespace Part;

// NOLINTBEGIN(readability-math-missing-parentheses)

gp_Pnt Deformation::twist(gp_Pnt from, double pitch, gp_Vec direction, gp_Pnt origin)
{
    if (std::abs(pitch) <= Precision::Confusion()) {
        return from;
    }

    direction.Normalize();
    gp_Trsf trsf;
    trsf.SetRotation(gp_Quaternion(direction, gp_Vec(1., 0., 0.)));

    from.Transform(trsf);

    gp_Pnt result;
    result = twistAlongX(from, pitch, origin);
    return result.Transformed(trsf.Inverted());
}

gp_Pnt Deformation::twistAlongX(gp_Pnt from, double pitch, gp_Pnt origin)
{
    if (std::abs(pitch) <= Precision::Confusion()) {
        return from;
    }

    const auto alpha = 2. * std::numbers::pi / pitch;
    const gp_Vec translated(origin, from);
    const auto x = from.X();
    const auto y = translated.Y() * cos(alpha * translated.X())
        - translated.Z() * sin(alpha * translated.X());
    const auto z = translated.Y() * sin(alpha * translated.X())
        + translated.Z() * cos(alpha * translated.X());

    return {x, y + origin.Y(), z + origin.Z()};
}

gp_Pnt Deformation::twistAlongY(gp_Pnt from, double pitch, gp_Pnt origin)
{
    if (std::abs(pitch) <= Precision::Confusion()) {
        return from;
    }
    const auto alpha = 2. * std::numbers::pi / pitch;
    const gp_Vec translated(origin, from);
    const auto x = translated.X() * cos(alpha * translated.Y())
        + translated.Z() * sin(alpha * translated.Y());
    const auto y = from.Y();
    const auto z = -translated.X() * sin(alpha * translated.Y())
        + translated.Z() * cos(alpha * translated.Y());

    return {x + origin.X(), y, z + origin.Z()};
}

gp_Pnt Deformation::twistAlongZ(gp_Pnt from, double pitch, gp_Pnt origin)
{
    if (std::abs(pitch) <= Precision::Confusion()) {
        return from;
    }
    const auto alpha = 2. * std::numbers::pi / pitch;
    const gp_Vec translated(origin, from);
    const auto x = translated.X() * cos(alpha * translated.Z())
        - translated.Y() * sin(alpha * translated.Z());
    const auto y = translated.X() * sin(alpha * translated.Z())
        + translated.Y() * cos(alpha * translated.Z());
    const auto z = from.Z();

    return {x + origin.X(), y + origin.Y(), z};
}

gp_Pnt Deformation::bendAlongCurve(gp_Pnt from, const BRepAdaptor_Curve& curve, double factor, gp_Vec direction)
{
    direction.Normalize();
    gp_Trsf trsf;
    trsf.SetRotation(gp_Quaternion(direction, gp_Vec(1., 0., 0.)));

    from.Transform(trsf);

    gp_Pnt result;
    result = bendXAlongCurve(from, curve, factor);
    return result.Transformed(trsf.Inverted());
}

gp_Pnt Deformation::bendXAlongCurve(gp_Pnt from, const BRepAdaptor_Curve& curve, double factor)
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

    return {x, y, z};
}

TopoDS_Edge Deformation::deform(
    const TopoDS_Edge& edge,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
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
        Approx_ParametrizationType::Approx_IsoParametric,
        3,
        samples / 2,
        GeomAbs_C2,
        Precision::Approximation()
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

TopoDS_Wire Deformation::deform(
    const TopoDS_Wire& wire,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
{
    BRepBuilderAPI_MakeWire wireBuilder;

    // for each edge
    for (BRepTools_WireExplorer exp(wire); exp.More(); exp.Next()) {
        auto edge = TopoDS::Edge(exp.Current());
        auto ne = deform(edge, deformFunction, samples);
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

TopoDS_Face Deformation::deform(
    const TopoDS_Face& face,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
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
    TopoDS_Shape face_shape;
    if (nurbs_convert.IsDone()) {
        face_shape = nurbs_convert.Shape();
    }
    else {
        face_shape = face;
    }
    auto surf = BRep_Tool::Surface(TopoDS::Face(face_shape));

    // deform the surface
    double u1, u2, v1, v2;
    surf->Bounds(u1, u2, v1, v2);
    auto uSampleRange = samples - 1;
    auto vSampleRange = samples - 1;
    // for periodic surface,
    // the last vertex shouldn't be equal to the fisrt
    if (surf->IsUPeriodic()) {
        uSampleRange += 2;
    }
    if (surf->IsVPeriodic()) {
        vSampleRange += 2;
    }
    TColgp_Array2OfPnt points(1, samples, 1, samples);
    for (auto i = 0; i < samples; i++) {
        auto u = u1 + (u2 - u1) / uSampleRange * i;
        for (auto j = 0; j < samples; j++) {
            auto v = v1 + (v2 - v1) / vSampleRange * j;
            auto pt = surf->Value(u, v);
            auto np = deformFunction(pt);
            points.SetValue(i + 1, j + 1, np);
            Base::Console().log("deform: %f %f %f", np.X(), np.Y(), np.Z());
        }
    }

    GeomAPI_PointsToBSplineSurface fiter;
    fiter.Init(
        points,
        Approx_IsoParametric,
        3,
        samples / 2,
        GeomAbs_C2,
        Precision::Approximation(),
        surf->IsUPeriodic() || surf->IsVPeriodic()
    );
    auto newSurf = fiter.Surface();

    // create a face with a possible broken wire
    auto tmpFace = BRepBuilderAPI_MakeFace(newSurf, newOuter, /*Inside*/ true).Face();

    // replace the BSplineSurface with a plane if possible
    BRepBuilderAPI_FindPlane findPlane(tmpFace, Precision::Approximation());
    if (findPlane.Found()) {
        auto plane = findPlane.Plane();
        tmpFace = BRepBuilderAPI_MakeFace(plane->Pln(), newOuter, /*Inside*/ true).Face();
    }

    // fix the wire on the face
    ShapeFix_Wire fixer(newOuter, tmpFace, Precision::Approximation());
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
            ShapeFix_Wire fixer(w, resultFace, Precision::Approximation());
            fixer.FixEdgeCurves();
            fixer.FixConnected();
            fixer.FixClosed();
            w = fixer.Wire();
            faceMaker.Add(w);
        }
    }

    // fix the face
    ShapeFix_Face fFixer(faceMaker.Face());
    fFixer.AutoCorrectPrecisionMode() = true;
    fFixer.FixPeriodicDegeneratedMode() = true;
    fFixer.SetPrecision(Precision::Approximation());
    return fFixer.Face();
}

TopoDS_Shell Deformation::deform(
    const TopoDS_Shell& shell,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
{
    TopoDS_Builder topoBuilder;

    TopoDS_Shell resultShell;
    topoBuilder.MakeShell(resultShell);

    // for each face
    for (TopExp_Explorer expF(shell, TopAbs_FACE); expF.More(); expF.Next()) {
        auto face = TopoDS::Face(expF.Current());
        auto resultFace = deform(face, deformFunction, samples);

        topoBuilder.Add(resultShell, Canonizer::canonize(resultFace, Precision::Approximation()));
    }
    // sew the shell
    ShapeUpgrade_ShellSewing sewShell;
    auto sewed = sewShell.ApplySewing(resultShell, Precision::Approximation());
    resultShell = TopoDS::Shell(sewed);

    // fix the shell
    ShapeFix_Shell fixer(resultShell);
    fixer.Perform();
    resultShell = fixer.Shell();

    if (resultShell.IsNull()) {
        throw Base::ValueError("deform: null shell");
    }
    return resultShell;
}

TopoDS_Solid Deformation::deform(
    const TopoDS_Solid& solid,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
{
    // TODO: multi-shell solid (i.e. with internal cavities) are not implemented

    TopExp_Explorer expS(solid, TopAbs_SHELL);
    auto resultShell = deform(TopoDS::Shell(expS.Current()), deformFunction, samples);
    if (resultShell.IsNull()) {
        return {};
    }

    BRepBuilderAPI_MakeSolid solidifer(resultShell);
    auto resultSolid = solidifer.Solid();
    ShapeFix_Solid fixer(resultSolid);
    fixer.SolidFromShell(resultShell);
    fixer.Perform();

    const auto result = fixer.Solid();
    if (result.IsNull()) {
        throw Base::ValueError("deform: null solid");
    }
    return TopoDS::Solid(result);
}

TopoDS_Compound Deformation::deform(
    const TopoDS_Compound& compound,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
{
    TopoDS_Builder topoBuilder;
    TopoDS_Compound result;
    topoBuilder.MakeCompound(result);

    for (TopExp_Explorer expC(compound, TopAbs_SOLID); expC.More(); expC.Next()) {
        auto solid = TopoDS::Solid(expC.Current());
        auto resultSolid = deform(solid, deformFunction, samples);

        topoBuilder.Add(result, resultSolid);
    }
    if (result.IsNull()) {
        throw Base::ValueError("deform: null compound");
    }
    return result;
}

TopoDS_Shape Deformation::deform(
    const TopoDS_Shape& shape,
    const std::function<gp_Pnt(gp_Pnt)>& deformFunction,
    int samples
)
{
    TopoDS_Shape result;
    switch (shape.ShapeType()) {
        case TopAbs_EDGE:
            result = deform(TopoDS::Edge(shape), deformFunction, samples);
            break;
        case TopAbs_WIRE:
            result = deform(TopoDS::Wire(shape), deformFunction, samples);
            break;
        case TopAbs_FACE:
            result = deform(TopoDS::Face(shape), deformFunction, samples);
            break;
        case TopAbs_SHELL:
            result = deform(TopoDS::Shell(shape), deformFunction, samples);
            break;
        case TopAbs_SOLID:
            result = deform(TopoDS::Solid(shape), deformFunction, samples);
            break;
        case TopAbs_COMPOUND:
            result = deform(TopoDS::Compound(shape), deformFunction, samples);
            break;
        default:
            return {};
    }

    ShapeFix_ShapeTolerance tolFixer;
    tolFixer.SetTolerance(result, Precision::Approximation(), TopAbs_SHAPE);
    return result;
}

// NOLINTEND(readability-math-missing-parentheses)
