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
#include <boost/core/ignore_unused.hpp>
#include <queue>
#include <utility>
#endif

#include <Base/Console.h>
#include <Mod/Mesh/App/WildMagic4/Wm4MeshCurvature.h>

#include "Evaluation.h"
#include "Iterator.h"
#include "MeshKernel.h"
#include "TopoAlgorithm.h"
#include "Triangulation.h"


using namespace MeshCore;

MeshTopoAlgorithm::MeshTopoAlgorithm(MeshKernel& rclM)
    : _rclMesh(rclM)
{}

MeshTopoAlgorithm::~MeshTopoAlgorithm()
{
    if (_needsCleanup) {
        Cleanup();
    }
    EndCache();
}

bool MeshTopoAlgorithm::InsertVertex(FacetIndex ulFacetPos, const Base::Vector3f& rclPoint)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet clNewFacet1, clNewFacet2;

    // insert new point
    PointIndex ulPtCnt = _rclMesh._aclPointArray.size();
    PointIndex ulPtInd = this->GetOrAddIndex(rclPoint);
    FacetIndex ulSize = _rclMesh._aclFacetArray.size();

    if (ulPtInd < ulPtCnt) {
        return false;  // the given point is already part of the mesh => creating new facets would
                       // be an illegal operation
    }

    // adjust the facets
    //
    // first new facet
    clNewFacet1._aulPoints[0] = rclF._aulPoints[1];
    clNewFacet1._aulPoints[1] = rclF._aulPoints[2];
    clNewFacet1._aulPoints[2] = ulPtInd;
    clNewFacet1._aulNeighbours[0] = rclF._aulNeighbours[1];
    clNewFacet1._aulNeighbours[1] = ulSize + 1;
    clNewFacet1._aulNeighbours[2] = ulFacetPos;
    // second new facet
    clNewFacet2._aulPoints[0] = rclF._aulPoints[2];
    clNewFacet2._aulPoints[1] = rclF._aulPoints[0];
    clNewFacet2._aulPoints[2] = ulPtInd;
    clNewFacet2._aulNeighbours[0] = rclF._aulNeighbours[2];
    clNewFacet2._aulNeighbours[1] = ulFacetPos;
    clNewFacet2._aulNeighbours[2] = ulSize;
    // adjust the neighbour facet
    if (rclF._aulNeighbours[1] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[1]].ReplaceNeighbour(ulFacetPos, ulSize);
    }
    if (rclF._aulNeighbours[2] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[2]].ReplaceNeighbour(ulFacetPos, ulSize + 1);
    }
    // original facet
    rclF._aulPoints[2] = ulPtInd;
    rclF._aulNeighbours[1] = ulSize;
    rclF._aulNeighbours[2] = ulSize + 1;

    // insert new facets
    _rclMesh._aclFacetArray.push_back(clNewFacet1);
    _rclMesh._aclFacetArray.push_back(clNewFacet2);

    return true;
}

bool MeshTopoAlgorithm::SnapVertex(FacetIndex ulFacetPos, const Base::Vector3f& rP)
{
    MeshFacet& rFace = _rclMesh._aclFacetArray[ulFacetPos];
    if (!rFace.HasOpenEdge()) {
        return false;
    }
    Base::Vector3f cNo1 = _rclMesh.GetNormal(rFace);
    for (unsigned short i = 0; i < 3; i++) {
        if (rFace._aulNeighbours[i] == FACET_INDEX_MAX) {
            const Base::Vector3f& rPt1 = _rclMesh._aclPointArray[rFace._aulPoints[i]];
            const Base::Vector3f& rPt2 = _rclMesh._aclPointArray[rFace._aulPoints[(i + 1) % 3]];
            Base::Vector3f cNo2 = (rPt2 - rPt1) % cNo1;
            Base::Vector3f cNo3 = (rP - rPt1) % (rPt2 - rPt1);
            float fD2 = Base::DistanceP2(rPt1, rPt2);
            float fTV = (rP - rPt1) * (rPt2 - rPt1);

            // Point is on the edge
            if (cNo3.Length() < FLOAT_EPS) {
                return SplitOpenEdge(ulFacetPos, i, rP);
            }
            else if ((rP - rPt1) * cNo2 > 0.0f && fD2 >= fTV && fTV >= 0.0f) {
                MeshFacet cTria;
                cTria._aulPoints[0] = this->GetOrAddIndex(rP);
                cTria._aulPoints[1] = rFace._aulPoints[(i + 1) % 3];
                cTria._aulPoints[2] = rFace._aulPoints[i];
                cTria._aulNeighbours[1] = ulFacetPos;
                rFace._aulNeighbours[i] = _rclMesh.CountFacets();
                _rclMesh._aclFacetArray.push_back(cTria);
                return true;
            }
        }
    }

    return false;
}

void MeshTopoAlgorithm::OptimizeTopology(float fMaxAngle)
{
    // For each internal edge get the adjacent facets. When doing an edge swap we must update
    // this structure.
    std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>> aEdge2Face;
    for (MeshFacetArray::_TIterator pI = _rclMesh._aclFacetArray.begin();
         pI != _rclMesh._aclFacetArray.end();
         ++pI) {
        for (int i = 0; i < 3; i++) {
            // ignore open edges
            if (pI->_aulNeighbours[i] != FACET_INDEX_MAX) {
                PointIndex ulPt0 =
                    std::min<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
                PointIndex ulPt1 =
                    std::max<PointIndex>(pI->_aulPoints[i], pI->_aulPoints[(i + 1) % 3]);
                aEdge2Face[std::pair<PointIndex, PointIndex>(ulPt0, ulPt1)].push_back(
                    pI - _rclMesh._aclFacetArray.begin());
            }
        }
    }

    // fill up this list with all internal edges and perform swap edges until this list is empty
    std::list<std::pair<PointIndex, PointIndex>> aEdgeList;
    std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>>::iterator pE;
    for (pE = aEdge2Face.begin(); pE != aEdge2Face.end(); ++pE) {
        if (pE->second.size() == 2) {  // make sure that we really have an internal edge
            aEdgeList.push_back(pE->first);
        }
    }

    // to be sure to avoid an endless loop
    unsigned long uMaxIter = 5 * aEdge2Face.size();

    // Perform a swap edge where needed
    while (!aEdgeList.empty() && uMaxIter > 0) {
        // get the first edge and remove it from the list
        std::pair<PointIndex, PointIndex> aEdge = aEdgeList.front();
        aEdgeList.pop_front();
        uMaxIter--;

        // get the adjacent facets to this edge
        pE = aEdge2Face.find(aEdge);

        // this edge has been removed some iterations before
        if (pE == aEdge2Face.end()) {
            continue;
        }

        // Is swap edge allowed and sensible?
        if (!ShouldSwapEdge(pE->second[0], pE->second[1], fMaxAngle)) {
            continue;
        }

        // ok, here we should perform a swap edge to minimize the maximum angle
        if (/*fMax12 > fMax34*/ true) {
            // swap the edge
            SwapEdge(pE->second[0], pE->second[1]);

            MeshFacet& rF1 = _rclMesh._aclFacetArray[pE->second[0]];
            MeshFacet& rF2 = _rclMesh._aclFacetArray[pE->second[1]];
            unsigned short side1 = rF1.Side(aEdge.first, aEdge.second);
            unsigned short side2 = rF2.Side(aEdge.first, aEdge.second);

            // adjust the edge list
            for (int i = 0; i < 3; i++) {
                std::map<std::pair<PointIndex, PointIndex>, std::vector<FacetIndex>>::iterator it;
                // first facet
                PointIndex ulPt0 =
                    std::min<PointIndex>(rF1._aulPoints[i], rF1._aulPoints[(i + 1) % 3]);
                PointIndex ulPt1 =
                    std::max<PointIndex>(rF1._aulPoints[i], rF1._aulPoints[(i + 1) % 3]);
                it = aEdge2Face.find(std::make_pair(ulPt0, ulPt1));
                if (it != aEdge2Face.end()) {
                    if (it->second[0] == pE->second[1]) {
                        it->second[0] = pE->second[0];
                    }
                    else if (it->second[1] == pE->second[1]) {
                        it->second[1] = pE->second[0];
                    }
                    aEdgeList.push_back(it->first);
                }

                // second facet
                ulPt0 = std::min<PointIndex>(rF2._aulPoints[i], rF2._aulPoints[(i + 1) % 3]);
                ulPt1 = std::max<PointIndex>(rF2._aulPoints[i], rF2._aulPoints[(i + 1) % 3]);
                it = aEdge2Face.find(std::make_pair(ulPt0, ulPt1));
                if (it != aEdge2Face.end()) {
                    if (it->second[0] == pE->second[0]) {
                        it->second[0] = pE->second[1];
                    }
                    else if (it->second[1] == pE->second[0]) {
                        it->second[1] = pE->second[1];
                    }
                    aEdgeList.push_back(it->first);
                }
            }

            // Now we must remove the edge and replace it through the new edge
            PointIndex ulPt0 = std::min<PointIndex>(rF1._aulPoints[(side1 + 1) % 3],
                                                    rF2._aulPoints[(side2 + 1) % 3]);
            PointIndex ulPt1 = std::max<PointIndex>(rF1._aulPoints[(side1 + 1) % 3],
                                                    rF2._aulPoints[(side2 + 1) % 3]);
            std::pair<PointIndex, PointIndex> aNewEdge = std::make_pair(ulPt0, ulPt1);
            aEdge2Face[aNewEdge] = pE->second;
            aEdge2Face.erase(pE);
        }
    }
}

// Cosine of the maximum angle in triangle (v1,v2,v3)
static float
cos_maxangle(const Base::Vector3f& v1, const Base::Vector3f& v2, const Base::Vector3f& v3)
{
    float a = Base::Distance(v2, v3);
    float b = Base::Distance(v3, v1);
    float c = Base::Distance(v1, v2);
    float A = a * (b * b + c * c - a * a);
    float B = b * (c * c + a * a - b * b);
    float C = c * (a * a + b * b - c * c);
    return 0.5f * std::min<float>(std::min<float>(A, B), C)
        / (a * b * c);  // min cosine == max angle
}

static float swap_benefit(const Base::Vector3f& v1,
                          const Base::Vector3f& v2,
                          const Base::Vector3f& v3,
                          const Base::Vector3f& v4)
{
    Base::Vector3f n124 = (v4 - v2) % (v1 - v2);
    Base::Vector3f n234 = (v3 - v2) % (v4 - v2);
    if ((n124 * n234) <= 0.0f) {
        return 0.0f;  // avoid normal flip
    }

    return std::max<float>(-cos_maxangle(v1, v2, v3), -cos_maxangle(v1, v3, v4))
        - std::max<float>(-cos_maxangle(v1, v2, v4), -cos_maxangle(v2, v3, v4));
}

float MeshTopoAlgorithm::SwapEdgeBenefit(FacetIndex f, int e) const
{
    const MeshFacetArray& faces = _rclMesh.GetFacets();
    const MeshPointArray& vertices = _rclMesh.GetPoints();

    FacetIndex n = faces[f]._aulNeighbours[e];
    if (n == FACET_INDEX_MAX) {
        return 0.0f;  // border edge
    }

    PointIndex v1 = faces[f]._aulPoints[e];
    PointIndex v2 = faces[f]._aulPoints[(e + 1) % 3];
    PointIndex v3 = faces[f]._aulPoints[(e + 2) % 3];
    unsigned short s = faces[n].Side(faces[f]);
    if (s == USHRT_MAX) {
        std::cerr << "MeshTopoAlgorithm::SwapEdgeBenefit: error in neighbourhood "
                  << "of faces " << f << " and " << n << std::endl;
        return 0.0f;  // topological error
    }
    PointIndex v4 = faces[n]._aulPoints[(s + 2) % 3];
    if (v3 == v4) {
        std::cerr << "MeshTopoAlgorithm::SwapEdgeBenefit: duplicate faces " << f << " and " << n
                  << std::endl;
        return 0.0f;  // duplicate faces
    }
    return swap_benefit(vertices[v2], vertices[v3], vertices[v1], vertices[v4]);
}

using FaceEdge = std::pair<FacetIndex, int>;  // (face, edge) pair
using FaceEdgePriority = std::pair<float, FaceEdge>;

void MeshTopoAlgorithm::OptimizeTopology()
{
    // Find all edges that can be swapped and insert them into a
    // priority queue
    const MeshFacetArray& faces = _rclMesh.GetFacets();
    FacetIndex nf = _rclMesh.CountFacets();
    std::priority_queue<FaceEdgePriority> todo;
    for (FacetIndex i = 0; i < nf; i++) {
        for (int j = 0; j < 3; j++) {
            float b = SwapEdgeBenefit(i, j);
            if (b > 0.0f) {
                todo.push(std::make_pair(b, std::make_pair(i, j)));
            }
        }
    }

    // Edges are sorted in decreasing order with respect to their benefit
    while (!todo.empty()) {
        FacetIndex f = todo.top().second.first;
        int e = todo.top().second.second;
        todo.pop();
        // Check again if the swap should still be done
        if (SwapEdgeBenefit(f, e) <= 0.0f) {
            continue;
        }
        // OK, swap the edge
        FacetIndex f2 = faces[f]._aulNeighbours[e];
        SwapEdge(f, f2);
        // Insert new edges into queue, if necessary
        for (int j = 0; j < 3; j++) {
            float b = SwapEdgeBenefit(f, j);
            if (b > 0.0f) {
                todo.push(std::make_pair(b, std::make_pair(f, j)));
            }
        }
        for (int j = 0; j < 3; j++) {
            float b = SwapEdgeBenefit(f2, j);
            if (b > 0.0f) {
                todo.push(std::make_pair(b, std::make_pair(f2, j)));
            }
        }
    }
}

void MeshTopoAlgorithm::DelaunayFlip(float fMaxAngle)
{
    // For each internal edge get the adjacent facets.
    std::set<std::pair<FacetIndex, FacetIndex>> aEdge2Face;
    FacetIndex index = 0;
    for (MeshFacetArray::_TIterator pI = _rclMesh._aclFacetArray.begin();
         pI != _rclMesh._aclFacetArray.end();
         ++pI, index++) {
        for (FacetIndex nbIndex : pI->_aulNeighbours) {
            // ignore open edges
            if (nbIndex != FACET_INDEX_MAX) {
                FacetIndex ulFt0 = std::min<FacetIndex>(index, nbIndex);
                FacetIndex ulFt1 = std::max<FacetIndex>(index, nbIndex);
                aEdge2Face.insert(std::pair<FacetIndex, FacetIndex>(ulFt0, ulFt1));
            }
        }
    }

    Base::Vector3f center;
    while (!aEdge2Face.empty()) {
        std::set<std::pair<FacetIndex, FacetIndex>>::iterator it = aEdge2Face.begin();
        std::pair<FacetIndex, FacetIndex> edge = *it;
        aEdge2Face.erase(it);
        if (ShouldSwapEdge(edge.first, edge.second, fMaxAngle)) {
            float radius = _rclMesh.GetFacet(edge.first).CenterOfCircumCircle(center);
            radius *= radius;
            const MeshFacet& face_1 = _rclMesh._aclFacetArray[edge.first];
            const MeshFacet& face_2 = _rclMesh._aclFacetArray[edge.second];
            unsigned short side = face_2.Side(edge.first);
            MeshPoint vertex = _rclMesh.GetPoint(face_2._aulPoints[(side + 1) % 3]);
            if (Base::DistanceP2(center, vertex) < radius) {
                SwapEdge(edge.first, edge.second);
                for (int i = 0; i < 3; i++) {
                    if (face_1._aulNeighbours[i] != FACET_INDEX_MAX
                        && face_1._aulNeighbours[i] != edge.second) {
                        FacetIndex ulFt0 =
                            std::min<FacetIndex>(edge.first, face_1._aulNeighbours[i]);
                        FacetIndex ulFt1 =
                            std::max<FacetIndex>(edge.first, face_1._aulNeighbours[i]);
                        aEdge2Face.insert(std::pair<FacetIndex, FacetIndex>(ulFt0, ulFt1));
                    }
                    if (face_2._aulNeighbours[i] != FACET_INDEX_MAX
                        && face_2._aulNeighbours[i] != edge.first) {
                        FacetIndex ulFt0 =
                            std::min<FacetIndex>(edge.second, face_2._aulNeighbours[i]);
                        FacetIndex ulFt1 =
                            std::max<FacetIndex>(edge.second, face_2._aulNeighbours[i]);
                        aEdge2Face.insert(std::pair<FacetIndex, FacetIndex>(ulFt0, ulFt1));
                    }
                }
            }
        }
    }
}

int MeshTopoAlgorithm::DelaunayFlip()
{
    int cnt_swap = 0;
    _rclMesh._aclFacetArray.ResetFlag(MeshFacet::TMP0);
    size_t cnt_facets = _rclMesh._aclFacetArray.size();
    for (size_t i = 0; i < cnt_facets; i++) {
        const MeshFacet& f_face = _rclMesh._aclFacetArray[i];
        if (f_face.IsFlag(MeshFacet::TMP0)) {
            continue;
        }
        for (int j = 0; j < 3; j++) {
            FacetIndex n = f_face._aulNeighbours[j];
            if (n != FACET_INDEX_MAX) {
                const MeshFacet& n_face = _rclMesh._aclFacetArray[n];
                if (n_face.IsFlag(MeshFacet::TMP0)) {
                    continue;
                }
                unsigned short k = n_face.Side(f_face);
                MeshGeomFacet f1 = _rclMesh.GetFacet(f_face);
                MeshGeomFacet f2 = _rclMesh.GetFacet(n_face);
                Base::Vector3f c1, c2, p1, p2;
                p1 = f1._aclPoints[(j + 2) % 3];
                p2 = f2._aclPoints[(k + 2) % 3];
                float r1 = f1.CenterOfCircumCircle(c1);
                r1 = r1 * r1;
                float r2 = f2.CenterOfCircumCircle(c2);
                r2 = r2 * r2;
                float d1 = Base::DistanceP2(c1, p2);
                float d2 = Base::DistanceP2(c2, p1);
                if (d1 < r1 || d2 < r2) {
                    SwapEdge(i, n);
                    cnt_swap++;
                    f_face.SetFlag(MeshFacet::TMP0);
                    n_face.SetFlag(MeshFacet::TMP0);
                }
            }
        }
    }

    return cnt_swap;
}

void MeshTopoAlgorithm::AdjustEdgesToCurvatureDirection()
{
    std::vector<Wm4::Vector3<float>> aPnts;
    MeshPointIterator cPIt(_rclMesh);
    aPnts.reserve(_rclMesh.CountPoints());
    for (cPIt.Init(); cPIt.More(); cPIt.Next()) {
        aPnts.emplace_back(cPIt->x, cPIt->y, cPIt->z);
    }

    // get all point connections
    std::vector<int> aIdx;
    const MeshFacetArray& raFts = _rclMesh.GetFacets();
    aIdx.reserve(3 * raFts.size());

    // Build map of edges to the referencing facets
    FacetIndex k = 0;
    std::map<std::pair<PointIndex, PointIndex>, std::list<FacetIndex>> aclEdgeMap;
    for (std::vector<MeshFacet>::const_iterator jt = raFts.begin(); jt != raFts.end(); ++jt, k++) {
        for (int i = 0; i < 3; i++) {
            PointIndex ulT0 = jt->_aulPoints[i];
            PointIndex ulT1 = jt->_aulPoints[(i + 1) % 3];
            PointIndex ulP0 = std::min<PointIndex>(ulT0, ulT1);
            PointIndex ulP1 = std::max<PointIndex>(ulT0, ulT1);
            aclEdgeMap[std::make_pair(ulP0, ulP1)].push_front(k);
            aIdx.push_back(static_cast<int>(jt->_aulPoints[i]));
        }
    }

    // compute vertex based curvatures
    Wm4::MeshCurvature<float> meshCurv(static_cast<int>(_rclMesh.CountPoints()),
                                       &(aPnts[0]),
                                       static_cast<int>(_rclMesh.CountFacets()),
                                       &(aIdx[0]));

    // get curvature information now
    const Wm4::Vector3<float>* aMaxCurvDir = meshCurv.GetMaxDirections();
    const Wm4::Vector3<float>* aMinCurvDir = meshCurv.GetMinDirections();
    const float* aMaxCurv = meshCurv.GetMaxCurvatures();
    const float* aMinCurv = meshCurv.GetMinCurvatures();

    raFts.ResetFlag(MeshFacet::VISIT);
    const MeshPointArray& raPts = _rclMesh.GetPoints();
    for (auto& kt : aclEdgeMap) {
        if (kt.second.size() == 2) {
            PointIndex uPt1 = kt.first.first;
            PointIndex uPt2 = kt.first.second;
            FacetIndex uFt1 = kt.second.front();
            FacetIndex uFt2 = kt.second.back();

            const MeshFacet& rFace1 = raFts[uFt1];
            const MeshFacet& rFace2 = raFts[uFt2];
            if (rFace1.IsFlag(MeshFacet::VISIT) || rFace2.IsFlag(MeshFacet::VISIT)) {
                continue;
            }

            PointIndex uPt3 {}, uPt4 {};
            unsigned short side = rFace1.Side(uPt1, uPt2);
            uPt3 = rFace1._aulPoints[(side + 2) % 3];
            side = rFace2.Side(uPt1, uPt2);
            uPt4 = rFace2._aulPoints[(side + 2) % 3];

            Wm4::Vector3<float> dir;
            float fActCurvature {};
            if (fabs(aMinCurv[uPt1]) > fabs(aMaxCurv[uPt1])) {
                fActCurvature = aMinCurv[uPt1];
                dir = aMaxCurvDir[uPt1];
            }
            else {
                fActCurvature = aMaxCurv[uPt1];
                dir = aMinCurvDir[uPt1];
            }

            Base::Vector3f cMinDir(dir.X(), dir.Y(), dir.Z());
            Base::Vector3f cEdgeDir1 = raPts[uPt1] - raPts[uPt2];
            Base::Vector3f cEdgeDir2 = raPts[uPt3] - raPts[uPt4];
            cMinDir.Normalize();
            cEdgeDir1.Normalize();
            cEdgeDir2.Normalize();

            // get the plane and calculate the distance to the fourth point
            MeshGeomFacet cPlane(raPts[uPt1], raPts[uPt2], raPts[uPt3]);
            // positive or negative distance
            float fDist = raPts[uPt4].DistanceToPlane(cPlane._aclPoints[0], cPlane.GetNormal());

            float fLength12 = Base::Distance(raPts[uPt1], raPts[uPt2]);
            float fLength34 = Base::Distance(raPts[uPt3], raPts[uPt4]);
            if (fabs(cEdgeDir1 * cMinDir) < fabs(cEdgeDir2 * cMinDir)) {
                if (IsSwapEdgeLegal(uFt1, uFt2) && fLength34 < 1.05f * fLength12
                    && fActCurvature * fDist > 0.0f) {
                    SwapEdge(uFt1, uFt2);
                    rFace1.SetFlag(MeshFacet::VISIT);
                    rFace2.SetFlag(MeshFacet::VISIT);
                }
            }
        }
    }
}

bool MeshTopoAlgorithm::InsertVertexAndSwapEdge(FacetIndex ulFacetPos,
                                                const Base::Vector3f& rclPoint,
                                                float fMaxAngle)
{
    if (!InsertVertex(ulFacetPos, rclPoint)) {
        return false;
    }

    // get the created elements
    FacetIndex ulF1Ind = _rclMesh._aclFacetArray.size() - 2;
    FacetIndex ulF2Ind = _rclMesh._aclFacetArray.size() - 1;
    MeshFacet& rclF1 = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclF2 = _rclMesh._aclFacetArray[ulF1Ind];
    MeshFacet& rclF3 = _rclMesh._aclFacetArray[ulF2Ind];

    // first facet
    for (FacetIndex uNeighbour : rclF1._aulNeighbours) {
        if (uNeighbour != FACET_INDEX_MAX && uNeighbour != ulF1Ind && uNeighbour != ulF2Ind) {
            if (ShouldSwapEdge(ulFacetPos, uNeighbour, fMaxAngle)) {
                SwapEdge(ulFacetPos, uNeighbour);
                break;
            }
        }
    }
    for (FacetIndex uNeighbour : rclF2._aulNeighbours) {
        // second facet
        if (uNeighbour != FACET_INDEX_MAX && uNeighbour != ulFacetPos && uNeighbour != ulF2Ind) {
            if (ShouldSwapEdge(ulF1Ind, uNeighbour, fMaxAngle)) {
                SwapEdge(ulF1Ind, uNeighbour);
                break;
            }
        }
    }

    // third facet
    for (FacetIndex uNeighbour : rclF3._aulNeighbours) {
        if (uNeighbour != FACET_INDEX_MAX && uNeighbour != ulFacetPos && uNeighbour != ulF1Ind) {
            if (ShouldSwapEdge(ulF2Ind, uNeighbour, fMaxAngle)) {
                SwapEdge(ulF2Ind, uNeighbour);
                break;
            }
        }
    }

    return true;
}

bool MeshTopoAlgorithm::IsSwapEdgeLegal(FacetIndex ulFacetPos, FacetIndex ulNeighbour) const
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uFSide = rclF.Side(rclN);
    unsigned short uNSide = rclN.Side(rclF);

    if (uFSide == USHRT_MAX || uNSide == USHRT_MAX) {
        return false;  // not neighbours
    }

    Base::Vector3f cP1 = _rclMesh._aclPointArray[rclF._aulPoints[uFSide]];
    Base::Vector3f cP2 = _rclMesh._aclPointArray[rclF._aulPoints[(uFSide + 1) % 3]];
    Base::Vector3f cP3 = _rclMesh._aclPointArray[rclF._aulPoints[(uFSide + 2) % 3]];
    Base::Vector3f cP4 = _rclMesh._aclPointArray[rclN._aulPoints[(uNSide + 2) % 3]];

    // do not allow to create degenerated triangles
    MeshGeomFacet cT3(cP4, cP3, cP1);
    if (cT3.IsDegenerated(MeshDefinitions::_fMinPointDistanceP2)) {
        return false;
    }
    MeshGeomFacet cT4(cP3, cP4, cP2);
    if (cT4.IsDegenerated(MeshDefinitions::_fMinPointDistanceP2)) {
        return false;
    }

    // We must make sure that the two adjacent triangles builds a convex polygon, otherwise
    // the swap edge operation is illegal
    Base::Vector3f cU = cP2 - cP1;
    Base::Vector3f cV = cP4 - cP3;
    // build a helper plane through cP1 that must separate cP3 and cP4
    Base::Vector3f cN1 = (cU % cV) % cU;
    if (((cP3 - cP1) * cN1) * ((cP4 - cP1) * cN1) >= 0.0f) {
        return false;  // not convex
    }
    // build a helper plane through cP3 that must separate cP1 and cP2
    Base::Vector3f cN2 = (cU % cV) % cV;
    if (((cP1 - cP3) * cN2) * ((cP2 - cP3) * cN2) >= 0.0f) {
        return false;  // not convex
    }

    return true;
}

bool MeshTopoAlgorithm::ShouldSwapEdge(FacetIndex ulFacetPos,
                                       FacetIndex ulNeighbour,
                                       float fMaxAngle) const
{
    if (!IsSwapEdgeLegal(ulFacetPos, ulNeighbour)) {
        return false;
    }

    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uFSide = rclF.Side(rclN);
    unsigned short uNSide = rclN.Side(rclF);

    Base::Vector3f cP1 = _rclMesh._aclPointArray[rclF._aulPoints[uFSide]];
    Base::Vector3f cP2 = _rclMesh._aclPointArray[rclF._aulPoints[(uFSide + 1) % 3]];
    Base::Vector3f cP3 = _rclMesh._aclPointArray[rclF._aulPoints[(uFSide + 2) % 3]];
    Base::Vector3f cP4 = _rclMesh._aclPointArray[rclN._aulPoints[(uNSide + 2) % 3]];

    MeshGeomFacet cT1(cP1, cP2, cP3);
    float fMax1 = cT1.MaximumAngle();
    MeshGeomFacet cT2(cP2, cP1, cP4);
    float fMax2 = cT2.MaximumAngle();
    MeshGeomFacet cT3(cP4, cP3, cP1);
    float fMax3 = cT3.MaximumAngle();
    MeshGeomFacet cT4(cP3, cP4, cP2);
    float fMax4 = cT4.MaximumAngle();

    // get the angle between the triangles
    Base::Vector3f cN1 = cT1.GetNormal();
    Base::Vector3f cN2 = cT2.GetNormal();
    if (cN1.GetAngle(cN2) > fMaxAngle) {
        return false;
    }

    float fMax12 = std::max<float>(fMax1, fMax2);
    float fMax34 = std::max<float>(fMax3, fMax4);

    return fMax12 > fMax34;
}

void MeshTopoAlgorithm::SwapEdge(FacetIndex ulFacetPos, FacetIndex ulNeighbour)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uFSide = rclF.Side(rclN);
    unsigned short uNSide = rclN.Side(rclF);

    if (uFSide == USHRT_MAX || uNSide == USHRT_MAX) {
        return;  // not neighbours
    }

    // adjust the neighbourhood
    if (rclF._aulNeighbours[(uFSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[(uFSide + 1) % 3]].ReplaceNeighbour(
            ulFacetPos,
            ulNeighbour);
    }
    if (rclN._aulNeighbours[(uNSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 1) % 3]].ReplaceNeighbour(ulNeighbour,
                                                                                        ulFacetPos);
    }

    // swap the point and neighbour indices
    rclF._aulPoints[(uFSide + 1) % 3] = rclN._aulPoints[(uNSide + 2) % 3];
    rclN._aulPoints[(uNSide + 1) % 3] = rclF._aulPoints[(uFSide + 2) % 3];
    rclF._aulNeighbours[uFSide] = rclN._aulNeighbours[(uNSide + 1) % 3];
    rclN._aulNeighbours[uNSide] = rclF._aulNeighbours[(uFSide + 1) % 3];
    rclF._aulNeighbours[(uFSide + 1) % 3] = ulNeighbour;
    rclN._aulNeighbours[(uNSide + 1) % 3] = ulFacetPos;
}

bool MeshTopoAlgorithm::SplitEdge(FacetIndex ulFacetPos,
                                  FacetIndex ulNeighbour,
                                  const Base::Vector3f& rP)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uFSide = rclF.Side(rclN);
    unsigned short uNSide = rclN.Side(rclF);

    if (uFSide == USHRT_MAX || uNSide == USHRT_MAX) {
        return false;  // not neighbours
    }

    PointIndex uPtCnt = _rclMesh._aclPointArray.size();
    PointIndex uPtInd = this->GetOrAddIndex(rP);
    FacetIndex ulSize = _rclMesh._aclFacetArray.size();

    // the given point is already part of the mesh => creating new facets would
    // be an illegal operation
    if (uPtInd < uPtCnt) {
        return false;
    }

    // adjust the neighbourhood
    if (rclF._aulNeighbours[(uFSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[(uFSide + 1) % 3]].ReplaceNeighbour(ulFacetPos,
                                                                                        ulSize);
    }
    if (rclN._aulNeighbours[(uNSide + 2) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 2) % 3]].ReplaceNeighbour(ulNeighbour,
                                                                                        ulSize + 1);
    }

    MeshFacet cNew1, cNew2;
    cNew1._aulPoints[0] = uPtInd;
    cNew1._aulPoints[1] = rclF._aulPoints[(uFSide + 1) % 3];
    cNew1._aulPoints[2] = rclF._aulPoints[(uFSide + 2) % 3];
    cNew1._aulNeighbours[0] = ulSize + 1;
    cNew1._aulNeighbours[1] = rclF._aulNeighbours[(uFSide + 1) % 3];
    cNew1._aulNeighbours[2] = ulFacetPos;

    cNew2._aulPoints[0] = rclN._aulPoints[uNSide];
    cNew2._aulPoints[1] = uPtInd;
    cNew2._aulPoints[2] = rclN._aulPoints[(uNSide + 2) % 3];
    cNew2._aulNeighbours[0] = ulSize;
    cNew2._aulNeighbours[1] = ulNeighbour;
    cNew2._aulNeighbours[2] = rclN._aulNeighbours[(uNSide + 2) % 3];

    // adjust the facets
    rclF._aulPoints[(uFSide + 1) % 3] = uPtInd;
    rclF._aulNeighbours[(uFSide + 1) % 3] = ulSize;
    rclN._aulPoints[uNSide] = uPtInd;
    rclN._aulNeighbours[(uNSide + 2) % 3] = ulSize + 1;

    // insert new facets
    _rclMesh._aclFacetArray.push_back(cNew1);
    _rclMesh._aclFacetArray.push_back(cNew2);

    return true;
}

bool MeshTopoAlgorithm::SplitOpenEdge(FacetIndex ulFacetPos,
                                      unsigned short uSide,
                                      const Base::Vector3f& rP)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    if (rclF._aulNeighbours[uSide] != FACET_INDEX_MAX) {
        return false;  // not open
    }

    PointIndex uPtCnt = _rclMesh._aclPointArray.size();
    PointIndex uPtInd = this->GetOrAddIndex(rP);
    FacetIndex ulSize = _rclMesh._aclFacetArray.size();

    if (uPtInd < uPtCnt) {
        return false;  // the given point is already part of the mesh => creating new facets would
                       // be an illegal operation
    }

    // adjust the neighbourhood
    if (rclF._aulNeighbours[(uSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[(uSide + 1) % 3]].ReplaceNeighbour(ulFacetPos,
                                                                                       ulSize);
    }

    MeshFacet cNew;
    cNew._aulPoints[0] = uPtInd;
    cNew._aulPoints[1] = rclF._aulPoints[(uSide + 1) % 3];
    cNew._aulPoints[2] = rclF._aulPoints[(uSide + 2) % 3];
    cNew._aulNeighbours[0] = FACET_INDEX_MAX;
    cNew._aulNeighbours[1] = rclF._aulNeighbours[(uSide + 1) % 3];
    cNew._aulNeighbours[2] = ulFacetPos;

    // adjust the facets
    rclF._aulPoints[(uSide + 1) % 3] = uPtInd;
    rclF._aulNeighbours[(uSide + 1) % 3] = ulSize;

    // insert new facets
    _rclMesh._aclFacetArray.push_back(cNew);
    return true;
}

bool MeshTopoAlgorithm::Vertex_Less::operator()(const Base::Vector3f& u,
                                                const Base::Vector3f& v) const
{
    if (fabs(u.x - v.x) > FLOAT_EPS) {
        return u.x < v.x;
    }
    if (fabs(u.y - v.y) > FLOAT_EPS) {
        return u.y < v.y;
    }
    if (fabs(u.z - v.z) > FLOAT_EPS) {
        return u.z < v.z;
    }
    return false;
}

void MeshTopoAlgorithm::BeginCache()
{
    if (_cache) {
        delete _cache;
    }
    _cache = new tCache();
    PointIndex nbPoints = _rclMesh._aclPointArray.size();
    for (unsigned int pntCpt = 0; pntCpt < nbPoints; ++pntCpt) {
        _cache->insert(std::make_pair(_rclMesh._aclPointArray[pntCpt], pntCpt));
    }
}

void MeshTopoAlgorithm::EndCache()
{
    if (_cache) {
        _cache->clear();
        delete _cache;
        _cache = nullptr;
    }
}

PointIndex MeshTopoAlgorithm::GetOrAddIndex(const MeshPoint& rclPoint)
{
    if (!_cache) {
        return _rclMesh._aclPointArray.GetOrAddIndex(rclPoint);
    }

    unsigned long sz = _rclMesh._aclPointArray.size();
    std::pair<tCache::iterator, bool> retval = _cache->insert(std::make_pair(rclPoint, sz));
    if (retval.second) {
        _rclMesh._aclPointArray.push_back(rclPoint);
    }
    return retval.first->second;
}

std::vector<FacetIndex> MeshTopoAlgorithm::GetFacetsToPoint(FacetIndex uFacetPos,
                                                            PointIndex uPointPos) const
{
    // get all facets this point is referenced by
    std::list<FacetIndex> aReference;
    aReference.push_back(uFacetPos);
    std::set<FacetIndex> aRefFacet;
    while (!aReference.empty()) {
        FacetIndex uIndex = aReference.front();
        aReference.pop_front();
        aRefFacet.insert(uIndex);
        MeshFacet& rFace = _rclMesh._aclFacetArray[uIndex];
        for (int i = 0; i < 3; i++) {
            if (rFace._aulPoints[i] == uPointPos) {
                if (rFace._aulNeighbours[i] != FACET_INDEX_MAX) {
                    if (aRefFacet.find(rFace._aulNeighbours[i]) == aRefFacet.end()) {
                        aReference.push_back(rFace._aulNeighbours[i]);
                    }
                }
                if (rFace._aulNeighbours[(i + 2) % 3] != FACET_INDEX_MAX) {
                    if (aRefFacet.find(rFace._aulNeighbours[(i + 2) % 3]) == aRefFacet.end()) {
                        aReference.push_back(rFace._aulNeighbours[(i + 2) % 3]);
                    }
                }
                break;
            }
        }
    }

    // copy the items
    std::vector<FacetIndex> aRefs;
    aRefs.insert(aRefs.end(), aRefFacet.begin(), aRefFacet.end());
    return aRefs;
}

void MeshTopoAlgorithm::Cleanup()
{
    _rclMesh.RemoveInvalids();
    _needsCleanup = false;
}

bool MeshTopoAlgorithm::CollapseVertex(const VertexCollapse& vc)
{
    if (vc._circumFacets.size() != vc._circumPoints.size()) {
        return false;
    }

    if (vc._circumFacets.size() != 3) {
        return false;
    }

    if (!_rclMesh._aclPointArray[vc._point].IsValid()) {
        return false;  // the point is marked invalid from a previous run
    }

    MeshFacet& rFace1 = _rclMesh._aclFacetArray[vc._circumFacets[0]];
    MeshFacet& rFace2 = _rclMesh._aclFacetArray[vc._circumFacets[1]];
    MeshFacet& rFace3 = _rclMesh._aclFacetArray[vc._circumFacets[2]];

    // get the point that is not shared by rFace1
    PointIndex ptIndex = POINT_INDEX_MAX;
    std::vector<PointIndex>::const_iterator it;
    for (it = vc._circumPoints.begin(); it != vc._circumPoints.end(); ++it) {
        if (!rFace1.HasPoint(*it)) {
            ptIndex = *it;
            break;
        }
    }

    if (ptIndex == POINT_INDEX_MAX) {
        return false;
    }

    FacetIndex neighbour1 = FACET_INDEX_MAX;
    FacetIndex neighbour2 = FACET_INDEX_MAX;

    const std::vector<FacetIndex>& faces = vc._circumFacets;
    // get neighbours that are not part of the faces to be removed
    for (int i = 0; i < 3; i++) {
        if (std::find(faces.begin(), faces.end(), rFace2._aulNeighbours[i]) == faces.end()) {
            neighbour1 = rFace2._aulNeighbours[i];
        }
        if (std::find(faces.begin(), faces.end(), rFace3._aulNeighbours[i]) == faces.end()) {
            neighbour2 = rFace3._aulNeighbours[i];
        }
    }

    // adjust point and neighbour indices
    rFace1.Transpose(vc._point, ptIndex);
    rFace1.ReplaceNeighbour(vc._circumFacets[1], neighbour1);
    rFace1.ReplaceNeighbour(vc._circumFacets[2], neighbour2);

    if (neighbour1 != FACET_INDEX_MAX) {
        MeshFacet& rFace4 = _rclMesh._aclFacetArray[neighbour1];
        rFace4.ReplaceNeighbour(vc._circumFacets[1], vc._circumFacets[0]);
    }
    if (neighbour2 != FACET_INDEX_MAX) {
        MeshFacet& rFace5 = _rclMesh._aclFacetArray[neighbour2];
        rFace5.ReplaceNeighbour(vc._circumFacets[2], vc._circumFacets[0]);
    }

    // the two facets and the point can be marked for removal
    rFace2.SetInvalid();
    rFace3.SetInvalid();
    _rclMesh._aclPointArray[vc._point].SetInvalid();

    _needsCleanup = true;

    return true;
}

bool MeshTopoAlgorithm::CollapseEdge(FacetIndex ulFacetPos, FacetIndex ulNeighbour)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uFSide = rclF.Side(rclN);
    unsigned short uNSide = rclN.Side(rclF);

    if (uFSide == USHRT_MAX || uNSide == USHRT_MAX) {
        return false;  // not neighbours
    }

    if (!rclF.IsValid() || !rclN.IsValid()) {
        return false;  // the facets are marked invalid from a previous run
    }

    // get the point index we want to remove
    PointIndex ulPointPos = rclF._aulPoints[uFSide];
    PointIndex ulPointNew = rclN._aulPoints[uNSide];

    // get all facets this point is referenced by
    std::vector<FacetIndex> aRefs = GetFacetsToPoint(ulFacetPos, ulPointPos);
    for (FacetIndex it : aRefs) {
        MeshFacet& rFace = _rclMesh._aclFacetArray[it];
        rFace.Transpose(ulPointPos, ulPointNew);
    }

    // set the new neighbourhood
    if (rclF._aulNeighbours[(uFSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[(uFSide + 1) % 3]].ReplaceNeighbour(
            ulFacetPos,
            rclF._aulNeighbours[(uFSide + 2) % 3]);
    }
    if (rclF._aulNeighbours[(uFSide + 2) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclF._aulNeighbours[(uFSide + 2) % 3]].ReplaceNeighbour(
            ulFacetPos,
            rclF._aulNeighbours[(uFSide + 1) % 3]);
    }
    if (rclN._aulNeighbours[(uNSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 1) % 3]].ReplaceNeighbour(
            ulNeighbour,
            rclN._aulNeighbours[(uNSide + 2) % 3]);
    }
    if (rclN._aulNeighbours[(uNSide + 2) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 2) % 3]].ReplaceNeighbour(
            ulNeighbour,
            rclN._aulNeighbours[(uNSide + 1) % 3]);
    }

    // isolate the both facets and the point
    rclF._aulNeighbours[0] = FACET_INDEX_MAX;
    rclF._aulNeighbours[1] = FACET_INDEX_MAX;
    rclF._aulNeighbours[2] = FACET_INDEX_MAX;
    rclF.SetInvalid();
    rclN._aulNeighbours[0] = FACET_INDEX_MAX;
    rclN._aulNeighbours[1] = FACET_INDEX_MAX;
    rclN._aulNeighbours[2] = FACET_INDEX_MAX;
    rclN.SetInvalid();
    _rclMesh._aclPointArray[ulPointPos].SetInvalid();

    _needsCleanup = true;

    return true;
}

bool MeshTopoAlgorithm::IsCollapseEdgeLegal(const EdgeCollapse& ec) const
{
    // http://stackoverflow.com/a/27049418/148668
    // Check connectivity
    //
    std::vector<PointIndex> commonPoints;
    std::set_intersection(ec._adjacentFrom.begin(),
                          ec._adjacentFrom.end(),
                          ec._adjacentTo.begin(),
                          ec._adjacentTo.end(),
                          std::back_insert_iterator<std::vector<PointIndex>>(commonPoints));
    if (commonPoints.size() > 2) {
        return false;
    }

    // Check geometry
    std::vector<FacetIndex>::const_iterator it;
    for (it = ec._changeFacets.begin(); it != ec._changeFacets.end(); ++it) {
        MeshFacet f = _rclMesh._aclFacetArray[*it];
        if (!f.IsValid()) {
            return false;
        }

        // ignore the facet(s) at this edge
        if (f.HasPoint(ec._fromPoint) && f.HasPoint(ec._toPoint)) {
            continue;
        }

        MeshGeomFacet tria1 = _rclMesh.GetFacet(f);
        f.Transpose(ec._fromPoint, ec._toPoint);
        MeshGeomFacet tria2 = _rclMesh.GetFacet(f);

        if (tria1.GetNormal() * tria2.GetNormal() < 0.0f) {
            return false;
        }
    }

    // If the data structure is valid and the algorithm works as expected
    // it should never happen to reject the edge-collapse here!
    for (it = ec._removeFacets.begin(); it != ec._removeFacets.end(); ++it) {
        MeshFacet f = _rclMesh._aclFacetArray[*it];
        if (!f.IsValid()) {
            return false;
        }
    }

    if (!_rclMesh._aclPointArray[ec._fromPoint].IsValid()) {
        return false;
    }

    if (!_rclMesh._aclPointArray[ec._toPoint].IsValid()) {
        return false;
    }

    return true;
}

bool MeshTopoAlgorithm::CollapseEdge(const EdgeCollapse& ec)
{
    std::vector<FacetIndex>::const_iterator it;
    for (it = ec._removeFacets.begin(); it != ec._removeFacets.end(); ++it) {
        MeshFacet& f = _rclMesh._aclFacetArray[*it];
        f.SetInvalid();

        // adjust the neighbourhood
        std::vector<FacetIndex> neighbours;
        for (FacetIndex nbIndex : f._aulNeighbours) {
            // get the neighbours of the facet that won't be invalidated
            if (nbIndex != FACET_INDEX_MAX) {
                if (std::find(ec._removeFacets.begin(), ec._removeFacets.end(), nbIndex)
                    == ec._removeFacets.end()) {
                    neighbours.push_back(nbIndex);
                }
            }
        }

        if (neighbours.size() == 2) {
            MeshFacet& n1 = _rclMesh._aclFacetArray[neighbours[0]];
            n1.ReplaceNeighbour(*it, neighbours[1]);
            MeshFacet& n2 = _rclMesh._aclFacetArray[neighbours[1]];
            n2.ReplaceNeighbour(*it, neighbours[0]);
        }
        else if (neighbours.size() == 1) {
            MeshFacet& n1 = _rclMesh._aclFacetArray[neighbours[0]];
            n1.ReplaceNeighbour(*it, FACET_INDEX_MAX);
        }
    }

    for (it = ec._changeFacets.begin(); it != ec._changeFacets.end(); ++it) {
        MeshFacet& f = _rclMesh._aclFacetArray[*it];
        f.Transpose(ec._fromPoint, ec._toPoint);
    }

    _rclMesh._aclPointArray[ec._fromPoint].SetInvalid();

    _needsCleanup = true;
    return true;
}

bool MeshTopoAlgorithm::CollapseFacet(FacetIndex ulFacetPos)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];
    if (!rclF.IsValid()) {
        return false;  // the facet is marked invalid from a previous run
    }

    // get the point index we want to remove
    PointIndex ulPointInd0 = rclF._aulPoints[0];
    PointIndex ulPointInd1 = rclF._aulPoints[1];
    PointIndex ulPointInd2 = rclF._aulPoints[2];

    // move the vertex to the gravity center
    Base::Vector3f cCenter = _rclMesh.GetGravityPoint(rclF);
    _rclMesh._aclPointArray[ulPointInd0] = cCenter;

    // set the new point indices for all facets that share one of the points to be deleted
    std::vector<FacetIndex> aRefs = GetFacetsToPoint(ulFacetPos, ulPointInd1);
    for (FacetIndex it : aRefs) {
        MeshFacet& rFace = _rclMesh._aclFacetArray[it];
        rFace.Transpose(ulPointInd1, ulPointInd0);
    }

    aRefs = GetFacetsToPoint(ulFacetPos, ulPointInd2);
    for (FacetIndex it : aRefs) {
        MeshFacet& rFace = _rclMesh._aclFacetArray[it];
        rFace.Transpose(ulPointInd2, ulPointInd0);
    }

    // set the neighbourhood of the circumjacent facets
    for (FacetIndex nbIndex : rclF._aulNeighbours) {
        if (nbIndex == FACET_INDEX_MAX) {
            continue;
        }
        MeshFacet& rclN = _rclMesh._aclFacetArray[nbIndex];
        unsigned short uNSide = rclN.Side(rclF);

        if (rclN._aulNeighbours[(uNSide + 1) % 3] != FACET_INDEX_MAX) {
            _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 1) % 3]].ReplaceNeighbour(
                nbIndex,
                rclN._aulNeighbours[(uNSide + 2) % 3]);
        }
        if (rclN._aulNeighbours[(uNSide + 2) % 3] != FACET_INDEX_MAX) {
            _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 2) % 3]].ReplaceNeighbour(
                nbIndex,
                rclN._aulNeighbours[(uNSide + 1) % 3]);
        }

        // Isolate the neighbours from the topology
        rclN._aulNeighbours[0] = FACET_INDEX_MAX;
        rclN._aulNeighbours[1] = FACET_INDEX_MAX;
        rclN._aulNeighbours[2] = FACET_INDEX_MAX;
        rclN.SetInvalid();
    }

    // Isolate this facet and make two of its points invalid
    rclF._aulNeighbours[0] = FACET_INDEX_MAX;
    rclF._aulNeighbours[1] = FACET_INDEX_MAX;
    rclF._aulNeighbours[2] = FACET_INDEX_MAX;
    rclF.SetInvalid();
    _rclMesh._aclPointArray[ulPointInd1].SetInvalid();
    _rclMesh._aclPointArray[ulPointInd2].SetInvalid();

    _needsCleanup = true;

    return true;
}

void MeshTopoAlgorithm::SplitFacet(FacetIndex ulFacetPos,
                                   const Base::Vector3f& rP1,
                                   const Base::Vector3f& rP2)
{
    float fEps = MESH_MIN_EDGE_LEN;
    MeshFacet& rFace = _rclMesh._aclFacetArray[ulFacetPos];
    MeshPoint& rVertex0 = _rclMesh._aclPointArray[rFace._aulPoints[0]];
    MeshPoint& rVertex1 = _rclMesh._aclPointArray[rFace._aulPoints[1]];
    MeshPoint& rVertex2 = _rclMesh._aclPointArray[rFace._aulPoints[2]];

    auto pointIndex = [=](const Base::Vector3f& rP) {
        unsigned short equalP = USHRT_MAX;
        if (Base::Distance(rVertex0, rP) < fEps) {
            equalP = 0;
        }
        else if (Base::Distance(rVertex1, rP) < fEps) {
            equalP = 1;
        }
        else if (Base::Distance(rVertex2, rP) < fEps) {
            equalP = 2;
        }
        return equalP;
    };

    unsigned short equalP1 = pointIndex(rP1);
    unsigned short equalP2 = pointIndex(rP2);

    // both points are coincident with the corner points
    if (equalP1 != USHRT_MAX && equalP2 != USHRT_MAX) {
        return;  // must not split the facet
    }

    if (equalP1 != USHRT_MAX) {
        // get the edge to the second given point and perform a split edge operation
        SplitFacetOnOneEdge(ulFacetPos, rP2);
    }
    else if (equalP2 != USHRT_MAX) {
        // get the edge to the first given point and perform a split edge operation
        SplitFacetOnOneEdge(ulFacetPos, rP1);
    }
    else {
        SplitFacetOnTwoEdges(ulFacetPos, rP1, rP2);
    }
}

void MeshTopoAlgorithm::SplitFacetOnOneEdge(FacetIndex ulFacetPos, const Base::Vector3f& rP)
{
    float fMinDist = FLOAT_MAX;
    unsigned short iEdgeNo = USHRT_MAX;
    MeshFacet& rFace = _rclMesh._aclFacetArray[ulFacetPos];

    for (unsigned short i = 0; i < 3; i++) {
        Base::Vector3f cBase(_rclMesh._aclPointArray[rFace._aulPoints[i]]);
        Base::Vector3f cEnd(_rclMesh._aclPointArray[rFace._aulPoints[(i + 1) % 3]]);
        Base::Vector3f cDir = cEnd - cBase;

        float fDist = rP.DistanceToLine(cBase, cDir);
        if (fDist < fMinDist) {
            fMinDist = fDist;
            iEdgeNo = i;
        }
    }

    if (fMinDist < 0.05f) {
        if (rFace._aulNeighbours[iEdgeNo] != FACET_INDEX_MAX) {
            SplitEdge(ulFacetPos, rFace._aulNeighbours[iEdgeNo], rP);
        }
        else {
            SplitOpenEdge(ulFacetPos, iEdgeNo, rP);
        }
    }
}

void MeshTopoAlgorithm::SplitFacetOnTwoEdges(FacetIndex ulFacetPos,
                                             const Base::Vector3f& rP1,
                                             const Base::Vector3f& rP2)
{
    // search for the matching edges
    unsigned short iEdgeNo1 = USHRT_MAX, iEdgeNo2 = USHRT_MAX;
    float fMinDist1 = FLOAT_MAX, fMinDist2 = FLOAT_MAX;
    MeshFacet& rFace = _rclMesh._aclFacetArray[ulFacetPos];

    for (unsigned short i = 0; i < 3; i++) {
        Base::Vector3f cBase(_rclMesh._aclPointArray[rFace._aulPoints[i]]);
        Base::Vector3f cEnd(_rclMesh._aclPointArray[rFace._aulPoints[(i + 1) % 3]]);
        Base::Vector3f cDir = cEnd - cBase;

        float fDist = rP1.DistanceToLine(cBase, cDir);
        if (fDist < fMinDist1) {
            fMinDist1 = fDist;
            iEdgeNo1 = i;
        }
        fDist = rP2.DistanceToLine(cBase, cDir);
        if (fDist < fMinDist2) {
            fMinDist2 = fDist;
            iEdgeNo2 = i;
        }
    }

    if (iEdgeNo1 == iEdgeNo2 || fMinDist1 >= 0.05f || fMinDist2 >= 0.05f) {
        return;  // no valid configuration
    }

    // make first point lying on the previous edge
    Base::Vector3f cP1 = rP1;
    Base::Vector3f cP2 = rP2;
    if ((iEdgeNo2 + 1) % 3 == iEdgeNo1) {
        std::swap(iEdgeNo1, iEdgeNo2);
        std::swap(cP1, cP2);
    }

    // insert new points
    PointIndex cntPts1 = this->GetOrAddIndex(cP1);
    PointIndex cntPts2 = this->GetOrAddIndex(cP2);
    FacetIndex cntFts = _rclMesh.CountFacets();

    unsigned short v0 = (iEdgeNo2 + 1) % 3;
    unsigned short v1 = iEdgeNo1;
    unsigned short v2 = iEdgeNo2;

    PointIndex p0 = rFace._aulPoints[v0];
    PointIndex p1 = rFace._aulPoints[v1];
    PointIndex p2 = rFace._aulPoints[v2];

    FacetIndex n0 = rFace._aulNeighbours[v0];
    FacetIndex n1 = rFace._aulNeighbours[v1];
    FacetIndex n2 = rFace._aulNeighbours[v2];

    // Modify and add facets
    //
    rFace._aulPoints[v0] = cntPts2;
    rFace._aulPoints[v1] = cntPts1;
    rFace._aulNeighbours[v0] = cntFts + 1;

    float dist1 = Base::DistanceP2(_rclMesh._aclPointArray[p0], cP1);
    float dist2 = Base::DistanceP2(_rclMesh._aclPointArray[p1], cP2);

    if (dist1 > dist2) {
        AddFacet(p0, p1, cntPts2, n0, cntFts + 1, n2);
        AddFacet(p1, cntPts1, cntPts2, n1, ulFacetPos, cntFts);
    }
    else {
        AddFacet(p0, p1, cntPts1, n0, n1, cntFts + 1);
        AddFacet(p0, cntPts1, cntPts2, cntFts, ulFacetPos, n2);
    }

    std::vector<FacetIndex> fixIndices;
    fixIndices.push_back(ulFacetPos);

    if (n0 != FACET_INDEX_MAX) {
        fixIndices.push_back(n0);
    }

    // split up the neighbour facets
    if (n1 != FACET_INDEX_MAX) {
        fixIndices.push_back(n1);
        MeshFacet& rN = _rclMesh._aclFacetArray[n1];
        for (FacetIndex nbIndex : rN._aulNeighbours) {
            fixIndices.push_back(nbIndex);
        }
        SplitFacet(n1, p1, p2, cntPts1);
    }

    if (n2 != FACET_INDEX_MAX) {
        fixIndices.push_back(n2);
        MeshFacet& rN = _rclMesh._aclFacetArray[n2];
        for (FacetIndex nbIndex : rN._aulNeighbours) {
            fixIndices.push_back(nbIndex);
        }
        SplitFacet(n2, p2, p0, cntPts2);
    }

    FacetIndex cntFts2 = _rclMesh.CountFacets();
    for (FacetIndex i = cntFts; i < cntFts2; i++) {
        fixIndices.push_back(i);
    }

    std::sort(fixIndices.begin(), fixIndices.end());
    fixIndices.erase(std::unique(fixIndices.begin(), fixIndices.end()), fixIndices.end());
    HarmonizeNeighbours(fixIndices);
}

void MeshTopoAlgorithm::SplitFacet(FacetIndex ulFacetPos,
                                   PointIndex P1,
                                   PointIndex P2,
                                   PointIndex Pn)
{
    MeshFacet& rFace = _rclMesh._aclFacetArray[ulFacetPos];
    unsigned short side = rFace.Side(P1, P2);
    if (side != USHRT_MAX) {
        PointIndex V1 = rFace._aulPoints[(side + 1) % 3];
        PointIndex V2 = rFace._aulPoints[(side + 2) % 3];
        FacetIndex size = _rclMesh._aclFacetArray.size();

        rFace._aulPoints[(side + 1) % 3] = Pn;
        FacetIndex N1 = rFace._aulNeighbours[(side + 1) % 3];
        if (N1 != FACET_INDEX_MAX) {
            _rclMesh._aclFacetArray[N1].ReplaceNeighbour(ulFacetPos, size);
        }

        rFace._aulNeighbours[(side + 1) % 3] = ulFacetPos;
        AddFacet(Pn, V1, V2);
    }
}

void MeshTopoAlgorithm::AddFacet(PointIndex P1, PointIndex P2, PointIndex P3)
{
    MeshFacet facet;
    facet._aulPoints[0] = P1;
    facet._aulPoints[1] = P2;
    facet._aulPoints[2] = P3;

    _rclMesh._aclFacetArray.push_back(facet);
}

void MeshTopoAlgorithm::AddFacet(PointIndex P1,
                                 PointIndex P2,
                                 PointIndex P3,
                                 FacetIndex N1,
                                 FacetIndex N2,
                                 FacetIndex N3)
{
    MeshFacet facet;
    facet._aulPoints[0] = P1;
    facet._aulPoints[1] = P2;
    facet._aulPoints[2] = P3;
    facet._aulNeighbours[0] = N1;
    facet._aulNeighbours[1] = N2;
    facet._aulNeighbours[2] = N3;

    _rclMesh._aclFacetArray.push_back(facet);
}

void MeshTopoAlgorithm::HarmonizeNeighbours(const std::vector<FacetIndex>& ulFacets)
{
    for (FacetIndex it : ulFacets) {
        for (FacetIndex jt : ulFacets) {
            HarmonizeNeighbours(it, jt);
        }
    }
}

void MeshTopoAlgorithm::HarmonizeNeighbours(FacetIndex facet1, FacetIndex facet2)
{
    if (facet1 == facet2) {
        return;
    }

    MeshFacet& rFace1 = _rclMesh._aclFacetArray[facet1];
    MeshFacet& rFace2 = _rclMesh._aclFacetArray[facet2];

    unsigned short side = rFace1.Side(rFace2);
    if (side != USHRT_MAX) {
        rFace1._aulNeighbours[side] = facet2;
    }

    side = rFace2.Side(rFace1);
    if (side != USHRT_MAX) {
        rFace2._aulNeighbours[side] = facet1;
    }
}

void MeshTopoAlgorithm::SplitNeighbourFacet(FacetIndex ulFacetPos,
                                            unsigned short uFSide,
                                            const Base::Vector3f rPoint)
{
    MeshFacet& rclF = _rclMesh._aclFacetArray[ulFacetPos];

    FacetIndex ulNeighbour = rclF._aulNeighbours[uFSide];
    MeshFacet& rclN = _rclMesh._aclFacetArray[ulNeighbour];

    unsigned short uNSide = rclN.Side(rclF);

    PointIndex uPtInd = this->GetOrAddIndex(rPoint);
    FacetIndex ulSize = _rclMesh._aclFacetArray.size();

    // adjust the neighbourhood
    if (rclN._aulNeighbours[(uNSide + 1) % 3] != FACET_INDEX_MAX) {
        _rclMesh._aclFacetArray[rclN._aulNeighbours[(uNSide + 1) % 3]].ReplaceNeighbour(ulNeighbour,
                                                                                        ulSize);
    }

    MeshFacet cNew;
    cNew._aulPoints[0] = uPtInd;
    cNew._aulPoints[1] = rclN._aulPoints[(uNSide + 1) % 3];
    cNew._aulPoints[2] = rclN._aulPoints[(uNSide + 2) % 3];
    cNew._aulNeighbours[0] = ulFacetPos;
    cNew._aulNeighbours[1] = rclN._aulNeighbours[(uNSide + 1) % 3];
    cNew._aulNeighbours[2] = ulNeighbour;

    // adjust the facet
    rclN._aulPoints[(uNSide + 1) % 3] = uPtInd;
    rclN._aulNeighbours[(uNSide + 1) % 3] = ulSize;

    // insert new facet
    _rclMesh._aclFacetArray.push_back(cNew);
}

#if 0
  // create 3 new facets
  MeshGeomFacet clFacet;

  // facet [P1, Ei+1, P2]
  clFacet._aclPoints[0] = cP1;
  clFacet._aclPoints[1] = _rclMesh._aclPointArray[rFace._aulPoints[(iEdgeNo1+1)%3]];
  clFacet._aclPoints[2] = cP2;
  clFacet.CalcNormal();
  _aclNewFacets.push_back(clFacet);
  // facet [P2, Ei+2, Ei]
  clFacet._aclPoints[0] = cP2;
  clFacet._aclPoints[1] = _rclMesh._aclPointArray[rFace._aulPoints[(iEdgeNo1+2)%3]];
  clFacet._aclPoints[2] = _rclMesh._aclPointArray[rFace._aulPoints[iEdgeNo1]];
  clFacet.CalcNormal();
  _aclNewFacets.push_back(clFacet);
  // facet [P2, Ei, P1]
  clFacet._aclPoints[0] = cP2;
  clFacet._aclPoints[1] = _rclMesh._aclPointArray[rFace._aulPoints[iEdgeNo1]];
  clFacet._aclPoints[2] = cP1;
  clFacet.CalcNormal();
  _aclNewFacets.push_back(clFacet);
#endif

bool MeshTopoAlgorithm::RemoveDegeneratedFacet(FacetIndex index)
{
    if (index >= _rclMesh._aclFacetArray.size()) {
        return false;
    }
    MeshFacet& rFace = _rclMesh._aclFacetArray[index];

    // coincident corners (either topological or geometrical)
    for (int i = 0; i < 3; i++) {
        const MeshPoint& rE0 = _rclMesh._aclPointArray[rFace._aulPoints[i]];
        const MeshPoint& rE1 = _rclMesh._aclPointArray[rFace._aulPoints[(i + 1) % 3]];
        if (rE0 == rE1) {
            FacetIndex uN1 = rFace._aulNeighbours[(i + 1) % 3];
            FacetIndex uN2 = rFace._aulNeighbours[(i + 2) % 3];
            if (uN2 != FACET_INDEX_MAX) {
                _rclMesh._aclFacetArray[uN2].ReplaceNeighbour(index, uN1);
            }
            if (uN1 != FACET_INDEX_MAX) {
                _rclMesh._aclFacetArray[uN1].ReplaceNeighbour(index, uN2);
            }

            // isolate the face and remove it
            rFace._aulNeighbours[0] = FACET_INDEX_MAX;
            rFace._aulNeighbours[1] = FACET_INDEX_MAX;
            rFace._aulNeighbours[2] = FACET_INDEX_MAX;
            _rclMesh.DeleteFacet(index);
            return true;
        }
    }

    // We have a facet of the form
    // P0 +----+------+P2
    //         P1
    for (int j = 0; j < 3; j++) {
        Base::Vector3f cVec1 = _rclMesh._aclPointArray[rFace._aulPoints[(j + 1) % 3]]
            - _rclMesh._aclPointArray[rFace._aulPoints[j]];
        Base::Vector3f cVec2 = _rclMesh._aclPointArray[rFace._aulPoints[(j + 2) % 3]]
            - _rclMesh._aclPointArray[rFace._aulPoints[j]];

        // adjust the neighbourhoods and point indices
        if (cVec1 * cVec2 < 0.0f) {
            FacetIndex uN1 = rFace._aulNeighbours[(j + 1) % 3];
            if (uN1 != FACET_INDEX_MAX) {
                // get the neighbour and common edge side
                MeshFacet& rNb = _rclMesh._aclFacetArray[uN1];
                unsigned short side = rNb.Side(index);

                // bend the point indices
                rFace._aulPoints[(j + 2) % 3] = rNb._aulPoints[(side + 2) % 3];
                rNb._aulPoints[(side + 1) % 3] = rFace._aulPoints[j];

                // set correct neighbourhood
                FacetIndex uN2 = rFace._aulNeighbours[(j + 2) % 3];
                rNb._aulNeighbours[side] = uN2;
                if (uN2 != FACET_INDEX_MAX) {
                    _rclMesh._aclFacetArray[uN2].ReplaceNeighbour(index, uN1);
                }
                FacetIndex uN3 = rNb._aulNeighbours[(side + 1) % 3];
                rFace._aulNeighbours[(j + 1) % 3] = uN3;
                if (uN3 != FACET_INDEX_MAX) {
                    _rclMesh._aclFacetArray[uN3].ReplaceNeighbour(uN1, index);
                }
                rNb._aulNeighbours[(side + 1) % 3] = index;
                rFace._aulNeighbours[(j + 2) % 3] = uN1;
            }
            else {
                _rclMesh.DeleteFacet(index);
            }

            return true;
        }
    }

    return false;
}

bool MeshTopoAlgorithm::RemoveCorruptedFacet(FacetIndex index)
{
    if (index >= _rclMesh._aclFacetArray.size()) {
        return false;
    }
    MeshFacet& rFace = _rclMesh._aclFacetArray[index];

    // coincident corners (topological)
    for (int i = 0; i < 3; i++) {
        if (rFace._aulPoints[i] == rFace._aulPoints[(i + 1) % 3]) {
            FacetIndex uN1 = rFace._aulNeighbours[(i + 1) % 3];
            FacetIndex uN2 = rFace._aulNeighbours[(i + 2) % 3];
            if (uN2 != FACET_INDEX_MAX) {
                _rclMesh._aclFacetArray[uN2].ReplaceNeighbour(index, uN1);
            }
            if (uN1 != FACET_INDEX_MAX) {
                _rclMesh._aclFacetArray[uN1].ReplaceNeighbour(index, uN2);
            }

            // isolate the face and remove it
            rFace._aulNeighbours[0] = FACET_INDEX_MAX;
            rFace._aulNeighbours[1] = FACET_INDEX_MAX;
            rFace._aulNeighbours[2] = FACET_INDEX_MAX;
            _rclMesh.DeleteFacet(index);
            return true;
        }
    }

    return false;
}

void MeshTopoAlgorithm::FillupHoles(unsigned long length,
                                    int level,
                                    AbstractPolygonTriangulator& cTria,
                                    std::list<std::vector<PointIndex>>& aFailed)
{
    // get the mesh boundaries as an array of point indices
    std::list<std::vector<PointIndex>> aBorders, aFillBorders;
    MeshAlgorithm cAlgo(_rclMesh);
    cAlgo.GetMeshBorders(aBorders);

    // split boundary loops if needed
    cAlgo.SplitBoundaryLoops(aBorders);

    for (const auto& aBorder : aBorders) {
        if (aBorder.size() - 1 <= length) {  // ignore boundary with too many edges
            aFillBorders.push_back(aBorder);
        }
    }

    if (!aFillBorders.empty()) {
        FillupHoles(level, cTria, aFillBorders, aFailed);
    }
}

void MeshTopoAlgorithm::FillupHoles(int level,
                                    AbstractPolygonTriangulator& cTria,
                                    const std::list<std::vector<PointIndex>>& aBorders,
                                    std::list<std::vector<PointIndex>>& aFailed)
{
    // get the facets to a point
    MeshRefPointToFacets cPt2Fac(_rclMesh);
    MeshAlgorithm cAlgo(_rclMesh);

    MeshFacetArray newFacets;
    MeshPointArray newPoints;
    unsigned long numberOfOldPoints = _rclMesh._aclPointArray.size();
    for (const auto& aBorder : aBorders) {
        MeshFacetArray cFacets;
        MeshPointArray cPoints;
        std::vector<PointIndex> bound = aBorder;
        if (cAlgo.FillupHole(bound, cTria, cFacets, cPoints, level, &cPt2Fac)) {
            if (bound.front() == bound.back()) {
                bound.pop_back();
            }
            // the triangulation may produce additional points which we must take into account when
            // appending to the mesh
            if (cPoints.size() > bound.size()) {
                unsigned long countBoundaryPoints = bound.size();
                unsigned long countDifference = cPoints.size() - countBoundaryPoints;
                MeshPointArray::_TIterator pt = cPoints.begin() + countBoundaryPoints;
                for (unsigned long i = 0; i < countDifference; i++, pt++) {
                    bound.push_back(numberOfOldPoints++);
                    newPoints.push_back(*pt);
                }
            }
            if (cTria.NeedsReindexing()) {
                for (auto& cFacet : cFacets) {
                    cFacet._aulPoints[0] = bound[cFacet._aulPoints[0]];
                    cFacet._aulPoints[1] = bound[cFacet._aulPoints[1]];
                    cFacet._aulPoints[2] = bound[cFacet._aulPoints[2]];
                    newFacets.push_back(cFacet);
                }
            }
            else {
                for (auto& cFacet : cFacets) {
                    newFacets.push_back(cFacet);
                }
            }
        }
        else {
            aFailed.push_back(aBorder);
        }
    }

    // insert new points and faces into the mesh structure
    _rclMesh._aclPointArray.insert(_rclMesh._aclPointArray.end(),
                                   newPoints.begin(),
                                   newPoints.end());
    for (const auto& newPoint : newPoints) {
        _rclMesh._clBoundBox.Add(newPoint);
    }
    if (!newFacets.empty()) {
        // Do some checks for invalid point indices
        MeshFacetArray addFacets;
        addFacets.reserve(newFacets.size());
        unsigned long ctPoints = _rclMesh.CountPoints();
        for (auto& newFacet : newFacets) {
            if (newFacet._aulPoints[0] >= ctPoints || newFacet._aulPoints[1] >= ctPoints
                || newFacet._aulPoints[2] >= ctPoints) {
                Base::Console().Log("Ignore invalid face <%d, %d, %d> (%d vertices)\n",
                                    newFacet._aulPoints[0],
                                    newFacet._aulPoints[1],
                                    newFacet._aulPoints[2],
                                    ctPoints);
            }
            else {
                addFacets.push_back(newFacet);
            }
        }
        _rclMesh.AddFacets(addFacets, true);
    }
}

void MeshTopoAlgorithm::FindHoles(unsigned long length,
                                  std::list<std::vector<PointIndex>>& aBorders) const
{
    std::list<std::vector<PointIndex>> border;
    MeshAlgorithm cAlgo(_rclMesh);
    cAlgo.GetMeshBorders(border);
    for (const auto& it : border) {
        if (it.size() <= length) {
            aBorders.push_back(it);
        }
    }
}

void MeshTopoAlgorithm::FindComponents(unsigned long count, std::vector<FacetIndex>& findIndices)
{
    std::vector<std::vector<FacetIndex>> segments;
    MeshComponents comp(_rclMesh);
    comp.SearchForComponents(MeshComponents::OverEdge, segments);

    for (const auto& segment : segments) {
        if (segment.size() <= count) {
            findIndices.insert(findIndices.end(), segment.begin(), segment.end());
        }
    }
}

void MeshTopoAlgorithm::RemoveComponents(unsigned long count)
{
    std::vector<FacetIndex> removeFacets;
    FindComponents(count, removeFacets);
    if (!removeFacets.empty()) {
        _rclMesh.DeleteFacets(removeFacets);
    }
}

void MeshTopoAlgorithm::HarmonizeNormals()
{
    std::vector<FacetIndex> uIndices = MeshEvalOrientation(_rclMesh).GetIndices();
    for (FacetIndex index : uIndices) {
        _rclMesh._aclFacetArray[index].FlipNormal();
    }
}

void MeshTopoAlgorithm::FlipNormals()
{
    for (MeshFacetArray::_TIterator i = _rclMesh._aclFacetArray.begin();
         i < _rclMesh._aclFacetArray.end();
         ++i) {
        i->FlipNormal();
    }
}

// ---------------------------------------------------------------------------

/**
 * Some important formulas:
 *
 * Ne = 3Nv - Nb + 3B + 6(G-R)
 * Nt = 2Nv - Nb + 2B + 4(G-R)
 *
 * Ne <= 3Nv + 6(G-R)
 * Nt <= 2Nv + 4(G-R)
 *
 * Ne ~ 3Nv, Nv >> G, Nv >> R
 * Nt ~ 2Nv, Nv >> G, Nv >> R
 *
 * Ne = #Edges
 * Nt = #Facets
 * Nv = #Vertices
 * Nb = #Boundary vertices
 * B  = #Boundaries
 * G  = Genus (Number of holes)
 * R  = #components
 *
 * See also http://max-limper.de/publications/Euler/
 */

MeshComponents::MeshComponents(const MeshKernel& rclMesh)
    : _rclMesh(rclMesh)
{}

void MeshComponents::SearchForComponents(TMode tMode,
                                         std::vector<std::vector<FacetIndex>>& aclT) const
{
    // all facets
    std::vector<FacetIndex> aulAllFacets(_rclMesh.CountFacets());
    FacetIndex k = 0;
    for (FacetIndex& aulAllFacet : aulAllFacets) {
        aulAllFacet = k++;
    }

    SearchForComponents(tMode, aulAllFacets, aclT);
}

void MeshComponents::SearchForComponents(TMode tMode,
                                         const std::vector<FacetIndex>& aSegment,
                                         std::vector<std::vector<FacetIndex>>& aclT) const
{
    FacetIndex ulStartFacet {};

    if (_rclMesh.CountFacets() == 0) {
        return;
    }

    // reset VISIT flags
    MeshAlgorithm cAlgo(_rclMesh);
    cAlgo.SetFacetFlag(MeshFacet::VISIT);
    cAlgo.ResetFacetsFlag(aSegment, MeshFacet::VISIT);

    const MeshFacetArray& rFAry = _rclMesh.GetFacets();
    MeshFacetArray::_TConstIterator iTri = rFAry.begin();
    MeshFacetArray::_TConstIterator iBeg = rFAry.begin();
    MeshFacetArray::_TConstIterator iEnd = rFAry.end();

    // start from the first not visited facet
    unsigned long ulVisited = cAlgo.CountFacetFlag(MeshFacet::VISIT);
    MeshIsNotFlag<MeshFacet> flag;
    iTri = std::find_if(iTri, iEnd, [flag](const MeshFacet& f) {
        return flag(f, MeshFacet::VISIT);
    });
    ulStartFacet = iTri - iBeg;

    // visitor
    std::vector<FacetIndex> aclComponent;
    std::vector<std::vector<FacetIndex>> aclConnectComp;
    MeshTopFacetVisitor clFVisitor(aclComponent);

    while (ulStartFacet != FACET_INDEX_MAX) {
        // collect all facets of a component
        aclComponent.clear();
        if (tMode == OverEdge) {
            ulVisited += _rclMesh.VisitNeighbourFacets(clFVisitor, ulStartFacet);
        }
        else if (tMode == OverPoint) {
            ulVisited += _rclMesh.VisitNeighbourFacetsOverCorners(clFVisitor, ulStartFacet);
        }

        // get also start facet
        aclComponent.push_back(ulStartFacet);
        aclConnectComp.push_back(aclComponent);

        // if the mesh consists of several topologic independent components
        // We can search from position 'iTri' on because all elements _before_ are already visited
        // what we know from the previous iteration.
        iTri = std::find_if(iTri, iEnd, [flag](const MeshFacet& f) {
            return flag(f, MeshFacet::VISIT);
        });

        if (iTri < iEnd) {
            ulStartFacet = iTri - iBeg;
        }
        else {
            ulStartFacet = FACET_INDEX_MAX;
        }
    }

    // sort components by size (descending order)
    std::sort(aclConnectComp.begin(), aclConnectComp.end(), CNofFacetsCompare());
    aclT = aclConnectComp;
    boost::ignore_unused(ulVisited);
}
