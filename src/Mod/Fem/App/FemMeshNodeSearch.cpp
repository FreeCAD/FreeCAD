// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Valentin Pohu                                      *
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


#include <algorithm>
#include <array>
#include <cmath>
#include <exception>
#include <limits>
#include <map>
#include <memory>
#include <numbers>
#include <optional>
#include <utility>
#include <vector>

#include <BRepAdaptor_Curve.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRepExtrema_ExtPC.hxx>
#include <BRepTools.hxx>
#include <BRepTopAdaptor_FClass2d.hxx>
#include <BRep_Tool.hxx>
#include <Extrema_ExtPS.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Curve.hxx>
#include <ElSLib.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_Surface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <GeomAdaptor_Surface.hxx>
#include <GeomLProp_SLProps.hxx>
#include <Precision.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Mesh.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <TColStd_Array1OfReal.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Face.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp.hxx>
#include <gp_Ax3.hxx>
#include <gp_Circ.hxx>
#include <gp_Dir.hxx>
#include <gp_Lin.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>
#include <gp_Torus.hxx>
#include <gp_Vec.hxx>

#ifdef _OPENMP
# include <omp.h>
#endif

#include <Base/Matrix.h>

#include "FemMesh.h"
#include "FemMeshNodeSearch.h"


namespace Fem::NodeSearch
{

namespace
{

// The least and the most cos x takes for x from a to b
std::pair<double, double> cosRange(double a, double b)
{
    const double turn = 2.0 * std::numbers::pi;
    if (b - a >= turn) {
        return {-1.0, 1.0};
    }
    double low = std::min(std::cos(a), std::cos(b));
    double high = std::max(std::cos(a), std::cos(b));
    if (std::ceil(a / turn) * turn <= b) {
        high = 1.0;
    }
    if (std::ceil((a - std::numbers::pi) / turn) * turn + std::numbers::pi <= b) {
        low = -1.0;
    }
    return {low, high};
}

std::pair<double, double> times(std::pair<double, double> x, std::pair<double, double> y)
{
    const std::array<double, 4> all {
        x.first * y.first,
        x.first * y.second,
        x.second * y.first,
        x.second * y.second
    };
    return {*std::min_element(all.begin(), all.end()), *std::max_element(all.begin(), all.end())};
}

}  // namespace

/* Add cuts some boxes short : a torus patch by 0.3 on a fillet, its offset, a parabola placed
 * askew by 0.018. Those get the box of the patch and the parabola points furthest along each axis
 */
Bnd_Box boxOf(const TopoDS_Shape& shape)
{
    Bnd_Box box;
    try {
        // An edge alone gets its box from the loop over the edges below
        if (shape.ShapeType() != TopAbs_EDGE) {
            BRepBndLib::Add(shape, box, Standard_False);
        }
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
            const TopoDS_Face& face = TopoDS::Face(ex.Current());
            // Located already, trimmed and offset ones unwrapped down to the torus
            const Handle(Geom_Surface) outer = BRep_Tool::Surface(face);
            Handle(Geom_Surface) surface = outer;
            double offset = 0.0;
            double shift = 0.0;
            while (!surface.IsNull()) {
                auto trimmed = Handle(Geom_RectangularTrimmedSurface)::DownCast(surface);
                auto shifted = Handle(Geom_OffsetSurface)::DownCast(surface);
                if (!trimmed.IsNull()) {
                    surface = trimmed->BasisSurface();
                }
                else if (!shifted.IsNull()) {
                    offset += std::abs(shifted->Offset());
                    shift += shifted->Offset();
                    surface = shifted->BasisSurface();
                }
                else {
                    break;
                }
            }
            auto toroid = Handle(Geom_ToroidalSurface)::DownCast(surface);
            if (toroid.IsNull()) {
                continue;
            }
            const gp_Torus torus = toroid->Torus();
            const gp_Pnt centre = torus.Location();
            const gp_Ax3& frame = torus.Position();
            const double big = torus.MajorRadius();
            const double r = torus.MinorRadius();
            double u0 = 0.0;
            double u1 = 0.0;
            double v0 = 0.0;
            double v1 = 0.0;
            BRepTools::UVBounds(face, u0, u1, v0, v1);
            /* P(u, v) = O + (R + r cos v) (cos u X + sin u Y) + r sin v Z, each factor bounded on
             * the patch. An offset d makes it the torus of r + d or r - d, read back on the face
             */
            const gp_Vec alongX(frame.XDirection());
            const gp_Vec alongY(frame.YDirection());
            const gp_Vec alongZ(frame.Direction());
            const auto onTorus = [&](double tube) {
                const std::array<std::pair<double, double>, 3> probes {
                    std::pair {u0, v0},
                    std::pair {u1, v1},
                    std::pair {0.5 * (u0 + u1), 0.5 * (v0 + v1)},
                };
                for (const auto& [u, v] : probes) {
                    const gp_Vec ring = alongX * std::cos(u) + alongY * std::sin(u);
                    const gp_Vec reach = ring * (big + tube * std::cos(v))
                        + alongZ * (tube * std::sin(v));
                    const double miss = outer->Value(u, v).Distance(centre.Translated(reach));
                    if (miss > 1e-9 * (big + r + offset)) {
                        return false;
                    }
                }
                return true;
            };
            /* The normal turns over where R + r cos v changes sign : on a spindle torus or a tube
             * out of (0, R), the probes and the patch between them lie on different tubes
             */
            const auto layered = [&](double sign) {
                const double tubeAfter = r + sign * shift;
                return offset == 0.0 || (r < big && tubeAfter > 0.0 && tubeAfter < big);
            };
            // OCC folds nested offsets into one, a trimmed one too : one shift, two signs
            std::pair<double, double> tube {r - offset, r + offset};
            bool patch = true;
            if (onTorus(r + shift) && layered(1.0)) {
                tube = {r + shift, r + shift};
            }
            else if (onTorus(r - shift) && layered(-1.0)) {
                tube = {r - shift, r - shift};
            }
            else {
                patch = false;
            }
            const auto ring = times(tube, cosRange(v0, v1));
            const std::pair<double, double> rho {big + ring.first, big + ring.second};
            const auto lift
                = times(tube, cosRange(v0 - 0.5 * std::numbers::pi, v1 - 0.5 * std::numbers::pi));
            double low[3];
            double high[3];
            for (int i = 0; i < 3; ++i) {
                const double x = frame.XDirection().Coord(i + 1);
                const double y = frame.YDirection().Coord(i + 1);
                const double z = frame.Direction().Coord(i + 1);
                // The ring of radius R lies across the axis, the tube adds r all round
                const double whole = big * std::sqrt(std::max(0.0, 1.0 - z * z)) + r + offset;
                // X_i cos u + Y_i sin u = a cos(u - phi)
                const double phi = std::atan2(y, x);
                const auto turn = cosRange(u0 - phi, u1 - phi);
                const double amplitude = std::hypot(x, y);
                const auto across = times(rho, {amplitude * turn.first, amplitude * turn.second});
                const auto up = times(lift, {z, z});
                low[i] = centre.Coord(i + 1) - whole;
                high[i] = centre.Coord(i + 1) + whole;
                if (patch) {
                    low[i] = std::max(low[i], centre.Coord(i + 1) + across.first + up.first);
                    high[i] = std::min(high[i], centre.Coord(i + 1) + across.second + up.second);
                }
            }
            box.Update(low[0], low[1], low[2], high[0], high[1], high[2]);
        }
        /* Add bounds a curved face by its surface alone and an edge by its curve alone : a
         * boolean may leave their edges and vertices off them, within their own tolerance
         */
        for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(ex.Current());
            // A degenerate edge is its vertex at its tolerance, Add samples its pcurve
            if (BRep_Tool::Degenerated(edge)) {
                Bnd_Box pole;
                pole.Add(BRep_Tool::Pnt(TopExp::FirstVertex(edge)));
                pole.Enlarge(BRep_Tool::Tolerance(edge));
                box.Add(pole);
                continue;
            }
            Bnd_Box curve;
            BRepBndLib::Add(edge, curve, Standard_False);
            box.Add(curve);
            double first = 0.0;
            double last = 0.0;
            Handle(Geom_Curve) geometry = BRep_Tool::Curve(edge, first, last);
            auto trimmed = Handle(Geom_TrimmedCurve)::DownCast(geometry);
            if (!trimmed.IsNull()) {
                geometry = trimmed->BasisCurve();
            }
            auto parabola = Handle(Geom_Parabola)::DownCast(geometry);
            if (parabola.IsNull()) {
                continue;
            }
            // P(t) = O + t^2 / 4F X + t Y, furthest along axis i where t X_i / 2F + Y_i = 0
            const gp_Parab parab = parabola->Parab();
            for (int i = 1; i <= 3; ++i) {
                const double x = parab.XAxis().Direction().Coord(i);
                if (std::abs(x) < gp::Resolution()) {
                    continue;
                }
                const double t = -2.0 * parab.Focal() * parab.YAxis().Direction().Coord(i) / x;
                if (t > std::min(first, last) && t < std::max(first, last)) {
                    box.Add(parabola->Value(t));
                }
            }
        }
        for (TopExp_Explorer ex(shape, TopAbs_VERTEX); ex.More(); ex.Next()) {
            box.Add(BRep_Tool::Pnt(TopoDS::Vertex(ex.Current())));
        }
    }
    catch (...) {
        // No box at all : every node gets measured, none is skipped
        box.SetWhole();
    }
    return box;
}

namespace
{

/* ExtPC gives up on a point on the axis of a circle : every point of it lies as far. From rho
 * off the axis, none is nearer than |R - rho|. -1 for a curve that isn't a circle
 */
double circleGap(const Handle(Geom_Curve) & curve, const gp_Pnt& point, double* rho = nullptr)
{
    Handle(Geom_Curve) basis = curve;
    auto trimmed = Handle(Geom_TrimmedCurve)::DownCast(basis);
    if (!trimmed.IsNull()) {
        basis = trimmed->BasisCurve();
    }
    auto circle = Handle(Geom_Circle)::DownCast(basis);
    if (circle.IsNull()) {
        return -1.0;
    }
    const gp_Circ circ = circle->Circ();
    const double off = gp_Lin(circ.Axis()).Distance(point);
    if (rho) {
        *rho = off;
    }
    return std::abs(circ.Radius() - off);
}

// The old measure, BRepExtrema_DistShapeShape, kept as the last fallback
bool isCloserThan(const TopoDS_Shape& shape, const gp_Pnt& point, double limit)
{
    // No exception can leave an OpenMP region, a failed measure counts as far
    try {
        BRepExtrema_DistShapeShape measure(shape, BRepBuilderAPI_MakeVertex(point).Vertex());
        return measure.IsDone() && measure.NbSolution() > 0 && measure.Value() < limit;
    }
    catch (...) {
        return false;
    }
}

/* The fallback keeps the loose vertices and edges as well : a node within their own tolerance
 * lies on them, and on the face or edge they bound
 */
class LooseParts
{
public:
    LooseParts(const TopoDS_Shape& shape, double limit)
    {
        /* The map keeps one of each vertex and seam. Every vertex counts : a node the vertex
         * search keeps, its faces and edges keep too
         */
        TopTools_IndexedMapOfShape parts;
        TopExp::MapShapes(shape, TopAbs_VERTEX, parts);
        for (int i = 1; i <= parts.Extent(); ++i) {
            const TopoDS_Vertex& vertex = TopoDS::Vertex(parts(i));
            vertices.emplace_back(BRep_Tool::Pnt(vertex), BRep_Tool::Tolerance(vertex));
        }
        parts.Clear();
        TopExp::MapShapes(shape, TopAbs_EDGE, parts);
        for (int i = 1; i <= parts.Extent(); ++i) {
            const TopoDS_Edge& edge = TopoDS::Edge(parts(i));
            const double tolerance = BRep_Tool::Tolerance(edge);
            if (tolerance > limit) {
                // The box reaches the own tolerance of the edge, whatever boxOf already counts
                Bnd_Box box = boxOf(edge);
                box.Enlarge(tolerance);
                edges.push_back({edge, box, tolerance});
            }
        }
    }

    bool hold(const gp_Pnt& point) const
    {
        for (const auto& [at, tolerance] : vertices) {
            // The vertex search keeps a node right at the tolerance, so do its faces and edges
            if (point.SquareDistance(at) <= tolerance * tolerance) {
                return true;
            }
        }
        std::optional<TopoDS_Vertex> vertex;
        for (const auto& edge : edges) {
            if (edge.box.IsOut(point)) {
                continue;
            }
            // Edge by edge : one failed measure leaves the others to be tried
            try {
                if (!vertex) {
                    vertex = BRepBuilderAPI_MakeVertex(point).Vertex();
                }
                BRepExtrema_DistShapeShape measure(edge.shape, *vertex);
                if (measure.IsDone() && measure.NbSolution() > 0 && measure.Value() < edge.tolerance) {
                    return true;
                }
            }
            catch (...) {
                continue;
            }
        }
        return false;
    }

private:
    struct Edge
    {
        TopoDS_Edge shape;
        Bnd_Box box;
        double tolerance;
    };
    std::vector<std::pair<gp_Pnt, double>> vertices;
    std::vector<Edge> edges;
};

// The probe can't tell : a projection that didn't converge, a foot it can't place
struct Unsure
{
};

/* The faces, edges and vertices of a shape, the parts BRepExtrema_DistShapeShape
 * measures. Read once, each thread then builds its own BoundaryProbe on them
 */
struct BoundaryParts
{
    BoundaryParts(const TopoDS_Shape& shape, double limit, const Bnd_Box& shapeBox);

    /* How far a parameter collapses at (u, v) : its speed over its own speed mid face, never a
     * radian against a millimetre. aroundU tells which one runs around the pole
     */
    double collapse(size_t f, double u, double v, bool& aroundU) const;

    // Normal leaving the solid, at a pole its limit at this u
    bool outwardNormal(size_t f, double u, double v, gp_Vec& normal) const;

    std::vector<TopoDS_Face> faces;
    std::vector<Bnd_Box> faceBoxes;
    std::vector<TopoDS_Edge> edges;
    std::vector<Handle(Geom_Curve)> edgeCurves;
    std::vector<Bnd_Box> edgeBoxes;
    std::vector<std::pair<double, double>> edgeRanges;
    std::vector<std::pair<gp_Pnt, gp_Pnt>> edgeEnds;
    std::vector<gp_Pnt> corners;
    /* Squared reach of each vertex and edge : the limit, or the own tolerance of an edge, or of a
     * degenerate edge at its vertex, when looser
     */
    std::vector<double> cornerReach2;
    std::vector<double> edgeReach2;
    // Up to its own tolerance a node is on the vertex, as the vertex search has it : <= there
    std::vector<double> cornerOwn2;
    bool fast = true;
    // A face or an edge folded C0 inside : the old measure takes the shape whole
    bool creased = false;
    double tolerance;
    // The loosest vertex : two edge ends meeting there lie this far apart at most
    double cornerTolerance = 0.0;

    std::vector<Handle(Geom_Surface)> surfaces;
    std::vector<std::array<double, 4>> faceBounds;
    /* The edges bounding each face, with the way the face runs along them. A seam is
     * left out : the face goes on across it
     */
    std::vector<std::vector<std::pair<size_t, bool>>> faceEdges;
    /* Inside each face though FClass2d calls ON by them : its seams, and the points its
     * degenerate edges stand on
     */
    std::vector<std::vector<size_t>> faceSeams;
    std::vector<std::vector<gp_Pnt>> facePoles;

private:
    // A parameter this much slower than mid face has collapsed into a pole
    static constexpr double poleRatio = 1e-9;

    static double poleStep(double at, double first, double last);
};

/* BRepExtrema_ExtPF without its face classifier, which calls OUT the face along an edge
 * extended past a reflex corner. FClass2d sorts the feet instead
 */
class FaceFeet
{
public:
    bool Load(const TopoDS_Face& face, const std::array<double, 4>& bounds);
    void Perform(const gp_Pnt& point);
    bool IsDone() const;
    int NbExt() const;

    /* Foot k on the face, polished, false past its range : it lies there on the other nappe
     * of a cone
     */
    bool Foot(int k, const gp_Pnt& point, double& u, double& v, gp_Pnt& at, double& d2) const;

private:
    /* Near a pole, not only on it : a foot stopped 1e-7 off the apex of a skewed cone sat on
     * the wrong generatrix. The normal waits for the pole itself, poleRatio of BoundaryParts
     */
    static constexpr double nearPoleRatio = 1e-3;

    void leanFromPole(
        const gp_Pnt& point,
        double& u,
        double& v,
        gp_Pnt& at,
        gp_Vec& su,
        gp_Vec& sv,
        double& d2
    ) const;

    // Extrema_ExtPS keeps a pointer to it : a FaceFeet never moves once initialised
    BRepAdaptor_Surface surface;
    Extrema_ExtPS extrema;
    double u0 = 0.0;
    double u1 = 0.0;
    double v0 = 0.0;
    double v1 = 0.0;
    bool usable = false;
};

// One per thread : the extrema keep state between two points
class BoundaryProbe
{
public:
    explicit BoundaryProbe(const BoundaryParts& parts);

    bool isReady() const;

    /* Closer than sqrt(limit2) to a face, or than its own tolerance to a vertex or an edge. Throws
     * Unsure when it can't tell
     */
    bool isNear(const gp_Pnt& point, double limit2);

private:
    // gap, when given, the distance from the foot to the nearest edge the face ends on
    int footOnFace(size_t f, const gp_Pnt& at, double u, double v, double* gap = nullptr);
    double closeToBoundary() const;
    // The squared distance from point to edge e at its foot, polished from t
    double polishOnEdge(size_t e, const gp_Pnt& point, double& t, gp_Pnt& at) const;

    const BoundaryParts& parts;
    std::vector<FaceFeet> onFaces;
    std::vector<std::unique_ptr<BRepTopAdaptor_FClass2d>> insideFaces;
    std::vector<BRepExtrema_ExtPC> onEdges;
    bool ready = false;
};

}  // namespace


/* Where a face or an edge breaks C1 its normal or its tangent turns : a crease. A rational
 * circle breaks C1 too but keeps its tangent
 */
static bool hasCrease(
    const TopoDS_Shape& shape,
    const std::vector<Handle(Geom_Surface)>& surfaces,
    const std::vector<std::array<double, 4>>& bounds
)
{
    const double turn = 1e-3;
    for (size_t f = 0; f < surfaces.size(); ++f) {
        // As BRepAdaptor_Surface restricted to the face, with the UV bounds read once already
        const auto& [u0, u1, v0, v1] = bounds[f];
        const GeomAdaptor_Surface surface(surfaces[f], u0, u1, v0, v1);
        const auto normal = [&](double u, double v, gp_Vec& n) {
            gp_Pnt p;
            gp_Vec du;
            gp_Vec dv;
            surface.D1(u, v, p, du, dv);
            n = du.Crossed(dv);
            return n.SquareMagnitude() > gp::Resolution();
        };
        for (const bool alongU : {true, false}) {
            const int count = alongU ? surface.NbUIntervals(GeomAbs_C1)
                                     : surface.NbVIntervals(GeomAbs_C1);
            if (count < 2) {
                continue;
            }
            TColStd_Array1OfReal breaks(1, count + 1);
            if (alongU) {
                surface.UIntervals(breaks, GeomAbs_C1);
            }
            else {
                surface.VIntervals(breaks, GeomAbs_C1);
            }
            const double step = 1e-9 * (alongU ? u1 - u0 : v1 - v0);
            for (int i = 2; i <= count; ++i) {
                // One side of the break, then the other : crossing it rebuilds the B-spline cache
                std::array<gp_Vec, 5> before;
                std::array<gp_Vec, 5> after;
                std::array<bool, 5> sound {};
                for (const double side : {-step, step}) {
                    for (int k = 0; k < 5; ++k) {
                        const double across = alongU ? v0 + (v1 - v0) * (k + 0.5) / 5
                                                     : u0 + (u1 - u0) * (k + 0.5) / 5;
                        const double at = breaks(i) + side;
                        gp_Vec& n = (side < 0.0) ? before[k] : after[k];
                        const bool fine = normal(alongU ? at : across, alongU ? across : at, n);
                        sound[k] = (side < 0.0) ? fine : (sound[k] && fine);
                    }
                }
                for (int k = 0; k < 5; ++k) {
                    if (sound[k] && before[k].Angle(after[k]) > turn) {
                        return true;
                    }
                }
            }
        }
    }
    for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
        const TopoDS_Edge& edge = TopoDS::Edge(ex.Current());
        if (BRep_Tool::Degenerated(edge)) {
            continue;
        }
        const BRepAdaptor_Curve curve(edge);
        const int count = curve.NbIntervals(GeomAbs_C1);
        if (count < 2) {
            continue;
        }
        TColStd_Array1OfReal breaks(1, count + 1);
        curve.Intervals(breaks, GeomAbs_C1);
        const double step = 1e-9 * (curve.LastParameter() - curve.FirstParameter());
        for (int i = 2; i <= count; ++i) {
            gp_Pnt p;
            gp_Vec before;
            gp_Vec after;
            curve.D1(breaks(i) - step, p, before);
            curve.D1(breaks(i) + step, p, after);
            if (before.SquareMagnitude() > gp::Resolution()
                && after.SquareMagnitude() > gp::Resolution() && before.Angle(after) > turn) {
                return true;
            }
        }
    }
    return false;
}

BoundaryParts::BoundaryParts(const TopoDS_Shape& shape, double limit, const Bnd_Box& shapeBox)
    : tolerance(limit)
{
    TopTools_IndexedMapOfShape faceMap;
    TopTools_IndexedMapOfShape edgeMap;
    TopTools_IndexedMapOfShape vertexMap;
    TopExp::MapShapes(shape, TopAbs_FACE, faceMap);
    TopExp::MapShapes(shape, TopAbs_EDGE, edgeMap);
    TopExp::MapShapes(shape, TopAbs_VERTEX, vertexMap);
    for (int i = 1; i <= faceMap.Extent(); ++i) {
        /* A distance has no side : an Internal or External face is read Forward. Else its edges
         * come Internal too, none is kept for the boundary and a disc passed for a closed face
         */
        TopoDS_Face face = TopoDS::Face(faceMap(i));
        if (face.Orientation() == TopAbs_INTERNAL || face.Orientation() == TopAbs_EXTERNAL) {
            face.Orientation(TopAbs_FORWARD);
        }
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        if (surface.IsNull()) {
            fast = false;
        }
        faces.push_back(face);
        surfaces.push_back(surface);
        // The box of a face searched alone is the one already built
        Bnd_Box box = (shape.ShapeType() == TopAbs_FACE) ? shapeBox : boxOf(face);
        box.Enlarge(limit);
        faceBoxes.push_back(box);
        // Read once : the crease test, the feet and the seams all take them from here
        std::array<double, 4> bounds {};
        try {
            Handle(Geom_Surface) basis = surface;
            auto trimmed = Handle(Geom_RectangularTrimmedSurface)::DownCast(basis);
            if (!trimmed.IsNull()) {
                basis = trimmed->BasisSurface();
            }
            auto plane = Handle(Geom_Plane)::DownCast(basis);
            if (!plane.IsNull() && !box.IsVoid() && !box.IsWhole()) {
                /* A plane keeps no pcurves and UVBounds projects every edge on it again. Its 3D box
                 * read in the plane holds the face
                 */
                double corner[6];
                box.Get(corner[0], corner[1], corner[2], corner[3], corner[4], corner[5]);
                const double huge = Precision::Infinite();
                bounds = {huge, -huge, huge, -huge};
                for (int k = 0; k < 8; ++k) {
                    const gp_Pnt at(
                        corner[(k & 1) ? 3 : 0],
                        corner[(k & 2) ? 4 : 1],
                        corner[(k & 4) ? 5 : 2]
                    );
                    double u = 0.0;
                    double v = 0.0;
                    ElSLib::Parameters(plane->Pln(), at, u, v);
                    bounds[0] = std::min(bounds[0], u);
                    bounds[1] = std::max(bounds[1], u);
                    bounds[2] = std::min(bounds[2], v);
                    bounds[3] = std::max(bounds[3], v);
                }
            }
            else {
                BRepTools::UVBounds(face, bounds[0], bounds[1], bounds[2], bounds[3]);
            }
        }
        catch (...) {
            fast = false;
        }
        faceBounds.push_back(bounds);
    }
    /* A face or an edge folded C0 inside misleads the extrema : they stop on the far side of the
     * crease and that foot decides. Such a shape goes the old way, no faster, no worse
     */
    try {
        creased = !fast || hasCrease(shape, surfaces, faceBounds);
    }
    catch (...) {
        creased = true;
    }
    if (creased) {
        fast = false;
        return;
    }
    std::vector<int> slot(edgeMap.Extent() + 1, -1);
    std::vector<std::pair<int, double>> poles;
    for (int i = 1; i <= edgeMap.Extent(); ++i) {
        const TopoDS_Edge& edge = TopoDS::Edge(edgeMap(i));
        if (BRep_Tool::Degenerated(edge)) {
            // A degenerate edge is its vertex : the vertex takes its tolerance when looser
            poles.emplace_back(
                vertexMap.FindIndex(TopExp::FirstVertex(edge)),
                BRep_Tool::Tolerance(edge)
            );
            continue;
        }
        double first;
        double last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        if (curve.IsNull()) {
            fast = false;
        }
        slot[i] = static_cast<int>(edges.size());
        edges.push_back(edge);
        edgeCurves.push_back(curve);
        edgeRanges.emplace_back(first, last);
        // The ends of the curve, not the vertices : a vertex can sit off them within its tolerance
        if (curve.IsNull()) {
            edgeEnds.emplace_back();
        }
        else {
            edgeEnds.emplace_back(curve->Value(first), curve->Value(last));
        }
        const double reach = std::max(limit, BRep_Tool::Tolerance(edge));
        edgeReach2.push_back(reach * reach);
        Bnd_Box box = boxOf(edge);
        box.Enlarge(reach);
        edgeBoxes.push_back(box);
    }
    for (int i = 1; i <= vertexMap.Extent(); ++i) {
        const TopoDS_Vertex& vertex = TopoDS::Vertex(vertexMap(i));
        corners.push_back(BRep_Tool::Pnt(vertex));
        cornerTolerance = std::max(cornerTolerance, BRep_Tool::Tolerance(vertex));
        cornerOwn2.push_back(BRep_Tool::Tolerance(vertex) * BRep_Tool::Tolerance(vertex));
        cornerReach2.push_back(limit * limit);
    }
    for (const auto& [index, tolerance] : poles) {
        if (index > 0) {
            cornerTolerance = std::max(cornerTolerance, tolerance);
            cornerReach2[index - 1] = std::max(cornerReach2[index - 1], tolerance * tolerance);
        }
    }
    faceEdges.resize(faces.size());
    faceSeams.resize(faces.size());
    facePoles.resize(faces.size());
    try {
        for (size_t f = 0; fast && f < faces.size(); ++f) {
            /* A seam is run both ways. IsClosed alone took for a seam the cut of a half torus
             * made along the old seam : 2 pcurves, one of them off the face
             */
            std::map<int, int> runs;
            for (TopExp_Explorer ex(faces[f], TopAbs_EDGE); ex.More(); ex.Next()) {
                const TopoDS_Edge& edge = TopoDS::Edge(ex.Current());
                const int e = slot[edgeMap.FindIndex(edge)];
                const TopAbs_Orientation along = edge.Orientation();
                if (e >= 0 && (along == TopAbs_FORWARD || along == TopAbs_REVERSED)) {
                    runs[e] |= (along == TopAbs_REVERSED) ? 2 : 1;
                }
                else if (e < 0 && BRep_Tool::Degenerated(edge)) {
                    facePoles[f].push_back(BRep_Tool::Pnt(TopExp::FirstVertex(edge)));
                }
            }
            for (const auto& [e, way] : runs) {
                if (way != 3) {
                    faceEdges[f].emplace_back(e, way == 2);
                }
                else {
                    faceSeams[f].push_back(e);
                }
            }
        }
    }
    catch (...) {
        fast = false;
    }
}

double BoundaryParts::collapse(size_t f, double u, double v, bool& aroundU) const
{
    const auto& [u0, u1, v0, v1] = faceBounds[f];
    GeomLProp_SLProps here(surfaces[f], u, v, 1, Precision::Confusion());
    GeomLProp_SLProps row(surfaces[f], u, 0.5 * (v0 + v1), 1, Precision::Confusion());
    GeomLProp_SLProps column(surfaces[f], 0.5 * (u0 + u1), v, 1, Precision::Confusion());
    const double alongU = here.D1U().Magnitude() / std::max(row.D1U().Magnitude(), gp::Resolution());
    const double alongV = here.D1V().Magnitude()
        / std::max(column.D1V().Magnitude(), gp::Resolution());
    aroundU = alongU <= alongV;
    return std::min(alongU, alongV);
}

bool BoundaryParts::outwardNormal(size_t f, double u, double v, gp_Vec& normal) const
{
    const auto& [u0, u1, v0, v1] = faceBounds[f];
    bool aroundU = true;
    /* On a pole the normal is the cross product of a vanishing derivative, upside down at a
     * NURBS cone apex : it's read one step off
     */
    if (collapse(f, u, v, aroundU) <= poleRatio) {
        (aroundU ? v : u) += aroundU ? poleStep(v, v0, v1) : poleStep(u, u0, u1);
    }
    GeomLProp_SLProps props(surfaces[f], u, v, 1, Precision::Confusion());
    if (!props.IsNormalDefined()) {
        return false;
    }
    normal = gp_Vec(props.Normal());
    if (faces[f].Orientation() == TopAbs_REVERSED) {
        normal.Reverse();
    }
    return true;
}

// A millionth of the range off a pole, towards the middle of the face
double BoundaryParts::poleStep(double at, double first, double last)
{
    return ((first + last) * 0.5 > at ? 1e-6 : -1e-6) * (last - first);
}

bool FaceFeet::Load(const TopoDS_Face& face, const std::array<double, 4>& bounds)
{
    surface.Initialize(face, Standard_False);
    usable = surface.GetType() != GeomAbs_OtherSurface;
    if (!usable) {
        return false;
    }
    u0 = bounds[0];
    u1 = bounds[1];
    v0 = bounds[2];
    v1 = bounds[3];
    /* Minima and maxima as ExtPF, minima alone found nothing on a flat patch scaled to 0.01. The
     * parameter tolerances are PConfusion : BuildGrid adds them to squared distances
     */
    extrema.Initialize(surface, u0, u1, v0, v1, Precision::PConfusion(), Precision::PConfusion());
    return true;
}

void FaceFeet::Perform(const gp_Pnt& point)
{
    if (usable) {
        extrema.Perform(point);
    }
}

bool FaceFeet::IsDone() const
{
    return usable && extrema.IsDone();
}

int FaceFeet::NbExt() const
{
    return extrema.NbExt();
}

bool FaceFeet::Foot(int k, const gp_Pnt& point, double& u, double& v, gp_Pnt& at, double& d2) const
{
    extrema.Point(k).Parameter(u, v);
    const double du = 1e-9 * std::max(1.0, u1 - u0);
    const double dv = 1e-9 * std::max(1.0, v1 - v0);
    if (u < u0 - du || u > u1 + du || v < v0 - dv || v > v1 + dv) {
        return false;
    }
    u = std::clamp(u, u0, u1);
    v = std::clamp(v, v0, v1);
    gp_Vec su;
    gp_Vec sv;
    surface.D1(u, v, at, su, sv);
    d2 = point.SquareDistance(at);
    leanFromPole(point, u, v, at, su, sv, d2);
    /* Newton on the squared distance : ExtPS stopped 0.03 radian off the foot by a NURBS
     * cone apex, and Gauss-Newton crawled there
     */
    for (int i = 0; i < 8 && d2 > 0.0; ++i) {
        gp_Vec suu;
        gp_Vec svv;
        gp_Vec suv;
        surface.D2(u, v, at, su, sv, suu, svv, suv);
        const gp_Vec r(point, at);
        const double a = su.SquareMagnitude();
        const double b = su.Dot(sv);
        const double c = sv.SquareMagnitude();
        const double hu = a + r.Dot(suu);
        const double huv = b + r.Dot(suv);
        const double hv = c + r.Dot(svv);
        double stepU = 0.0;
        double stepV = 0.0;
        // On a pole one derivative vanishes, a step along the other leaves it
        if (a <= 1e-18 * c && c > 0.0) {
            stepV = -sv.Dot(r) / c;
        }
        else if (c <= 1e-18 * a && a > 0.0) {
            stepU = -su.Dot(r) / a;
        }
        else if (hu > 0.0 && hu * hv - huv * huv > 1e-12 * hu * hv) {
            const double det = hu * hv - huv * huv;
            stepU = (huv * sv.Dot(r) - hv * su.Dot(r)) / det;
            stepV = (huv * su.Dot(r) - hu * sv.Dot(r)) / det;
        }
        // Where the curvature holds no minimum : the step of Gauss-Newton
        else if (a * c - b * b > 1e-12 * a * c) {
            const double det = a * c - b * b;
            stepU = (b * sv.Dot(r) - c * su.Dot(r)) / det;
            stepV = (b * su.Dot(r) - a * sv.Dot(r)) / det;
        }
        else {
            break;
        }
        // Converged once the step moves the foot by rounding alone
        const double move = (su * stepU + sv * stepV).Magnitude();
        if (move <= 8.0 * std::numeric_limits<double>::epsilon() * std::max(1.0, at.XYZ().Modulus())) {
            break;
        }
        /* A step that goes farther is halved, not given up : by a pole the linear step in u
         * overshoots, and the foot stopped 3.4 degrees off its normal 1.5e-7 from the apex
         */
        bool moved = false;
        for (double share = 1.0; !moved && share > 1e-6; share *= 0.5) {
            const double nextU = std::clamp(u + share * stepU, u0, u1);
            const double nextV = std::clamp(v + share * stepV, v0, v1);
            const gp_Pnt next = surface.Value(nextU, nextV);
            const double nextD2 = point.SquareDistance(next);
            if (nextD2 < d2) {
                u = nextU;
                v = nextV;
                at = next;
                d2 = nextD2;
                moved = true;
            }
        }
        if (!moved) {
            break;
        }
    }
    return true;
}

/* By a pole ExtPS leaves the parameter running around it anywhere. The foot turns to the
 * generatrix leaning most to the point, when that brings it no farther
 */
void FaceFeet::leanFromPole(
    const gp_Pnt& point,
    double& u,
    double& v,
    gp_Pnt& at,
    gp_Vec& su,
    gp_Vec& sv,
    double& d2
) const
{
    gp_Pnt middle;
    gp_Vec middleU;
    gp_Vec middleV;
    surface.D1(u, 0.5 * (v0 + v1), middle, middleU, middleV);
    const bool aroundU = su.Magnitude() < nearPoleRatio * middleU.Magnitude();
    if (!aroundU) {
        surface.D1(0.5 * (u0 + u1), v, middle, middleU, middleV);
        if (sv.Magnitude() >= nearPoleRatio * middleV.Magnitude()) {
            return;
        }
    }
    const gp_Vec toPoint(at, point);
    const double leave = aroundU ? ((v - v0 < v1 - v) ? 1.0 : -1.0)
                                 : ((u - u0 < u1 - u) ? 1.0 : -1.0);
    const double low = aroundU ? u0 : v0;
    const double high = aroundU ? u1 : v1;
    const auto leanAt = [&](double around) {
        gp_Pnt q;
        gp_Vec alongU;
        gp_Vec alongV;
        surface.D1(aroundU ? around : u, aroundU ? v : around, q, alongU, alongV);
        const gp_Vec& way = aroundU ? alongV : alongU;
        const double magnitude = way.Magnitude();
        return magnitude > 0.0 ? leave * way.Dot(toPoint) / magnitude
                               : -std::numeric_limits<double>::infinity();
    };
    double lean = -std::numeric_limits<double>::infinity();
    double pick = aroundU ? u : v;
    const int samples = 64;
    for (int n = 0; n < samples; ++n) {
        const double around = low + (n + 0.5) / samples * (high - low);
        const double tmp = leanAt(around);
        if (tmp > lean) {
            lean = tmp;
            pick = around;
        }
    }
    /* A sample lies up to 2.3 degrees off the generatrix, too far for Newton to turn : the
     * step in u grows as the foot nears the pole. Golden section on the lean, one sample around
     */
    if (lean > -std::numeric_limits<double>::infinity()) {
        const double golden = 0.5 * (std::sqrt(5.0) - 1.0);
        double a = std::max(low, pick - (high - low) / samples);
        double b = std::min(high, pick + (high - low) / samples);
        double left = b - golden * (b - a);
        double right = a + golden * (b - a);
        double leanLeft = leanAt(left);
        double leanRight = leanAt(right);
        // Closer than 1e-8 of the range 2 leans differ by rounding alone, sqrt(eps)
        for (int i = 0; i < 60 && b - a > 1e-8 * (high - low); ++i) {
            if (leanLeft < leanRight) {
                a = left;
                left = right;
                leanLeft = leanRight;
                right = a + golden * (b - a);
                leanRight = leanAt(right);
            }
            else {
                b = right;
                right = left;
                leanRight = leanLeft;
                left = b - golden * (b - a);
                leanLeft = leanAt(left);
            }
        }
        if (std::max(leanLeft, leanRight) > lean) {
            pick = leanLeft > leanRight ? left : right;
        }
    }
    double pickU = aroundU ? pick : u;
    double pickV = aroundU ? v : pick;
    gp_Pnt there;
    gp_Vec thereU;
    gp_Vec thereV;
    surface.D1(pickU, pickV, there, thereU, thereV);
    /* A generatrix is judged by its point closest to the node, not the one at this height :
     * 3.78e-6 down it against 6.08e-6 level with the foot on a cone sheared by 3
     */
    const gp_Vec& down = aroundU ? thereV : thereU;
    if (down.SquareMagnitude() > 0.0) {
        const double slide = gp_Vec(there, point).Dot(down) / down.SquareMagnitude();
        (aroundU ? pickV : pickU) = aroundU ? std::clamp(pickV + slide, v0, v1)
                                            : std::clamp(pickU + slide, u0, u1);
        surface.D1(pickU, pickV, there, thereU, thereV);
    }
    if (point.SquareDistance(there) <= d2) {
        u = pickU;
        v = pickV;
        at = there;
        su = thereU;
        sv = thereV;
        d2 = point.SquareDistance(there);
    }
}


BoundaryProbe::BoundaryProbe(const BoundaryParts& parts)
    : parts(parts)
    , onFaces(parts.faces.size())
    , onEdges(parts.edges.size())
{
    ready = parts.fast;
    // The threads read the same faces, one at a time while the adaptors are built
#pragma omp critical(boundaryProbe)
    {
        try {
            for (size_t k = 0; ready && k < parts.faces.size(); ++k) {
                // A face with no surface to project on would be skipped in silence
                ready = onFaces[k].Load(parts.faces[k], parts.faceBounds[k]);
                insideFaces.push_back(
                    std::make_unique<BRepTopAdaptor_FClass2d>(parts.faces[k], Precision::Confusion())
                );
            }
            for (size_t k = 0; ready && k < parts.edges.size(); ++k) {
                onEdges[k].Initialize(parts.edges[k]);
            }
        }
        catch (...) {
            ready = false;
        }
    }
}

bool BoundaryProbe::isReady() const
{
    return ready;
}

// Up to where an OUT foot of FClass2d is judged again by the nearest edge of its face
double BoundaryProbe::closeToBoundary() const
{
    return 100.0 * std::max(parts.tolerance, parts.cornerTolerance);
}

bool BoundaryProbe::isNear(const gp_Pnt& point, double limit2)
{
    const double closeOut = closeToBoundary();
    for (size_t c = 0; c < parts.corners.size(); ++c) {
        // Right at its own tolerance is on the vertex, the limit and the edges stay strict
        const double d2 = point.SquareDistance(parts.corners[c]);
        if (d2 <= parts.cornerOwn2[c] || d2 < parts.cornerReach2[c]) {
            return true;
        }
    }
    const TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(point).Vertex();
    bool unsure = false;
    for (size_t f = 0; f < onFaces.size(); ++f) {
        if (parts.faceBoxes[f].IsOut(point)) {
            continue;
        }
        onFaces[f].Perform(point);
        // Left to the end : an edge or a vertex can still take the node without the fallback
        if (!onFaces[f].IsDone()) {
            unsure = true;
            continue;
        }
        for (int k = 1; k <= onFaces[f].NbExt(); ++k) {
            double u;
            double v;
            gp_Pnt at;
            double d2;
            if (!onFaces[f].Foot(k, point, u, v, at, d2) || d2 >= limit2) {
                continue;
            }
            /* A foot past the boundary is OUT, the edges and vertices measure it. FClass2d calls
             * ON a foot by a seam, and OUT feet 3.7e-6 inside a torus sliver : footOnFace decides
             */
            const TopAbs_State where = insideFaces[f]->Perform(gp_Pnt2d(u, v));
            if (where == TopAbs_IN) {
                return true;
            }
            /* An OUT foot farther than closeOut from every edge it could end on never counts, no
             * projection needed. A round hole keeps its centre in the box of its circle
             */
            if (where == TopAbs_OUT && !parts.faceEdges[f].empty()) {
                Bnd_Box around;
                around.Add(at);
                around.Enlarge(closeOut);
                bool clear = true;
                for (const auto& side : parts.faceEdges[f]) {
                    clear = clear && parts.edgeBoxes[side.first].IsOut(around);
                }
                if (clear) {
                    continue;
                }
            }
            double gap = 0.0;
            const int on = footOnFace(f, at, u, v, &gap);
            if (on > 0 && (where == TopAbs_ON || gap <= closeOut)) {
                return true;
            }
            // Undecided only matters for a foot that would have counted
            unsure = unsure || (on == 0 && (where == TopAbs_ON || gap <= closeOut));
        }
    }
    for (size_t e = 0; e < onEdges.size(); ++e) {
        if (parts.edgeBoxes[e].IsOut(point)) {
            continue;
        }
        const double reach2 = std::max(limit2, parts.edgeReach2[e]);
        const auto& [start, end] = parts.edgeEnds[e];
        if (point.SquareDistance(start) < reach2 || point.SquareDistance(end) < reach2) {
            return true;
        }
        onEdges[e].Perform(vertex);
        if (!onEdges[e].IsDone()) {
            // No point of a circle is nearer than circleGap, the old measure isn't needed
            const double gap = circleGap(parts.edgeCurves[e], point);
            if (gap < 0.0 || gap * gap < reach2) {
                unsure = true;
            }
            continue;
        }
        const auto& [first, last] = parts.edgeRanges[e];
        for (int k = 1; k <= onEdges[e].NbExt(); ++k) {
            double t = onEdges[e].Parameter(k);
            if (t < std::min(first, last) || t > std::max(first, last)) {
                continue;
            }
            gp_Pnt at;
            if (polishOnEdge(e, point, t, at) < std::max(limit2, parts.edgeReach2[e])) {
                return true;
            }
        }
    }
    if (unsure) {
        throw Unsure();
    }
    return false;
}

/* ExtPC puts a foot closer than 1e-7 to an edge end onto that end, 5e-8 off it on a circle of
 * radius 5. Newton on (C(t) - p) . C'(t) brings it back, round and round a closed edge
 */
double BoundaryProbe::polishOnEdge(size_t e, const gp_Pnt& point, double& t, gp_Pnt& at) const
{
    const Handle(Geom_Curve) & curve = parts.edgeCurves[e];
    const auto& [first, last] = parts.edgeRanges[e];
    const double low = std::min(first, last);
    const double high = std::max(first, last);
    const bool round = curve->IsPeriodic()
        && std::abs(high - low - curve->Period()) <= Precision::PConfusion();
    at = curve->Value(t);
    double d2 = point.SquareDistance(at);
    for (int i = 0; i < 8 && d2 > 0.0; ++i) {
        gp_Pnt q;
        gp_Vec d1;
        gp_Vec dd;
        curve->D2(t, q, d1, dd);
        const gp_Vec r(point, q);
        const double bend = d1.SquareMagnitude() + r.Dot(dd);
        if (bend <= 0.0) {
            break;
        }
        double next = t - r.Dot(d1) / bend;
        if (round) {
            next = low + std::fmod(std::fmod(next - low, high - low) + (high - low), high - low);
        }
        else {
            next = std::clamp(next, low, high);
        }
        const gp_Pnt there = curve->Value(next);
        const double nextD2 = point.SquareDistance(there);
        if (nextD2 >= d2) {
            break;
        }
        t = next;
        at = there;
        d2 = nextD2;
    }
    return d2;
}

/* 1 when a face foot lies on the face, -1 off it, 0 undecided : left of the nearest edge
 * seen from outside, or by a vertex as its corner is convex or reflex
 */
int BoundaryProbe::footOnFace(size_t f, const gp_Pnt& at, double u, double v, double* gap)
{
    if (gap) {
        *gap = Precision::Infinite();
    }
    // Undecided : a nil gap sends the foot to the fallback, whatever FClass2d said
    const auto undecided = [gap]() {
        if (gap) {
            *gap = 0.0;
        }
        return 0;
    };
    Bnd_Box footBox;
    footBox.Add(at);
    // An edge ExtPC gave up on, judged by its ends : it lies no nearer than its box
    std::vector<size_t> undone;
    gp_Vec normal;
    if (!parts.outwardNormal(f, u, v, normal)) {
        return undecided();
    }
    struct Near
    {
        double gap;
        double left;
        int end;
        gp_Vec travel;
        gp_Pnt onEdge;
    };
    std::vector<Near> nearby;
    const TopoDS_Vertex vertex = BRepBuilderAPI_MakeVertex(at).Vertex();
    for (const auto& [e, reversed] : parts.faceEdges[f]) {
        const auto& [first, last] = parts.edgeRanges[e];
        std::vector<double> params {first, last};
        onEdges[e].Perform(vertex);
        const bool done = onEdges[e].IsDone();
        /* Within Confusion of a circle axis its ends lie as near as any of its points. ExtPC
         * gives up a little farther on a small circle, those stay undone
         */
        double rho = 1.0;
        if (!done
            && !(circleGap(parts.edgeCurves[e], at, &rho) >= 0.0 && rho <= Precision::Confusion())) {
            undone.push_back(e);
        }
        for (int k = 1; done && k <= onEdges[e].NbExt(); ++k) {
            double t = onEdges[e].Parameter(k);
            if (t < std::min(first, last) || t > std::max(first, last)) {
                continue;
            }
            gp_Pnt q;
            polishOnEdge(e, at, t, q);
            if (t > std::min(first, last) && t < std::max(first, last)) {
                params.push_back(t);
            }
        }
        for (size_t n = 0; n < params.size(); ++n) {
            gp_Pnt q;
            gp_Vec travel;
            parts.edgeCurves[e]->D1(params[n], q, travel);
            if (travel.SquareMagnitude() < gp::Resolution()) {
                continue;
            }
            if (reversed) {
                travel.Reverse();
            }
            travel.Normalize();
            const gp_Vec left = normal.Crossed(travel);
            // 1 where the travel along the edge starts, 2 where it ends, 0 inside it
            int end = 0;
            if (n < 2) {
                end = ((n == 0) != reversed) ? 1 : 2;
            }
            nearby.push_back({at.Distance(q), gp_Vec(q, at).Dot(left), end, travel, q});
        }
    }
    // A closed face, a sphere or a torus, has no boundary to be off
    if (parts.faceEdges[f].empty()) {
        if (gap) {
            *gap = 0.0;
        }
        return 1;
    }
    const Near* nearest = nullptr;
    for (const Near& n : nearby) {
        if (!nearest || n.gap < nearest->gap) {
            nearest = &n;
        }
    }
    if (!nearest) {
        return undecided();
    }
    for (const size_t e : undone) {
        if (parts.edgeBoxes[e].Distance(footBox) < nearest->gap) {
            return undecided();
        }
    }
    if (gap) {
        *gap = nearest->gap;
    }
    /* The face goes on across a seam and reaches its poles : a foot well nearer to them than to
     * any edge it ends on is on it. A needle apex and a half torus equator had such feet judged
     * off the face by an edge far away
     */
    double inner = Precision::Infinite();
    for (const gp_Pnt& pole : parts.facePoles[f]) {
        inner = std::min(inner, at.Distance(pole));
    }
    for (const size_t e : parts.faceSeams[f]) {
        const auto& [first, last] = parts.edgeRanges[e];
        inner = std::min(
            {inner,
             at.Distance(parts.edgeCurves[e]->Value(first)),
             at.Distance(parts.edgeCurves[e]->Value(last))}
        );
        onEdges[e].Perform(vertex);
        if (!onEdges[e].IsDone()) {
            // Only a seam that could pass the rule below matters
            if (parts.edgeBoxes[e].Distance(footBox) < 0.5 * nearest->gap) {
                return undecided();
            }
            continue;
        }
        for (int k = 1; k <= onEdges[e].NbExt(); ++k) {
            double t = onEdges[e].Parameter(k);
            if (t >= std::min(first, last) && t <= std::max(first, last)) {
                gp_Pnt q;
                inner = std::min(inner, std::sqrt(polishOnEdge(e, at, t, q)));
            }
        }
    }
    if (inner < Precision::Infinite() && inner < 0.5 * nearest->gap) {
        return 1;
    }
    /* Inside an edge, the side of that edge decides alone : the ends of a curved edge
     * look the other way and must not vote against it
     */
    if (nearest->end == 0) {
        return (nearest->left >= 0.0) ? 1 : -1;
    }
    // A vertex : the one edge leaving it and the one edge reaching it
    const Near* start = nullptr;
    const Near* finish = nullptr;
    for (const Near& n : nearby) {
        const double meet = 2.0 * std::max(parts.tolerance, parts.cornerTolerance);
        if (n.end == 0 || n.onEdge.Distance(nearest->onEdge) > meet) {
            continue;
        }
        const Near*& slotFor = (n.end == 1) ? start : finish;
        if (slotFor) {
            return 0;
        }
        slotFor = &n;
    }
    if (!start || !finish) {
        return 0;
    }
    if ((start->left >= 0.0) == (finish->left >= 0.0)) {
        return (start->left >= 0.0) ? 1 : -1;
    }
    // Left of both edges past a convex corner, of either past a reflex one
    const gp_Vec back = -finish->travel;
    const double angle = std::atan2(start->travel.Crossed(back).Dot(normal), start->travel.Dot(back));
    return (angle > 0.0 && angle < std::numbers::pi) ? -1 : 1;
}


std::set<int> nodesCloserThan(const FemMesh& mesh, const TopoDS_Shape& shape, double limit)
{
    std::set<int> result;

    // https://forum.freecad.org/viewtopic.php?f=18&t=21571&start=70#p221591
    Bnd_Box box = boxOf(shape);
    // A loose vertex or edge reaches past the face or edge itself
    ShapeAnalysis_ShapeTolerance analysis;
    box.Enlarge(analysis.Tolerance(shape, 1));
    const double limit2 = limit * limit;

    // The nodes listed one by one, then placed and boxed in parallel
    std::vector<const SMDS_MeshNode*> meshNodes;
    SMDS_NodeIteratorPtr aNodeIter = mesh.getSMesh()->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        meshNodes.push_back(aNodeIter->next());
    }
    const Base::Matrix4D placement(mesh.getTransform());
    // GetXYZ, never X() Y() Z() : SMDS says those aren't thread safe, a node read twice raced
    const auto placed = [&](const SMDS_MeshNode* aNode) {
        double xyz[3];
        aNode->GetXYZ(xyz);
        const Base::Vector3d vec = placement * Base::Vector3d(xyz[0], xyz[1], xyz[2]);
        return gp_Pnt(vec.x, vec.y, vec.z);
    };
    // A thread per 64 candidates at most : setting up a probe outweighs measuring a few nodes
    const int chunk = 64;
    int most = 1;
#ifdef _OPENMP
    most = omp_get_max_threads();
#endif
    /* One pass counts the nodes in the box and lists them while they're at most 64 x threads, each
     * thread up to that many. Past it the lists, a pointer a candidate, go and the whole mesh is
     * walked again
     */
    const long long cap = static_cast<long long>(chunk) * most;
    long long inBox = 0;
    bool broken = false;
    std::vector<const SMDS_MeshNode*> few;
#pragma omp parallel reduction(+ : inBox)
    {
        std::vector<const SMDS_MeshNode*> mine;
#pragma omp for schedule(static)
        for (size_t i = 0; i < meshNodes.size(); ++i) {
            // No exception can leave the loop : a failed list falls back on the whole mesh
            try {
                if (!box.IsOut(placed(meshNodes[i]))) {
                    ++inBox;
                    if (static_cast<long long>(mine.size()) < cap) {
                        mine.push_back(meshNodes[i]);
                    }
                }
            }
            catch (...) {
#pragma omp critical(nodeSearchList)
                broken = true;
            }
        }
#pragma omp critical(nodeSearchList)
        {
            try {
                few.insert(few.end(), mine.begin(), mine.end());
            }
            catch (...) {
                broken = true;
            }
        }
    }
    // No node in the box, no parts to read : a face far from the mesh costs its box only
    if (inBox == 0 && !broken) {
        return result;
    }
    const bool listed = !broken && inBox <= cap;
    if (!listed) {
        std::vector<const SMDS_MeshNode*>().swap(few);
    }
    const std::vector<const SMDS_MeshNode*>& walk = listed ? few : meshNodes;

    /* Same parts as BRepExtrema_DistShapeShape : face inside, edges and vertices. The extrema
     * are set up once per thread, a node only projects on them
     */
    std::unique_ptr<const BoundaryParts> parts;
    try {
        parts = std::make_unique<const BoundaryParts>(shape, limit, box);
    }
    catch (...) {
        // Unreadable parts : every node goes the old way
        parts.reset();
    }
    std::optional<const LooseParts> loose;
    try {
        loose.emplace(shape, limit);
    }
    catch (...) {
        // Unreadable vertices or edges : the fallback measures without them
        loose.reset();
    }
    const auto fallback = [&](const gp_Pnt& point) {
        return isCloserThan(shape, point, limit) || (loose && loose->hold(point));
    };
    // No exception can leave the region or the loop : the first one is kept and thrown after
    std::exception_ptr failure;
    int threads = 1;
#ifdef _OPENMP
    threads = static_cast<int>(std::min<long long>(most, (inBox + chunk - 1) / chunk));
#endif
    // A failed pass counted nothing for sure : every thread walks the whole mesh
    if (broken) {
        threads = most;
    }

#pragma omp parallel num_threads(threads)
    {
        std::optional<BoundaryProbe> probe;
        std::vector<int> kept;
        try {
            if (parts) {
                probe.emplace(*parts);
            }
        }
        catch (...) {
            probe.reset();
        }
        // Building a probe touches the shared faces : no thread measures before all are built
#pragma omp barrier

#pragma omp for schedule(dynamic, chunk)
        for (size_t i = 0; i < walk.size(); ++i) {
            try {
                const gp_Pnt point = placed(walk[i]);
                if (box.IsOut(point)) {
                    continue;
                }
                const int id = walk[i]->GetID();
                bool close = false;
                try {
                    close = probe && probe->isReady() ? probe->isNear(point, limit2)
                                                      : fallback(point);
                }
                catch (...) {
                    close = fallback(point);
                }
                if (close) {
                    kept.push_back(id);
                }
            }
            catch (...) {
#pragma omp critical(nodeSearchResult)
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }

#pragma omp critical(nodeSearchResult)
        {
            try {
                result.insert(kept.begin(), kept.end());
            }
            catch (...) {
                if (!failure) {
                    failure = std::current_exception();
                }
            }
        }
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
    return result;
}

}  // namespace Fem::NodeSearch
