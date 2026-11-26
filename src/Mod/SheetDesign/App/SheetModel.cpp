// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 sliptonic <shopinthewoods@gmail.com>
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <algorithm>
#include <map>
#include <cmath>

#include <BOPAlgo_GlueEnum.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAlgoAPI_Common.hxx>
#include <BRepAlgoAPI_Cut.hxx>
#include <BRepAlgoAPI_Fuse.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakePolygon.hxx>
#include <BRepBuilderAPI_Transform.hxx>
#include <BRepClass_FaceClassifier.hxx>
#include <BRepGProp.hxx>
#include <BRepPrimAPI_MakePrism.hxx>
#include <BRepPrimAPI_MakeRevol.hxx>
#include <BRep_Builder.hxx>
#include <Bnd_Box.hxx>
#include <GProp_GProps.hxx>
#include <ShapeUpgrade_UnifySameDomain.hxx>
#include <BRepBuilderAPI_MakeSolid.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepFill.hxx>
#include <BRepLProp_SLProps.hxx>
#include <BRepLib.hxx>
#include <BRepTools.hxx>
#include <BRepTools_WireExplorer.hxx>
#include <Geom2dAdaptor_Curve.hxx>
#include <Geom2dConvert.hxx>
#include <Geom2d_BSplineCurve.hxx>
#include <Geom2d_Line.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <GeomAPI.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_Curve.hxx>
#include <TColStd_Array1OfInteger.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TColgp_Array1OfPnt.hxx>
#include <Geom_CylindricalSurface.hxx>
#include <Geom_Plane.hxx>
#include <TopoDS_Shell.hxx>
#include <TopoDS_Wire.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_MapOfShape.hxx>
#include <BRep_Tool.hxx>
#include <gp_Cylinder.hxx>
#include <deque>
#include <TopLoc_Location.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <gp_Ax1.hxx>
#include <gp_Circ.hxx>
#include <gp_Pln.hxx>
#include <gp_Vec2d.hxx>

#include <Base/Exception.h>

#include "SheetModel.h"


using namespace SheetDesign;

namespace
{

const double tol = 1.0e-6;

gp_Pnt to3d(const gp_Pnt2d& p, double z = 0.0)
{
    return {p.X(), p.Y(), z};
}

/// A quadrilateral face in the plane z = 0 with the normal +z.
TopoDS_Face makeFlatQuad(const gp_Pnt2d& a, const gp_Pnt2d& b, const gp_Pnt2d& c, const gp_Pnt2d& d)
{
    BRepBuilderAPI_MakePolygon poly(to3d(a), to3d(b), to3d(c), to3d(d), Standard_True);
    BRepBuilderAPI_MakeFace mk(gp_Pln(gp::XOY()), poly.Wire(), Standard_True);
    if (!mk.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make a flat face");
    }
    return mk.Face();
}

double distanceToSegment(const gp_Pnt2d& p, const gp_Pnt2d& s, const gp_Pnt2d& e)
{
    gp_Vec2d se(s, e);
    const double len2 = se.SquareMagnitude();
    if (len2 < tol * tol) {
        return p.Distance(s);
    }
    double u = gp_Vec2d(s, p).Dot(se) / len2;
    u = std::clamp(u, 0.0, 1.0);
    return p.Distance(s.Translated(se * u));
}

/// True if the two segments are on the same line and have a common length.
bool segmentsOverlap(const gp_Pnt2d& a, const gp_Pnt2d& b, const gp_Pnt2d& c, const gp_Pnt2d& d)
{
    gp_Vec2d ab(a, b);
    const double len = ab.Magnitude();
    if (len < tol) {
        return false;
    }
    ab /= len;
    const gp_Vec2d ac(a, c);
    const gp_Vec2d ad(a, d);
    if (std::fabs(ab.Crossed(ac)) > tol || std::fabs(ab.Crossed(ad)) > tol) {
        return false;
    }
    const double u1 = std::min(ab.Dot(ac), ab.Dot(ad));
    const double u2 = std::max(ab.Dot(ac), ab.Dot(ad));
    return std::min(len, u2) - std::max(0.0, u1) > tol;
}

bool isInside(const TopoDS_Face& face, const gp_Pnt2d& p)
{
    BRepClass_FaceClassifier cls(face, to3d(p), tol);
    return cls.State() == TopAbs_IN;
}

double area(const TopoDS_Shape& shape)
{
    GProp_GProps props;
    BRepGProp::SurfaceProperties(shape, props);
    return props.Mass();
}

bool boxesOverlap(const TopoDS_Shape& a, const TopoDS_Shape& b)
{
    Bnd_Box boxA;
    Bnd_Box boxB;
    BRepBndLib::Add(a, boxA);
    BRepBndLib::Add(b, boxB);
    return !boxA.IsOut(boxB);
}

double volumeOf(const TopoDS_Shape& shape)
{
    // The fixed integration is exact for planes and cylinders. A free-form face, for example
    // the side of a round opening in a bend, needs the adaptive integration: the fixed one was
    // wrong by 4 % on such a part.
    bool freeForm = false;
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More() && !freeForm; ex.Next()) {
        const GeomAbs_SurfaceType type = BRepAdaptor_Surface(TopoDS::Face(ex.Current())).GetType();
        freeForm = type != GeomAbs_Plane && type != GeomAbs_Cylinder;
    }
    GProp_GProps props;
    if (freeForm) {
        BRepGProp::VolumeProperties(shape, props, 1.0e-6);
    }
    else {
        BRepGProp::VolumeProperties(shape, props);
    }
    return props.Mass();
}

/// Make sure that a solid has its material on the inner side. Gives the volume of the solid.
TopoDS_Shape orientSolid(const TopoDS_Shape& solid, double& volume)
{
    const double signedVolume = volumeOf(solid);
    volume = std::fabs(signedVolume);
    return signedVolume < 0.0 ? solid.Reversed() : solid;
}

/// The unit normal of the boundary segment a-b that points out of the face.
gp_Vec2d outwardNormal(const TopoDS_Face& face, const gp_Pnt2d& a, const gp_Pnt2d& b)
{
    gp_Vec2d dir(a, b);
    dir.Normalize();
    gp_Vec2d n(dir.Y(), -dir.X());
    const gp_Pnt2d mid((a.X() + b.X()) / 2.0, (a.Y() + b.Y()) / 2.0);
    const double probe = 1.0e-4;
    if (isInside(face, mid.Translated(n * probe))) {
        n.Reverse();
    }
    else if (!isInside(face, mid.Translated(n * -probe))) {
        throw Base::ValueError("Sheet design: the edge is not on the boundary of a panel");
    }
    return n;
}

std::vector<TopoDS_Face> facesOf(const TopoDS_Shape& shape)
{
    std::vector<TopoDS_Face> faces;
    for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
        faces.push_back(TopoDS::Face(ex.Current()));
    }
    return faces;
}

/// The frame of a bend in the flat space of its parent panel.
struct BendFrame
{
    gp_Pnt center;  ///< A point of the bend axis, at the start of the bend line.
    gp_Dir axisDir;
    gp_Vec2d along;     ///< Unit vector of the bend line.
    gp_Vec2d across;    ///< Unit vector from the parent panel into the bend.
    double sign {1.0};  ///< Direction of the bend line against the axis.

    BendFrame(const Bend& bend, double t)
        : center(to3d(bend.start, bend.up ? t + bend.radius : -bend.radius))
        , axisDir(bend.normal.Y(), -bend.normal.X(), 0.0)
        , along(bend.start, bend.end)
        , across(bend.normal)
    {
        if (!bend.up) {
            axisDir.Reverse();
        }
        along.Normalize();
        sign = axisDir.X() * along.X() + axisDir.Y() * along.Y();
    }
};

/// The solid of a bend with the full rectangle as its zone: an exact solid of revolution.
TopoDS_Shape revolveBend(const Bend& bend, double t)
{
    const BendFrame frame(bend, t);
    BRepBuilderAPI_MakePolygon
        poly(to3d(bend.start), to3d(bend.end), to3d(bend.end, t), to3d(bend.start, t), Standard_True);
    BRepBuilderAPI_MakeFace section(poly.Wire(), Standard_True);
    BRepPrimAPI_MakeRevol revol(section.Face(), gp_Ax1(frame.center, frame.axisDir), bend.angle);
    if (!revol.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make a bend");
    }
    return revol.Shape();
}

/// Make a face on a surface from wires. The first wire is the outer wire.
TopoDS_Face faceOnSurface(const Handle(Geom_Surface) & surface, const std::vector<TopoDS_Wire>& wires)
{
    BRepBuilderAPI_MakeFace maker(surface, wires.front(), Standard_True);
    if (!maker.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make a face of a bend");
    }
    for (std::size_t i = 1; i < wires.size(); ++i) {
        // A face from the hole wire gives the wire the same turn direction as the outer wire.
        BRepBuilderAPI_MakeFace hole(surface, wires[i], Standard_True);
        if (!hole.IsDone()) {
            throw Base::CADKernelError("Sheet design: cannot make an opening of a bend");
        }
        maker.Add(TopoDS::Wire(BRepTools::OuterWire(hole.Face()).Reversed()));
    }
    return maker.Face();
}

TopoDS_Face planarFace(const std::vector<TopoDS_Edge>& edges)
{
    BRepBuilderAPI_MakeWire wire;
    for (const TopoDS_Edge& edge : edges) {
        wire.Add(edge);
    }
    if (!wire.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make a side wire of a bend");
    }
    BRepBuilderAPI_MakeFace face(wire.Wire(), Standard_True);
    if (!face.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make a side face of a bend");
    }
    return face.Face();
}

/**
 * The solid of a bend with a zone of any shape.
 *
 * Each boundary curve of the zone becomes a curve on the inner and on the outer cylinder of the
 * bend, with the same parameters on the two cylinders. A side face joins the two curves. With
 * the radius zero there is no inner cylinder, and the side faces go to the bend axis.
 */
TopoDS_Shape wrapBendFace(const Bend& bend, const TopoDS_Face& zone, double t)
{
    const BendFrame frame(bend, t);
    const double inner = bend.radius;
    const double outer = bend.radius + t;
    const bool hasInner = inner > 1.0e-7;
    const double turn = bend.angle / bend.allowance;

    // U is the angle from the start of the bend. V is the position along the axis.
    const gp_Dir xDir = bend.up ? gp_Dir(0.0, 0.0, -1.0) : gp_Dir(0.0, 0.0, 1.0);
    const gp_Ax3 system(frame.center, frame.axisDir, xDir);
    Handle(Geom_CylindricalSurface) outerSurface = new Geom_CylindricalSurface(system, outer);
    Handle(Geom_CylindricalSurface) innerSurface;
    if (hasInner) {
        innerSurface = new Geom_CylindricalSurface(system, inner);
    }
    auto toUV = [&](const gp_Pnt2d& p) {
        const gp_Vec2d w(bend.start, p);
        return gp_Pnt2d(w.Dot(frame.across) * turn, frame.sign * w.Dot(frame.along));
    };
    auto axisPoint = [&](double v) {
        return frame.center.Translated(gp_Vec(frame.axisDir) * v);
    };

    std::vector<TopoDS_Wire> outerWires;
    std::vector<TopoDS_Wire> innerWires;
    BRepBuilderAPI_Sewing sewing(1.0e-5);

    std::vector<TopoDS_Wire> zoneWires;
    const TopoDS_Wire zoneOuter = BRepTools::OuterWire(zone);
    zoneWires.push_back(zoneOuter);
    for (TopExp_Explorer ex(zone, TopAbs_WIRE); ex.More(); ex.Next()) {
        if (!ex.Current().IsSame(zoneOuter)) {
            zoneWires.push_back(TopoDS::Wire(ex.Current()));
        }
    }

    for (const TopoDS_Wire& zoneWire : zoneWires) {
        BRepBuilderAPI_MakeWire outerWire;
        BRepBuilderAPI_MakeWire innerWire;
        for (BRepTools_WireExplorer ex(zoneWire, zone); ex.More(); ex.Next()) {
            const TopoDS_Edge zoneEdge = ex.Current();
            BRepAdaptor_Curve curve(zoneEdge);
            const gp_Pnt first3 = curve.Value(curve.FirstParameter());
            const gp_Pnt last3 = curve.Value(curve.LastParameter());
            const gp_Pnt2d uv0 = toUV(gp_Pnt2d(first3.X(), first3.Y()));
            const gp_Pnt2d uv1 = toUV(gp_Pnt2d(last3.X(), last3.Y()));
            const bool line = curve.GetType() == GeomAbs_Line;

            Handle(Geom2d_Curve) onSurface;
            double firstParam = 0.0;
            double lastParam = 0.0;
            if (line) {
                lastParam = uv0.Distance(uv1);
                if (lastParam < 1.0e-9) {
                    continue;
                }
                onSurface = new Geom2d_Line(uv0, gp_Dir2d(gp_Vec2d(uv0, uv1)));
            }
            else {
                double f = 0.0;
                double l = 0.0;
                Handle(Geom_Curve) curve3d = BRep_Tool::Curve(zoneEdge, f, l);
                Handle(Geom2d_Curve) flat2d = GeomAPI::To2d(curve3d, gp_Pln(gp::XOY()));
                Handle(Geom2d_BSplineCurve)
                    spline = Geom2dConvert::CurveToBSplineCurve(new Geom2d_TrimmedCurve(flat2d, f, l));
                // The map from flat space to the cylinder parameters is affine, thus it is
                // correct to move only the poles.
                for (int i = 1; i <= spline->NbPoles(); ++i) {
                    spline->SetPole(i, toUV(spline->Pole(i)));
                }
                onSurface = spline;
                firstParam = spline->FirstParameter();
                lastParam = spline->LastParameter();
            }

            TopoDS_Edge outerEdge
                = BRepBuilderAPI_MakeEdge(onSurface, outerSurface, firstParam, lastParam).Edge();
            BRepLib::BuildCurves3d(outerEdge);
            outerWire.Add(outerEdge);
            const gp_Pnt o0 = outerSurface->Value(uv0.X(), uv0.Y());
            const gp_Pnt o1 = outerSurface->Value(uv1.X(), uv1.Y());

            TopoDS_Edge innerEdge;
            gp_Pnt i0 = axisPoint(uv0.Y());
            gp_Pnt i1 = axisPoint(uv1.Y());
            if (hasInner) {
                innerEdge
                    = BRepBuilderAPI_MakeEdge(onSurface, innerSurface, firstParam, lastParam).Edge();
                BRepLib::BuildCurves3d(innerEdge);
                innerWire.Add(innerEdge);
                i0 = innerSurface->Value(uv0.X(), uv0.Y());
                i1 = innerSurface->Value(uv1.X(), uv1.Y());
            }

            // The side face between the inner and the outer curve.
            const bool alongAxis = line && std::fabs(uv0.X() - uv1.X()) < 1.0e-9;
            const bool aroundAxis = line && std::fabs(uv0.Y() - uv1.Y()) < 1.0e-9;
            if (alongAxis) {
                BRepBuilderAPI_MakePolygon poly(o0, o1, i1, i0, Standard_True);
                sewing.Add(BRepBuilderAPI_MakeFace(poly.Wire(), Standard_True).Face());
            }
            else if (aroundAxis) {
                std::vector<TopoDS_Edge> edges {outerEdge, BRepBuilderAPI_MakeEdge(o1, i1).Edge()};
                if (hasInner) {
                    edges.push_back(innerEdge);
                }
                edges.push_back(BRepBuilderAPI_MakeEdge(i0, o0).Edge());
                sewing.Add(planarFace(edges));
            }
            else if (hasInner) {
                sewing.Add(BRepFill::Face(innerEdge, outerEdge));
            }
            else if (line) {
                sewing.Add(BRepFill::Face(BRepBuilderAPI_MakeEdge(i0, i1).Edge(), outerEdge));
            }
            else {
                // With the radius zero the side face goes from the curve on the outer cylinder
                // to the bend axis. The axis curve has the same parameters as the outer curve,
                // thus each line of the side face points to the axis.
                Handle(Geom2d_BSplineCurve) uv = Handle(Geom2d_BSplineCurve)::DownCast(onSurface);
                bool rising = false;
                bool falling = false;
                double previous = uv->Value(firstParam).Y();
                const int samples = 64;
                for (int k = 1; k <= samples; ++k) {
                    const double v = uv->Value(firstParam + (lastParam - firstParam) * k / samples).Y();
                    rising = rising || v > previous + 1.0e-9;
                    falling = falling || v < previous - 1.0e-9;
                    previous = v;
                }
                if (rising && falling) {
                    // The side face would go along the axis in two directions. The solid then
                    // touches itself on the axis.
                    throw Base::ValueError(
                        "Sheet design: with the bend radius zero, a curved edge in a bend zone "
                        "must go in one direction along the bend line. An opening inside the "
                        "zone is not available"
                    );
                }
                if (!rising && !falling) {
                    sewing.Add(planarFace(
                        {outerEdge,
                         BRepBuilderAPI_MakeEdge(o1, i1).Edge(),
                         BRepBuilderAPI_MakeEdge(i0, o0).Edge()}
                    ));
                }
                else {
                    const int count = uv->NbPoles();
                    TColgp_Array1OfPnt poles(1, count);
                    TColStd_Array1OfReal weights(1, count);
                    for (int k = 1; k <= count; ++k) {
                        poles(k) = axisPoint(uv->Pole(k).Y());
                        weights(k) = uv->Weight(k);
                    }
                    TColStd_Array1OfReal knots(1, uv->NbKnots());
                    TColStd_Array1OfInteger mults(1, uv->NbKnots());
                    uv->Knots(knots);
                    uv->Multiplicities(mults);
                    Handle(Geom_BSplineCurve) axisCurve = new Geom_BSplineCurve(
                        poles,
                        weights,
                        knots,
                        mults,
                        uv->Degree(),
                        uv->IsPeriodic()
                    );
                    sewing.Add(BRepFill::Face(BRepBuilderAPI_MakeEdge(axisCurve).Edge(), outerEdge));
                }
            }
        }
        if (!outerWire.IsDone() || (hasInner && !innerWire.IsDone())) {
            throw Base::CADKernelError("Sheet design: cannot make the outline of a bend");
        }
        outerWires.push_back(outerWire.Wire());
        if (hasInner) {
            innerWires.push_back(innerWire.Wire());
        }
    }

    sewing.Add(faceOnSurface(outerSurface, outerWires));
    if (hasInner) {
        sewing.Add(faceOnSurface(innerSurface, innerWires));
    }
    sewing.Perform();
    TopoDS_Shell shell;
    int shells = 0;
    for (TopExp_Explorer ex(sewing.SewedShape(), TopAbs_SHELL); ex.More(); ex.Next()) {
        shell = TopoDS::Shell(ex.Current());
        ++shells;
    }
    if (shells != 1 || !BRep_Tool::IsClosed(shell)) {
        throw Base::CADKernelError("Sheet design: the faces of a bend do not make a closed shell");
    }
    BRepBuilderAPI_MakeSolid solid(shell);
    if (!solid.IsDone()) {
        throw Base::CADKernelError("Sheet design: cannot make the solid of a bend");
    }
    return solid.Solid();
}

/// Put the faces of two zone shapes into one compound.
TopoDS_Shape joinZones(const TopoDS_Shape& first, const TopoDS_Shape& second)
{
    BRep_Builder builder;
    TopoDS_Compound both;
    builder.MakeCompound(both);
    for (const TopoDS_Shape& part : {first, second}) {
        for (const TopoDS_Face& face : facesOf(part)) {
            builder.Add(both, face);
        }
    }
    return both;
}

TopoDS_Shape fuseAll(const std::vector<TopoDS_Shape>& shapes, bool glue = true)
{
    TopTools_ListOfShape arguments;
    TopTools_ListOfShape tools;
    arguments.Append(shapes.front());
    for (std::size_t i = 1; i < shapes.size(); ++i) {
        tools.Append(shapes[i]);
    }
    BRepAlgoAPI_Fuse op;
    op.SetArguments(arguments);
    op.SetTools(tools);
    if (glue) {
        // The parts touch only on coincident faces or edges. The glue mode skips the general
        // intersection of all faces.
        op.SetGlue(BOPAlgo_GlueShift);
    }
    op.SetRunParallel(Standard_True);
    op.Build();
    if (!op.IsDone() || op.HasErrors()) {
        throw Base::CADKernelError("Sheet design: the fuse operation failed");
    }
    return op.Shape();
}

TopoDS_Shape unify(const TopoDS_Shape& shape)
{
    ShapeUpgrade_UnifySameDomain op(shape, Standard_True, Standard_True, Standard_False);
    op.Build();
    return op.Shape();
}

}  // namespace


SheetModel SheetModel::makeBase(
    const TopoDS_Face& flatProfile,
    const gp_Trsf& placement,
    const SheetParams& params,
    const std::string& tag
)
{
    if (params.thickness <= tol) {
        throw Base::ValueError("Sheet design: the thickness must be larger than zero");
    }
    if (params.radius < 0.0 || params.kFactor < 0.0 || params.kFactor > 1.0) {
        throw Base::ValueError("Sheet design: the bend radius or the K-factor is not valid");
    }
    SheetModel model;
    model.params = params;
    Panel panel;
    panel.profile = flatProfile;
    panel.folded = placement;
    panel.tag = tag;
    model.panels.push_back(panel);
    model.addOrigins(flatProfile, gp_Trsf(), tag + "_E", 0);
    return model;
}

void SheetModel::addOrigins(
    const TopoDS_Shape& shape,
    const gp_Trsf& toFlat,
    const std::string& prefix,
    int panel
)
{
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(shape, TopAbs_EDGE, edges);
    for (int i = 1; i <= edges.Extent(); ++i) {
        BRepAdaptor_Curve curve(TopoDS::Edge(edges(i)));
        if (curve.GetType() != GeomAbs_Line) {
            continue;
        }
        const gp_Pnt p = curve.Value(curve.FirstParameter()).Transformed(toFlat);
        const gp_Pnt q = curve.Value(curve.LastParameter()).Transformed(toFlat);
        Origin origin;
        origin.tag = prefix + std::to_string(i);
        origin.panel = panel;
        origin.a = gp_Pnt2d(p.X(), p.Y());
        origin.b = gp_Pnt2d(q.X(), q.Y());
        origins.push_back(origin);
    }
}

std::string SheetModel::segmentTag(int panel, const gp_Pnt2d& a, const gp_Pnt2d& b) const
{
    const gp_Pnt2d mid((a.X() + b.X()) / 2.0, (a.Y() + b.Y()) / 2.0);
    const double loose = 1.0e-5;
    // A later origin replaces an earlier origin on the same line.
    for (auto it = origins.rbegin(); it != origins.rend(); ++it) {
        if (panel >= 0 && it->panel >= 0 && it->panel != panel) {
            continue;
        }
        if (distanceToSegment(mid, it->a, it->b) > loose) {
            continue;
        }
        gp_Vec2d dir(it->a, it->b);
        if (dir.Magnitude() < tol) {
            continue;
        }
        dir.Normalize();
        if (std::fabs(dir.Crossed(gp_Vec2d(it->a, a))) < loose
            && std::fabs(dir.Crossed(gp_Vec2d(it->a, b))) < loose) {
            return it->tag;
        }
    }
    return {};
}

std::string SheetModel::bendLineName(std::size_t bend) const
{
    const Bend& item = bends[bend];
    if (item.closing) {
        // The two panels have their names from other bends.
        return panels[item.parentPanel].tag + "_to_" + panels[item.childPanel].tag + "_BendLine";
    }
    return panels[item.childPanel].tag + "_BendLine";
}

namespace
{

/// Give each element its name. Elements with the same name get a number, in the sequence of
/// their position, thus the names are unique.
void assignNames(
    std::map<std::string, std::vector<std::pair<std::pair<double, double>, int>>>& groups,
    std::vector<std::pair<int, std::string>>& names
)
{
    for (auto& group : groups) {
        std::sort(group.second.begin(), group.second.end());
        int count = 0;
        for (const auto& item : group.second) {
            ++count;
            names.emplace_back(
                item.second,
                count == 1 ? group.first : group.first + "_" + std::to_string(count)
            );
        }
    }
    std::sort(names.begin(), names.end());
}

}  // namespace

ElementNames SheetModel::nameFolded(const TopoDS_Shape& folded) const
{
    using Groups = std::map<std::string, std::vector<std::pair<std::pair<double, double>, int>>>;
    ElementNames names;
    const double t = params.thickness;

    Groups edgeGroups;
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(folded, TopAbs_EDGE, edges);
    for (int i = 1; i <= edges.Extent(); ++i) {
        EdgeRef ref;
        if (!findEdge(TopoDS::Edge(edges(i)), ref)) {
            continue;
        }
        const std::string tag = segmentTag(ref.panel, ref.a, ref.b);
        if (tag.empty()) {
            continue;
        }
        edgeGroups[tag + (ref.top ? "_T" : "_B")].push_back(
            {{ref.a.X() + ref.b.X(), ref.a.Y() + ref.b.Y()}, i}
        );
    }
    assignNames(edgeGroups, names.edges);

    Groups faceGroups;
    TopTools_IndexedMapOfShape faces;
    TopExp::MapShapes(folded, TopAbs_FACE, faces);
    for (int i = 1; i <= faces.Extent(); ++i) {
        const TopoDS_Face face = TopoDS::Face(faces(i));
        BRepAdaptor_Surface surface(face);
        if (surface.GetType() != GeomAbs_Plane) {
            continue;
        }
        const gp_Pln plane = surface.Plane();
        TopExp_Explorer vertex(face, TopAbs_VERTEX);
        if (!vertex.More()) {
            continue;
        }
        const gp_Pnt corner = BRep_Tool::Pnt(TopoDS::Vertex(vertex.Current()));
        std::string name;
        std::pair<double, double> position {0.0, 0.0};

        // A top or bottom face of a panel.
        for (const Panel& panel : panels) {
            const gp_Trsf inverse = panel.folded.Inverted();
            const gp_Dir normal = plane.Axis().Direction().Transformed(inverse);
            if (std::fabs(normal.Z()) < 1.0 - 1.0e-9) {
                continue;
            }
            const gp_Pnt p = corner.Transformed(inverse);
            const bool bottom = std::fabs(p.Z()) < tol;
            const bool top = std::fabs(p.Z() - t) < tol;
            if (!bottom && !top) {
                continue;
            }
            BRepClass_FaceClassifier cls(panel.profile, gp_Pnt(p.X(), p.Y(), 0.0), 1.0e-5);
            if (cls.State() == TopAbs_OUT) {
                continue;
            }
            name = panel.tag + (top ? "_Top" : "_Bottom");
            position = {p.X(), p.Y()};
            break;
        }
        // A side face: it has the top and the bottom edge of one boundary segment.
        if (name.empty()) {
            for (TopExp_Explorer ex(face, TopAbs_EDGE); ex.More(); ex.Next()) {
                EdgeRef ref;
                if (!findEdge(TopoDS::Edge(ex.Current()), ref)) {
                    continue;
                }
                const std::string tag = segmentTag(ref.panel, ref.a, ref.b);
                if (!tag.empty()) {
                    name = tag + "_S";
                    position = {ref.a.X() + ref.b.X(), ref.a.Y() + ref.b.Y()};
                    break;
                }
            }
        }
        if (!name.empty()) {
            faceGroups[name].push_back({position, i});
        }
    }
    assignNames(faceGroups, names.faces);
    return names;
}

ElementNames SheetModel::nameFlat(const TopoDS_Shape& flat) const
{
    std::map<std::string, std::vector<std::pair<std::pair<double, double>, int>>> groups;
    ElementNames names;
    TopTools_IndexedMapOfShape edges;
    TopExp::MapShapes(flat, TopAbs_EDGE, edges);
    for (int i = 1; i <= edges.Extent(); ++i) {
        BRepAdaptor_Curve curve(TopoDS::Edge(edges(i)));
        if (curve.GetType() != GeomAbs_Line) {
            continue;
        }
        const gp_Pnt p = curve.Value(curve.FirstParameter());
        const gp_Pnt q = curve.Value(curve.LastParameter());
        const gp_Pnt2d a(p.X(), p.Y());
        const gp_Pnt2d b(q.X(), q.Y());
        const std::string tag = segmentTag(-1, a, b);
        if (!tag.empty()) {
            groups[tag + "_F"].push_back({{a.X() + b.X(), a.Y() + b.Y()}, i});
        }
    }
    assignNames(groups, names.edges);
    return names;
}

double SheetModel::bendAllowance(double angle) const
{
    return bendAllowance(angle, params.radius);
}

double SheetModel::bendAllowance(double angle, double radius) const
{
    return (radius + params.kFactor * params.thickness) * angle;
}

gp_Trsf SheetModel::rootPlacement() const
{
    return panels.empty() ? gp_Trsf() : panels.front().folded;
}

bool SheetModel::findEdge(const TopoDS_Edge& foldedEdge, EdgeRef& ref) const
{
    BRepAdaptor_Curve curve(foldedEdge);
    if (curve.GetType() != GeomAbs_Line) {
        return false;
    }
    const gp_Pnt first = curve.Value(curve.FirstParameter());
    const gp_Pnt last = curve.Value(curve.LastParameter());
    const double t = params.thickness;

    for (std::size_t i = 0; i < panels.size(); ++i) {
        const gp_Trsf inverse = panels[i].folded.Inverted();
        const gp_Pnt p = first.Transformed(inverse);
        const gp_Pnt q = last.Transformed(inverse);
        const bool bottom = std::fabs(p.Z()) < tol && std::fabs(q.Z()) < tol;
        const bool top = std::fabs(p.Z() - t) < tol && std::fabs(q.Z() - t) < tol;
        if (!bottom && !top) {
            continue;
        }
        const gp_Pnt2d a(p.X(), p.Y());
        const gp_Pnt2d b(q.X(), q.Y());
        if (a.Distance(b) < tol) {
            continue;
        }
        for (TopExp_Explorer ex(panels[i].profile, TopAbs_EDGE); ex.More(); ex.Next()) {
            BRepAdaptor_Curve boundary(TopoDS::Edge(ex.Current()));
            if (boundary.GetType() != GeomAbs_Line) {
                continue;
            }
            const gp_Pnt s3 = boundary.Value(boundary.FirstParameter());
            const gp_Pnt e3 = boundary.Value(boundary.LastParameter());
            const gp_Pnt2d s(s3.X(), s3.Y());
            const gp_Pnt2d e(e3.X(), e3.Y());
            if (distanceToSegment(a, s, e) < tol && distanceToSegment(b, s, e) < tol) {
                ref.panel = static_cast<int>(i);
                ref.a = a;
                ref.b = b;
                ref.top = top;
                return true;
            }
        }
    }
    return false;
}

int SheetModel::addFlange(
    const EdgeRef& ref,
    double length,
    double angle,
    bool up,
    const FlangeOptions& options,
    const std::string& tag
)
{
    const double gap1 = options.gap1;
    const double gap2 = options.gap2;
    const double offset = options.offset;
    const Relief& relief = options.relief;
    if (ref.panel < 0 || ref.panel >= static_cast<int>(panels.size())) {
        throw Base::ValueError("Sheet design: the edge reference is not valid");
    }
    if (angle <= tol || angle > M_PI + tol) {
        throw Base::ValueError("Sheet design: the bend angle must be between 0 and 180 degrees");
    }
    if (length <= tol) {
        throw Base::ValueError("Sheet design: the flange length must be larger than zero");
    }
    if (gap1 < 0.0 || gap2 < 0.0) {
        throw Base::ValueError("Sheet design: a gap cannot be negative");
    }
    gp_Vec2d dir(ref.a, ref.b);
    const double edgeLength = dir.Magnitude();
    if (edgeLength - gap1 - gap2 <= tol) {
        throw Base::ValueError("Sheet design: the gaps are longer than the edge");
    }
    dir /= edgeLength;
    gp_Pnt2d a = ref.a.Translated(dir * gap1);
    gp_Pnt2d b = ref.b.Translated(dir * -gap2);
    if (!options.profile.IsNull()) {
        // The outline of the flange gives the bend line: its part on the edge.
        double low = 0.0;
        double high = 0.0;
        bool found = false;
        for (TopExp_Explorer ex(options.profile, TopAbs_EDGE); ex.More(); ex.Next()) {
            BRepAdaptor_Curve curve(TopoDS::Edge(ex.Current()));
            if (curve.GetType() != GeomAbs_Line) {
                continue;
            }
            const gp_Pnt p3 = curve.Value(curve.FirstParameter());
            const gp_Pnt q3 = curve.Value(curve.LastParameter());
            const gp_Vec2d toP(ref.a, gp_Pnt2d(p3.X(), p3.Y()));
            const gp_Vec2d toQ(ref.a, gp_Pnt2d(q3.X(), q3.Y()));
            if (std::fabs(toP.Crossed(dir)) > 1.0e-5 || std::fabs(toQ.Crossed(dir)) > 1.0e-5) {
                continue;
            }
            const double first = std::min(toP.Dot(dir), toQ.Dot(dir));
            const double last = std::max(toP.Dot(dir), toQ.Dot(dir));
            low = found ? std::min(low, first) : first;
            high = found ? std::max(high, last) : last;
            found = true;
        }
        if (!found || high - low <= tol) {
            throw Base::ValueError(
                "Sheet design: the profile of the flange has no straight side on the selected edge"
            );
        }
        if (low < -1.0e-5 || high > edgeLength + 1.0e-5) {
            throw Base::ValueError(
                "Sheet design: the profile of the flange is longer than the selected edge"
            );
        }
        a = ref.a.Translated(dir * low);
        b = ref.a.Translated(dir * high);
    }

    const gp_Vec2d n = outwardNormal(panels[ref.panel].profile, a, b);

    if (std::fabs(offset) > tol) {
        // Move the bend line: add a rectangle to the parent panel, or remove one from it.
        Panel& parent = panels[ref.panel];
        const TopoDS_Face patch
            = makeFlatQuad(a, b, b.Translated(n * offset), a.Translated(n * offset));
        TopoDS_Shape changed;
        if (offset > 0.0) {
            changed = unify(BRepAlgoAPI_Fuse(parent.profile, patch).Shape());
        }
        else {
            changed = BRepAlgoAPI_Cut(parent.profile, patch).Shape();
        }
        const std::vector<TopoDS_Face> faces = facesOf(changed);
        if (faces.size() != 1) {
            throw Base::ValueError("Sheet design: the offset of the bend divides the panel");
        }
        parent.profile = faces.front();
        parent.solid.Nullify();
        origins.push_back({tag + "_Offset1", ref.panel, a, a.Translated(n * offset)});
        origins.push_back({tag + "_Offset2", ref.panel, b, b.Translated(n * offset)});
        a.Translate(n * offset);
        b.Translate(n * offset);
    }

    if (relief.type != Relief::None) {
        if (relief.width <= tol || relief.depth <= tol) {
            throw Base::ValueError(
                "Sheet design: the width and the depth of a relief must be larger than zero"
            );
        }
        // A relief is a slot in the gap beside the bend. It goes from the bend line into the
        // parent panel by its depth. If the bend line is inside the panel, the slot also goes
        // out to the edge of the panel.
        Panel& parent = panels[ref.panel];
        const double outer = std::max(0.0, -offset) + 1.0;
        TopTools_ListOfShape slots;
        for (int end = 0; end < 2; ++end) {
            const double gap = end == 0 ? gap1 : gap2;
            if (gap <= tol) {
                continue;
            }
            // The slot starts at the end of the bend and goes away from the flange.
            const gp_Pnt2d corner = end == 0 ? a : b;
            const gp_Vec2d away = end == 0 ? -dir : dir;
            const double width = std::min(relief.width, gap);
            const gp_Pnt2d far = corner.Translated(away * width);
            double flatDepth = relief.depth;
            if (relief.type == Relief::Round) {
                // A half circle is the inner end of the slot.
                flatDepth = std::max(relief.depth - width / 2.0, 0.0);
                const gp_Pnt2d center = corner.Translated(away * (width / 2.0) + n * -flatDepth);
                BRepBuilderAPI_MakeEdge circle(gp_Circ(gp_Ax2(to3d(center), gp::DZ()), width / 2.0));
                BRepBuilderAPI_MakeWire wire(circle.Edge());
                slots.Append(
                    BRepBuilderAPI_MakeFace(gp_Pln(gp::XOY()), wire.Wire(), Standard_True).Face()
                );
            }
            slots.Append(makeFlatQuad(
                corner.Translated(n * -flatDepth),
                far.Translated(n * -flatDepth),
                far.Translated(n * outer),
                corner.Translated(n * outer)
            ));
        }
        if (!slots.IsEmpty()) {
            TopTools_ListOfShape parentList;
            parentList.Append(parent.profile);
            BRepAlgoAPI_Cut cutter;
            cutter.SetArguments(parentList);
            cutter.SetTools(slots);
            cutter.Build();
            if (!cutter.IsDone() || cutter.HasErrors()) {
                throw Base::CADKernelError("Sheet design: cannot make the relief");
            }
            const std::vector<TopoDS_Face> faces = facesOf(cutter.Shape());
            if (faces.size() != 1) {
                throw Base::ValueError("Sheet design: the relief divides the panel");
            }
            parent.profile = faces.front();
            parent.solid.Nullify();
            int slotNumber = 0;
            for (const TopoDS_Shape& slot : slots) {
                addOrigins(
                    slot,
                    gp_Trsf(),
                    tag + "_Relief" + std::to_string(++slotNumber) + "_E",
                    ref.panel
                );
            }
        }
    }

    // An edge can hold only one bend.
    for (const Bend& other : bends) {
        if (other.parentPanel == ref.panel && segmentsOverlap(a, b, other.start, other.end)) {
            throw Base::ValueError("Sheet design: the edge already has a bend");
        }
        if (other.childPanel == ref.panel) {
            const gp_Vec2d shift = gp_Vec2d(other.normal) * other.allowance;
            if (segmentsOverlap(a, b, other.start.Translated(shift), other.end.Translated(shift))) {
                throw Base::ValueError("Sheet design: the edge already has a bend");
            }
        }
    }

    const double allowance = bendAllowance(angle);
    // With a miter, each side of the flange is at an angle from the bend line.
    const double slope1 = std::tan(options.miter1);
    const double slope2 = std::tan(options.miter2);
    const bool mitered = options.profile.IsNull()
        && (std::fabs(slope1) > 1.0e-9 || std::fabs(slope2) > 1.0e-9);
    auto corner = [&](bool second, double depth) {
        const gp_Pnt2d& base = second ? b : a;
        const double along = second ? -depth * slope2 : depth * slope1;
        return base.Translated(n * depth + dir * along);
    };
    const gp_Pnt2d a1 = corner(false, allowance);
    const gp_Pnt2d b1 = corner(true, allowance);
    const gp_Pnt2d a2 = corner(false, allowance + length);
    const gp_Pnt2d b2 = corner(true, allowance + length);
    if (gp_Vec2d(a2, b2).Dot(dir) <= tol || gp_Vec2d(a1, b1).Dot(dir) <= tol) {
        throw Base::ValueError("Sheet design: the miter angles leave no width for the flange");
    }
    Panel child;
    child.tag = tag;
    TopoDS_Shape zone;
    if (!options.profile.IsNull()) {
        // Move the outline out of the panel by the offset and the bend allowance.
        GProp_GProps props;
        BRepGProp::SurfaceProperties(options.profile, props);
        const gp_Pnt center = props.CentreOfMass();
        const gp_Vec2d toCenter(ref.a, gp_Pnt2d(center.X(), center.Y()));
        if (toCenter.Dot(n) <= 0.0) {
            throw Base::ValueError(
                "Sheet design: the profile of the flange must be on the outer side of the edge"
            );
        }
        gp_Trsf move;
        const gp_Vec2d shift = n * (offset + allowance);
        move.SetTranslation(gp_Vec(shift.X(), shift.Y(), 0.0));
        child.profile = TopoDS::Face(
            BRepBuilderAPI_Transform(options.profile, move, Standard_True).Shape()
        );
    }
    else {
        child.profile = makeFlatQuad(a1, b1, b2, a2);
        if (mitered) {
            zone = makeFlatQuad(a, b, b1, a1);
        }
    }
    panels.push_back(child);
    const int childIndex = static_cast<int>(panels.size()) - 1;
    if (options.profile.IsNull()) {
        origins.push_back({tag + "_End", childIndex, a2, b2});
        origins.push_back({tag + "_Side1", childIndex, a1, a2});
        origins.push_back({tag + "_Side2", childIndex, b1, b2});
    }
    else {
        addOrigins(panels[childIndex].profile, gp_Trsf(), tag + "_E", childIndex);
    }
    connect(ref.panel, childIndex, a, b, n, angle, up, params.radius, zone);
    const int bendIndex = static_cast<int>(bends.size()) - 1;
    bends[bendIndex].farStart = a2;
    bends[bendIndex].farEnd = b2;

    if (options.autoMiter) {
        // Two flanges on adjacent edges of one panel can overlap in the flat pattern, for
        // example with the bend lines inside the panel, or at an inner corner. Cut the two on
        // the line through the point where their bend lines meet, in the middle between them.
        const double big = 1.0e4;
        auto cutShape = [](const TopoDS_Shape& shape, const TopoDS_Face& tool) {
            BRepAlgoAPI_Cut op(shape, tool);
            if (!op.IsDone() || op.HasErrors()) {
                throw Base::CADKernelError("Sheet design: cannot make the miter");
            }
            return op.Shape();
        };
        auto regionOf = [this](const Bend& bend) {
            return joinZones(bend.strip, panels[bend.childPanel].profile);
        };
        for (int index = 0; index < bendIndex; ++index) {
            Bend& other = bends[index];
            if (other.closing || other.parentPanel != ref.panel) {
                continue;
            }
            gp_Vec2d otherDir(other.start, other.end);
            otherDir.Normalize();
            const double sine = dir.Crossed(otherDir);
            if (std::fabs(sine) < 1.0e-6) {
                continue;
            }
            const TopoDS_Shape mine = regionOf(bends[bendIndex]);
            const TopoDS_Shape theirs = regionOf(other);
            if (!boxesOverlap(mine, theirs) || area(BRepAlgoAPI_Common(mine, theirs).Shape()) <= tol) {
                continue;
            }
            // The point where the two bend lines meet, and the direction between the flanges.
            const double along = gp_Vec2d(a, other.start).Crossed(otherDir) / sine;
            const gp_Pnt2d meet = a.Translated(dir * along);
            gp_Vec2d between = n + gp_Vec2d(other.normal);
            if (between.Magnitude() < 1.0e-6) {
                continue;
            }
            between.Normalize();
            const gp_Vec2d left(-between.Y(), between.X());
            const gp_Pnt2d myMid((a.X() + b.X()) / 2.0, (a.Y() + b.Y()) / 2.0);
            const double mySide = gp_Vec2d(meet, myMid).Dot(left) > 0.0 ? 1.0 : -1.0;
            // The part of the plane to remove from one flange: the side of the other flange,
            // and half of the gap on its own side.
            auto halfPlane = [&](double side) {
                const gp_Pnt2d origin = meet.Translated(left * (-side * options.miterGap / 2.0));
                return makeFlatQuad(
                    origin.Translated(between * -big),
                    origin.Translated(between * big),
                    origin.Translated(between * big + left * (side * big)),
                    origin.Translated(between * -big + left * (side * big))
                );
            };
            auto trim = [&](Bend& bend, double side) {
                const TopoDS_Face tool = halfPlane(-side);
                const std::vector<TopoDS_Face> profile = facesOf(
                    cutShape(panels[bend.childPanel].profile, tool)
                );
                const TopoDS_Shape strip = cutShape(bend.strip, tool);
                if (profile.size() != 1 || facesOf(strip).empty()) {
                    throw Base::ValueError("Sheet design: the miter removes a flange or divides it");
                }
                panels[bend.childPanel].profile = profile.front();
                panels[bend.childPanel].solid.Nullify();
                bend.strip = strip;
                bend.plain = false;
            };
            trim(bends[bendIndex], mySide);
            trim(other, -mySide);
            const gp_Pnt2d seamEnd = meet.Translated(between * big);
            origins.push_back({tag + "_Miter", childIndex, meet, seamEnd});
            origins.push_back({tag + "_Miter", other.childPanel, meet, seamEnd});
        }
    }
    return bendIndex;
}

gp_Trsf SheetModel::afterBend(
    const gp_Trsf& parent,
    const gp_Pnt2d& start,
    const gp_Vec2d& normal,
    double angle,
    bool up,
    double radius
) const
{
    // The child moves back by the bend allowance, then turns around the bend axis.
    const double t = params.thickness;
    const double allowance = bendAllowance(angle, radius);
    gp_Dir axisDir(normal.Y(), -normal.X(), 0.0);  // normal x Z
    if (!up) {
        axisDir.Reverse();
    }
    const gp_Ax1 axis(to3d(start, up ? t + radius : -radius), axisDir);
    gp_Trsf rotation;
    rotation.SetRotation(axis, angle);
    gp_Trsf translation;
    translation.SetTranslation(gp_Vec(-allowance * normal.X(), -allowance * normal.Y(), 0.0));
    return parent * rotation * translation;
}

int SheetModel::addContour(
    const EdgeRef& ref,
    const std::vector<gp_Pnt>& points,
    const FlangeOptions& options,
    const std::string& tag
)
{
    if (ref.panel < 0 || ref.panel >= static_cast<int>(panels.size())) {
        throw Base::ValueError("Sheet design: the edge reference is not valid");
    }
    if (points.size() < 2) {
        throw Base::ValueError("Sheet design: the profile of the contour has no segment");
    }
    const double t = params.thickness;
    const double r = params.radius;
    const double loose = 1.0e-4;
    gp_Vec2d dir(ref.a, ref.b);
    dir.Normalize();
    const gp_Vec2d n = outwardNormal(panels[ref.panel].profile, ref.a, ref.b);

    // The section in the plane at 90 degrees to the edge: u goes out of the panel, z goes up.
    struct SectionPoint
    {
        double along;
        double u;
        double z;
    };
    std::vector<SectionPoint> section;
    for (const gp_Pnt& p : points) {
        const gp_Vec2d w(ref.a, gp_Pnt2d(p.X(), p.Y()));
        section.push_back({w.Dot(dir), w.Dot(n), p.Z()});
    }
    // The line starts at its end that is on the edge.
    if (std::fabs(section.back().u) < std::fabs(section.front().u)) {
        std::reverse(section.begin(), section.end());
    }
    for (const SectionPoint& p : section) {
        if (std::fabs(p.along - section.front().along) > loose) {
            throw Base::ValueError("Sheet design: the profile of the contour must be in a plane at 90 degrees to the edge");
        }
    }
    const bool topSide = std::fabs(section.front().z - t) < loose;
    const bool bottomSide = std::fabs(section.front().z) < loose;
    if (std::fabs(section.front().u) > loose || (!topSide && !bottomSide)) {
        throw Base::ValueError(
            "Sheet design: the profile of the contour must start on the selected edge, on the top "
            "or the bottom side of the panel"
        );
    }

    // Each segment gives one bend and one panel. The line is the outline of one side of the
    // sheet with sharp corners, thus each bend uses a part of the two segments at its corner.
    struct Step
    {
        double length;
        double turn;
        double corner;
    };
    std::vector<Step> steps;
    double direction = 0.0;
    for (std::size_t i = 1; i < section.size(); ++i) {
        const double du = section[i].u - section[i - 1].u;
        const double dz = section[i].z - section[i - 1].z;
        const double length = std::hypot(du, dz);
        if (length < tol) {
            throw Base::ValueError("Sheet design: a segment of the contour has no length");
        }
        const double next = std::atan2(dz, du);
        double turn = next - direction;
        while (turn > M_PI) {
            turn -= 2.0 * M_PI;
        }
        while (turn <= -M_PI) {
            turn += 2.0 * M_PI;
        }
        if (std::fabs(turn) < 1.0e-6) {
            throw Base::ValueError("Sheet design: two segments of the contour are in one line");
        }
        // The side of the sheet that the line shows is the inner side of a bend to that side.
        const bool inner = (turn > 0.0) == topSide;
        steps.push_back({length, turn, (inner ? r : r + t) * std::tan(std::fabs(turn) / 2.0)});
        direction = next;
    }

    int firstBend = -1;
    EdgeRef edge = ref;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const double leg = steps[i].length - steps[i].corner
            - (i + 1 < steps.size() ? steps[i + 1].corner : 0.0);
        if (leg <= tol) {
            throw Base::ValueError("Sheet design: a segment of the contour is too short for its bends");
        }
        FlangeOptions stepOptions;
        std::string stepTag = tag;
        if (i == 0) {
            // The first corner is on the edge: the bend line is inside the panel.
            stepOptions = options;
            stepOptions.offset -= steps[i].corner;
            stepOptions.profile.Nullify();
        }
        else {
            stepOptions.autoMiter = false;
            stepTag += "_S" + std::to_string(i + 1);
        }
        const int bend
            = addFlange(edge, leg, std::fabs(steps[i].turn), steps[i].turn > 0.0, stepOptions, stepTag);
        if (i == 0) {
            firstBend = bend;
        }
        edge.panel = bends[bend].childPanel;
        edge.a = bends[bend].farStart;
        edge.b = bends[bend].farEnd;
        edge.top = true;
    }
    return firstBend;
}

void SheetModel::connect(
    int parent,
    int child,
    const gp_Pnt2d& start,
    const gp_Pnt2d& end,
    const gp_Vec2d& normal,
    double angle,
    bool up,
    double radius,
    const TopoDS_Shape& strip
)
{
    const double allowance = bendAllowance(angle, radius);

    Bend bend;
    bend.parentPanel = parent;
    bend.childPanel = child;
    bend.start = start;
    bend.end = end;
    bend.normal = gp_Dir2d(normal);
    bend.angle = angle;
    bend.up = up;
    bend.radius = radius;
    bend.allowance = allowance;
    bend.plain = strip.IsNull();
    if (strip.IsNull()) {
        bend.strip = makeFlatQuad(
            start,
            end,
            end.Translated(normal * allowance),
            start.Translated(normal * allowance)
        );
    }
    else {
        bend.strip = strip;
    }

    panels[child].folded = afterBend(panels[parent].folded, start, normal, angle, up, radius);
    panels[child].parentBend = static_cast<int>(bends.size());
    panels[child].solid.Nullify();
    bends.push_back(bend);
}

int SheetModel::cut(const TopoDS_Shape& faces, bool flat, const std::string& tag)
{
    const double t = params.thickness;
    int changed = 0;

    int faceNumber = 0;
    for (TopExp_Explorer ex(faces, TopAbs_FACE); ex.More(); ex.Next()) {
        bool used = false;
        ++faceNumber;
        // In flat mode the root panel gives the frame. In folded mode each panel is a candidate.
        const std::size_t frames = flat ? 1 : panels.size();
        for (std::size_t frame = 0; frame < frames; ++frame) {
            BRepBuilderAPI_Transform toFlat(ex.Current(), panels[frame].folded.Inverted(), Standard_True);
            TopoDS_Shape local = toFlat.Shape();
            BRepAdaptor_Surface surface(TopoDS::Face(local));
            if (surface.GetType() != GeomAbs_Plane) {
                continue;
            }
            const gp_Pln plane = surface.Plane();
            if (std::fabs(plane.Axis().Direction().Z()) < 1.0 - 1.0e-9) {
                continue;
            }
            const double z = plane.Location().Z();
            if (z < -tol || z > t + tol) {
                continue;
            }
            if (std::fabs(z) > tol) {
                gp_Trsf drop;
                drop.SetTranslation(gp_Vec(0.0, 0.0, -z));
                local = BRepBuilderAPI_Transform(local, drop, Standard_True).Shape();
            }
            used = true;

            for (Bend& bend : bends) {
                const bool adjacent = bend.parentPanel == static_cast<int>(frame)
                    || bend.childPanel == static_cast<int>(frame);
                if (!flat && !adjacent) {
                    continue;
                }
                if (!boxesOverlap(local, bend.strip)) {
                    continue;
                }
                BRepAlgoAPI_Cut op(bend.strip, local);
                if (!op.IsDone() || op.HasErrors()) {
                    throw Base::CADKernelError("Sheet design: the cut operation failed");
                }
                if (!op.HasModified() && !op.HasDeleted()) {
                    continue;
                }
                if (!flat) {
                    // A profile on a panel is in the plane of that panel. In the bend it has
                    // no clear position.
                    throw Base::ValueError(
                        "Sheet design: a cut across a bend zone must be in the flat space"
                    );
                }
                if (facesOf(op.Shape()).empty()) {
                    throw Base::ValueError("Sheet design: the cut removes a full bend");
                }
                bend.strip = op.Shape();
                bend.plain = false;
                ++changed;
            }

            for (std::size_t i = 0; i < panels.size(); ++i) {
                if (!flat && i != frame) {
                    continue;
                }
                Panel& panel = panels[i];
                if (!boxesOverlap(local, panel.profile)) {
                    continue;
                }
                BRepAlgoAPI_Cut op(panel.profile, local);
                if (!op.IsDone() || op.HasErrors()) {
                    throw Base::CADKernelError("Sheet design: the cut operation failed");
                }
                if (!op.HasModified() && !op.HasDeleted()) {
                    continue;
                }
                TopoDS_Face result;
                int count = 0;
                for (TopExp_Explorer fx(op.Shape(), TopAbs_FACE); fx.More(); fx.Next()) {
                    result = TopoDS::Face(fx.Current());
                    ++count;
                }
                if (count == 0) {
                    throw Base::ValueError("Sheet design: the cut removes a full panel");
                }
                if (count > 1) {
                    throw Base::ValueError("Sheet design: the cut divides a panel into parts");
                }
                panel.profile = result;
                panel.solid.Nullify();
                addOrigins(
                    ex.Current(),
                    panels[frame].folded.Inverted(),
                    tag + "_F" + std::to_string(faceNumber) + "_E",
                    static_cast<int>(i)
                );
                ++changed;
            }
        }
        if (!used) {
            throw Base::ValueError("Sheet design: the cut profile is not in the plane of a panel");
        }
    }
    return changed;
}

TopoDS_Shape SheetModel::makeFolded(bool fuse, bool refine, double* overlap) const
{
    const double t = params.thickness;
    std::vector<TopoDS_Shape> solids;
    solids.reserve(panels.size() + bends.size());
    double partsVolume = 0.0;
    if (overlap) {
        *overlap = 0.0;
    }

    for (const Panel& panel : panels) {
        if (panel.solid.IsNull()) {
            BRepPrimAPI_MakePrism prism(panel.profile, gp_Vec(0.0, 0.0, t));
            panel.solid = orientSolid(prism.Shape(), panel.volume);
        }
        partsVolume += panel.volume;
        solids.push_back(panel.solid.Moved(TopLoc_Location(panel.folded)));
    }

    for (const Bend& bend : bends) {
        if (bend.closing) {
            continue;
        }
        std::vector<TopoDS_Shape> raw;
        if (bend.plain) {
            raw.push_back(revolveBend(bend, t));
        }
        else {
            // A cut can divide the zone of a bend into parts. Each part gives one solid.
            for (const TopoDS_Face& zone : facesOf(bend.strip)) {
                raw.push_back(wrapBendFace(bend, zone, t));
            }
        }
        for (const TopoDS_Shape& part : raw) {
            double bendVolume = 0.0;
            const TopoDS_Shape solid = orientSolid(part, bendVolume);
            partsVolume += bendVolume;
            solids.push_back(solid.Moved(TopLoc_Location(panels[bend.parentPanel].folded)));
        }
    }

    if (solids.size() == 1) {
        // A Part feature replaces the location of its shape with its placement. Thus put the
        // position into the geometry.
        return BRepBuilderAPI_Transform(panels.front().solid, panels.front().folded, Standard_True)
            .Shape();
    }
    if (!fuse) {
        BRep_Builder builder;
        TopoDS_Compound compound;
        builder.MakeCompound(compound);
        for (const TopoDS_Shape& solid : solids) {
            builder.Add(compound, solid);
        }
        return compound;
    }

    // The glue mode is correct only if no two parts have material in the same space. The volume
    // shows this: the fused solid then has the volume of all parts together.
    const double limit = 1.0e-6 * partsVolume + 1.0e-3;
    TopoDS_Shape result;
    double difference = 0.0;
    bool glued = true;
    try {
        result = fuseAll(solids, true);
        difference = partsVolume - volumeOf(result);
    }
    catch (const Base::Exception&) {
        glued = false;
    }
    catch (const Standard_Failure&) {
        glued = false;
    }
    if (!glued || std::fabs(difference) > limit) {
        result = fuseAll(solids, false);
        difference = partsVolume - volumeOf(result);
    }
    if (overlap && difference > limit) {
        *overlap = difference;
    }
    return refine ? unify(result) : result;
}

TopoDS_Shape SheetModel::makeFlat() const
{
    std::vector<TopoDS_Shape> faces;
    faces.reserve(panels.size() + bends.size());
    for (const Panel& panel : panels) {
        faces.push_back(panel.profile);
    }
    for (const Bend& bend : bends) {
        for (const TopoDS_Face& zone : facesOf(bend.strip)) {
            faces.push_back(zone);
        }
    }
    if (faces.size() == 1) {
        return faces.front();
    }
    return unify(fuseAll(faces));
}

std::vector<TopoDS_Edge> SheetModel::bendLines() const
{
    std::vector<TopoDS_Edge> lines;
    lines.reserve(bends.size());
    for (const Bend& bend : bends) {
        const gp_Vec2d half = gp_Vec2d(bend.normal) * (bend.allowance / 2.0);
        BRepBuilderAPI_MakeEdge edge(to3d(bend.start.Translated(half)), to3d(bend.end.Translated(half)));
        lines.push_back(edge.Edge());
    }
    return lines;
}


// ------------------------------------------------------------------------------------------------
// Entry from a finished flat pattern

SheetModel SheetModel::fromFlat(
    const TopoDS_Face& flatProfile,
    const std::vector<BendLine>& lines,
    const gp_Trsf& placement,
    const SheetParams& params,
    const std::string& tag
)
{
    SheetModel model = makeBase(flatProfile, placement, params, tag);
    // The outline is the same for all panels of the divided profile.
    for (Origin& origin : model.origins) {
        origin.panel = -1;
    }
    if (lines.empty()) {
        return model;
    }
    const double looseTol = 1.0e-5;

    Bnd_Box box;
    BRepBndLib::Add(flatProfile, box);
    const double reach = std::sqrt(box.SquareExtent()) + 1.0;

    // One band for each bend line: the bend zone, longer than the profile.
    struct Band
    {
        gp_Pnt2d origin;
        gp_Vec2d dir;
        gp_Vec2d normal;
        double length {0.0};
        double allowance {0.0};
        TopoDS_Face face;
    };
    std::vector<Band> bands;
    for (const BendLine& line : lines) {
        if (line.angle <= tol || line.angle > M_PI + tol) {
            throw Base::ValueError("Sheet design: a bend angle must be between 0 and 180 degrees");
        }
        gp_Vec2d dir(line.a, line.b);
        const double lineLength = dir.Magnitude();
        if (lineLength < tol) {
            throw Base::ValueError("Sheet design: a bend line has no length");
        }
        dir.Normalize();
        Band band;
        band.origin = line.a;
        band.dir = dir;
        band.length = lineLength;
        band.normal = gp_Vec2d(-dir.Y(), dir.X());
        band.allowance = model.bendAllowance(line.angle);
        bands.push_back(band);
    }
    // Where bend lines meet at a point, their zones must not overlap. A line goes through the
    // point if it does not end there, or if a second line continues it on the same axis.
    //   - A line that goes through stops the zones of the other lines at its own zone.
    //   - Two lines that go through get a relief opening where their zones cross. The sheet
    //     cannot fold there.
    //   - A line that only ends at the point, for example the diagonal of a corner web, stops
    //     at its end. Its zone gets what the lines that go through leave.
    // At an end with no other line, a band goes on, thus a short line bends its full region.
    double widestBand = 0.0;
    for (const Band& other : bands) {
        widestBand = std::max(widestBand, other.allowance);
    }
    const double snap = 1.5 * widestBand + looseTol;
    auto endNear = [snap](const Band& item, const gp_Pnt2d& point) {
        const gp_Pnt2d last = item.origin.Translated(item.dir * item.length);
        return std::min(point.Distance(item.origin), point.Distance(last)) <= snap;
    };
    // The largest allowance of a line and of the lines that continue it at the point.
    // Zero: no line continues it.
    auto partnerAllowance = [&](std::size_t j, const gp_Pnt2d& point) {
        const Band& item = bands[j];
        double largest = 0.0;
        for (std::size_t m = 0; m < bands.size(); ++m) {
            const Band& partner = bands[m];
            if (m == j || !endNear(partner, point)) {
                continue;
            }
            const bool parallel = std::fabs(item.dir.Crossed(partner.dir)) < 1.0e-6;
            const bool sameAxis = std::fabs(gp_Vec2d(item.origin, partner.origin).Crossed(item.dir))
                < 1.0e-4;
            if (parallel && sameAxis) {
                largest = std::max({largest, partner.allowance, item.allowance});
            }
        }
        return largest;
    };
    // The allowance of a line that goes through the point. Zero: it does not go through.
    auto throughAllowance = [&](std::size_t j, const gp_Pnt2d& point) {
        const Band& item = bands[j];
        const gp_Vec2d to(item.origin, point);
        if (std::fabs(to.Crossed(item.dir)) > snap) {
            return 0.0;
        }
        const double along = to.Dot(item.dir);
        if (along > snap && along < item.length - snap) {
            return item.allowance;  // The point is in the middle of the line.
        }
        return endNear(item, point) ? partnerAllowance(j, point) : 0.0;
    };

    TopTools_ListOfShape reliefs;
    std::vector<bool> goesThrough(bands.size(), false);
    for (std::size_t i = 0; i < bands.size(); ++i) {
        Band& band = bands[i];
        const gp_Vec2d& dir = band.dir;
        double low = -reach;
        double high = band.length + reach;
        const gp_Pnt2d ends[2] = {band.origin, band.origin.Translated(dir * band.length)};
        for (int e = 0; e < 2; ++e) {
            const double myAllowance = partnerAllowance(i, ends[e]);
            goesThrough[i] = goesThrough[i] || myAllowance > 0.0;
            bool meets = false;
            for (std::size_t j = 0; j < bands.size(); ++j) {
                const Band& other = bands[j];
                const double sine = dir.Crossed(other.dir);
                if (j == i || std::fabs(sine) < 1.0e-6) {
                    continue;
                }
                meets = meets || endNear(other, ends[e]);
                const double otherAllowance = throughAllowance(j, ends[e]);
                if (otherAllowance <= 0.0 || myAllowance <= 0.0) {
                    continue;
                }
                // Two lines that go through: stop at the zone of the other, and make a relief.
                const double mine = gp_Vec2d(band.origin, other.origin).Crossed(other.dir) / sine;
                const double alongMe = otherAllowance / 2.0 / std::fabs(sine);
                const double alongOther = myAllowance / 2.0 / std::fabs(sine);
                if (e == 0) {
                    low = std::max(low, mine + alongMe);
                }
                else {
                    high = std::min(high, mine - alongMe);
                }
                const gp_Pnt2d cross = band.origin.Translated(dir * mine);
                reliefs.Append(makeFlatQuad(
                    cross.Translated(dir * -alongMe + other.dir * -alongOther),
                    cross.Translated(dir * alongMe + other.dir * -alongOther),
                    cross.Translated(dir * alongMe + other.dir * alongOther),
                    cross.Translated(dir * -alongMe + other.dir * alongOther)
                ));
            }
            if (meets && myAllowance <= 0.0) {
                // The line ends at a point with other lines: its zone ends with it.
                if (e == 0) {
                    low = 0.0;
                }
                else {
                    high = band.length;
                }
            }
        }
        if (high - low < tol) {
            throw Base::ValueError(
                "Sheet design: a bend line is too short between two other bend lines"
            );
        }
        const gp_Vec2d half = band.normal * (band.allowance / 2.0);
        const gp_Pnt2d a = band.origin.Translated(dir * low);
        const gp_Pnt2d b = band.origin.Translated(dir * high);
        band.face = makeFlatQuad(
            a.Translated(-half),
            b.Translated(-half),
            b.Translated(half),
            a.Translated(half)
        );
    }

    // The sheet without the relief openings.
    TopoDS_Shape sheet = flatProfile;
    if (!reliefs.IsEmpty()) {
        TopTools_ListOfShape sheetList;
        sheetList.Append(flatProfile);
        BRepAlgoAPI_Cut relieve;
        relieve.SetArguments(sheetList);
        relieve.SetTools(reliefs);
        relieve.Build();
        if (!relieve.IsDone() || relieve.HasErrors()) {
            throw Base::CADKernelError("Sheet design: cannot make the relief openings");
        }
        sheet = relieve.Shape();
    }
    // Lines that go through a point get their zones first.
    std::vector<std::size_t> order;
    for (int pass = 0; pass < 2; ++pass) {
        for (std::size_t i = 0; i < bands.size(); ++i) {
            if (goesThrough[i] == (pass == 0)) {
                order.push_back(i);
            }
        }
    }

    // A score acts only on the parts of the profile that its line reaches. A part of the band
    // that is apart from the line, for example behind a slot, stays flat.
    std::vector<std::pair<std::size_t, TopoDS_Face>> zoneParts;
    TopTools_ListOfShape zoneFaces;
    for (const std::size_t i : order) {
        const Band& band = bands[i];
        BRepAlgoAPI_Common common(sheet, band.face);
        if (!common.IsDone() || common.HasErrors()) {
            throw Base::CADKernelError("Sheet design: cannot make a bend zone");
        }
        // A zone does not take a region that an earlier zone has.
        TopoDS_Shape free = common.Shape();
        if (!zoneFaces.IsEmpty() && !facesOf(free).empty()) {
            TopTools_ListOfShape freeList;
            freeList.Append(free);
            BRepAlgoAPI_Cut rest;
            rest.SetArguments(freeList);
            rest.SetTools(zoneFaces);
            rest.Build();
            if (!rest.IsDone() || rest.HasErrors()) {
                throw Base::CADKernelError("Sheet design: cannot make a bend zone");
            }
            free = rest.Shape();
        }
        bool reached = false;
        for (const TopoDS_Face& part : facesOf(free)) {
            double first = 0.0;
            double last = 0.0;
            bool started = false;
            for (TopExp_Explorer vx(part, TopAbs_VERTEX); vx.More(); vx.Next()) {
                const gp_Pnt p3 = BRep_Tool::Pnt(TopoDS::Vertex(vx.Current()));
                const double along = gp_Vec2d(band.origin, gp_Pnt2d(p3.X(), p3.Y())).Dot(band.dir);
                first = started ? std::min(first, along) : along;
                last = started ? std::max(last, along) : along;
                started = true;
            }
            if (started && std::min(last, band.length) - std::max(first, 0.0) > looseTol) {
                zoneParts.emplace_back(i, part);
                zoneFaces.Append(part);
                reached = true;
            }
        }
        if (!reached) {
            throw Base::ValueError("Sheet design: a bend line is not on the flat pattern");
        }
    }

    // The panels are the parts of the profile that are not in a bend zone.
    TopTools_ListOfShape profileList;
    profileList.Append(sheet);
    BRepAlgoAPI_Cut split;
    split.SetArguments(profileList);
    split.SetTools(zoneFaces);
    split.Build();
    if (!split.IsDone() || split.HasErrors()) {
        throw Base::CADKernelError("Sheet design: cannot divide the flat pattern");
    }
    std::vector<TopoDS_Face> pieces = facesOf(split.Shape());
    if (pieces.size() < 2) {
        throw Base::ValueError("Sheet design: the bend lines do not divide the flat pattern");
    }
    // Where bend lines meet, a small part of the profile is between the ends of their zones.
    // The sheet cannot fold there. Remove such a part: it is a relief opening at the corner.
    double widest = 0.0;
    for (const Band& band : bands) {
        widest = std::max(widest, band.allowance);
    }
    pieces.erase(
        std::remove_if(
            pieces.begin(),
            pieces.end(),
            [widest](const TopoDS_Face& piece) {
                Bnd_Box pieceBox;
                BRepBndLib::Add(piece, pieceBox);
                return std::sqrt(pieceBox.SquareExtent()) < 2.5 * widest;
            }
        ),
        pieces.end()
    );
    if (pieces.size() < 2) {
        throw Base::ValueError("Sheet design: the bend lines do not divide the flat pattern");
    }

    // The largest panel is the root panel.
    std::size_t largest = 0;
    double largestArea = 0.0;
    for (std::size_t i = 0; i < pieces.size(); ++i) {
        const double pieceArea = area(pieces[i]);
        if (pieceArea > largestArea) {
            largestArea = pieceArea;
            largest = i;
        }
    }
    std::swap(pieces[0], pieces[largest]);
    model.panels.clear();
    for (const TopoDS_Face& piece : pieces) {
        Panel panel;
        panel.profile = piece;
        panel.folded = placement;
        panel.tag = tag + "_Root";
        model.panels.push_back(panel);
    }

    auto panelAt = [&model](const gp_Pnt2d& p) {
        for (std::size_t i = 0; i < model.panels.size(); ++i) {
            if (isInside(model.panels[i].profile, p)) {
                return static_cast<int>(i);
            }
        }
        return -1;
    };

    // Each part of a bend zone joins the panel on its minus side to the panel on its plus side.
    struct Joint
    {
        int minusPanel {-1};
        int plusPanel {-1};
        gp_Pnt2d minusA, minusB, plusA, plusB;
        std::size_t line {0};
        TopoDS_Shape strip;
        bool plain {true};
        bool used {false};
    };
    std::vector<Joint> joints;
    for (const auto& zonePart : zoneParts) {
        const std::size_t i = zonePart.first;
        const Band& band = bands[i];
        const TopoDS_Face& strip = zonePart.second;
        {
            // Find the edges of the zone on its two long sides. They join it to the panels.
            struct Side
            {
                double low {0.0};
                double high {0.0};
                bool found {false};
                gp_Pnt2d mid;
            };
            Side minus;
            Side plus;
            TopTools_IndexedMapOfShape zoneEdges;
            TopExp::MapShapes(strip, TopAbs_EDGE, zoneEdges);
            for (int e = 1; e <= zoneEdges.Extent(); ++e) {
                BRepAdaptor_Curve curve(TopoDS::Edge(zoneEdges(e)));
                if (curve.GetType() != GeomAbs_Line) {
                    continue;
                }
                const gp_Pnt p3 = curve.Value(curve.FirstParameter());
                const gp_Pnt q3 = curve.Value(curve.LastParameter());
                const gp_Pnt2d p(p3.X(), p3.Y());
                const gp_Pnt2d q(q3.X(), q3.Y());
                const double sideP = gp_Vec2d(band.origin, p).Dot(band.normal);
                const double sideQ = gp_Vec2d(band.origin, q).Dot(band.normal);
                for (Side* side : {&minus, &plus}) {
                    const double level = (side == &minus ? -0.5 : 0.5) * band.allowance;
                    if (std::fabs(sideP - level) > looseTol || std::fabs(sideQ - level) > looseTol) {
                        continue;
                    }
                    const double alongP = gp_Vec2d(band.origin, p).Dot(band.dir);
                    const double alongQ = gp_Vec2d(band.origin, q).Dot(band.dir);
                    if (!side->found) {
                        side->mid = gp_Pnt2d((p.X() + q.X()) / 2.0, (p.Y() + q.Y()) / 2.0);
                        side->low = std::min(alongP, alongQ);
                        side->high = std::max(alongP, alongQ);
                        side->found = true;
                    }
                    else {
                        side->low = std::min({side->low, alongP, alongQ});
                        side->high = std::max({side->high, alongP, alongQ});
                    }
                }
            }
            if (!minus.found || !plus.found) {
                throw Base::ValueError("Sheet design: a bend line must have a panel on each side");
            }
            auto pointAt = [&band](double along, double level) {
                return band.origin.Translated(band.dir * along + band.normal * level);
            };
            Joint joint;
            joint.line = i;
            joint.strip = strip;
            joint.minusA = pointAt(minus.low, -band.allowance / 2.0);
            joint.minusB = pointAt(minus.high, -band.allowance / 2.0);
            joint.plusA = pointAt(plus.low, band.allowance / 2.0);
            joint.plusB = pointAt(plus.high, band.allowance / 2.0);
            // A full rectangle gives an exact solid of revolution.
            joint.plain = zoneEdges.Extent() == 4 && std::fabs(minus.low - plus.low) < looseTol
                && std::fabs(minus.high - plus.high) < looseTol;
            const double probe = 1.0e-4;
            joint.minusPanel = panelAt(minus.mid.Translated(band.normal * -probe));
            joint.plusPanel = panelAt(plus.mid.Translated(band.normal * probe));
            if (joint.minusPanel < 0 || joint.plusPanel < 0) {
                throw Base::ValueError("Sheet design: a bend line must have a panel on each side");
            }
            // A cut on the bend line divides the zone. The parts between the same two panels
            // make one bend.
            bool merged = false;
            for (Joint& other : joints) {
                if (other.line != joint.line || other.minusPanel != joint.minusPanel
                    || other.plusPanel != joint.plusPanel) {
                    continue;
                }
                auto along = [&band](const gp_Pnt2d& p) {
                    return gp_Vec2d(band.origin, p).Dot(band.dir);
                };
                other.minusA = along(joint.minusA) < along(other.minusA) ? joint.minusA
                                                                         : other.minusA;
                other.minusB = along(joint.minusB) > along(other.minusB) ? joint.minusB
                                                                         : other.minusB;
                other.plusA = along(joint.plusA) < along(other.plusA) ? joint.plusA : other.plusA;
                other.plusB = along(joint.plusB) > along(other.plusB) ? joint.plusB : other.plusB;
                other.strip = joinZones(other.strip, joint.strip);
                other.plain = false;
                merged = true;
                break;
            }
            if (!merged) {
                joints.push_back(joint);
            }
        }
    }

    // Go from the root panel through the joints and set the folded position of each panel.
    std::vector<bool> reached(model.panels.size(), false);
    std::map<std::size_t, int> panelsOfLine;
    std::deque<int> queue {0};
    reached[0] = true;
    while (!queue.empty()) {
        const int current = queue.front();
        queue.pop_front();
        for (Joint& joint : joints) {
            if (joint.used || (joint.minusPanel != current && joint.plusPanel != current)) {
                continue;
            }
            joint.used = true;
            const bool fromMinus = joint.minusPanel == current;
            const int child = fromMinus ? joint.plusPanel : joint.minusPanel;
            const Band& band = bands[joint.line];
            if (reached[child]) {
                // The two panels have their position from other bends. This bend closes a loop
                // of panels, for example a corner with a web. The angles must agree: the
                // position that this bend gives must be the position that the panel has.
                const gp_Pnt2d start = fromMinus ? joint.minusA : joint.plusA;
                const gp_Vec2d normal = fromMinus ? band.normal : -band.normal;
                const gp_Trsf expected = model.afterBend(
                    model.panels[current].folded,
                    start,
                    normal,
                    lines[joint.line].angle,
                    lines[joint.line].up,
                    params.radius
                );
                const gp_Trsf error = expected.Inverted() * model.panels[child].folded;
                gp_XYZ axis;
                double turn = 0.0;
                error.GetRotation(axis, turn);
                // The middle of the bend line is the reference for the position.
                const gp_Pnt2d mid2(
                    (joint.minusA.X() + joint.plusB.X()) / 2.0,
                    (joint.minusA.Y() + joint.plusB.Y()) / 2.0
                );
                const gp_Pnt mid = to3d(mid2);
                const double shift = mid.Transformed(expected).Distance(
                    mid.Transformed(model.panels[child].folded)
                );
                // A fold of 180 degrees puts panels in layers, thus the position can differ by
                // some thicknesses.
                const double limit = 4.0 * params.thickness + 2.0 * band.allowance + 1.0e-6;
                if (std::fabs(turn) > 1.0e-4 || shift > limit) {
                    throw Base::ValueError(
                        "Sheet design: the folds around a corner do not close. The angles of the "
                        "bend lines do not agree (bend line "
                        + std::to_string(joint.line + 1) + ": angle error "
                        + std::to_string(turn * 180.0 / M_PI) + " degrees, position error "
                        + std::to_string(shift) + " mm)"
                    );
                }
                Bend closing;
                closing.parentPanel = current;
                closing.childPanel = child;
                closing.start = start;
                closing.end = fromMinus ? joint.minusB : joint.plusB;
                closing.normal = gp_Dir2d(normal);
                closing.angle = lines[joint.line].angle;
                closing.up = lines[joint.line].up;
                closing.radius = params.radius;
                closing.allowance = band.allowance;
                closing.strip = joint.strip;
                closing.plain = joint.plain;
                closing.closing = true;
                model.bends.push_back(closing);
                continue;
            }
            // The name of a panel comes from the bend line that joins it to the part.
            const int ordinal = ++panelsOfLine[joint.line];
            model.panels[child].tag = tag + "_L" + std::to_string(joint.line + 1)
                + (ordinal > 1 ? "_" + std::to_string(ordinal) : std::string());
            model.connect(
                current,
                child,
                fromMinus ? joint.minusA : joint.plusA,
                fromMinus ? joint.minusB : joint.plusB,
                fromMinus ? band.normal : -band.normal,
                lines[joint.line].angle,
                lines[joint.line].up,
                params.radius,
                joint.plain ? TopoDS_Shape() : joint.strip
            );
            reached[child] = true;
            queue.push_back(child);
        }
    }
    if (std::find(reached.begin(), reached.end(), false) != reached.end()) {
        throw Base::ValueError(
            "Sheet design: a part of the flat pattern has no bend to the root panel"
        );
    }
    return model;
}


// ------------------------------------------------------------------------------------------------
// Entry from a folded solid

namespace
{

/// The normal of a planar face that points out of the material.
gp_Dir outerNormal(const TopoDS_Face& face)
{
    BRepAdaptor_Surface surface(face);
    BRepLProp_SLProps props(
        surface,
        (surface.FirstUParameter() + surface.LastUParameter()) / 2.0,
        (surface.FirstVParameter() + surface.LastVParameter()) / 2.0,
        1,
        tol
    );
    gp_Dir normal = props.Normal();
    if (face.Orientation() == TopAbs_REVERSED) {
        normal.Reverse();
    }
    return normal;
}

}  // namespace

SheetModel SheetModel::fromSolid(
    const TopoDS_Shape& solid,
    const TopoDS_Face& rootFace,
    const SheetParams& params,
    const std::string& tag
)
{
    const double looseTol = 1.0e-5;
    BRepAdaptor_Surface rootSurface(rootFace);
    if (rootSurface.GetType() != GeomAbs_Plane) {
        throw Base::ValueError("Sheet design: select a flat face of the solid");
    }
    const gp_Pln rootPlane = rootSurface.Plane();
    const gp_Dir normal = outerNormal(rootFace);

    // The thickness is the distance to the nearest flat face that looks to the opposite side.
    double thickness = -1.0;
    for (TopExp_Explorer ex(solid, TopAbs_FACE); ex.More(); ex.Next()) {
        const TopoDS_Face face = TopoDS::Face(ex.Current());
        BRepAdaptor_Surface surface(face);
        if (surface.GetType() != GeomAbs_Plane || face.IsSame(rootFace)) {
            continue;
        }
        if (outerNormal(face).Dot(normal) > -1.0 + 1.0e-9) {
            continue;
        }
        const double distance = gp_Vec(surface.Plane().Location(), rootPlane.Location()).Dot(normal);
        if (distance > looseTol && (thickness < 0.0 || distance < thickness)) {
            thickness = distance;
        }
    }
    if (thickness < 0.0) {
        throw Base::ValueError("Sheet design: cannot find the thickness of the solid");
    }

    // Flat space: the root face is on the top side (z = thickness), its normal is +z.
    const gp_Pnt origin = rootPlane.Location().Translated(gp_Vec(normal) * -thickness);
    gp_Trsf placement;
    placement.SetTransformation(gp_Ax3(origin, normal, rootPlane.XAxis().Direction()));
    placement.Invert();

    SheetParams sheet = params;
    sheet.thickness = thickness;

    gp_Trsf drop;
    drop.SetTranslation(gp_Vec(0.0, 0.0, -thickness));
    // Move a face of the top side into flat space. Returns a null face if it is not a panel.
    auto toFlat = [&](const TopoDS_Face& face, const gp_Trsf& folded) {
        BRepBuilderAPI_Transform move(face, drop * folded.Inverted(), Standard_True);
        TopoDS_Face flat = TopoDS::Face(move.Shape());
        BRepAdaptor_Surface surface(flat);
        if (surface.GetType() != GeomAbs_Plane) {
            return TopoDS_Face();
        }
        if (outerNormal(flat).Z() < 1.0 - 1.0e-7
            || std::fabs(surface.Plane().Location().Z()) > looseTol) {
            return TopoDS_Face();
        }
        return flat;
    };

    const TopoDS_Face rootProfile = toFlat(rootFace, placement);
    if (rootProfile.IsNull()) {
        throw Base::CADKernelError("Sheet design: cannot move the root face into flat space");
    }
    SheetModel model = makeBase(rootProfile, placement, sheet, tag + "_P1");

    TopTools_IndexedDataMapOfShapeListOfShape edgeFaces;
    TopExp::MapShapesAndAncestors(solid, TopAbs_EDGE, TopAbs_FACE, edgeFaces);
    auto otherFace = [&edgeFaces](const TopoDS_Shape& edge, const TopoDS_Face& face) {
        if (edgeFaces.Contains(edge)) {
            for (const TopoDS_Shape& candidate : edgeFaces.FindFromKey(edge)) {
                if (!candidate.IsSame(face)) {
                    return TopoDS::Face(candidate);
                }
            }
        }
        return TopoDS_Face();
    };

    TopTools_MapOfShape visited;
    visited.Add(rootFace);
    std::vector<std::pair<TopoDS_Face, int>> panelFaces {{rootFace, 0}};
    std::deque<std::pair<int, TopoDS_Face>> queue;
    queue.emplace_back(0, rootFace);

    while (!queue.empty()) {
        const int current = queue.front().first;
        const TopoDS_Face face = queue.front().second;
        queue.pop_front();
        const gp_Trsf inverse = model.panels[current].folded.Inverted();

        for (TopExp_Explorer ex(face, TopAbs_EDGE); ex.More(); ex.Next()) {
            const TopoDS_Edge edge = TopoDS::Edge(ex.Current());
            BRepAdaptor_Curve curve(edge);
            if (curve.GetType() != GeomAbs_Line) {
                continue;
            }
            const TopoDS_Face bendFace = otherFace(edge, face);
            if (bendFace.IsNull() || visited.Contains(bendFace)) {
                continue;
            }
            BRepAdaptor_Surface bendSurface(bendFace);
            if (bendSurface.GetType() != GeomAbs_Cylinder) {
                continue;
            }
            const gp_Cylinder cylinder = bendSurface.Cylinder();
            if (std::fabs(cylinder.Axis().Direction().Dot(curve.Line().Direction())) < 1.0 - 1.0e-9) {
                continue;  // For example the wall of a hole.
            }

            // Look at the bend in the flat space of the current panel.
            const gp_Pnt p = curve.Value(curve.FirstParameter()).Transformed(inverse);
            const gp_Pnt q = curve.Value(curve.LastParameter()).Transformed(inverse);
            const gp_Pnt center = cylinder.Location().Transformed(inverse);
            const gp_Pnt2d a(p.X(), p.Y());
            const gp_Pnt2d b(q.X(), q.Y());
            const double rho = cylinder.Radius();
            bool up = true;
            double radius = 0.0;
            if (std::fabs(center.Z() - thickness - rho) < looseTol) {
                up = true;
                radius = rho;
            }
            else if (std::fabs(center.Z() - thickness + rho) < looseTol) {
                up = false;
                radius = rho - thickness;
            }
            else {
                continue;  // The cylinder is not tangent to the panel.
            }
            if (radius < -looseTol) {
                continue;
            }
            radius = std::max(radius, 0.0);
            const double angle = bendSurface.LastUParameter() - bendSurface.FirstUParameter();

            // The position of the bend face in the parameters of its cylinder: U goes around
            // the axis, V goes along it.
            double pcFirst = 0.0;
            double pcLast = 0.0;
            Handle(Geom2d_Curve) onBend = BRep_Tool::CurveOnSurface(edge, bendFace, pcFirst, pcLast);
            if (onBend.IsNull()) {
                continue;
            }
            const gp_Pnt2d uvP = onBend->Value(curve.FirstParameter());
            const gp_Pnt2d uvQ = onBend->Value(curve.LastParameter());
            const double uFirst = bendSurface.FirstUParameter();
            const double uLast = bendSurface.LastUParameter();
            const bool forward = std::fabs(uvP.X() - uFirst) < std::fabs(uvP.X() - uLast);
            const double uFar = forward ? uLast : uFirst;
            if (std::fabs(uvQ.Y() - uvP.Y()) < tol) {
                continue;
            }

            // The panel after the bend is on a straight edge at the far side of the cylinder.
            TopoDS_Face nextFace;
            int straightEdges = 0;
            int allEdges = 0;
            for (TopExp_Explorer bx(bendFace, TopAbs_EDGE); bx.More(); bx.Next()) {
                ++allEdges;
                const TopoDS_Edge bendEdge = TopoDS::Edge(bx.Current());
                if (BRepAdaptor_Curve(bendEdge).GetType() != GeomAbs_Line) {
                    continue;
                }
                ++straightEdges;
                double ef = 0.0;
                double el = 0.0;
                Handle(Geom2d_Curve) pc = BRep_Tool::CurveOnSurface(bendEdge, bendFace, ef, el);
                if (pc.IsNull() || std::fabs(pc->Value(ef).X() - uFar) > looseTol
                    || std::fabs(pc->Value(el).X() - uFar) > looseTol) {
                    continue;
                }
                const TopoDS_Face candidate = otherFace(bendEdge, bendFace);
                if (nextFace.IsNull() && !candidate.IsNull()
                    && BRepAdaptor_Surface(candidate).GetType() == GeomAbs_Plane) {
                    nextFace = candidate;
                }
            }
            visited.Add(bendFace);
            if (nextFace.IsNull()) {
                throw Base::ValueError(
                    "Sheet design: a bend of the solid has no flat panel after it. "
                    "This is not available in this version"
                );
            }
            const gp_Vec2d n = outwardNormal(model.panels[current].profile, a, b);

            // A bend face with four edges is the full rectangle. Other faces are moved from the
            // cylinder into flat space, edge by edge.
            TopoDS_Face zone;
            if (allEdges != 4 || straightEdges != 2) {
                const double allowance = model.bendAllowance(angle, radius);
                auto toFlatPoint = [&](const gp_Pnt2d& uv) {
                    const double fraction = (uv.Y() - uvP.Y()) / (uvQ.Y() - uvP.Y());
                    const double across = allowance * (forward ? uv.X() - uFirst : uLast - uv.X())
                        / angle;
                    return a.Translated(gp_Vec2d(a, b) * fraction + n * across);
                };
                Handle(Geom_Plane) flatPlane = new Geom_Plane(gp_Pln(gp::XOY()));
                std::vector<TopoDS_Wire> wires;
                const TopoDS_Wire bendOuter = BRepTools::OuterWire(bendFace);
                std::vector<TopoDS_Wire> bendWires {bendOuter};
                for (TopExp_Explorer wx(bendFace, TopAbs_WIRE); wx.More(); wx.Next()) {
                    if (!wx.Current().IsSame(bendOuter)) {
                        bendWires.push_back(TopoDS::Wire(wx.Current()));
                    }
                }
                for (const TopoDS_Wire& bendWire : bendWires) {
                    BRepBuilderAPI_MakeWire wire;
                    for (BRepTools_WireExplorer wx(bendWire, bendFace); wx.More(); wx.Next()) {
                        double ef = 0.0;
                        double el = 0.0;
                        Handle(Geom2d_Curve)
                            pc = BRep_Tool::CurveOnSurface(wx.Current(), bendFace, ef, el);
                        if (pc.IsNull()) {
                            throw Base::CADKernelError(
                                "Sheet design: an edge of a bend has no curve on its surface"
                            );
                        }
                        if (Geom2dAdaptor_Curve(pc, ef, el).GetType() == GeomAbs_Line) {
                            const gp_Pnt2d p0 = toFlatPoint(pc->Value(ef));
                            const gp_Pnt2d p1 = toFlatPoint(pc->Value(el));
                            if (p0.Distance(p1) > tol) {
                                wire.Add(BRepBuilderAPI_MakeEdge(to3d(p0), to3d(p1)).Edge());
                            }
                            continue;
                        }
                        Handle(Geom2d_BSplineCurve) spline = Geom2dConvert::CurveToBSplineCurve(
                            new Geom2d_TrimmedCurve(pc, ef, el)
                        );
                        for (int k = 1; k <= spline->NbPoles(); ++k) {
                            spline->SetPole(k, toFlatPoint(spline->Pole(k)));
                        }
                        TopoDS_Edge flatEdge = BRepBuilderAPI_MakeEdge(spline, flatPlane).Edge();
                        BRepLib::BuildCurves3d(flatEdge);
                        wire.Add(flatEdge);
                    }
                    if (!wire.IsDone()) {
                        throw Base::CADKernelError(
                            "Sheet design: cannot move the outline of a bend into flat space"
                        );
                    }
                    wires.push_back(wire.Wire());
                }
                zone = faceOnSurface(flatPlane, wires);
            }
            if (visited.Contains(nextFace)) {
                // A cut can divide a bend into two cylinder faces between the same two panels.
                // The second face is a further part of the zone of the same bend.
                int other = -1;
                for (const auto& known : panelFaces) {
                    if (known.first.IsSame(nextFace)) {
                        other = known.second;
                    }
                }
                Bend* same = nullptr;
                for (Bend& candidate : model.bends) {
                    if (candidate.parentPanel == current && candidate.childPanel == other) {
                        same = &candidate;
                    }
                }
                if (!same) {
                    throw Base::ValueError("Sheet design: the panels of the solid make a closed loop");
                }
                TopoDS_Shape part = zone;
                if (zone.IsNull()) {
                    const double allowance = model.bendAllowance(angle, radius);
                    part = makeFlatQuad(a, b, b.Translated(n * allowance), a.Translated(n * allowance));
                }
                same->strip = joinZones(same->strip, part);
                same->plain = false;
                continue;
            }

            Panel child;
            model.panels.push_back(child);
            const int childIndex = static_cast<int>(model.panels.size()) - 1;
            panelFaces.emplace_back(nextFace, childIndex);
            model.connect(current, childIndex, a, b, n, angle, up, radius, zone);
            const TopoDS_Face childProfile = toFlat(nextFace, model.panels[childIndex].folded);
            if (childProfile.IsNull()) {
                throw Base::ValueError(
                    "Sheet design: a bend of the solid has no flat panel after it. "
                    "This is not available in this version"
                );
            }
            model.panels[childIndex].profile = childProfile;
            model.panels[childIndex].tag = tag + "_P" + std::to_string(childIndex + 1);
            model.addOrigins(childProfile, gp_Trsf(), model.panels[childIndex].tag + "_E", childIndex);
            visited.Add(nextFace);
            queue.emplace_back(childIndex, nextFace);
        }
    }
    return model;
}
