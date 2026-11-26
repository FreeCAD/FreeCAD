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
#include <BRepLProp_SLProps.hxx>
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
#include <gp_Pln.hxx>
#include <gp_Vec2d.hxx>

#include <Base/Exception.h>

#include "SheetModel.h"


using namespace SheetMetalCore;

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
        throw Base::CADKernelError("Sheet metal: cannot make a flat face");
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

/// Make sure that a solid has its material on the inner side.
TopoDS_Shape orientSolid(const TopoDS_Shape& solid)
{
    GProp_GProps props;
    BRepGProp::VolumeProperties(solid, props);
    return props.Mass() < 0.0 ? solid.Reversed() : solid;
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
        throw Base::ValueError("Sheet metal: the edge is not on the boundary of a panel");
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

TopoDS_Shape fuseAll(const std::vector<TopoDS_Shape>& shapes)
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
    // The parts touch only on coincident faces or edges. The glue mode skips the general
    // intersection of all faces.
    op.SetGlue(BOPAlgo_GlueShift);
    op.SetRunParallel(Standard_True);
    op.Build();
    if (!op.IsDone() || op.HasErrors()) {
        throw Base::CADKernelError("Sheet metal: the fuse operation failed");
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
    const SheetParams& params
)
{
    if (params.thickness <= tol) {
        throw Base::ValueError("Sheet metal: the thickness must be larger than zero");
    }
    if (params.radius < 0.0 || params.kFactor < 0.0 || params.kFactor > 1.0) {
        throw Base::ValueError("Sheet metal: the bend radius or the K-factor is not valid");
    }
    SheetModel model;
    model.params = params;
    Panel panel;
    panel.profile = flatProfile;
    panel.folded = placement;
    model.panels.push_back(panel);
    return model;
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

int SheetModel::addFlange(const EdgeRef& ref, double length, double angle, bool up, double gap1, double gap2)
{
    if (ref.panel < 0 || ref.panel >= static_cast<int>(panels.size())) {
        throw Base::ValueError("Sheet metal: the edge reference is not valid");
    }
    if (angle <= tol || angle > M_PI + tol) {
        throw Base::ValueError("Sheet metal: the bend angle must be between 0 and 180 degrees");
    }
    if (length <= tol) {
        throw Base::ValueError("Sheet metal: the flange length must be larger than zero");
    }
    if (gap1 < 0.0 || gap2 < 0.0) {
        throw Base::ValueError("Sheet metal: a gap cannot be negative");
    }
    gp_Vec2d dir(ref.a, ref.b);
    const double edgeLength = dir.Magnitude();
    if (edgeLength - gap1 - gap2 <= tol) {
        throw Base::ValueError("Sheet metal: the gaps are longer than the edge");
    }
    dir /= edgeLength;
    const gp_Pnt2d a = ref.a.Translated(dir * gap1);
    const gp_Pnt2d b = ref.b.Translated(dir * -gap2);

    const gp_Vec2d n = outwardNormal(panels[ref.panel].profile, a, b);

    // An edge can hold only one bend.
    for (const Bend& other : bends) {
        if (other.parentPanel == ref.panel && segmentsOverlap(a, b, other.start, other.end)) {
            throw Base::ValueError("Sheet metal: the edge already has a bend");
        }
        if (other.childPanel == ref.panel) {
            const gp_Vec2d shift = gp_Vec2d(other.normal) * other.allowance;
            if (segmentsOverlap(a, b, other.start.Translated(shift), other.end.Translated(shift))) {
                throw Base::ValueError("Sheet metal: the edge already has a bend");
            }
        }
    }

    const double allowance = bendAllowance(angle);
    Panel child;
    child.profile = makeFlatQuad(
        a.Translated(n * allowance),
        b.Translated(n * allowance),
        b.Translated(n * (allowance + length)),
        a.Translated(n * (allowance + length))
    );
    panels.push_back(child);
    connect(ref.panel, static_cast<int>(panels.size()) - 1, a, b, n, angle, up, params.radius, TopoDS_Face());
    return static_cast<int>(bends.size()) - 1;
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
    const TopoDS_Face& strip
)
{
    const double t = params.thickness;
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
    bend.strip = strip.IsNull()
        ? makeFlatQuad(start, end, end.Translated(normal * allowance), start.Translated(normal * allowance))
        : strip;

    // The child moves back by the bend allowance, then turns around the bend axis.
    gp_Dir axisDir(normal.Y(), -normal.X(), 0.0);  // normal x Z
    if (!up) {
        axisDir.Reverse();
    }
    const gp_Ax1 axis(to3d(start, up ? t + radius : -radius), axisDir);
    gp_Trsf rotation;
    rotation.SetRotation(axis, angle);
    gp_Trsf translation;
    translation.SetTranslation(gp_Vec(-allowance * normal.X(), -allowance * normal.Y(), 0.0));
    panels[child].folded = panels[parent].folded * rotation * translation;
    panels[child].parentBend = static_cast<int>(bends.size());
    panels[child].solid.Nullify();
    bends.push_back(bend);
}

int SheetModel::cut(const TopoDS_Shape& faces, bool flat)
{
    const double t = params.thickness;
    int changed = 0;

    for (TopExp_Explorer ex(faces, TopAbs_FACE); ex.More(); ex.Next()) {
        bool used = false;
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

            for (const Bend& bend : bends) {
                const bool adjacent = bend.parentPanel == static_cast<int>(frame)
                    || bend.childPanel == static_cast<int>(frame);
                if (!flat && !adjacent) {
                    continue;
                }
                if (boxesOverlap(local, bend.strip)
                    && area(BRepAlgoAPI_Common(bend.strip, local).Shape()) > tol) {
                    throw Base::ValueError(
                        "Sheet metal: a cut across a bend zone is not available in this version"
                    );
                }
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
                    throw Base::CADKernelError("Sheet metal: the cut operation failed");
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
                    throw Base::ValueError("Sheet metal: the cut removes a full panel");
                }
                if (count > 1) {
                    throw Base::ValueError("Sheet metal: the cut divides a panel into parts");
                }
                panel.profile = result;
                panel.solid.Nullify();
                ++changed;
            }
        }
        if (!used) {
            throw Base::ValueError("Sheet metal: the cut profile is not in the plane of a panel");
        }
    }
    return changed;
}

TopoDS_Shape SheetModel::makeFolded(bool fuse, bool refine) const
{
    const double t = params.thickness;
    std::vector<TopoDS_Shape> solids;
    solids.reserve(panels.size() + bends.size());

    for (const Panel& panel : panels) {
        if (panel.solid.IsNull()) {
            BRepPrimAPI_MakePrism prism(panel.profile, gp_Vec(0.0, 0.0, t));
            panel.solid = orientSolid(prism.Shape());
        }
        solids.push_back(panel.solid.Moved(TopLoc_Location(panel.folded)));
    }

    for (const Bend& bend : bends) {
        BRepBuilderAPI_MakePolygon poly(
            to3d(bend.start),
            to3d(bend.end),
            to3d(bend.end, t),
            to3d(bend.start, t),
            Standard_True
        );
        BRepBuilderAPI_MakeFace section(poly.Wire(), Standard_True);
        gp_Dir axisDir(bend.normal.Y(), -bend.normal.X(), 0.0);
        if (!bend.up) {
            axisDir.Reverse();
        }
        const gp_Ax1 axis(to3d(bend.start, bend.up ? t + bend.radius : -bend.radius), axisDir);
        BRepPrimAPI_MakeRevol revol(section.Face(), axis, bend.angle);
        if (!revol.IsDone()) {
            throw Base::CADKernelError("Sheet metal: cannot make a bend");
        }
        const TopoDS_Shape solid = orientSolid(revol.Shape());
        solids.push_back(solid.Moved(TopLoc_Location(panels[bend.parentPanel].folded)));
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
    TopoDS_Shape result = fuseAll(solids);
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
        faces.push_back(bend.strip);
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
    const SheetParams& params
)
{
    SheetModel model = makeBase(flatProfile, placement, params);
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
        gp_Vec2d normal;
        double allowance {0.0};
        TopoDS_Face face;
    };
    std::vector<Band> bands;
    TopTools_ListOfShape bandFaces;
    for (const BendLine& line : lines) {
        if (line.angle <= tol || line.angle > M_PI + tol) {
            throw Base::ValueError("Sheet metal: a bend angle must be between 0 and 180 degrees");
        }
        gp_Vec2d dir(line.a, line.b);
        if (dir.Magnitude() < tol) {
            throw Base::ValueError("Sheet metal: a bend line has no length");
        }
        dir.Normalize();
        Band band;
        band.origin = line.a;
        band.normal = gp_Vec2d(-dir.Y(), dir.X());
        band.allowance = model.bendAllowance(line.angle);
        const gp_Vec2d half = band.normal * (band.allowance / 2.0);
        const gp_Pnt2d a = line.a.Translated(dir * -reach);
        const gp_Pnt2d b = line.b.Translated(dir * reach);
        band.face = makeFlatQuad(
            a.Translated(-half),
            b.Translated(-half),
            b.Translated(half),
            a.Translated(half)
        );
        bandFaces.Append(band.face);
        bands.push_back(band);
    }

    // The panels are the parts of the profile that are not in a bend zone.
    TopTools_ListOfShape profileList;
    profileList.Append(flatProfile);
    BRepAlgoAPI_Cut split;
    split.SetArguments(profileList);
    split.SetTools(bandFaces);
    split.Build();
    if (!split.IsDone() || split.HasErrors()) {
        throw Base::CADKernelError("Sheet metal: cannot divide the flat pattern");
    }
    std::vector<TopoDS_Face> pieces = facesOf(split.Shape());
    if (pieces.size() < 2) {
        throw Base::ValueError("Sheet metal: the bend lines do not divide the flat pattern");
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
        TopoDS_Face strip;
        bool used {false};
    };
    std::vector<Joint> joints;
    for (std::size_t i = 0; i < bands.size(); ++i) {
        const Band& band = bands[i];
        BRepAlgoAPI_Common common(flatProfile, band.face);
        if (!common.IsDone() || common.HasErrors()) {
            throw Base::CADKernelError("Sheet metal: cannot make a bend zone");
        }
        const std::vector<TopoDS_Face> strips = facesOf(common.Shape());
        if (strips.empty()) {
            throw Base::ValueError("Sheet metal: a bend line is not on the flat pattern");
        }
        for (const TopoDS_Face& strip : strips) {
            TopTools_IndexedMapOfShape vertices;
            TopExp::MapShapes(strip, TopAbs_VERTEX, vertices);
            std::vector<gp_Pnt2d> minus;
            std::vector<gp_Pnt2d> plus;
            for (int v = 1; v <= vertices.Extent(); ++v) {
                const gp_Pnt p3 = BRep_Tool::Pnt(TopoDS::Vertex(vertices(v)));
                const gp_Pnt2d p(p3.X(), p3.Y());
                const double side = gp_Vec2d(band.origin, p).Dot(band.normal);
                if (std::fabs(side + band.allowance / 2.0) < looseTol) {
                    minus.push_back(p);
                }
                else if (std::fabs(side - band.allowance / 2.0) < looseTol) {
                    plus.push_back(p);
                }
            }
            if (vertices.Extent() != 4 || minus.size() != 2 || plus.size() != 2) {
                throw Base::ValueError(
                    "Sheet metal: a bend zone must be a rectangle without a cut in this version"
                );
            }
            // Put the plus points in the same sequence as the minus points.
            const gp_Vec2d across = band.normal * band.allowance;
            if (minus[0].Translated(across).Distance(plus[0]) > looseTol) {
                std::swap(plus[0], plus[1]);
            }
            if (minus[0].Translated(across).Distance(plus[0]) > looseTol
                || minus[1].Translated(across).Distance(plus[1]) > looseTol) {
                throw Base::ValueError(
                    "Sheet metal: the outline must cross a bend zone at 90 degrees in this version"
                );
            }
            Joint joint;
            joint.line = i;
            joint.strip = strip;
            joint.minusA = minus[0];
            joint.minusB = minus[1];
            joint.plusA = plus[0];
            joint.plusB = plus[1];
            const double probe = 1.0e-4;
            const gp_Pnt2d minusMid(
                (minus[0].X() + minus[1].X()) / 2.0,
                (minus[0].Y() + minus[1].Y()) / 2.0
            );
            joint.minusPanel = panelAt(minusMid.Translated(band.normal * -probe));
            joint.plusPanel = panelAt(minusMid.Translated(band.normal * (band.allowance + probe)));
            if (joint.minusPanel < 0 || joint.plusPanel < 0) {
                throw Base::ValueError("Sheet metal: a bend line must have a panel on each side");
            }
            joints.push_back(joint);
        }
    }

    // Go from the root panel through the joints and set the folded position of each panel.
    std::vector<bool> reached(model.panels.size(), false);
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
            if (reached[child]) {
                throw Base::ValueError("Sheet metal: the bend lines make a closed loop of panels");
            }
            const Band& band = bands[joint.line];
            model.connect(
                current,
                child,
                fromMinus ? joint.minusA : joint.plusA,
                fromMinus ? joint.minusB : joint.plusB,
                fromMinus ? band.normal : -band.normal,
                lines[joint.line].angle,
                lines[joint.line].up,
                params.radius,
                joint.strip
            );
            reached[child] = true;
            queue.push_back(child);
        }
    }
    if (std::find(reached.begin(), reached.end(), false) != reached.end()) {
        throw Base::ValueError("Sheet metal: a part of the flat pattern has no bend to the root panel");
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
    const SheetParams& params
)
{
    const double looseTol = 1.0e-5;
    BRepAdaptor_Surface rootSurface(rootFace);
    if (rootSurface.GetType() != GeomAbs_Plane) {
        throw Base::ValueError("Sheet metal: select a flat face of the solid");
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
        throw Base::ValueError("Sheet metal: cannot find the thickness of the solid");
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
        throw Base::CADKernelError("Sheet metal: cannot move the root face into flat space");
    }
    SheetModel model = makeBase(rootProfile, placement, sheet);

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

            // The panel after the bend is on the other straight edge of the cylinder.
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
                if (!bendEdge.IsSame(edge)) {
                    nextFace = otherFace(bendEdge, bendFace);
                }
            }
            if (allEdges != 4 || straightEdges != 2) {
                throw Base::ValueError(
                    "Sheet metal: a bend of the solid has a cut or is not a rectangle. "
                    "This is not available in this version"
                );
            }
            visited.Add(bendFace);
            if (nextFace.IsNull() || visited.Contains(nextFace)) {
                throw Base::ValueError("Sheet metal: the panels of the solid make a closed loop");
            }

            Panel child;
            model.panels.push_back(child);
            const int childIndex = static_cast<int>(model.panels.size()) - 1;
            const gp_Vec2d n = outwardNormal(model.panels[current].profile, a, b);
            model.connect(current, childIndex, a, b, n, angle, up, radius, TopoDS_Face());
            const TopoDS_Face childProfile = toFlat(nextFace, model.panels[childIndex].folded);
            if (childProfile.IsNull()) {
                throw Base::ValueError(
                    "Sheet metal: a bend of the solid has no flat panel after it. "
                    "This is not available in this version"
                );
            }
            model.panels[childIndex].profile = childProfile;
            visited.Add(nextFace);
            queue.emplace_back(childIndex, nextFace);
        }
    }
    return model;
}
