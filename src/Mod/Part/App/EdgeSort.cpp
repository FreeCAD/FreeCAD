// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileNotice: Part of the xwzCAD project.

/***************************************************************************
 *                                                                         *
 *   xwzCAD is free software: you can redistribute it and/or modify        *
 *   it under the terms of the GNU Lesser General Public License as        *
 *   published by the Free Software Foundation, either version 2.1         *
 *   of the License, or (at your option) any later version.                *
 *                                                                         *
 *   xwzCAD is distributed in the hope that it will be useful,             *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty           *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.               *
 *   See the GNU Lesser General Public License for more details.           *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with xwzCAD. If not, see https://www.gnu.org/licenses   *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRep_Tool.hxx>
# include <BRepLib.hxx>
# include <Geom_Curve.hxx>
# include <gp_Pnt.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Vertex.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include "EdgeSort.h"

using namespace Part;

EdgeSort::EdgeSort(double tol3d, const std::list<TopoDS_Edge>& edges)
    : tolerance(tol3d)
    , edges(edges)
{}

EdgeSort::Wire EdgeSort::performOne()
{
    return sortEdges(tolerance);
}

std::list<EdgeSort::Wire> EdgeSort::performAll()
{
    std::list<EdgeSort::Wire> wires;
    while (!edges.empty()) {
        Wire wire = sortEdges(tolerance);
        wires.emplace_back(wire);
    }

    return wires;
}

std::list<TopoDS_Edge> EdgeSort::getRemainingEdges() const
{
    return edges;
}

namespace
{

struct EdgePoints
{
    gp_Pnt v1, v2;
    std::list<TopoDS_Edge>::iterator it;
    TopoDS_Edge edge;
};

}  // namespace

EdgeSort::Wire EdgeSort::sortEdges(double tol3d)
{
    tol3d = tol3d * tol3d;
    std::list<EdgePoints> edge_points;
    TopExp_Explorer xp;
    for (auto it = edges.begin(); it != edges.end(); ++it) {
        EdgePoints ep;
        xp.Init(*it, TopAbs_VERTEX);
        ep.v1 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        xp.Next();
        ep.v2 = BRep_Tool::Pnt(TopoDS::Vertex(xp.Current()));
        ep.it = it;
        ep.edge = *it;
        edge_points.push_back(ep);
    }

    if (edge_points.empty()) {
        return {};
    }

    EdgeSort::Wire sorted;
    gp_Pnt first;
    gp_Pnt last;
    first = edge_points.front().v1;
    last = edge_points.front().v2;

    sorted.push_back(edge_points.front().edge);
    edges.erase(edge_points.front().it);
    edge_points.erase(edge_points.begin());

    while (!edge_points.empty()) {
        // search for adjacent edge
        std::list<EdgePoints>::iterator pEI;
        for (pEI = edge_points.begin(); pEI != edge_points.end(); ++pEI) {
            if (pEI->v1.SquareDistance(last) <= tol3d) {
                last = pEI->v2;
                sorted.push_back(pEI->edge);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            if (pEI->v2.SquareDistance(first) <= tol3d) {
                first = pEI->v1;
                sorted.push_front(pEI->edge);
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            if (pEI->v2.SquareDistance(last) <= tol3d) {
                last = pEI->v1;
                sorted.push_back(TopoDS::Edge(pEI->edge.Reversed()));
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
            if (pEI->v1.SquareDistance(first) <= tol3d) {
                first = pEI->v2;
                sorted.push_front(TopoDS::Edge(pEI->edge.Reversed()));
                edges.erase(pEI->it);
                edge_points.erase(pEI);
                pEI = edge_points.begin();
                break;
            }
        }

        if ((pEI == edge_points.end()) || (last.SquareDistance(first) <= tol3d)) {
            // no adjacent edge found or polyline is closed
            return sorted;
        }
    }

    return sorted;
}
