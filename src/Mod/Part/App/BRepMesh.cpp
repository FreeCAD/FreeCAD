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
#include <Precision.hxx>
#endif

#include "BRepMesh.h"
#include <Base/Tools.h>

using namespace Part;

namespace {
struct MeshVertex
{
    Base::Vector3d p;
    std::size_t i = 0;

    explicit MeshVertex(const Base::Vector3d& p)
        : p(p)
    {
    }

    Base::Vector3d toPoint() const
    {
        return p;
    }

    bool operator < (const MeshVertex &v) const
    {
        if (p.x != v.p.x) {
            return p.x < v.p.x;
        }
        if (p.y != v.p.y) {
            return p.y < v.p.y;
        }
        if (p.z != v.p.z) {
            return p.z < v.p.z;
        }

        // points are equal
        return false;
    }
};

class MergeVertex
{
public:
    using Facet = BRepMesh::Facet;

    MergeVertex(std::vector<Base::Vector3d> points,
                std::vector<Facet> faces,
                double tolerance)
        : points{std::move(points)}
        , faces{std::move(faces)}
        , tolerance{tolerance}
    {
        setDefaultMap();
        check();
    }

    bool hasDuplicatedPoints() const
    {
        return duplicatedPoints > 0;
    }

    void mergeDuplicatedPoints()
    {
        if (!hasDuplicatedPoints()) {
            return;
        }

        redirectPointIndex();
        auto degreeMap = getPointDegrees();
        decrementPointIndex(degreeMap);
        removeUnusedPoints(degreeMap);
        reset();
    }

    std::vector<Base::Vector3d> getPoints() const
    {
        return points;
    }

    std::vector<Facet> getFacets() const
    {
        return faces;
    }

private:
    void setDefaultMap()
    {
        // by default map point index to itself
        mapPointIndex.resize(points.size());
        std::generate(mapPointIndex.begin(),
                      mapPointIndex.end(),
                      Base::iotaGen<std::size_t>(0));
    }

    void reset()
    {
        mapPointIndex.clear();
        duplicatedPoints = 0;
    }

    void check()
    {
        using VertexIterator = std::vector<Base::Vector3d>::const_iterator;

        double tol3d = tolerance;
        auto vertexLess = [tol3d](const VertexIterator& v1,
                                  const VertexIterator& v2)
        {
            if (fabs(v1->x - v2->x) >= tol3d) {
                return v1->x < v2->x;
            }
            if (fabs(v1->y - v2->y) >= tol3d) {
                return v1->y < v2->y;
            }
            if (fabs(v1->z - v2->z) >= tol3d) {
                return v1->z < v2->z;
            }
            return false;  // points are considered to be equal
        };
        auto vertexEqual = [&](const VertexIterator& v1,
                               const VertexIterator& v2)
        {
            if (vertexLess(v1, v2)) {
                return false;
            }
            if (vertexLess(v2, v1)) {
                return false;
            }
            return true;
        };

        std::vector<VertexIterator> vertices;
        vertices.reserve(points.size());
        for (auto it = points.cbegin(); it != points.cend(); ++it) {
            vertices.push_back(it);
        }

        std::sort(vertices.begin(), vertices.end(), vertexLess);

        auto next = vertices.begin();
        while (next != vertices.end()) {
            next = std::adjacent_find(next, vertices.end(), vertexEqual);
            if (next != vertices.end()) {
                auto first = next;
                std::size_t first_index = *first - points.begin();
                ++next;
                while (next != vertices.end() && vertexEqual(*first, *next)) {
                    std::size_t next_index = *next - points.begin();
                    mapPointIndex[next_index] = first_index;
                    ++duplicatedPoints;
                    ++next;
                }
            }
        }
    }

    void redirectPointIndex()
    {
        for (auto& face : faces) {
            face.I1 = int(mapPointIndex[face.I1]);
            face.I2 = int(mapPointIndex[face.I2]);
            face.I3 = int(mapPointIndex[face.I3]);
        }
    }

    std::vector<std::size_t> getPointDegrees() const
    {
        std::vector<std::size_t> degreeMap;
        degreeMap.resize(points.size());
        for (const auto& face : faces) {
            degreeMap[face.I1]++;
            degreeMap[face.I2]++;
            degreeMap[face.I3]++;
        }

        return degreeMap;
    }

    void decrementPointIndex(const std::vector<std::size_t>& degreeMap)
    {
        std::vector<std::size_t> decrements;
        decrements.resize(points.size());

        std::size_t decr = 0;
        for (std::size_t pos = 0; pos < points.size(); pos++) {
            decrements[pos] = decr;
            if (degreeMap[pos] == 0) {
                decr++;
            }
        }

        for (auto& face : faces) {
            face.I1 -= int(decrements[face.I1]);
            face.I2 -= int(decrements[face.I2]);
            face.I3 -= int(decrements[face.I3]);
        }
    }

    void removeUnusedPoints(const std::vector<std::size_t>& degreeMap)
    {
        // remove unreferenced points
        std::vector<Base::Vector3d> new_points;
        new_points.reserve(points.size() - duplicatedPoints);
        for (std::size_t pos = 0; pos < points.size(); ++pos) {
            if (degreeMap[pos] > 0) {
                new_points.push_back(points[pos]);
            }
        }

        points.swap(new_points);
    }

private:
    std::vector<Base::Vector3d> points;
    std::vector<Facet> faces;
    double tolerance = 0.0;
    std::size_t duplicatedPoints = 0;
    std::vector<std::size_t> mapPointIndex;
};

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

    MergeVertex merge(points, faces, Precision::Confusion());
    if (merge.hasDuplicatedPoints()) {
        merge.mergeDuplicatedPoints();
        points = merge.getPoints();
        faces = merge.getFacets();
    }
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
