// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
#include <algorithm>
#endif

#include "BRepMesh.h"
#include <Base/Tools.h>

using namespace Part;

namespace Part {
struct MeshVertex
{
    Base::Vector3d p;
    std::size_t i;

    explicit MeshVertex(const Base::Vector3d& p)
        : p(p), i(0)
    {
    }

    Base::Vector3d toPoint() const
    { return p; }

    bool operator < (const MeshVertex &v) const
    {
        if (fabs ( p.x - v.p.x) >= epsilon)
            return p.x < v.p.x;
        if (fabs ( p.y - v.p.y) >= epsilon)
            return p.y < v.p.y;
        if (fabs ( p.z - v.p.z) >= epsilon)
            return p.z < v.p.z;
        return false; // points are considered to be equal
    }

private:
    static const double epsilon;
};

const double MeshVertex::epsilon = 10 * std::numeric_limits<double>::epsilon();

}

void BRepMesh::getFacesFromDomains(const std::vector<Domain>& domains,
                                   std::vector<Base::Vector3d>& points,
                                   std::vector<Facet>& faces)
{
    std::size_t numFaces = 0;
    for (const auto& it : domains) {
        numFaces += it.facets.size();
    }
    faces.reserve(numFaces);

    std::set<MeshVertex> vertices;
    auto addVertex = [&vertices](const Base::Vector3d& pnt, uint32_t& pointIndex) {
        MeshVertex vertex(pnt);
        vertex.i = vertices.size();
        auto it = vertices.insert(vertex);
        pointIndex = it.first->i;
    };

    for (const auto & domain : domains) {
        std::size_t numDomainFaces = 0;
        for (const Facet& df : domain.facets) {
            Facet face;

            // 1st vertex
            addVertex(domain.points[df.I1], face.I1);

            // 2nd vertex
            addVertex(domain.points[df.I2], face.I2);

            // 3rd vertex
            addVertex(domain.points[df.I3], face.I3);

            // make sure that we don't insert invalid facets
            if (face.I1 != face.I2 &&
                face.I2 != face.I3 &&
                face.I3 != face.I1) {
                faces.push_back(face);
                numDomainFaces++;
            }
        }

        domainSizes.push_back(numDomainFaces);
    }

    std::vector<Base::Vector3d> meshPoints;
    meshPoints.resize(vertices.size());
    for (const auto & vertex : vertices) {
        meshPoints[vertex.i] = vertex.toPoint();
    }
    points.swap(meshPoints);
}

std::vector<BRepMesh::Segment> BRepMesh::createSegments() const
{
    std::size_t numMeshFaces = 0;
    std::vector<Segment> segm;
    for (size_t numDomainFaces : domainSizes) {
        Segment segment(numDomainFaces);
        std::generate(segment.begin(),
                      segment.end(),
                      Base::iotaGen<std::size_t>(numMeshFaces));
        numMeshFaces += numDomainFaces;
        segm.push_back(segment);
    }

    return segm;
}
