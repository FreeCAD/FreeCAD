/***************************************************************************
 *   Copyright (c) 2005 Imetric 3D GmbH                                    *
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

#ifndef _PreComp_
#include <algorithm>
#include <map>
#include <queue>
#endif

#include <boost/math/special_functions/fpclassify.hpp>

#include "Degeneration.h"
#include "Grid.h"
#include "Iterator.h"
#include "TopoAlgorithm.h"
#include "Triangulation.h"


using namespace MeshCore;

bool MeshEvalInvalids::Evaluate()
{
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    for (const auto& it : rFaces) {
        if (!it.IsValid()) {
            return false;
        }
    }

    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    for (const auto& it : rPoints) {
        if (!it.IsValid()) {
            return false;
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalInvalids::GetIndices() const
{
    std::vector<FacetIndex> aInds;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    FacetIndex ind = 0;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++) {
        if (!it->IsValid()) {
            aInds.push_back(ind);
        }
        else if (!rPoints[it->_aulPoints[0]].IsValid()) {
            aInds.push_back(ind);
        }
        else if (!rPoints[it->_aulPoints[1]].IsValid()) {
            aInds.push_back(ind);
        }
        else if (!rPoints[it->_aulPoints[2]].IsValid()) {
            aInds.push_back(ind);
        }
    }

    return aInds;
}

bool MeshFixInvalids::Fixup()
{
    _rclMesh.RemoveInvalids();
    return true;
}

// ----------------------------------------------------------------------

namespace MeshCore
{

using VertexIterator = MeshPointArray::_TConstIterator;
/*
 * When building up a mesh then usually the class MeshBuilder is used. This
 * class uses internally a std::set<MeshPoint> which uses the '<' operator of
 * MeshPoint to sort the points. Thus to be consistent (and avoid using the
 * '==' operator of MeshPoint) we use the same operator when comparing the
 * points in the function object.
 */
struct Vertex_EqualTo
{
    bool operator()(const VertexIterator& x, const VertexIterator& y) const
    {
        if ((*x) < (*y)) {
            return false;
        }
        else if ((*y) < (*x)) {
            return false;
        }
        return true;
    }
};

struct Vertex_Less
{
    bool operator()(const VertexIterator& x, const VertexIterator& y) const
    {
        return (*x) < (*y);
    }
};

}  // namespace MeshCore

bool MeshEvalDuplicatePoints::Evaluate()
{
    // get an const iterator to each vertex and sort them in ascending order by
    // their (x,y,z) coordinates
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // if there are two adjacent vertices which have the same coordinates
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());
    if (std::adjacent_find(vertices.begin(), vertices.end(), Vertex_EqualTo()) < vertices.end()) {
        return false;
    }
    return true;
}

std::vector<PointIndex> MeshEvalDuplicatePoints::GetIndices() const
{
    // Note: We must neither use map or set to get duplicated indices because
    // the sort algorithms deliver different results compared to std::sort of
    // a vector.
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // if there are two adjacent vertices which have the same coordinates
    std::vector<PointIndex> aInds;
    Vertex_EqualTo pred;
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());

    std::vector<VertexIterator>::iterator vt = vertices.begin();
    while (vt < vertices.end()) {
        // get first item which adjacent element has the same vertex
        vt = std::adjacent_find(vt, vertices.end(), pred);
        if (vt < vertices.end()) {
            ++vt;
            aInds.push_back(*vt - rPoints.begin());
        }
    }

    return aInds;
}

bool MeshFixDuplicatePoints::Fixup()
{
    // Note: We must neither use map or set to get duplicated indices because
    // the sort algorithms deliver different results compared to std::sort of
    // a vector.
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    std::vector<VertexIterator> vertices;
    vertices.reserve(rPoints.size());
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        vertices.push_back(it);
    }

    // get the indices of adjacent vertices which have the same coordinates
    std::sort(vertices.begin(), vertices.end(), Vertex_Less());

    Vertex_EqualTo pred;
    std::vector<VertexIterator>::iterator next = vertices.begin();
    std::map<PointIndex, PointIndex> mapPointIndex;
    std::vector<PointIndex> pointIndices;
    while (next < vertices.end()) {
        next = std::adjacent_find(next, vertices.end(), pred);
        if (next < vertices.end()) {
            std::vector<VertexIterator>::iterator first = next;
            PointIndex first_index = *first - rPoints.begin();
            ++next;
            while (next < vertices.end() && pred(*first, *next)) {
                PointIndex next_index = *next - rPoints.begin();
                mapPointIndex[next_index] = first_index;
                pointIndices.push_back(next_index);
                ++next;
            }
        }
    }

    // now set all facets to the correct index
    MeshFacetArray& rFacets = _rclMesh._aclFacetArray;
    for (auto& it : rFacets) {
        for (PointIndex& point : it._aulPoints) {
            std::map<PointIndex, PointIndex>::iterator pt = mapPointIndex.find(point);
            if (pt != mapPointIndex.end()) {
                point = pt->second;
            }
        }
    }

    // remove invalid indices
    _rclMesh.DeletePoints(pointIndices);
    _rclMesh.RebuildNeighbours();

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalNaNPoints::Evaluate()
{
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    for (const auto& it : rPoints) {
        if (boost::math::isnan(it.x) || boost::math::isnan(it.y) || boost::math::isnan(it.z)) {
            return false;
        }
    }

    return true;
}

std::vector<PointIndex> MeshEvalNaNPoints::GetIndices() const
{
    std::vector<PointIndex> aInds;
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        if (boost::math::isnan(it->x) || boost::math::isnan(it->y) || boost::math::isnan(it->z)) {
            aInds.push_back(it - rPoints.begin());
        }
    }

    return aInds;
}

bool MeshFixNaNPoints::Fixup()
{
    std::vector<PointIndex> aInds;
    const MeshPointArray& rPoints = _rclMesh.GetPoints();
    for (MeshPointArray::_TConstIterator it = rPoints.begin(); it != rPoints.end(); ++it) {
        if (boost::math::isnan(it->x) || boost::math::isnan(it->y) || boost::math::isnan(it->z)) {
            aInds.push_back(it - rPoints.begin());
        }
    }

    // remove invalid indices
    _rclMesh.DeletePoints(aInds);
    _rclMesh.RebuildNeighbours();

    return true;
}

// ----------------------------------------------------------------------

namespace MeshCore
{

using FaceIterator = MeshFacetArray::_TConstIterator;
/*
 * The facet with the lowset index is regarded as 'less'.
 */
struct MeshFacet_Less
{
    bool operator()(const FaceIterator& x, const FaceIterator& y) const
    {
        PointIndex tmp {};
        PointIndex x0 = x->_aulPoints[0];
        PointIndex x1 = x->_aulPoints[1];
        PointIndex x2 = x->_aulPoints[2];
        PointIndex y0 = y->_aulPoints[0];
        PointIndex y1 = y->_aulPoints[1];
        PointIndex y2 = y->_aulPoints[2];

        if (x0 > x1) {
            tmp = x0;
            x0 = x1;
            x1 = tmp;
        }
        if (x0 > x2) {
            tmp = x0;
            x0 = x2;
            x2 = tmp;
        }
        if (x1 > x2) {
            tmp = x1;
            x1 = x2;
            x2 = tmp;
        }
        if (y0 > y1) {
            tmp = y0;
            y0 = y1;
            y1 = tmp;
        }
        if (y0 > y2) {
            tmp = y0;
            y0 = y2;
            y2 = tmp;
        }
        if (y1 > y2) {
            tmp = y1;
            y1 = y2;
            y2 = tmp;
        }

        if (x0 < y0) {
            return true;
        }
        else if (x0 > y0) {
            return false;
        }
        else if (x1 < y1) {
            return true;
        }
        else if (x1 > y1) {
            return false;
        }
        else if (x2 < y2) {
            return true;
        }
        else {
            return false;
        }
    }
};

}  // namespace MeshCore

/*
 * Two facets are equal if all its three point indices refer to the same
 * location in the point array of the mesh kernel they belong to.
 */
struct MeshFacet_EqualTo
{
    bool operator()(const FaceIterator& x, const FaceIterator& y) const
    {
        for (int i = 0; i < 3; i++) {
            if (x->_aulPoints[0] == y->_aulPoints[i]) {
                if (x->_aulPoints[1] == y->_aulPoints[(i + 1) % 3]
                    && x->_aulPoints[2] == y->_aulPoints[(i + 2) % 3]) {
                    return true;
                }
                else if (x->_aulPoints[1] == y->_aulPoints[(i + 2) % 3]
                         && x->_aulPoints[2] == y->_aulPoints[(i + 1) % 3]) {
                    return true;
                }
            }
        }

        return false;
    }
};

bool MeshEvalDuplicateFacets::Evaluate()
{
    std::set<FaceIterator, MeshFacet_Less> aFaces;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool> pI = aFaces.insert(it);
        if (!pI.second) {
            return false;
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalDuplicateFacets::GetIndices() const
{
#if 1
    const MeshFacetArray& rFacets = _rclMesh.GetFacets();
    std::vector<FaceIterator> faces;
    faces.reserve(rFacets.size());
    for (MeshFacetArray::_TConstIterator it = rFacets.begin(); it != rFacets.end(); ++it) {
        faces.push_back(it);
    }

    // if there are two adjacent faces which references the same vertices
    std::vector<FacetIndex> aInds;
    MeshFacet_EqualTo pred;
    std::sort(faces.begin(), faces.end(), MeshFacet_Less());

    std::vector<FaceIterator>::iterator ft = faces.begin();
    while (ft < faces.end()) {
        // get first item which adjacent element has the same face
        ft = std::adjacent_find(ft, faces.end(), pred);
        if (ft < faces.end()) {
            ++ft;
            aInds.push_back(*ft - rFacets.begin());
        }
    }

    return aInds;
#else
    std::vector<FacetIndex> aInds;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    FacetIndex uIndex = 0;

    // get all facets
    std::set<FaceIterator, MeshFacet_Less> aFaceSet;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool> pI = aFaceSet.insert(it);
        if (!pI.second) {
            aInds.push_back(uIndex);
        }
    }

    return aInds;
#endif
}

bool MeshFixDuplicateFacets::Fixup()
{
    FacetIndex uIndex = 0;
    std::vector<FacetIndex> aRemoveFaces;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();

    // get all facets
    std::set<FaceIterator, MeshFacet_Less> aFaceSet;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool> pI = aFaceSet.insert(it);
        if (!pI.second) {
            aRemoveFaces.push_back(uIndex);
        }
    }

    _rclMesh.DeleteFacets(aRemoveFaces);
    _rclMesh.RebuildNeighbours();  // needs to be done here

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalInternalFacets::Evaluate()
{
    _indices.clear();
    FacetIndex uIndex = 0;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();

    // get all facets
    std::set<FaceIterator, MeshFacet_Less> aFaceSet;
    MeshFacetArray::_TConstIterator first = rFaces.begin();
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, uIndex++) {
        std::pair<std::set<FaceIterator, MeshFacet_Less>::iterator, bool> pI = aFaceSet.insert(it);
        if (!pI.second) {
            // collect both elements
            _indices.push_back(*pI.first - first);
            _indices.push_back(uIndex);
        }
    }

    return _indices.empty();
}

// ----------------------------------------------------------------------

bool MeshEvalDegeneratedFacets::Evaluate()
{
    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it->IsDegenerated(fEpsilon)) {
            return false;
        }
    }

    return true;
}

unsigned long MeshEvalDegeneratedFacets::CountEdgeTooSmall(float fMinEdgeLength) const
{
    MeshFacetIterator clFIter(_rclMesh);
    unsigned long k = 0;

    while (!clFIter.EndReached()) {
        for (int i = 0; i < 3; i++) {
            if (Base::Distance(clFIter->_aclPoints[i], clFIter->_aclPoints[(i + 1) % 3])
                < fMinEdgeLength) {
                k++;
            }
        }
        ++clFIter;
    }

    return k;
}

std::vector<FacetIndex> MeshEvalDegeneratedFacets::GetIndices() const
{
    std::vector<FacetIndex> aInds;
    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it->IsDegenerated(fEpsilon)) {
            aInds.push_back(it.Position());
        }
    }

    return aInds;
}

bool MeshFixDegeneratedFacets::Fixup()
{
    MeshTopoAlgorithm cTopAlg(_rclMesh);

    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it->IsDegenerated(fEpsilon)) {
            FacetIndex uId = it.Position();
            bool removed = cTopAlg.RemoveDegeneratedFacet(uId);
            if (removed) {
                // due to a modification of the array the iterator became invalid
                it.Set(uId - 1);
            }
        }
    }

    return true;
}

bool MeshRemoveNeedles::Fixup()
{
    using FaceEdge = std::pair<unsigned long, int>;  // (face, edge) pair
    using FaceEdgePriority = std::pair<float, FaceEdge>;

    MeshTopoAlgorithm topAlg(_rclMesh);
    MeshRefPointToFacets vf_it(_rclMesh);
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    const MeshPointArray& rclPAry = _rclMesh.GetPoints();
    rclFAry.ResetInvalid();
    rclPAry.ResetInvalid();
    rclPAry.ResetFlag(MeshPoint::VISIT);
    std::size_t facetCount = rclFAry.size();

    std::priority_queue<FaceEdgePriority, std::vector<FaceEdgePriority>, std::greater<>> todo;
    for (std::size_t index = 0; index < facetCount; index++) {
        const MeshFacet& facet = rclFAry[index];
        MeshGeomFacet tria(_rclMesh.GetFacet(facet));
        float perimeter = tria.Perimeter();
        float fMinLen = perimeter * fMinEdgeLength;
        for (int i = 0; i < 3; i++) {
            const Base::Vector3f& p1 = rclPAry[facet._aulPoints[i]];
            const Base::Vector3f& p2 = rclPAry[facet._aulPoints[(i + 1) % 3]];

            float distance = Base::Distance(p1, p2);
            if (distance < fMinLen) {
                unsigned long facetIndex = static_cast<unsigned long>(index);
                todo.push(std::make_pair(distance, std::make_pair(facetIndex, i)));
            }
        }
    }

    bool removedEdge = false;
    while (!todo.empty()) {
        FaceEdge faceedge = todo.top().second;
        todo.pop();

        // check if one of the face pairs was already processed
        if (!rclFAry[faceedge.first].IsValid()) {
            continue;
        }

        // the facet points may have changed, so check the current distance again
        const MeshFacet& facet = rclFAry[faceedge.first];
        MeshGeomFacet tria(_rclMesh.GetFacet(facet));
        float perimeter = tria.Perimeter();
        float fMinLen = perimeter * fMinEdgeLength;
        const Base::Vector3f& p1 = rclPAry[facet._aulPoints[faceedge.second]];
        const Base::Vector3f& p2 = rclPAry[facet._aulPoints[(faceedge.second + 1) % 3]];
        float distance = Base::Distance(p1, p2);
        if (distance >= fMinLen) {
            continue;
        }

        // collect the collapse-edge information
        EdgeCollapse ce;
        ce._fromPoint = rclFAry[faceedge.first]._aulPoints[faceedge.second];
        ce._toPoint = rclFAry[faceedge.first]._aulPoints[(faceedge.second + 1) % 3];

        ce._removeFacets.push_back(faceedge.first);
        FacetIndex neighbour = rclFAry[faceedge.first]._aulNeighbours[faceedge.second];
        if (neighbour != FACET_INDEX_MAX) {
            ce._removeFacets.push_back(neighbour);
        }

        std::set<FacetIndex> vf = vf_it[ce._fromPoint];
        vf.erase(faceedge.first);
        if (neighbour != FACET_INDEX_MAX) {
            vf.erase(neighbour);
        }
        ce._changeFacets.insert(ce._changeFacets.begin(), vf.begin(), vf.end());

        // get adjacent points
        std::set<PointIndex> vv;
        vv = vf_it.NeighbourPoints(ce._fromPoint);
        ce._adjacentFrom.insert(ce._adjacentFrom.begin(), vv.begin(), vv.end());
        vv = vf_it.NeighbourPoints(ce._toPoint);
        ce._adjacentTo.insert(ce._adjacentTo.begin(), vv.begin(), vv.end());

        if (topAlg.IsCollapseEdgeLegal(ce)) {
            topAlg.CollapseEdge(ce);
            for (auto it : ce._removeFacets) {
                vf_it.RemoveFacet(it);
            }
            for (auto it : ce._changeFacets) {
                vf_it.RemoveNeighbour(ce._fromPoint, it);
                vf_it.AddNeighbour(ce._toPoint, it);
            }
            removedEdge = true;
        }
    }

    if (removedEdge) {
        topAlg.Cleanup();
        _rclMesh.RebuildNeighbours();
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshFixCaps::Fixup()
{
    using FaceVertex = std::pair<unsigned long, int>;  // (face, vertex) pair
    using FaceVertexPriority = std::pair<float, FaceVertex>;

    MeshTopoAlgorithm topAlg(_rclMesh);
    const MeshFacetArray& rclFAry = _rclMesh.GetFacets();
    const MeshPointArray& rclPAry = _rclMesh.GetPoints();
    std::size_t facetCount = rclFAry.size();

    float fCosMaxAngle = static_cast<float>(cos(fMaxAngle));

    std::priority_queue<FaceVertexPriority, std::vector<FaceVertexPriority>, std::greater<>> todo;
    for (std::size_t index = 0; index < facetCount; index++) {
        for (int i = 0; i < 3; i++) {
            const MeshFacet& facet = rclFAry[index];
            const Base::Vector3f& p1 = rclPAry[facet._aulPoints[i]];
            const Base::Vector3f& p2 = rclPAry[facet._aulPoints[(i + 1) % 3]];
            const Base::Vector3f& p3 = rclPAry[facet._aulPoints[(i + 2) % 3]];
            Base::Vector3f dir1(p2 - p1);
            dir1.Normalize();
            Base::Vector3f dir2(p3 - p1);
            dir2.Normalize();

            float fCosAngle = dir1.Dot(dir2);
            if (fCosAngle < fCosMaxAngle) {
                unsigned long facetIndex = static_cast<unsigned long>(index);
                todo.push(std::make_pair(fCosAngle, std::make_pair(facetIndex, i)));
            }
        }
    }

    while (!todo.empty()) {
        FaceVertex facevertex = todo.top().second;
        todo.pop();

        // the facet points may have changed, so check the current distance again
        const MeshFacet& facet = rclFAry[facevertex.first];
        const Base::Vector3f& p1 = rclPAry[facet._aulPoints[facevertex.second]];
        const Base::Vector3f& p2 = rclPAry[facet._aulPoints[(facevertex.second + 1) % 3]];
        const Base::Vector3f& p3 = rclPAry[facet._aulPoints[(facevertex.second + 2) % 3]];
        Base::Vector3f dir1(p2 - p1);
        dir1.Normalize();
        Base::Vector3f dir2(p3 - p1);
        dir2.Normalize();

        // check that the criterion is still OK in case
        // an earlier edge-swap has an impact
        float fCosAngle = dir1.Dot(dir2);
        if (fCosAngle >= fCosMaxAngle) {
            continue;
        }

        // the triangle shouldn't be a needle, therefore the projection of the point with
        // the maximum angle must have a clear distance to the other corner points
        // as factor we choose a default value of 25% of the corresponding edge length
        Base::Vector3f p4 = p1.Perpendicular(p2, p3 - p2);
        float distP2P3 = Base::Distance(p2, p3);
        float distP2P4 = Base::Distance(p2, p4);
        float distP3P4 = Base::Distance(p3, p4);
        if (distP2P4 / distP2P3 < fSplitFactor || distP3P4 / distP2P3 < fSplitFactor) {
            continue;
        }

        FacetIndex facetpos = facevertex.first;
        FacetIndex neighbour = rclFAry[facetpos]._aulNeighbours[(facevertex.second + 1) % 3];
        if (neighbour != FACET_INDEX_MAX) {
            topAlg.SwapEdge(facetpos, neighbour);
        }
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalDeformedFacets::Evaluate()
{
    float fCosMinAngle = cos(fMinAngle);
    float fCosMaxAngle = cos(fMaxAngle);

    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it->IsDeformed(fCosMinAngle, fCosMaxAngle)) {
            return false;
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalDeformedFacets::GetIndices() const
{
    float fCosMinAngle = cos(fMinAngle);
    float fCosMaxAngle = cos(fMaxAngle);

    std::vector<FacetIndex> aInds;
    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it->IsDeformed(fCosMinAngle, fCosMaxAngle)) {
            aInds.push_back(it.Position());
        }
    }

    return aInds;
}

bool MeshFixDeformedFacets::Fixup()
{
    float fCosMinAngle = cos(fMinAngle);
    float fCosMaxAngle = cos(fMaxAngle);

    Base::Vector3f u, v;
    MeshTopoAlgorithm cTopAlg(_rclMesh);

    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        // possibly deformed but not degenerated
        if (!it->IsDegenerated(fEpsilon)) {
            // store the angles to avoid to compute twice
            float fCosAngles[3] = {0, 0, 0};
            bool done = false;

            for (int i = 0; i < 3; i++) {
                u = it->_aclPoints[(i + 1) % 3] - it->_aclPoints[i];
                v = it->_aclPoints[(i + 2) % 3] - it->_aclPoints[i];
                u.Normalize();
                v.Normalize();

                float fCosAngle = u * v;
                fCosAngles[i] = fCosAngle;
            }

            // first check for angle > 120 deg: in this case we swap with the opposite edge
            for (int i = 0; i < 3; i++) {
                float fCosAngle = fCosAngles[i];
                if (fCosAngle < fCosMaxAngle) {
                    const MeshFacet& face = it.GetReference();
                    FacetIndex uNeighbour = face._aulNeighbours[(i + 1) % 3];
                    if (uNeighbour != FACET_INDEX_MAX
                        && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxSwapAngle)) {
                        cTopAlg.SwapEdge(it.Position(), uNeighbour);
                        done = true;
                    }
                    break;
                }
            }

            // we have swapped already
            if (done) {
                continue;
            }

            // now check for angle < 30 deg: in this case we swap with one of the edges the corner
            // is part of
            for (int j = 0; j < 3; j++) {
                float fCosAngle = fCosAngles[j];
                if (fCosAngle > fCosMinAngle) {
                    const MeshFacet& face = it.GetReference();

                    FacetIndex uNeighbour = face._aulNeighbours[j];
                    if (uNeighbour != FACET_INDEX_MAX
                        && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxSwapAngle)) {
                        cTopAlg.SwapEdge(it.Position(), uNeighbour);
                        break;
                    }

                    uNeighbour = face._aulNeighbours[(j + 2) % 3];
                    if (uNeighbour != FACET_INDEX_MAX
                        && cTopAlg.ShouldSwapEdge(it.Position(), uNeighbour, fMaxSwapAngle)) {
                        cTopAlg.SwapEdge(it.Position(), uNeighbour);
                        break;
                    }
                }
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshFixMergeFacets::Fixup()
{
    MeshCore::MeshRefPointToPoints vv_it(_rclMesh);
    MeshCore::MeshRefPointToFacets vf_it(_rclMesh);
    unsigned long countPoints = _rclMesh.CountPoints();

    std::vector<MeshFacet> newFacets;
    newFacets.reserve(countPoints / 20);  // 5% should be sufficient

    MeshTopoAlgorithm topAlg(_rclMesh);
    for (unsigned long i = 0; i < countPoints; i++) {
        if (vv_it[i].size() == 3 && vf_it[i].size() == 3) {
            VertexCollapse vc;
            vc._point = i;
            const std::set<PointIndex>& adjPts = vv_it[i];
            vc._circumPoints.insert(vc._circumPoints.begin(), adjPts.begin(), adjPts.end());
            const std::set<FacetIndex>& adjFts = vf_it[i];
            vc._circumFacets.insert(vc._circumFacets.begin(), adjFts.begin(), adjFts.end());
            topAlg.CollapseVertex(vc);
        }
    }

    topAlg.Cleanup();
    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalDentsOnSurface::Evaluate()
{
    this->indices.clear();
    MeshRefPointToFacets clPt2Facets(_rclMesh);
    const MeshPointArray& rPntAry = _rclMesh.GetPoints();
    MeshFacetArray::_TConstIterator f_beg = _rclMesh.GetFacets().begin();

    MeshGeomFacet rTriangle;
    Base::Vector3f tmp;
    unsigned long ctPoints = _rclMesh.CountPoints();
    for (unsigned long index = 0; index < ctPoints; index++) {
        std::vector<PointIndex> point;
        point.push_back(index);

        // get the local neighbourhood of the point
        std::set<PointIndex> nb = clPt2Facets.NeighbourPoints(point, 1);
        const std::set<FacetIndex>& faces = clPt2Facets[index];

        for (PointIndex pt : nb) {
            const MeshPoint& mp = rPntAry[pt];
            for (FacetIndex ft : faces) {
                // the point must not be part of the facet we test
                if (f_beg[ft]._aulPoints[0] == pt) {
                    continue;
                }
                if (f_beg[ft]._aulPoints[1] == pt) {
                    continue;
                }
                if (f_beg[ft]._aulPoints[2] == pt) {
                    continue;
                }
                // is the point projectable onto the facet?
                rTriangle = _rclMesh.GetFacet(f_beg[ft]);
                if (rTriangle.IntersectWithLine(mp, rTriangle.GetNormal(), tmp)) {
                    const std::set<FacetIndex>& f = clPt2Facets[pt];
                    this->indices.insert(this->indices.end(), f.begin(), f.end());
                    break;
                }
            }
        }
    }

    // remove duplicates
    std::sort(this->indices.begin(), this->indices.end());
    this->indices.erase(std::unique(this->indices.begin(), this->indices.end()),
                        this->indices.end());

    return this->indices.empty();
}

std::vector<FacetIndex> MeshEvalDentsOnSurface::GetIndices() const
{
    return this->indices;
}

/*
Forbidden is:
 + two facets share a common point but not a common edge

 Repair:
 + store the point indices which can be projected on a face
 + store the face indices on which a point can be projected
 + remove faces with an edge length smaller than a certain threshold (e.g. 0.01) from the stored
triangles or that reference one of the stored points
 + for this edge merge the two points
 + if a point of a face can be projected onto another face and they have a common point then split
the second face if the distance is under a certain threshold
 */
bool MeshFixDentsOnSurface::Fixup()
{
    MeshEvalDentsOnSurface eval(_rclMesh);
    if (!eval.Evaluate()) {
        std::vector<FacetIndex> inds = eval.GetIndices();
        _rclMesh.DeleteFacets(inds);
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldsOnSurface::Evaluate()
{
    this->indices.clear();
    const MeshFacetArray& rFAry = _rclMesh.GetFacets();
    unsigned long ct = 0;
    for (MeshFacetArray::const_iterator it = rFAry.begin(); it != rFAry.end(); ++it, ct++) {
        for (int i = 0; i < 3; i++) {
            FacetIndex n1 = it->_aulNeighbours[i];
            FacetIndex n2 = it->_aulNeighbours[(i + 1) % 3];
            Base::Vector3f v1 = _rclMesh.GetFacet(*it).GetNormal();
            if (n1 != FACET_INDEX_MAX && n2 != FACET_INDEX_MAX) {
                Base::Vector3f v2 = _rclMesh.GetFacet(n1).GetNormal();
                Base::Vector3f v3 = _rclMesh.GetFacet(n2).GetNormal();
                if (v2 * v3 > 0.0f) {
                    if (v1 * v2 < -0.1f && v1 * v3 < -0.1f) {
                        indices.push_back(n1);
                        indices.push_back(n2);
                        indices.push_back(ct);
                    }
                }
            }
        }
    }

    // remove duplicates
    std::sort(this->indices.begin(), this->indices.end());
    this->indices.erase(std::unique(this->indices.begin(), this->indices.end()),
                        this->indices.end());
    return this->indices.empty();
}

std::vector<FacetIndex> MeshEvalFoldsOnSurface::GetIndices() const
{
    return this->indices;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldsOnBoundary::Evaluate()
{
    // remove all boundary facets with two open edges and where
    // the angle to the neighbour is more than 60 degree
    this->indices.clear();
    const MeshFacetArray& rFacAry = _rclMesh.GetFacets();
    for (MeshFacetArray::_TConstIterator it = rFacAry.begin(); it != rFacAry.end(); ++it) {
        if (it->CountOpenEdges() == 2) {
            for (FacetIndex nbIndex : it->_aulNeighbours) {
                if (nbIndex != FACET_INDEX_MAX) {
                    MeshGeomFacet f1 = _rclMesh.GetFacet(*it);
                    MeshGeomFacet f2 = _rclMesh.GetFacet(nbIndex);
                    float cos_angle = f1.GetNormal() * f2.GetNormal();
                    if (cos_angle <= 0.5f) {  // ~ 60 degree
                        indices.push_back(it - rFacAry.begin());
                    }
                }
            }
        }
    }

    return this->indices.empty();
}

std::vector<FacetIndex> MeshEvalFoldsOnBoundary::GetIndices() const
{
    return this->indices;
}

bool MeshFixFoldsOnBoundary::Fixup()
{
    MeshEvalFoldsOnBoundary eval(_rclMesh);
    if (!eval.Evaluate()) {
        std::vector<FacetIndex> inds = eval.GetIndices();
        _rclMesh.DeleteFacets(inds);
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalFoldOversOnSurface::Evaluate()
{
    this->indices.clear();
    const MeshCore::MeshFacetArray& facets = _rclMesh.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator f_it, f_beg = facets.begin(), f_end = facets.end();

    Base::Vector3f n1, n2;
    for (f_it = facets.begin(); f_it != f_end; ++f_it) {
        for (int i = 0; i < 3; i++) {
            FacetIndex index1 = f_it->_aulNeighbours[i];
            FacetIndex index2 = f_it->_aulNeighbours[(i + 1) % 3];
            if (index1 != FACET_INDEX_MAX && index2 != FACET_INDEX_MAX) {
                // if the topology is correct but the normals flip from
                // two neighbours we have a fold
                if (f_it->HasSameOrientation(f_beg[index1])
                    && f_it->HasSameOrientation(f_beg[index2])) {
                    n1 = _rclMesh.GetFacet(index1).GetNormal();
                    n2 = _rclMesh.GetFacet(index2).GetNormal();
                    if (n1 * n2 < -0.5f) {  // angle > 120 deg
                        this->indices.push_back(f_it - f_beg);
                        break;
                    }
                }
            }
        }
    }

    return this->indices.empty();
}

// ----------------------------------------------------------------

bool MeshEvalBorderFacet::Evaluate()
{
    const MeshCore::MeshFacetArray& facets = _rclMesh.GetFacets();
    MeshCore::MeshFacetArray::_TConstIterator f_it, f_beg = facets.begin(), f_end = facets.end();
    MeshCore::MeshRefPointToPoints vv_it(_rclMesh);
    MeshCore::MeshRefPointToFacets vf_it(_rclMesh);

    for (f_it = facets.begin(); f_it != f_end; ++f_it) {
        bool ok = true;
        for (PointIndex index : f_it->_aulPoints) {
            if (vv_it[index].size() == vf_it[index].size()) {
                ok = false;
                break;
            }
        }

        if (ok) {
            _facets.push_back(f_it - f_beg);
        }
    }

    return _facets.empty();
}

// ----------------------------------------------------------------------

bool MeshEvalRangeFacet::Evaluate()
{
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    FacetIndex ulCtFacets = rFaces.size();

    for (const auto& it : rFaces) {
        for (FacetIndex nbFacet : it._aulNeighbours) {
            if ((nbFacet >= ulCtFacets) && (nbFacet < FACET_INDEX_MAX)) {
                return false;
            }
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalRangeFacet::GetIndices() const
{
    std::vector<FacetIndex> aInds;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    FacetIndex ulCtFacets = rFaces.size();

    FacetIndex ind = 0;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++) {
        for (FacetIndex nbIndex : it->_aulNeighbours) {
            if ((nbIndex >= ulCtFacets) && (nbIndex < FACET_INDEX_MAX)) {
                aInds.push_back(ind);
                break;
            }
        }
    }

    return aInds;
}

bool MeshFixRangeFacet::Fixup()
{
    _rclMesh.RebuildNeighbours();
    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalRangePoint::Evaluate()
{
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    PointIndex ulCtPoints = _rclMesh.CountPoints();

    for (const auto& it : rFaces) {
        if (std::find_if(it._aulPoints,
                         it._aulPoints + 3,
                         [ulCtPoints](PointIndex i) {
                             return i >= ulCtPoints;
                         })
            < it._aulPoints + 3) {
            return false;
        }
    }

    return true;
}

std::vector<PointIndex> MeshEvalRangePoint::GetIndices() const
{
    std::vector<PointIndex> aInds;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    PointIndex ulCtPoints = _rclMesh.CountPoints();

    PointIndex ind = 0;
    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++) {
        if (std::find_if(it->_aulPoints,
                         it->_aulPoints + 3,
                         [ulCtPoints](PointIndex i) {
                             return i >= ulCtPoints;
                         })
            < it->_aulPoints + 3) {
            aInds.push_back(ind);
        }
    }

    return aInds;
}

bool MeshFixRangePoint::Fixup()
{
    MeshEvalRangePoint eval(_rclMesh);
    if (_rclMesh.CountPoints() == 0) {
        // if no points are there but facets then the whole mesh can be cleared
        _rclMesh.Clear();
    }
    else {
        // facets with point indices out of range cannot be directly deleted because
        // 'DeleteFacets' will segfault. But setting all point indices to 0 works.
        std::vector<PointIndex> invalid = eval.GetIndices();
        if (!invalid.empty()) {
            for (PointIndex it : invalid) {
                _rclMesh.SetFacetPoints(it, 0, 0, 0);
            }

            _rclMesh.DeleteFacets(invalid);
        }
    }
    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalCorruptedFacets::Evaluate()
{
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();

    for (const auto& it : rFaces) {
        // duplicated point indices
        if (it.IsDegenerated()) {
            return false;
        }
    }

    return true;
}

std::vector<FacetIndex> MeshEvalCorruptedFacets::GetIndices() const
{
    std::vector<FacetIndex> aInds;
    const MeshFacetArray& rFaces = _rclMesh.GetFacets();
    FacetIndex ind = 0;

    for (MeshFacetArray::_TConstIterator it = rFaces.begin(); it != rFaces.end(); ++it, ind++) {
        if (it->IsDegenerated()) {
            aInds.push_back(ind);
        }
    }

    return aInds;
}

bool MeshFixCorruptedFacets::Fixup()
{
    MeshTopoAlgorithm cTopAlg(_rclMesh);

    MeshFacetIterator it(_rclMesh);
    for (it.Init(); it.More(); it.Next()) {
        if (it.GetReference().IsDegenerated()) {
            unsigned long uId = it.Position();
            bool removed = cTopAlg.RemoveCorruptedFacet(uId);
            if (removed) {
                // due to a modification of the array the iterator became invalid
                it.Set(uId - 1);
            }
        }
    }

    return true;
}

// ----------------------------------------------------------------------

bool MeshEvalPointOnEdge::Evaluate()
{
    MeshFacetGrid facetGrid(_rclMesh);
    const MeshPointArray& points = _rclMesh.GetPoints();
    const MeshFacetArray& facets = _rclMesh.GetFacets();

    auto IsPointOnEdge = [&points](PointIndex idx, const MeshFacet& facet) {
        // point must not be a corner of the facet
        if (!facet.HasPoint(idx)) {
            for (int i = 0; i < 3; i++) {
                MeshGeomEdge edge;
                edge._aclPoints[0] = points[facet._aulPoints[i]];
                edge._aclPoints[1] = points[facet._aulPoints[(i + 1) % 3]];

                if (edge.GetBoundBox().IsInBox(points[idx])) {
                    if (edge.IsPointOf(points[idx], 0.001f)) {
                        return true;
                    }
                }
            }
        }
        return false;
    };

    PointIndex maxPoints = _rclMesh.CountPoints();
    for (PointIndex i = 0; i < maxPoints; i++) {
        std::vector<FacetIndex> elements;
        facetGrid.GetElements(points[i], elements);

        for (const auto& it : elements) {
            const MeshFacet& face = facets[it];
            if (IsPointOnEdge(i, face)) {
                pointsIndices.push_back(i);
                if (face.HasOpenEdge()) {
                    facetsIndices.push_back(it);
                }
            }
        }
    }
    return pointsIndices.empty();
}

std::vector<PointIndex> MeshEvalPointOnEdge::GetPointIndices() const
{
    return pointsIndices;
}

std::vector<FacetIndex> MeshEvalPointOnEdge::GetFacetIndices() const
{
    return facetsIndices;
}

bool MeshFixPointOnEdge::Fixup()
{
    MeshEvalPointOnEdge eval(_rclMesh);
    eval.Evaluate();
    std::vector<PointIndex> pointsIndices = eval.GetPointIndices();
    std::vector<FacetIndex> facetsIndices = eval.GetFacetIndices();

    if (!pointsIndices.empty()) {
        if (fillBoundary) {
            MarkBoundaries(facetsIndices);
        }

        _rclMesh.DeletePoints(pointsIndices);

        if (fillBoundary) {
            std::list<std::vector<PointIndex>> borderList;
            FindBoundaries(borderList);
            if (!borderList.empty()) {
                FillBoundaries(borderList);
            }
        }
    }

    return true;
}

void MeshFixPointOnEdge::MarkBoundaries(const std::vector<FacetIndex>& facetsIndices)
{
    MeshAlgorithm meshalg(_rclMesh);
    meshalg.ResetFacetFlag(MeshFacet::TMP0);
    meshalg.SetFacetsFlag(facetsIndices, MeshFacet::TMP0);
}

void MeshFixPointOnEdge::FindBoundaries(std::list<std::vector<PointIndex>>& borderList)
{
    std::vector<FacetIndex> tmp;
    MeshAlgorithm meshalg(_rclMesh);
    meshalg.GetFacetsFlag(tmp, MeshFacet::TMP0);

    if (!tmp.empty()) {
        meshalg.GetFacetsBorders(tmp, borderList);
    }
}

void MeshFixPointOnEdge::FillBoundaries(const std::list<std::vector<PointIndex>>& borderList)
{
    FlatTriangulator tria;
    tria.SetVerifier(new MeshCore::TriangulationVerifierV2);
    MeshTopoAlgorithm topalg(_rclMesh);
    std::list<std::vector<PointIndex>> failed;
    topalg.FillupHoles(1, tria, borderList, failed);
}
