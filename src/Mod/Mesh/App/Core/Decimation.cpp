/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "PreCompiled.h"

#include "Decimation.h"
#include "MeshKernel.h"
#include "Simplify.h"


using namespace MeshCore;

MeshSimplify::MeshSimplify(MeshKernel& mesh)
    : myKernel(mesh)
{}

void MeshSimplify::simplify(float tolerance, float reduction)
{
    Simplify alg;

    const MeshPointArray& points = myKernel.GetPoints();
    for (std::size_t i = 0; i < points.size(); i++) {
        Simplify::Vertex v;
        v.tstart = 0;
        v.tcount = 0;
        v.border = 0;
        v.p = points[i];
        alg.vertices.push_back(v);
    }

    const MeshFacetArray& facets = myKernel.GetFacets();
    for (std::size_t i = 0; i < facets.size(); i++) {
        Simplify::Triangle t;
        t.deleted = 0;
        t.dirty = 0;
        for (double& j : t.err) {
            j = 0.0;
        }
        for (int j = 0; j < 3; j++) {
            t.v[j] = facets[i]._aulPoints[j];
        }
        alg.triangles.push_back(t);
    }

    int target_count = static_cast<int>(static_cast<float>(facets.size()) * (1.0f - reduction));

    // Simplification starts
    alg.simplify_mesh(target_count, tolerance);

    // Simplification done
    MeshPointArray new_points;
    new_points.reserve(alg.vertices.size());
    for (const auto& vertex : alg.vertices) {
        new_points.push_back(vertex.p);
    }

    std::size_t numFacets = 0;
    for (const auto& triangle : alg.triangles) {
        if (!triangle.deleted) {
            numFacets++;
        }
    }
    MeshFacetArray new_facets;
    new_facets.reserve(numFacets);
    for (const auto& triangle : alg.triangles) {
        if (!triangle.deleted) {
            MeshFacet face;
            face._aulPoints[0] = triangle.v[0];
            face._aulPoints[1] = triangle.v[1];
            face._aulPoints[2] = triangle.v[2];
            new_facets.push_back(face);
        }
    }

    myKernel.Adopt(new_points, new_facets, true);
}

void MeshSimplify::simplify(int targetSize)
{
    Simplify alg;

    const MeshPointArray& points = myKernel.GetPoints();
    for (std::size_t i = 0; i < points.size(); i++) {
        Simplify::Vertex v;
        v.tstart = 0;
        v.tcount = 0;
        v.border = 0;
        v.p = points[i];
        alg.vertices.push_back(v);
    }

    const MeshFacetArray& facets = myKernel.GetFacets();
    for (std::size_t i = 0; i < facets.size(); i++) {
        Simplify::Triangle t;
        t.deleted = 0;
        t.dirty = 0;
        for (double& j : t.err) {
            j = 0.0;
        }
        for (int j = 0; j < 3; j++) {
            t.v[j] = facets[i]._aulPoints[j];
        }
        alg.triangles.push_back(t);
    }

    // Simplification starts
    alg.simplify_mesh(targetSize, FLT_MAX);

    // Simplification done
    MeshPointArray new_points;
    new_points.reserve(alg.vertices.size());
    for (const auto& vertex : alg.vertices) {
        new_points.push_back(vertex.p);
    }

    std::size_t numFacets = 0;
    for (const auto& triangle : alg.triangles) {
        if (!triangle.deleted) {
            numFacets++;
        }
    }
    MeshFacetArray new_facets;
    new_facets.reserve(numFacets);
    for (const auto& triangle : alg.triangles) {
        if (!triangle.deleted) {
            MeshFacet face;
            face._aulPoints[0] = triangle.v[0];
            face._aulPoints[1] = triangle.v[1];
            face._aulPoints[2] = triangle.v[2];
            new_facets.push_back(face);
        }
    }

    myKernel.Adopt(new_points, new_facets, true);
}
