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
#include <cmath>
#include <exception>
#include <utility>
#include <vector>

#include <BRepBndLib.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <BRepExtrema_DistShapeShape.hxx>
#include <BRep_Tool.hxx>
#include <Geom_OffsetSurface.hxx>
#include <Geom_Parabola.hxx>
#include <Geom_RectangularTrimmedSurface.hxx>
#include <Geom_ToroidalSurface.hxx>
#include <Geom_TrimmedCurve.hxx>
#include <SMDS_MeshNode.hxx>
#include <SMESHDS_Mesh.hxx>
#include <SMESH_Mesh.hxx>
#include <ShapeAnalysis_ShapeTolerance.hxx>
#include <Standard_Failure.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <gp.hxx>
#include <gp_Dir.hxx>
#include <gp_Parab.hxx>
#include <gp_Pnt.hxx>
#include <gp_Torus.hxx>

#include <Base/Matrix.h>

#include "FemMesh.h"
#include "FemMeshNodeSearch.h"


namespace Fem::NodeSearch
{

/* Add cuts some boxes short : a torus patch by 0.3 on a fillet, its offset, a parabola placed
 * askew by 0.018. Those take the box of the whole torus and the parabola points furthest out
 */
Bnd_Box boxOf(const TopoDS_Shape& shape)
{
    Bnd_Box box;
    BRepBndLib::Add(shape, box, Standard_False);
    try {
        for (TopExp_Explorer ex(shape, TopAbs_FACE); ex.More(); ex.Next()) {
            // Located already, trimmed and offset ones unwrapped down to the torus
            Handle(Geom_Surface) surface = BRep_Tool::Surface(TopoDS::Face(ex.Current()));
            double offset = 0.0;
            while (!surface.IsNull()) {
                auto trimmed = Handle(Geom_RectangularTrimmedSurface)::DownCast(surface);
                auto shifted = Handle(Geom_OffsetSurface)::DownCast(surface);
                if (!trimmed.IsNull()) {
                    surface = trimmed->BasisSurface();
                }
                else if (!shifted.IsNull()) {
                    offset += std::abs(shifted->Offset());
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
            const gp_Dir& axis = torus.Axis().Direction();
            const double r = torus.MinorRadius() + offset;
            // The ring of radius R lies across the axis, the tube adds r all round
            double reach[3];
            for (int i = 0; i < 3; ++i) {
                const double along = axis.Coord(i + 1);
                reach[i] = torus.MajorRadius() * std::sqrt(std::max(0.0, 1.0 - along * along)) + r;
            }
            box.Update(
                centre.X() - reach[0],
                centre.Y() - reach[1],
                centre.Z() - reach[2],
                centre.X() + reach[0],
                centre.Y() + reach[1],
                centre.Z() + reach[2]
            );
        }
        /* Add bounds a curved face by its surface alone and an edge by its curve alone : a
         * boolean may leave their edges and vertices off them, within their own tolerance
         */
        for (TopExp_Explorer ex(shape, TopAbs_EDGE); ex.More(); ex.Next()) {
            const TopoDS_Edge& edge = TopoDS::Edge(ex.Current());
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
    }
    catch (const Standard_Failure&) {
        // An unreadable surface or curve : no box at all, every node gets measured
        box.SetWhole();
        return box;
    }
    for (TopExp_Explorer ex(shape, TopAbs_VERTEX); ex.More(); ex.Next()) {
        box.Add(BRep_Tool::Pnt(TopoDS::Vertex(ex.Current())));
    }
    return box;
}

namespace
{

/* A boolean or a fillet leaves vertices and edges looser than their face : a node within
 * their own tolerance lies on them, and on the face as well
 */
struct LooseParts
{
    LooseParts(const TopoDS_Shape& shape, double limit)
    {
        // A vertex comes once per edge and a seam twice per face, the map keeps one of each
        TopTools_IndexedMapOfShape parts;
        TopExp::MapShapes(shape, TopAbs_VERTEX, parts);
        // Every vertex : a node the vertex search keeps, its faces and edges keep too
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
                edges.push_back({edge, boxOf(edge), tolerance});
            }
        }
    }

    bool hold(const gp_Pnt& point, const TopoDS_Shape& vertex) const
    {
        for (const auto& [at, tolerance] : vertices) {
            // The vertex search keeps a node right at the tolerance, so do its faces and edges
            if (point.SquareDistance(at) <= tolerance * tolerance) {
                return true;
            }
        }
        for (const auto& edge : edges) {
            if (edge.box.IsOut(point)) {
                continue;
            }
            BRepExtrema_DistShapeShape measure(edge.shape, vertex);
            if (measure.IsDone() && measure.NbSolution() > 0 && measure.Value() < edge.tolerance) {
                return true;
            }
        }
        return false;
    }

    struct Edge
    {
        TopoDS_Edge shape;
        Bnd_Box box;
        double tolerance;
    };
    std::vector<std::pair<gp_Pnt, double>> vertices;
    std::vector<Edge> edges;
};

}  // namespace

std::set<int> nodesCloserThan(const FemMesh& mesh, const TopoDS_Shape& shape, double limit)
{
    std::set<int> result;

    // https://forum.freecad.org/viewtopic.php?f=18&t=21571&start=70#p221591
    Bnd_Box box = boxOf(shape);
    // A loose vertex or edge reaches past the face or edge itself
    ShapeAnalysis_ShapeTolerance analysis;
    box.Enlarge(analysis.Tolerance(shape, 1));
    const LooseParts loose(shape, limit);

    // Get the current transform of the FemMesh
    const Base::Matrix4D Mtrx(mesh.getTransform());

    std::vector<const SMDS_MeshNode*> nodes;
    SMDS_NodeIteratorPtr aNodeIter = mesh.getSMesh()->GetMeshDS()->nodesIterator();
    while (aNodeIter->more()) {
        const SMDS_MeshNode* aNode = aNodeIter->next();
        nodes.push_back(aNode);
    }

    // No exception can leave the region : the first one is kept and thrown after
    std::exception_ptr failure;
#pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < nodes.size(); ++i) {
        try {
            const SMDS_MeshNode* aNode = nodes[i];
            double xyz[3];
            aNode->GetXYZ(xyz);
            Base::Vector3d vec(xyz[0], xyz[1], xyz[2]);
            // Apply the matrix to hold the BoundBox in absolute space
            vec = Mtrx * vec;

            if (!box.IsOut(gp_Pnt(vec.x, vec.y, vec.z))) {
                // Create a vertex
                BRepBuilderAPI_MakeVertex aBuilder(gp_Pnt(vec.x, vec.y, vec.z));
                TopoDS_Shape s = aBuilder.Vertex();
                // measure distance
                BRepExtrema_DistShapeShape measure(shape, s);
                const bool close = measure.IsDone() && measure.NbSolution() > 0
                    && measure.Value() < limit;

                if (close || loose.hold(gp_Pnt(vec.x, vec.y, vec.z), s))
#pragma omp critical(nodeSearchResult)
                {
                    result.insert(aNode->GetID());
                }
            }
        }
        catch (...) {
#pragma omp critical(nodeSearchResult)
            if (!failure) {
                failure = std::current_exception();
            }
        }
    }

    if (failure) {
        std::rethrow_exception(failure);
    }
    return result;
}

}  // namespace Fem::NodeSearch
